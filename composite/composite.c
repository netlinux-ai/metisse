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
 *
 * Phase 4 remaps the pointer so tilted windows stay usable. The compositor
 * tracks a virtual pointer (where the user sees the cursor), finds the window
 * drawn under it, and warps the real X pointer to the matching point on that
 * window's flat position, so clicks, drags and wheel events arrive as genuine
 * input. The real cursor is hidden and a software cursor is drawn at the
 * virtual position. Remapping stands down while Openbox holds a pointer grab
 * (_METISSE_GRAB on the root), e.g. during a move, resize or tilt.
 *   -absolute  the pointer device reports absolute positions (e.g. Xephyr)
 *   -noanim    apply transforms instantly instead of easing
 *   -verbose   log each remapped pointer position
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
#include <time.h>
#include <sys/select.h>

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif

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
	float trx, try_, trz, tscale, topacity;	/* targets; the above ease to them */
	struct Win *next;	/* towards the top of the stack */
} Win;

static Display *dpy;
static int scr;
static Window root, overlay;
static Win *bottom;		/* bottom-most window; list runs upwards */
static int damage_event, damage_error;
static int xerr;
static Atom transform_atom, grab_atom;
static int xfixes_event;
static int grab_active;		/* Openbox holds the pointer */
static int absolute_input, verbose, no_animate;
static int cursor_hidden;
static int virt_valid;
static double vx, vy;		/* virtual pointer: where the cursor is drawn */
static int last_x, last_y;	/* where we last left the real pointer */
static struct Win *sticky;	/* window mapped for the duration of a press */
static int sticky_valid;
static GLuint cursor_tex;
static int cur_w, cur_h, cur_xhot, cur_yhot, cursor_dirty = 1;
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

static void read_transform(Win *w, int snap)
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
	w->trx = v[0] / 1000.0f;
	w->try_ = v[1] / 1000.0f;
	w->trz = v[2] / 1000.0f;
	w->tscale = v[3] / 1000.0f;
	w->topacity = v[4] < 0 ? 0 : v[4] > 1000 ? 1 : v[4] / 1000.0f;
	if (snap || no_animate) {
		w->rx = w->trx; w->ry = w->try_; w->rz = w->trz;
		w->scale = w->tscale; w->opacity = w->topacity;
	}
	need_paint = 1;
}

/* ---- easing -------------------------------------------------------------- */

#define EASE_TAU 0.07	/* seconds; each step closes 1 - exp(-dt/tau) of the gap */

static double now_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
}

static float ease1(float cur, float tgt, double k, float eps, int *moving)
{
	if (fabsf(tgt - cur) <= eps)
		return tgt;
	*moving = 1;
	return cur + (tgt - cur) * (float)k;
}

/* Advance every window towards its target; returns non-zero while any is
 * still moving. */
static int animate(void)
{
	static double last;
	double t = now_sec(), dt = last ? t - last : 0.016;
	int moving = 0;

	last = t;
	if (dt > 0.1)
		dt = 0.1;
	double k = 1.0 - exp(-dt / EASE_TAU);
	for (Win *w = bottom; w; w = w->next) {
		w->rx = ease1(w->rx, w->trx, k, 0.05f, &moving);
		w->ry = ease1(w->ry, w->try_, k, 0.05f, &moving);
		w->rz = ease1(w->rz, w->trz, k, 0.05f, &moving);
		w->scale = ease1(w->scale, w->tscale, k, 0.0005f, &moving);
		w->opacity = ease1(w->opacity, w->topacity, k, 0.002f, &moving);
	}
	if (!moving)
		last = 0;
	else
		need_paint = 1;
	return moving;
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
	w->scale = w->opacity = w->tscale = w->topacity = 1.0f;
	w->mapped = (a.map_state == IsViewable);
	w->damage = XDamageCreate(dpy, id, XDamageReportNonEmpty);
	XSelectInput(dpy, id, PropertyChangeMask);
	read_transform(w, 1);
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
	if (sticky == w)
		sticky = NULL;
	unlink_win(w);
	free_pixmap(w);
	free(w);
	need_paint = 1;
}

static void read_grab(void)
{
	Atom type;
	int fmt, was = grab_active;
	unsigned long n = 0, after;
	unsigned char *data = NULL;

	grab_active = XGetWindowProperty(dpy, root, grab_atom, 0, 1, False,
					 AnyPropertyType, &type, &fmt, &n,
					 &after, &data) == Success && n > 0;
	if (data)
		XFree(data);
	if (was != grab_active)
		virt_valid = 0;	/* resynchronise with the real pointer */
}

static void handle_event(XEvent *e)
{
	Win *w;

	if (e->type == xfixes_event + XFixesCursorNotify) {
		cursor_dirty = 1;
		need_paint = 1;
		return;
	}
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
		if (e->xproperty.window == root && e->xproperty.atom == grab_atom) {
			read_grab();
			break;
		}
		if (e->xproperty.atom == transform_atom &&
		    (w = find_win(e->xproperty.window)))
			read_transform(w, 0);
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

/* ---- 3D camera and transforms ------------------------------------------ */

typedef struct { double m[16]; } Mat4;	/* column-major, like OpenGL */

static Mat4 m_ident(void)
{
	Mat4 r;
	memset(&r, 0, sizeof r);
	r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1;
	return r;
}

static Mat4 m_mul(Mat4 a, Mat4 b)
{
	Mat4 r;
	for (int c = 0; c < 4; c++)
		for (int row = 0; row < 4; row++) {
			double sum = 0;
			for (int k = 0; k < 4; k++)
				sum += a.m[k * 4 + row] * b.m[c * 4 + k];
			r.m[c * 4 + row] = sum;
		}
	return r;
}

static Mat4 m_translate(double x, double y, double z)
{
	Mat4 r = m_ident();
	r.m[12] = x; r.m[13] = y; r.m[14] = z;
	return r;
}

static Mat4 m_scale(double x, double y, double z)
{
	Mat4 r = m_ident();
	r.m[0] = x; r.m[5] = y; r.m[10] = z;
	return r;
}

static Mat4 m_rotate(double deg, double x, double y, double z)	/* glRotate */
{
	double a = deg * 3.14159265358979 / 180.0, c = cos(a), s = sin(a);
	double n = sqrt(x * x + y * y + z * z);
	Mat4 r = m_ident();
	x /= n; y /= n; z /= n;
	r.m[0] = x * x * (1 - c) + c;     r.m[4] = x * y * (1 - c) - z * s;
	r.m[8] = x * z * (1 - c) + y * s;
	r.m[1] = y * x * (1 - c) + z * s; r.m[5] = y * y * (1 - c) + c;
	r.m[9] = y * z * (1 - c) - x * s;
	r.m[2] = x * z * (1 - c) - y * s; r.m[6] = y * z * (1 - c) + x * s;
	r.m[10] = z * z * (1 - c) + c;
	return r;
}

static Mat4 m_frustum(double l, double r, double b, double t, double n, double f)
{
	Mat4 m;
	memset(&m, 0, sizeof m);
	m.m[0] = 2 * n / (r - l);
	m.m[5] = 2 * n / (t - b);
	m.m[8] = (r + l) / (r - l);
	m.m[9] = (t + b) / (t - b);
	m.m[10] = -(f + n) / (f - n);
	m.m[11] = -1;
	m.m[14] = -2 * f * n / (f - n);
	return m;
}

/*
 * Perspective camera chosen so that the z=0 plane maps 1:1 to X pixels:
 * the eye sits at distance d from the screen plane with a vertical field of
 * view of FOV, and y is flipped because X's y axis points down. Drawing and
 * pointer hit-testing both use these matrices, so they cannot disagree.
 */
#define FOV_DEG 40.0
static void camera(int vw, int vh, Mat4 *proj, Mat4 *view)
{
	const double n = 10.0, f = 10000.0;
	double half = tan(FOV_DEG * 3.14159265358979 / 360.0);
	double d = (vh / 2.0) / half;
	double top = n * half, right = top * vw / (double)vh;
	*proj = m_frustum(-right, right, -top, top, n, f);
	*view = m_mul(m_translate(0, 0, -d),
		      m_mul(m_scale(1, -1, 1), m_translate(-vw / 2.0, -vh / 2.0, 0)));
}

static void setup_camera(int vw, int vh)
{
	Mat4 p, v;
	camera(vw, vh, &p, &v);
	glViewport(0, 0, vw, vh);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(p.m);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(v.m);
}

static int win_is_tilted(const Win *w)
{
	return fabsf(w->rx) > 1e-3f || fabsf(w->ry) > 1e-3f ||
	       fabsf(w->rz) > 1e-3f || fabsf(w->scale - 1.0f) > 1e-3f;
}

/* The window's own transform, applied about its centre. */
static Mat4 win_model(const Win *w)
{
	double fw = w->w + 2 * w->bw, fh = w->h + 2 * w->bw;
	double cx = w->x + fw / 2, cy = w->y + fh / 2;
	Mat4 m = m_translate(cx, cy, 0);
	m = m_mul(m, m_rotate(w->rz, 0, 0, 1));
	m = m_mul(m, m_rotate(w->ry, 0, 1, 0));
	m = m_mul(m, m_rotate(w->rx, 1, 0, 0));
	m = m_mul(m, m_scale(w->scale, w->scale, w->scale));
	return m_mul(m, m_translate(-cx, -cy, 0));
}

static void draw_window(Win *w)
{
	if (!update_texture(w))
		return;
	float fw = w->w + 2 * w->bw, fh = w->h + 2 * w->bw;
	float t0 = w->inverted ? 1.0f : 0.0f, t1 = 1.0f - t0;
	Mat4 model = win_model(w);

	glPushMatrix();
	glMultMatrixd(model.m);
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

/* ---- pointer remapping -------------------------------------------------- */

/* Solve the 8x8 system in a (augmented with a ninth column). */
static int solve8(double a[8][9])
{
	for (int col = 0; col < 8; col++) {
		int piv = col;
		for (int r = col + 1; r < 8; r++)
			if (fabs(a[r][col]) > fabs(a[piv][col]))
				piv = r;
		if (fabs(a[piv][col]) < 1e-12)
			return 0;
		if (piv != col)
			for (int k = 0; k < 9; k++) {
				double t = a[col][k]; a[col][k] = a[piv][k]; a[piv][k] = t;
			}
		for (int k = 8; k >= col; k--)
			a[col][k] /= a[col][col];
		for (int r = 0; r < 8; r++) {
			if (r == col)
				continue;
			double f = a[r][col];
			for (int k = col; k < 9; k++)
				a[r][k] -= f * a[col][k];
		}
	}
	return 1;
}

/* Homography h (row-major 3x3, h[8] = 1) taking src[i] to dst[i], i < 4. */
static int fit_homography(const double src[4][2], const double dst[4][2],
			  double h[9])
{
	double a[8][9];
	for (int i = 0; i < 4; i++) {
		double u = src[i][0], v = src[i][1], x = dst[i][0], y = dst[i][1];
		double r0[9] = { u, v, 1, 0, 0, 0, -u * x, -v * x, x };
		double r1[9] = { 0, 0, 0, u, v, 1, -u * y, -v * y, y };
		memcpy(a[2 * i], r0, sizeof r0);
		memcpy(a[2 * i + 1], r1, sizeof r1);
	}
	if (!solve8(a))
		return 0;
	for (int i = 0; i < 8; i++)
		h[i] = a[i][8];
	h[8] = 1;
	return 1;
}

static int invert3(const double h[9], double r[9])
{
	double c0 = h[4] * h[8] - h[5] * h[7], c1 = h[5] * h[6] - h[3] * h[8];
	double c2 = h[3] * h[7] - h[4] * h[6];
	double det = h[0] * c0 + h[1] * c1 + h[2] * c2;
	if (fabs(det) < 1e-18)
		return 0;
	r[0] = c0 / det; r[1] = (h[2] * h[7] - h[1] * h[8]) / det;
	r[2] = (h[1] * h[5] - h[2] * h[4]) / det;
	r[3] = c1 / det; r[4] = (h[0] * h[8] - h[2] * h[6]) / det;
	r[5] = (h[2] * h[3] - h[0] * h[5]) / det;
	r[6] = c2 / det; r[7] = (h[1] * h[6] - h[0] * h[7]) / det;
	r[8] = (h[0] * h[4] - h[1] * h[3]) / det;
	return 1;
}

/* Where the screen point (px, py) lands on window w, in window-local pixels.
 * Returns 0 if the window cannot be inverted. Does not check bounds. */
static int win_unproject(const Win *w, int vw, int vh, double px, double py,
			 double *u, double *v)
{
	double fw = w->w + 2 * w->bw, fh = w->h + 2 * w->bw;
	if (!win_is_tilted(w)) {
		*u = px - w->x;
		*v = py - w->y;
		return 1;
	}
	Mat4 p, view;
	camera(vw, vh, &p, &view);
	Mat4 pv = m_mul(m_mul(p, view), win_model(w));
	const double src[4][2] = { {0, 0}, {fw, 0}, {fw, fh}, {0, fh} };
	double dst[4][2];
	for (int i = 0; i < 4; i++) {
		double in[4] = { w->x + src[i][0], w->y + src[i][1], 0, 1 };
		double c[4];
		for (int r = 0; r < 4; r++)
			c[r] = pv.m[r] * in[0] + pv.m[4 + r] * in[1] +
			       pv.m[8 + r] * in[2] + pv.m[12 + r] * in[3];
		if (c[3] <= 1e-9)
			return 0;	/* corner behind the camera */
		dst[i][0] = (c[0] / c[3] + 1) / 2 * vw;
		dst[i][1] = (1 - c[1] / c[3]) / 2 * vh;
	}
	double h[9], hi[9];
	if (!fit_homography(src, dst, h) || !invert3(h, hi))
		return 0;
	double X = hi[0] * px + hi[1] * py + hi[2];
	double Y = hi[3] * px + hi[4] * py + hi[5];
	double W = hi[6] * px + hi[7] * py + hi[8];
	if (fabs(W) < 1e-12)
		return 0;
	*u = X / W;
	*v = Y / W;
	return 1;
}

/* Topmost window drawn under the screen point; fills the local position. */
static Win *locate(int vw, int vh, double px, double py, double *u, double *v)
{
	Win *hit = NULL;
	for (Win *w = bottom; w; w = w->next) {
		double lu, lv;
		if (!w->mapped || !win_unproject(w, vw, vh, px, py, &lu, &lv))
			continue;
		if (lu >= 0 && lv >= 0 && lu < w->w + 2 * w->bw &&
		    lv < w->h + 2 * w->bw) {
			hit = w;	/* later windows are higher up */
			*u = lu;
			*v = lv;
		}
	}
	return hit;
}

static int count_tilted(void)
{
	int n = 0;
	for (Win *w = bottom; w; w = w->next)
		if (w->mapped && win_is_tilted(w))
			n++;
	return n;
}

static double clampd(double x, double lo, double hi)
{
	return x < lo ? lo : x > hi ? hi : x;
}

/*
 * Track the virtual pointer and keep the real pointer on the matching point
 * of whichever window is drawn under it.
 *
 * The device is assumed relative: the virtual pointer moves by however far
 * the real pointer travelled since we last placed it. With -absolute the
 * virtual pointer is taken to be wherever the device last reported.
 */
static void pointer_update(void)
{
	Window rw, cw;
	int rx, ry, wx, wy;
	unsigned int mask;
	int sw = DisplayWidth(dpy, scr), sh = DisplayHeight(dpy, scr);
	double ovx = vx, ovy = vy;

	if (!XQueryPointer(dpy, root, &rw, &cw, &rx, &ry, &wx, &wy, &mask))
		return;
	if (!virt_valid || grab_active) {
		/* no remapping while Openbox itself drives the pointer */
		vx = rx; vy = ry;
		last_x = rx; last_y = ry;
		virt_valid = 1;
		sticky_valid = 0;
		sticky = NULL;
		if (vx != ovx || vy != ovy)
			need_paint = 1;
		return;
	}
	if (rx != last_x || ry != last_y) {
		if (absolute_input) {
			vx = rx; vy = ry;
		} else {
			vx += rx - last_x;
			vy += ry - last_y;
		}
		vx = clampd(vx, 0, sw - 1);
		vy = clampd(vy, 0, sh - 1);
	}

	unsigned int btn = mask & (Button1Mask | Button2Mask | Button3Mask |
				   Button4Mask | Button5Mask);
	double u = vx, v = vy, lu, lv;
	Win *under = NULL;
	if (btn && sticky_valid) {
		under = sticky;	/* a press keeps its window until released */
	} else {
		under = locate(sw, sh, vx, vy, &lu, &lv);
		if (btn) {
			sticky = under;
			sticky_valid = 1;
		}
	}
	if (!btn)
		sticky_valid = 0;
	if (under && win_unproject(under, sw, sh, vx, vy, &lu, &lv)) {
		u = under->x + lu;
		v = under->y + lv;
	}
	int tx = (int)lround(clampd(u, 0, sw - 1));
	int ty = (int)lround(clampd(v, 0, sh - 1));
	if (tx != rx || ty != ry) {
		XWarpPointer(dpy, None, root, 0, 0, 0, 0, tx, ty);
		XFlush(dpy);
		if (verbose)
			fprintf(stderr, "composite: virtual (%.0f,%.0f) -> "
				"real (%d,%d) window 0x%lx\n", vx, vy, tx, ty,
				under ? under->id : 0UL);
	}
	last_x = tx;
	last_y = ty;
	if (vx != ovx || vy != ovy)
		need_paint = 1;
}

/* ---- software cursor ---------------------------------------------------- */

static void refresh_cursor(void)
{
	XFixesCursorImage *ci = XFixesGetCursorImage(dpy);
	cursor_dirty = 0;
	if (!ci)
		return;
	size_t n = (size_t)ci->width * ci->height;
	uint32_t *px = malloc(n * sizeof *px);
	if (px) {
		for (size_t i = 0; i < n; i++)
			px[i] = (uint32_t)ci->pixels[i];	/* premultiplied ARGB */
		if (!cursor_tex)
			glGenTextures(1, &cursor_tex);
		glBindTexture(GL_TEXTURE_2D, cursor_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ci->width, ci->height, 0,
			     GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, px);
		cur_w = ci->width;
		cur_h = ci->height;
		cur_xhot = ci->xhot;
		cur_yhot = ci->yhot;
		free(px);
	}
	XFree(ci);
}

static void draw_cursor(void)
{
	if (!cursor_hidden || !virt_valid)
		return;
	if (cursor_dirty || !cursor_tex)
		refresh_cursor();
	if (!cursor_tex)
		return;
	float x0 = (float)vx - cur_xhot, y0 = (float)vy - cur_yhot;
	glBindTexture(GL_TEXTURE_2D, cursor_tex);
	glColor4f(1, 1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(x0, y0);
	glTexCoord2f(1, 0); glVertex2f(x0 + cur_w, y0);
	glTexCoord2f(1, 1); glVertex2f(x0 + cur_w, y0 + cur_h);
	glTexCoord2f(0, 1); glVertex2f(x0, y0 + cur_h);
	glEnd();
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
	draw_cursor();
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
		else if (!strcmp(argv[i], "-absolute"))
			absolute_input = 1;
		else if (!strcmp(argv[i], "-noanim"))
			no_animate = 1;
		else if (!strcmp(argv[i], "-verbose"))
			verbose = 1;
		else {
			fprintf(stderr, "usage: %s [-display D] [-shot F.ppm] [-absolute] [-noanim] [-verbose]\n", argv[0]);
			return 2;
		}
	}
	if (!(dpy = XOpenDisplay(dname))) {
		fprintf(stderr, "composite: cannot open display\n");
		return 1;
	}
	scr = DefaultScreen(dpy);
	transform_atom = XInternAtom(dpy, "_METISSE_TRANSFORM", False);
	grab_atom = XInternAtom(dpy, "_METISSE_GRAB", False);
	root = RootWindow(dpy, scr);
	XSetErrorHandler(on_xerror);

	int ev, er, maj = 0, min = 0;
	if (!XCompositeQueryExtension(dpy, &ev, &er) ||
	    (XCompositeQueryVersion(dpy, &maj, &min), maj == 0 && min < 4)) {
		fprintf(stderr, "composite: need XComposite >= 0.4\n");
		return 1;
	}
	if (!XDamageQueryExtension(dpy, &damage_event, &damage_error) ||
	    !XFixesQueryExtension(dpy, &xfixes_event, &er)) {
		fprintf(stderr, "composite: need XDamage and XFixes\n");
		return 1;
	}
	XFixesQueryVersion(dpy, &maj, &min);
	if (maj < 4) {
		fprintf(stderr, "composite: need XFixes >= 4 (cursor hiding)\n");
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
	XSelectInput(dpy, root, SubstructureNotifyMask | StructureNotifyMask |
		     PropertyChangeMask);
	XFixesSelectCursorInput(dpy, root, XFixesDisplayCursorNotifyMask);
	read_grab();
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
		/* Any tilted window means the real pointer must be remapped. */
		int tilted = count_tilted();
		if (tilted && !cursor_hidden) {
			XFixesHideCursor(dpy, root);
			cursor_hidden = 1;
			virt_valid = 0;
			need_paint = 1;
		} else if (!tilted && cursor_hidden) {
			XFixesShowCursor(dpy, root);
			cursor_hidden = 0;
			virt_valid = 0;
			need_paint = 1;
		}
		int moving = animate();
		tilted = count_tilted();
		if (tilted)
			pointer_update();
		if (need_paint || want_shot) {
			need_paint = 0;
			paint(gw);
		} else if (!XPending(dpy)) {
			fd_set fds;
			/* poll the pointer quickly only while remapping it */
			struct timeval tv = { 0, moving ? 8000 : tilted ? 8000 : 200000 };
			FD_ZERO(&fds);
			FD_SET(ConnectionNumber(dpy), &fds);
			select(ConnectionNumber(dpy) + 1, &fds, NULL, NULL, &tv);
		}
	}

	if (cursor_hidden)
		XFixesShowCursor(dpy, root);
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
