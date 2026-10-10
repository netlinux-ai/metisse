/*
 * composite.c - phase 1 of the Composite-based rework of Metisse.
 *
 * A minimal OpenGL compositor for a plain X server: it redirects every
 * top-level window off-screen (XComposite), turns each window's pixmap into
 * a GL texture without copying (GLX_EXT_texture_from_pixmap) and draws them
 * bottom-to-top on the Composite Overlay Window. There is no second X server
 * and no VNC-style copy, unlike the original Metisse.
 *
 * Phase 2 adds a per-window 3D transform, read from the window property
 * _METISSE_TRANSFORM (type INTEGER, format 32, values in thousandths):
 *     rx ry rz   rotation about the X, Y and Z axes through the window
 *                centre, in degrees (so 30000 = 30 degrees)
 *     scale      uniform scale (1000 = 1.0)
 *     opacity    0..1000
 * Missing values default to 0 0 0 1000 1000. A perspective camera is set up
 * so an untransformed window lands exactly on its X position and size.
 * (Pointer input is not yet remapped: clicks still hit the real flat window.)
 *
 * Usage:  composite [-display NAME] [-shot FILE.ppm]
 *   SIGUSR1  write the next frame to FILE.ppm (when -shot is given)
 *   SIGINT / SIGTERM  restore the display and exit
 *
 * Licence: GPL, like the rest of Metisse.
 */
#define _POSIX_C_SOURCE 200809L
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

typedef struct Win {
	Window id;
	int x, y, w, h, bw;	/* geometry; w,h exclude the border */
	int depth;
	VisualID vid;
	int mapped;
	int dirty;		/* contents changed since the texture was bound */
	Damage damage;
	Pixmap pix;		/* named pixmap, 0 when not valid */
	GLXPixmap gpix;
	GLuint tex;
	int inverted;		/* texture is stored top-down */
	float rx, ry, rz;	/* rotation in degrees */
	float scale, opacity;
	struct Win *next;	/* towards the top of the stack */
} Win;

static Display *dpy;
static int scr;
static Window root, overlay;
static Win *bottom;		/* bottom-most window; list runs upwards */
static int damage_event, damage_error;
static int xerr;
static Atom transform_atom;
static volatile sig_atomic_t quit, want_shot;
static int need_paint = 1;
static const char *shot_file;

static PFNGLXBINDTEXIMAGEEXTPROC bind_tex_image;
static PFNGLXRELEASETEXIMAGEEXTPROC release_tex_image;

static int on_xerror(Display *d, XErrorEvent *e)
{
	(void)d; (void)e;
	xerr = 1;		/* windows vanish at any time; ignore */
	return 0;
}

static void on_signal(int sig)
{
	if (sig == SIGUSR1)
		want_shot = 1;
	else
		quit = 1;
}

/* ---- window list ------------------------------------------------------ */

static Win *find_win(Window id)
{
	for (Win *w = bottom; w; w = w->next)
		if (w->id == id)
			return w;
	return NULL;
}

static void unlink_win(Win *w)
{
	for (Win **p = &bottom; *p; p = &(*p)->next)
		if (*p == w) {
			*p = w->next;
			w->next = NULL;
			return;
		}
}

#define ON_TOP ((Window)~0UL)

/* Place w directly above 'above'; None = very bottom, ON_TOP = very top. */
static void restack_win(Win *w, Window above)
{
	unlink_win(w);
	Win *a = above ? find_win(above) : NULL;
	if (a) {
		w->next = a->next;
		a->next = w;
	} else if (above == None) {
		w->next = bottom;
		bottom = w;
	} else {		/* ON_TOP, or a sibling we do not track */
		Win **p = &bottom;
		while (*p)
			p = &(*p)->next;
		*p = w;
	}
}

/* ---- GL pixmap handling ----------------------------------------------- */

static int fbconfig_for(VisualID vid, int depth, GLXFBConfig *out)
{
	int n, found = 0;
	GLXFBConfig *c = glXGetFBConfigs(dpy, scr, &n);
	for (int i = 0; c && i < n && !found; i++) {
		int v, rgb = 0, rgba = 0;
		glXGetFBConfigAttrib(dpy, c[i], GLX_VISUAL_ID, &v);
		if ((VisualID)v != vid)
			continue;
		glXGetFBConfigAttrib(dpy, c[i], GLX_BIND_TO_TEXTURE_RGB_EXT, &rgb);
		glXGetFBConfigAttrib(dpy, c[i], GLX_BIND_TO_TEXTURE_RGBA_EXT, &rgba);
		/* windows with alpha need an RGBA binding; others can use either */
		if (depth == 32 ? rgba : (rgb || rgba)) {
			*out = c[i];
			found = 1;
		}
	}
	if (c)
		XFree(c);
	return found;
}

static void free_pixmap(Win *w)
{
	if (w->gpix) {
		release_tex_image(dpy, w->gpix, GLX_FRONT_LEFT_EXT);
		glXDestroyPixmap(dpy, w->gpix);
		w->gpix = 0;
	}
	if (w->tex) {
		glDeleteTextures(1, &w->tex);
		w->tex = 0;
	}
	if (w->pix) {
		XFreePixmap(dpy, w->pix);
		w->pix = 0;
	}
}

/* Make sure w has an up-to-date texture. Returns 0 if it cannot be drawn. */
static int update_texture(Win *w)
{
	if (!w->mapped)
		return 0;
	if (!w->pix) {
		GLXFBConfig fbc;
		int kind = fbconfig_for(w->vid, w->depth, &fbc);
		if (!kind)
			return 0;
		xerr = 0;
		w->pix = XCompositeNameWindowPixmap(dpy, w->id);
		XSync(dpy, False);
		if (xerr || !w->pix) {
			w->pix = 0;
			return 0;
		}
		int rgba = (w->depth == 32);
		int attr[] = {
			GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
			GLX_TEXTURE_FORMAT_EXT,
			rgba ? GLX_TEXTURE_FORMAT_RGBA_EXT
			     : GLX_TEXTURE_FORMAT_RGB_EXT,
			None };
		w->gpix = glXCreatePixmap(dpy, fbc, w->pix, attr);
		int inv = 1;
		glXGetFBConfigAttrib(dpy, fbc, GLX_Y_INVERTED_EXT, &inv);
		w->inverted = inv;
		glGenTextures(1, &w->tex);
		glBindTexture(GL_TEXTURE_2D, w->tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		bind_tex_image(dpy, w->gpix, GLX_FRONT_LEFT_EXT, NULL);
		w->dirty = 0;
	} else if (w->dirty) {
		glBindTexture(GL_TEXTURE_2D, w->tex);
		release_tex_image(dpy, w->gpix, GLX_FRONT_LEFT_EXT);
		bind_tex_image(dpy, w->gpix, GLX_FRONT_LEFT_EXT, NULL);
		w->dirty = 0;
	}
	return w->gpix != 0;
}

static void read_transform(Win *w)
{
	Atom type;
	int fmt;
	unsigned long n, after;
	unsigned char *data = NULL;
	long v[5] = { 0, 0, 0, 1000, 1000 };

	xerr = 0;
	if (XGetWindowProperty(dpy, w->id, transform_atom, 0, 5, False,
			       XA_INTEGER, &type, &fmt, &n, &after, &data) ==
	    Success && !xerr && type == XA_INTEGER && fmt == 32 && data) {
		for (unsigned long i = 0; i < n && i < 5; i++)
			v[i] = (int32_t)((long *)data)[i];
	}
	if (data)
		XFree(data);
	w->rx = v[0] / 1000.0f;
	w->ry = v[1] / 1000.0f;
	w->rz = v[2] / 1000.0f;
	w->scale = v[3] / 1000.0f;
	w->opacity = v[4] < 0 ? 0 : v[4] > 1000 ? 1 : v[4] / 1000.0f;
	need_paint = 1;
}

/* ---- tracking top-level windows ---------------------------------------- */

static void add_win(Window id)
{
	if (id == overlay || find_win(id))
		return;
	XWindowAttributes a;
	xerr = 0;
	if (!XGetWindowAttributes(dpy, id, &a) || xerr || a.class == InputOnly)
		return;
	Win *w = calloc(1, sizeof *w);
	w->id = id;
	w->x = a.x; w->y = a.y; w->w = a.width; w->h = a.height;
	w->bw = a.border_width;
	w->depth = a.depth;
	w->vid = XVisualIDFromVisual(a.visual);
	w->scale = w->opacity = 1.0f;
	w->mapped = (a.map_state == IsViewable);
	w->damage = XDamageCreate(dpy, id, XDamageReportNonEmpty);
	XSelectInput(dpy, id, PropertyChangeMask);
	read_transform(w);
	Win **p = &bottom;
	while (*p)
		p = &(*p)->next;
	*p = w;			/* new windows start on top */
	need_paint = 1;
}

static void remove_win(Window id)
{
	Win *w = find_win(id);
	if (!w)
		return;
	unlink_win(w);
	free_pixmap(w);
	free(w);
	need_paint = 1;
}

static void handle_event(XEvent *e)
{
	Win *w;

	if (e->type == damage_event + XDamageNotify) {
		XDamageNotifyEvent *d = (XDamageNotifyEvent *)e;
		if ((w = find_win(d->drawable))) {
			XDamageSubtract(dpy, w->damage, None, None);
			w->dirty = 1;
			need_paint = 1;
		}
		return;
	}
	switch (e->type) {
	case CreateNotify:
		if (e->xcreatewindow.parent == root)
			add_win(e->xcreatewindow.window);
		break;
	case DestroyNotify:
		remove_win(e->xdestroywindow.window);
		break;
	case ReparentNotify:
		if (e->xreparent.parent == root)
			add_win(e->xreparent.window);
		else
			remove_win(e->xreparent.window);
		break;
	case MapNotify:
		add_win(e->xmap.window);
		if ((w = find_win(e->xmap.window))) {
			w->mapped = 1;
			need_paint = 1;
		}
		break;
	case UnmapNotify:
		if ((w = find_win(e->xunmap.window))) {
			w->mapped = 0;
			free_pixmap(w);
			need_paint = 1;
		}
		break;
	case ConfigureNotify: {
		XConfigureEvent *c = &e->xconfigure;
		if (c->window == root)
			break;
		if ((w = find_win(c->window))) {
			if (c->width != w->w || c->height != w->h ||
			    c->border_width != w->bw)
				free_pixmap(w);	/* old pixmap is stale */
			w->x = c->x; w->y = c->y;
			w->w = c->width; w->h = c->height;
			w->bw = c->border_width;
			restack_win(w, c->above);
			need_paint = 1;
		}
		break;
	}
	case CirculateNotify:
		if ((w = find_win(e->xcirculate.window))) {
			if (e->xcirculate.place == PlaceOnTop)
				restack_win(w, ON_TOP);
			else
				restack_win(w, None);
			need_paint = 1;
		}
		break;
	case PropertyNotify:
		if (e->xproperty.atom == transform_atom &&
		    (w = find_win(e->xproperty.window)))
			read_transform(w);
		break;
	case SelectionClear:
		quit = 1;	/* another compositor took over */
		break;
	}
}

/* ---- painting ---------------------------------------------------------- */

static void write_ppm(const char *path, int width, int height)
{
	unsigned char *px = malloc((size_t)width * height * 4);
	FILE *f = fopen(path, "wb");
	if (!px || !f) {
		perror(path);
		free(px);
		if (f)
			fclose(f);
		return;
	}
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, px);
	fprintf(f, "P6\n%d %d\n255\n", width, height);
	for (int y = height - 1; y >= 0; y--)	/* GL origin is bottom-left */
		for (int x = 0; x < width; x++)
			fwrite(px + ((size_t)y * width + x) * 4, 1, 3, f);
	fclose(f);
	free(px);
	fprintf(stderr, "composite: wrote %s\n", path);
}

static void draw_window(Win *w)
{
	if (!update_texture(w))
		return;
	float fw = w->w + 2 * w->bw, fh = w->h + 2 * w->bw;
	float cx = w->x + fw / 2, cy = w->y + fh / 2;
	float t0 = w->inverted ? 1.0f : 0.0f, t1 = 1.0f - t0;

	glPushMatrix();
	glTranslatef(cx, cy, 0);
	glRotatef(w->rz, 0, 0, 1);
	glRotatef(w->ry, 0, 1, 0);
	glRotatef(w->rx, 1, 0, 0);
	glScalef(w->scale, w->scale, w->scale);
	glTranslatef(-cx, -cy, 0);
	glColor4f(w->opacity, w->opacity, w->opacity, w->opacity);
	glBindTexture(GL_TEXTURE_2D, w->tex);
	glBegin(GL_QUADS);
	glTexCoord2f(0, t1); glVertex2f(w->x, w->y);
	glTexCoord2f(1, t1); glVertex2f(w->x + fw, w->y);
	glTexCoord2f(1, t0); glVertex2f(w->x + fw, w->y + fh);
	glTexCoord2f(0, t0); glVertex2f(w->x, w->y + fh);
	glEnd();
	glPopMatrix();
}

/*
 * Perspective camera chosen so that the z=0 plane maps 1:1 to X pixels:
 * the eye sits at distance d from the screen plane with a vertical field of
 * view of FOV, and y is flipped because X's y axis points down.
 */
#define FOV_DEG 40.0
static void setup_camera(int vw, int vh)
{
	const double n = 10.0, f = 10000.0;
	double half = tan(FOV_DEG * 3.14159265358979 / 360.0);
	double d = (vh / 2.0) / half;
	double top = n * half, right = top * vw / (double)vh;
	glViewport(0, 0, vw, vh);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-right, right, -top, top, n, f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslated(0, 0, -d);
	glScaled(1, -1, 1);
	glTranslated(-vw / 2.0, -vh / 2.0, 0);
}

static void paint(GLXWindow gw)
{
	XWindowAttributes a;
	XGetWindowAttributes(dpy, overlay, &a);
	setup_camera(a.width, a.height);
	glClearColor(0.15f, 0.20f, 0.30f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);	/* X alpha is premultiplied */
	for (Win *w = bottom; w; w = w->next)
		draw_window(w);
	glDisable(GL_TEXTURE_2D);
	if (want_shot && shot_file) {
		write_ppm(shot_file, a.width, a.height);
		want_shot = 0;
	}
	glXSwapBuffers(dpy, gw);
}

/* ---- setup ------------------------------------------------------------- */

static GLXFBConfig overlay_fbconfig(void)
{
	VisualID rv = XVisualIDFromVisual(DefaultVisual(dpy, scr));
	int n;
	GLXFBConfig *c = glXGetFBConfigs(dpy, scr, &n), best = NULL;
	for (int i = 0; c && i < n; i++) {
		int v, db, dt, rt;
		glXGetFBConfigAttrib(dpy, c[i], GLX_VISUAL_ID, &v);
		glXGetFBConfigAttrib(dpy, c[i], GLX_DOUBLEBUFFER, &db);
		glXGetFBConfigAttrib(dpy, c[i], GLX_DRAWABLE_TYPE, &dt);
		glXGetFBConfigAttrib(dpy, c[i], GLX_RENDER_TYPE, &rt);
		if ((VisualID)v == rv && db && (dt & GLX_WINDOW_BIT) &&
		    (rt & GLX_RGBA_BIT)) {
			best = c[i];
			break;
		}
	}
	if (c)
		XFree(c);
	return best;
}

static int have_glx_ext(const char *name)
{
	const char *s = glXQueryExtensionsString(dpy, scr);
	return s && strstr(s, name);
}

int main(int argc, char **argv)
{
	const char *dname = NULL;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-display") && i + 1 < argc)
			dname = argv[++i];
		else if (!strcmp(argv[i], "-shot") && i + 1 < argc)
			shot_file = argv[++i];
		else {
			fprintf(stderr, "usage: %s [-display D] [-shot F.ppm]\n", argv[0]);
			return 2;
		}
	}
	if (!(dpy = XOpenDisplay(dname))) {
		fprintf(stderr, "composite: cannot open display\n");
		return 1;
	}
	scr = DefaultScreen(dpy);
	transform_atom = XInternAtom(dpy, "_METISSE_TRANSFORM", False);
	root = RootWindow(dpy, scr);
	XSetErrorHandler(on_xerror);

	int ev, er, maj = 0, min = 0;
	if (!XCompositeQueryExtension(dpy, &ev, &er) ||
	    (XCompositeQueryVersion(dpy, &maj, &min), maj == 0 && min < 4)) {
		fprintf(stderr, "composite: need XComposite >= 0.4\n");
		return 1;
	}
	if (!XDamageQueryExtension(dpy, &damage_event, &damage_error) ||
	    !XFixesQueryExtension(dpy, &ev, &er)) {
		fprintf(stderr, "composite: need XDamage and XFixes\n");
		return 1;
	}
	if (!have_glx_ext("GLX_EXT_texture_from_pixmap")) {
		fprintf(stderr, "composite: GLX_EXT_texture_from_pixmap missing\n");
		return 1;
	}
	bind_tex_image = (PFNGLXBINDTEXIMAGEEXTPROC)
		glXGetProcAddress((const GLubyte *)"glXBindTexImageEXT");
	release_tex_image = (PFNGLXRELEASETEXIMAGEEXTPROC)
		glXGetProcAddress((const GLubyte *)"glXReleaseTexImageEXT");
	if (!bind_tex_image || !release_tex_image) {
		fprintf(stderr, "composite: cannot resolve glXBindTexImageEXT\n");
		return 1;
	}

	/* Claim the compositing-manager selection so others know we are here. */
	char sel[32];
	snprintf(sel, sizeof sel, "_NET_WM_CM_S%d", scr);
	Atom cm = XInternAtom(dpy, sel, False);
	if (XGetSelectionOwner(dpy, cm) != None) {
		fprintf(stderr, "composite: another compositor is running\n");
		return 1;
	}
	Window owner = XCreateSimpleWindow(dpy, root, -1, -1, 1, 1, 0, 0, 0);
	XSetSelectionOwner(dpy, cm, owner, CurrentTime);

	XGrabServer(dpy);
	XSelectInput(dpy, root, SubstructureNotifyMask | StructureNotifyMask);
	XCompositeRedirectSubwindows(dpy, root, CompositeRedirectManual);
	overlay = XCompositeGetOverlayWindow(dpy, root);
	/* let all pointer/keyboard input fall through the overlay */
	XserverRegion none = XFixesCreateRegion(dpy, NULL, 0);
	XFixesSetWindowShapeRegion(dpy, overlay, ShapeInput, 0, 0, none);
	XFixesDestroyRegion(dpy, none);

	GLXFBConfig fbc = overlay_fbconfig();
	if (!fbc) {
		fprintf(stderr, "composite: no double-buffered GLX config\n");
		return 1;
	}
	GLXWindow gw = glXCreateWindow(dpy, fbc, overlay, NULL);
	GLXContext ctx = glXCreateNewContext(dpy, fbc, GLX_RGBA_TYPE, NULL, True);
	if (!ctx || !glXMakeContextCurrent(dpy, gw, gw, ctx)) {
		fprintf(stderr, "composite: cannot create GL context\n");
		return 1;
	}
	fprintf(stderr, "composite: GL renderer: %s (direct=%d)\n",
		glGetString(GL_RENDERER), glXIsDirect(dpy, ctx));

	Window r, p, *kids;
	unsigned int nk;
	if (XQueryTree(dpy, root, &r, &p, &kids, &nk)) {
		for (unsigned int i = 0; i < nk; i++)	/* bottom to top */
			add_win(kids[i]);
		XFree(kids);
	}
	XUngrabServer(dpy);

	/* sigaction, not signal(): under _POSIX_C_SOURCE signal() resets the
	 * handler after one delivery, so a second SIGUSR1 would kill us. */
	struct sigaction sa;
	memset(&sa, 0, sizeof sa);
	sa.sa_handler = on_signal;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);

	while (!quit) {
		while (XPending(dpy)) {
			XEvent e;
			XNextEvent(dpy, &e);
			handle_event(&e);
		}
		if (need_paint || want_shot) {
			need_paint = 0;
			paint(gw);
		} else if (!XPending(dpy)) {
			fd_set fds;
			struct timeval tv = { 0, 200000 };
			FD_ZERO(&fds);
			FD_SET(ConnectionNumber(dpy), &fds);
			select(ConnectionNumber(dpy) + 1, &fds, NULL, NULL, &tv);
		}
	}

	while (bottom)
		remove_win(bottom->id);
	glXMakeContextCurrent(dpy, None, None, NULL);
	glXDestroyContext(dpy, ctx);
	glXDestroyWindow(dpy, gw);
	XCompositeReleaseOverlayWindow(dpy, root);
	XCompositeUnredirectSubwindows(dpy, root, CompositeRedirectManual);
	XDestroyWindow(dpy, owner);
	XSync(dpy, False);
	XCloseDisplay(dpy);
	return 0;
}
