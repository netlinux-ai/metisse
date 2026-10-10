/*
 * Copyright (C) 2003 Olivier Chapuis
 *
 *
 */

/*
 * Generic rootless to Aqua specific glue code
 *
 * This code acts as a glue between the generic rootless X server code
 * and the Aqua specific implementation, which includes definitions that
 * conflict with stardard X types.
 *
 * Greg Parker     gparker@cs.stanford.edu
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MAX_NUMBER_OF_RECTANGLE 256

#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
#include <errno.h>

#include "rootlessConfig.h"
#include "rootlessCommon.h"
#include "rootless.h"

#include "regionstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "globals.h" // dixScreenOrigins[]
#include "propertyst.h"
#include "inputstr.h" //inputInfo

#include <X11/X.h>
#include <X11/Xatom.h>

#include "rfb.h"
#include "mipointer.h"

#define TRACE_RFBROOTLESS 0

/*
 * Private data of the rootless glue.
 */

typedef struct _WindowClients {
	struct _WindowClients *next;

	rfbClientPtr cl;

	RegionRec modifiedRegion;
	int shapeChanged;

	Bool firstTime;
	Bool updateRequested;

} WindowClients;

struct _rfbWindowRec {
	rfbWindowRec *next;      /* doubly linked list */

	WindowClients *clients;

	char *pixelData;
	int bytesPerRow;

	RegionRec bShape;  /* bounding shape for shaped window */

	RootlessWindowPtr frame;     /* backpointer to frame */
};


/*
 * List of all Windows created by rfb.
 */
static rfbWindowRec *Windows = NULL;


/*
 * manage window client
 */
static
WindowClients *add_client(WindowClients *clients, rfbClientPtr cl)
{
	WindowClients *t,*prev = NULL;
	WindowClients *new;
	//BoxRec box;

	t = clients;
	while(t != NULL)
	{
		if (t->cl == cl)
		{
			/* it's already there, do nothing */
			return clients;
		}
		prev = t;
		t = t->next;
	}

	new = (WindowClients *)malloc(sizeof(WindowClients));
	new->cl = cl;
	new->next = NULL;

	REGION_INIT(pScreen,&new->modifiedRegion,NullBox,0);

	new->firstTime = TRUE;
	new->shapeChanged = 0;
	new->updateRequested = FALSE;

	if (clients == NULL)
	{
		return new;
	}
	prev->next = new;
	return clients;
}

static
WindowClients *remove_client(WindowClients *clients, rfbClientPtr cl) {
	WindowClients  *tl = clients;
	WindowClients *prev = NULL;

	while (tl && tl->cl != cl)
	{
		prev = tl;
		tl = tl->next;
	}

	if (tl == NULL)
	{
		return clients;
	}

	if (prev)
	{
		prev->next = tl->next;
	}
	if (clients == tl)
	{
		clients = clients->next;
	}
	free(tl);

	return clients;
}

static
WindowClients *find_client(rfbWindowRec *rw, rfbClientPtr cl)
{
	WindowClients *t = rw->clients;

	while(t!= NULL && t->cl != cl)
	{
		t = t->next;
	}
	return t;
}

/* ************************************************************************* */

static
rfbWindowRec *add_window(RootlessWindowPtr pFrame) {
	rfbWindowRec *t,**prev;
	rfbClientPtr cl;

	t = Windows;
	prev = &Windows;

	while(t!= NULL)
	{
		if (t->frame == pFrame)
		{
			/* it's already there, do nothing */
			return NULL;
		}
		prev = &(t->next);
		t = t->next;
	}
	*prev = (rfbWindowRec *)malloc(sizeof(rfbWindowRec));
	(*prev)->frame = pFrame;
	(*prev)->clients = NULL;
	(*prev)->pixelData = NULL;
	(*prev)->next = NULL;
	for (cl = rfbClientHead; cl; cl = cl->next)
	{
		(*prev)->clients = add_client((*prev)->clients, cl);
	}
	return (*prev);
}

static
void remove_window(rfbWindowRec *rw)
{
	rfbWindowRec *t,**prev;

	t = Windows;
	prev = &Windows;
	while((t!= NULL)&&(t != rw))
	{
		prev = &(t->next);
		t = t->next;
	}
	if(t!= NULL)
	{
		if(prev != NULL)
		{
			*prev = t->next;
		}

		while(t->clients)
		{
			t->clients = remove_client(t->clients, t->clients->cl);
		}
		free(t);
	}
}

static
unsigned long getrfbFlags(
	WindowPtr w, Window *transientFor, Window *umFor, Window *grabWin,
	Window *dupFor, Window *cutClient, unsigned long *facadeReal,
	rfb_facades_prop_t *rfp)
{
	unsigned long flags = 0;
	PropertyPtr pProp;
	GrabPtr grab = inputInfo.pointer->grab;
	WindowPtr winProp = w;
	static Atom netCheckingAtom = None;
	static Atom netWindowTypeAtom = None;
	static Atom netEwmhDesktopAtom = None;
	static Atom netUmForAtom = None;
	static Atom netUmWindowType = None;
	static Atom netUmWindowTypeToolTips = None;
	static Atom metisseNetWMDuplicate = None;
	static Atom metisseNetWMFacade = None;
	static Atom metisseNetWMFacadeSaved = None;

	if (w->overrideRedirect)
	{
		flags |= rfbWindowFlagsOverrideRedirect;
		flags |= rfbWindowFlagsUnmanaged;
	}
	if (w->drawable.class == InputOnly)
	{
		flags |= rfbWindowFlagsInputOnly;
	}
	/* FIXME ? */
	if (!w->firstChild)
	{
		flags |= rfbWindowFlagsUnmanaged;
	}

	if (netCheckingAtom == None)
	{
		netCheckingAtom = MakeAtom(
			"_NET_SUPPORTING_WM_CHECK", 24, TRUE);
	}
	if (netWindowTypeAtom == None)
	{
		netWindowTypeAtom = MakeAtom(
			"_NET_WM_WINDOW_TYPE", 19, TRUE);
	}
	if (netEwmhDesktopAtom == None)
	{
		netEwmhDesktopAtom = MakeAtom(
			"_NET_WM_WINDOW_TYPE_DESKTOP", 27, TRUE);
	}
	if (netUmForAtom == None)
	{
		netUmForAtom = MakeAtom(
			"_NET_UM_FOR", 11, TRUE);
	}
	if (netUmWindowType == None)
	{
		netUmWindowType = MakeAtom(
			"_NET_UM_WINDOW_TYPE", 19, TRUE);
	}
	if (netUmWindowTypeToolTips == None)
	{
		netUmWindowTypeToolTips = MakeAtom(
			"_NET_UM_WINDOW_TYPE_TOOLTIPS", 28, TRUE);
	}
	if (metisseNetWMDuplicate  == None)
	{
		metisseNetWMDuplicate = MakeAtom(
			"_METISSE_NET_WM_DUPLICATE", 25, TRUE);
	}
	if (metisseNetWMFacade  == None)
	{
		metisseNetWMFacade = MakeAtom(
			"_METISSE_NET_WM_FACADE", 22, TRUE);
	}
	if (metisseNetWMFacadeSaved  == None)
	{
		metisseNetWMFacadeSaved = MakeAtom(
			"_METISSE_NET_WM_FACADE_SAVED", 28, TRUE);
	}

	if (w->firstChild)
	{
		if (w->firstChild->firstChild)
		{
			pProp = wUserProps(w->firstChild->firstChild);
			winProp = w->firstChild->firstChild;
		}
		else
		{
			pProp = wUserProps(w->firstChild);
			winProp = w->firstChild;
		}
	}
	else
	{
		pProp = wUserProps(w);
		winProp = w;
	}

	*umFor = 0;
	*cutClient = 0;
	*grabWin = 0;
	*dupFor = 0;
	*facadeReal = 0;

	while(pProp)
	{
		if (pProp->propertyName == netCheckingAtom)
		{
			flags |= rfbWindowFlagsNetChecking;
		}
		if (pProp->propertyName == netWindowTypeAtom)
		{
			int i = 0;
			int found = 0;
			int size = (pProp->format/8) * pProp->size;
			Atom *data = (Atom *)pProp->data;
			while(i < size && !found)
			{
				if (data[i] == netEwmhDesktopAtom)
				{
					found = 1;
				}
				i++;
			}
			if (found)
			{
				flags |= rfbWindowFlagsEwmhDesktop;
			}
		}
		if (pProp->propertyName == XA_WM_TRANSIENT_FOR && pProp->size)
		{
			Window *data = (Window *)pProp->data;
			Window w;
			WindowPtr transWin;
			WindowPtr topTransWin;

			w = data[0];
			transWin = (WindowPtr)SecurityLookupIDByType(
				NullClient, w, RT_WINDOW, SecurityReadAccess);
			if (transWin)
			{
				flags |= rfbWindowFlagsTransient;
				topTransWin = TopLevelParent(transWin);
				*transientFor = topTransWin->drawable.id;
			}
		}
		if (pProp->propertyName == netUmForAtom && pProp->size)
		{
			Window *data = (Window *)pProp->data;
			Window w;
			WindowPtr umWin;
			WindowPtr topUmWin;

			flags |= rfbWindowFlagsUnmanaged;
			w = data[0];
			umWin = (WindowPtr)SecurityLookupIDByType(
				NullClient, w, RT_WINDOW, SecurityReadAccess);
			if (umWin)
			{
				flags |= rfbWindowFlagsUnmanaged;
				topUmWin = TopLevelParent(umWin);
				*umFor = topUmWin->drawable.id;
			}
		}
		if (pProp->propertyName == netUmWindowType && pProp->size)
		{
			Atom *data = (Atom *)pProp->data;
			
			flags |= rfbWindowFlagsUnmanaged;
			if (data[0] == netUmWindowTypeToolTips)
			{
				flags |= rfbWindowFlagsToolTips;
			}
		}
		if (pProp->propertyName == metisseNetWMFacade && pProp->size)
		{
			CARD32 *data = (CARD32 *)pProp->data;
			Window w;
			WindowPtr dupWin;
			WindowPtr topDupWin;
			w = data[0];
			dupWin = (WindowPtr)SecurityLookupIDByType(
				NullClient, w, RT_WINDOW, SecurityReadAccess);
			if (dupWin)
			{
				topDupWin = TopLevelParent(dupWin);
				*dupFor = topDupWin->drawable.id;
			}
			*cutClient = winProp->drawable.id;
			flags |= rfbWindowFlagsFacade;
			rfp->size = pProp->size;
			if (pProp->size > 0)
			{
				rfp->data = (CARD32 *)xalloc(pProp->size*4);
			}
			else
			{
				rfp->data = NULL;
			}
			memcpy((char *)rfp->data, (char *)data, (pProp->size*4));
		}
		if (pProp->propertyName == metisseNetWMFacadeSaved)
		{
			CARD32 *data = (CARD32 *)pProp->data;
			flags |= rfbWindowFlagsFacadeSaved;
			*facadeReal = data[0];
		}
		if (pProp->propertyName == metisseNetWMDuplicate && pProp->size)
		{
			CARD32 *data = (CARD32 *)pProp->data;
			Window w;
			WindowPtr dupWin;
			WindowPtr topDupWin;

			w = data[0];
			dupWin = (WindowPtr)SecurityLookupIDByType(
				NullClient, w, RT_WINDOW, SecurityReadAccess);
			if (dupWin)
			{
				flags |= rfbWindowFlagsDuplicate;
				topDupWin = TopLevelParent(dupWin);
				*dupFor = topDupWin->drawable.id;
			}
		}
		pProp = pProp->next;
	}

	*grabWin = 0;
	if (grab && grab->window)
	{
		WindowPtr tlpWin =  TopLevelParent(grab->window);
		if (!IsRoot(tlpWin))
		{
			*grabWin = tlpWin->drawable.id;
		}
	}

	return flags;
}

/* ************************************************************************** 
 *
 *
 *
 * ************************************************************************** */

void window_add_client(rfbClientPtr cl) {
	rfbWindowRec *w;

	for(w = Windows; w != NULL; w = w->next) {
		w->clients = add_client(w->clients, cl);
	}
}

void window_remove_client(rfbClientPtr cl) {
	rfbWindowRec *w;

	for(w = Windows; w != NULL; w = w->next) {
		w->clients = remove_client(w->clients, cl);
	}
}

void window_set_updateRequested(rfbClientPtr cl)
{
	rfbWindowRec *w;
	WindowClients *c;

	for(w = Windows; w != NULL; w = w->next) {
		if ((c = find_client(w, cl)) != NULL)
		{
			c->updateRequested = TRUE;
		}
	}
}

void window_union_modifiedRegion(
	rfbClientPtr cl, rfbWindowRec *w, RegionRec *reg)
{
	WindowClients *c;

	if ((c = find_client(w, cl)) == NULL)
	{
		return;
	}
	REGION_UNION(pScreen,&c->modifiedRegion,&c->modifiedRegion, reg);
}

void window_union_modifiedRegion_allwin(rfbClientPtr cl, RegionRec *reg)
{
	rfbWindowRec *w;

	for(w = Windows; w != NULL; w = w->next)
	{
		window_union_modifiedRegion(cl, w, reg);
	}
}

void
window_region_uninit(rfbClientPtr cl)
{
	rfbWindowRec *w;
	WindowClients *c;

	for(w = Windows; w != NULL; w = w->next) {
		if ((c = find_client(w, cl)) != NULL)
		{
			REGION_UNINIT(pScreen, &c->modifiedRegion);
		}
	}
}

void
window_region_init(rfbClientPtr cl, BoxRec *box)
{
	rfbWindowRec *w;
	WindowClients *c;

	for(w = Windows; w != NULL; w = w->next) {
		if ((c = find_client(w, cl)) != NULL)
		{
			REGION_INIT(pScreen, &c->modifiedRegion,box,0);
		}
	}
}

/* ************************************************************************** 
 *
 *
 *
 * ************************************************************************** */

static
Bool window_MakeWindowShapeUpdate(
	rfbClientPtr cl, ScreenPtr pScreen,  rfbWindowRec *rw)
{
	rfbFramebufferUpdateRectHeader *rect;
	rfbWindowShapeHeader *sh;
	int i;

	unsigned int nRects = REGION_NUM_RECTS(&rw->bShape);

	if (ublen + sz_rfbFramebufferUpdateRectHeader + sz_rfbWindowShapeHeader
		> UPDATE_BUF_SIZE)
	{
		if (!rfbSendUpdateBuf(cl))
			return FALSE;
	}
	rect = (rfbFramebufferUpdateRectHeader*)&updateBuf[ublen];
	ublen += sz_rfbFramebufferUpdateRectHeader;
	sh = (rfbWindowShapeHeader*)&updateBuf[ublen];
	ublen += sz_rfbWindowShapeHeader;

	rect->encoding = Swap32IfLE(rfbEncodingWindowShape);
	rect->r.x = 0;
	rect->r.y = 0;
	rect->r.w = Swap16IfLE(rw->frame->width);
	rect->r.h = Swap16IfLE(rw->frame->height);

	sh->nrects = Swap32IfLE(nRects);

	if (nRects == 0)
	{
		return TRUE;
	}

	for (i = 0; i < nRects; i++)
	{
		if (UPDATE_BUF_SIZE - ublen < sz_rfbRectangle)
		{
			if (!rfbSendUpdateBuf(cl))
				return FALSE;
		}

		rfbRectangle *r = (rfbRectangle*)&updateBuf[ublen];
		ublen += sz_rfbRectangle;

		r->x = (CARD16)REGION_RECTS(&rw->bShape)[i].x1;
		r->y = (CARD16)REGION_RECTS(&rw->bShape)[i].y1;
		r->w = (CARD16)REGION_RECTS(&rw->bShape)[i].x2 - r->x;
		r->h = (CARD16)REGION_RECTS(&rw->bShape)[i].y2 - r->y;
		
		if (cl->format.bigEndian != rfbServerFormat.bigEndian)
		{
			r->x = Swap16(r->x);
			r->y = Swap16(r->y);
			r->w = Swap16(r->w);
			r->h = Swap16(r->h);
		}
	}
	
	cl->rfbWindowShapeUpdatesSent++;
	cl->rfbWindowShapeBytesSent += nRects * sz_rfbRectangle;
	return TRUE;
}


static
Bool window_SendWindowShapeUpdate(WindowClients *c, rfbWindowRec *rw)
{
	ScreenPtr pScreen = screenInfo.screens[0];
	rfbFramebufferUpdateMsg *fu = (rfbFramebufferUpdateMsg *)updateBuf;

	if (!((c->cl)->enableShapeUpdate || !c->shapeChanged))
	{
		return TRUE;
	}

	c->shapeChanged = 0;
	c->cl->rfbFramebufferUpdateMessagesSent++;
	
	fu->type = rfbFramebufferUpdate;
	fu->window = Swap32IfLE((CARD32)rw->frame->win->drawable.id);
	if (IsRoot(rw->frame->win))
		fu->topWindow = 0;
	else
		fu->topWindow = Swap32IfLE((CARD32)rw->frame->win->drawable.id);

	fu->nRects = Swap16IfLE(1);
	fu->shmid = 0;
#if !defined(NO_RFB_PADDING_IN_METISSE)
	fu->pad = 0;
#endif
	ublen = sz_rfbFramebufferUpdateMsg;

	if (!window_MakeWindowShapeUpdate(c->cl, pScreen,  rw))
		return FALSE;

	if (!rfbSendUpdateBuf(c->cl))
		return FALSE;

	return TRUE;
}

static
void window_shapeChanged(rfbWindowRec *w)
{
	WindowClients *c;
	rfbClientPtr cl;

	for (cl = rfbClientHead; cl; cl = cl->next)
	{
		if ((c = find_client(w, cl)) == NULL)
		{
			continue;
		}
		c->shapeChanged = 1;
		window_SendWindowShapeUpdate(c, w);
	}
}


static
Bool window_sendFacadesUpdate(
	rfbClientPtr cl, rfbWindowRec *rw, rfb_facades_prop_t *rfp)
{
  // ScreenPtr pScreen = screenInfo.screens[0];
	rfbFramebufferUpdateMsg *fu = (rfbFramebufferUpdateMsg *)updateBuf;
	rfbFramebufferUpdateRectHeader rect;
	rfbFacadesHeader fh;
	int i,size;
	char *buf;
	CARD32 *data;

	if (!(cl->enableFacadesEncoding))
	{
		return TRUE;
	}

	fu->type = rfbFramebufferUpdate;
	fu->window = Swap32IfLE((CARD32)rw->frame->win->drawable.id);
	if (IsRoot(rw->frame->win))
		fu->topWindow = 0;
	else
		fu->topWindow = Swap32IfLE((CARD32)rw->frame->win->drawable.id);

	fu->nRects = Swap16IfLE(1);
	fu->shmid = 0;
#if !defined(NO_RFB_PADDING_IN_METISSE)
	fu->pad = 0;
#endif
	ublen = sz_rfbFramebufferUpdateMsg;

	rect.encoding = Swap32IfLE(rfbEncodingFacades);

	/* not useful any way  */
	rect.r.x = Swap16IfLE(0);
	rect.r.y = Swap16IfLE(0);
	rect.r.w = Swap16IfLE(rw->frame->width);
	rect.r.h = Swap16IfLE(rw->frame->height);

	memmove(
		&updateBuf[ublen],(char *)&rect,
		sz_rfbFramebufferUpdateRectHeader);

	ublen += sz_rfbFramebufferUpdateRectHeader;

	fh.ncuts = Swap32IfLE(rfp->size/7);
	memmove(&updateBuf[ublen],(char *)&fh, sz_rfbFacadesHeader);
	ublen += sz_rfbFacadesHeader;

	cl->rfbWindowFacadesUpdatesSent++;

	size = rfp->size * sizeof(CARD32);
	if (size > 0)
	{
		if (cl->format.bigEndian != rfbServerFormat.bigEndian)
		{
			buf = (char *)malloc(size);
			data = (CARD32 *)buf;
	
			for (i = 0; i < rfp->size ; i++)
			{
				data[i] = Swap32IfLE(rfp->data[i]);
			}
			memmove(&updateBuf[ublen],(char *)buf, size);
			free(buf);
		}
		else
		{
			memcpy(&updateBuf[ublen], (char *)rfp->data, size);
		}
		ublen += size;

		cl->rfbWindowFacadesBytesSent += size;
	}

	if (!rfbSendUpdateBuf(cl))
	{
		return FALSE;
	}

	return TRUE;
}

/* ************************************************************************** 
 *
 *
 *
 * ************************************************************************** */

static
Bool my_rfbSendFramebufferUpdate(WindowClients *c, rfbWindowRec *rw)
{
    ScreenPtr pScreen = screenInfo.screens[0];
    int i;
    CARD16 nUpdateRegionRects;
    rfbFramebufferUpdateMsg *fu = (rfbFramebufferUpdateMsg *)updateBuf;
    RegionRec updateRegion;
    Bool sendCursorShape = FALSE;
    Bool sendCursorPos = FALSE;
    Bool sendWindowShape = FALSE;
    int shmid = 0;
    char *shm = NULL;
    unsigned int shmsize = 0;

#if 0
    /* do this in cursor.c and dispcur.c only ... info can be not yet
     * valide! */
    if (c->cl->enableCursorShapeUpdates)
    {
	    if (!rfbScreen.cursorIsDrawn && c->cl->cursorWasChanged)
	    {
		    sendCursorShape = TRUE;
	    }
    }


    if (0 && c->cl->enableCursorPosUpdates && c->cl->cursorWasMoved)
    {
	    sendCursorPos = TRUE;
    }
#endif

    if ((c->cl)->enableShapeUpdate && c->shapeChanged)
    {
	    sendWindowShape = TRUE;
    }

    REGION_INIT(pScreen,&updateRegion,NullBox,0);
    REGION_UNION(pScreen, &updateRegion, &updateRegion, &c->modifiedRegion);
    if (!(REGION_NOTEMPTY(pScreen,&updateRegion) && c->updateRequested) &&
	!sendCursorShape && !sendCursorPos)
    {
	    REGION_UNINIT(pScreen,&updateRegion);
	    return TRUE;
    }

    /*
     * Now send the update.
     */

    nUpdateRegionRects = 0;

    if (REGION_NOTEMPTY(pScreen,&updateRegion) && c->updateRequested)
    {
	    if (c->cl->preferredEncoding == rfbEncodingCoRRE) {

		    for (i = 0; i < REGION_NUM_RECTS(&updateRegion); i++) {
			    int x = REGION_RECTS(&updateRegion)[i].x1;
			    int y = REGION_RECTS(&updateRegion)[i].y1;
			    int w = REGION_RECTS(&updateRegion)[i].x2 - x;
			    int h = REGION_RECTS(&updateRegion)[i].y2 - y;
			    nUpdateRegionRects += 
				    (((w-1) / c->cl->correMaxWidth + 1)
				     * ((h-1) / c->cl->correMaxHeight + 1));
		    }
	    } else {
		    nUpdateRegionRects = REGION_NUM_RECTS(&updateRegion);
	    }
    }

    /* some clients do not like too many rectangles at once (subTexture) */
    if (c->cl->preferredEncoding != rfbEncodingCoRRE &&
	nUpdateRegionRects >= MAX_NUMBER_OF_RECTANGLE)
    {
	    BoxRec box;
	    BoxPtr pb;
	    
	    /* fprintf(stderr,"Lot of Rectangles %i\n", nUpdateRegionRects); */
	    pb = REGION_EXTENTS(pScreen, &updateRegion);
	    box.x1 = pb->x1;
	    box.y1 = pb->y1;
	    box.x2 = pb->x2;
	    box.y2 = pb->y2;
	    REGION_UNINIT(pScreen,&updateRegion);
	    nUpdateRegionRects = 1;
	    REGION_INIT(pScreen,&updateRegion,&box,0);
    }

    c->cl->rfbFramebufferUpdateMessagesSent++;

    fu->type = rfbFramebufferUpdate;
    fu->window = Swap32IfLE((CARD32)rw->frame->win->drawable.id);
    if (IsRoot(rw->frame->win))
    {
	    fu->topWindow = 0;
    }
    else
    {
	    fu->topWindow = Swap32IfLE((CARD32)rw->frame->win->drawable.id);
    }
    if (nUpdateRegionRects != 0xFFFF)
    {
	    fu->nRects = Swap16IfLE(
		    nUpdateRegionRects + !!sendCursorShape + !!sendCursorPos +
		    !!sendWindowShape);
    }
    else
    {
	    fu->nRects = 0xFFFF;
    }

#if !defined(NO_RFB_PADDING_IN_METISSE)
    fu->pad = 0;
#endif
    fu->shmid = 0;

    if (c->cl->preferredEncoding == rfbEncodingRawShm &&
	REGION_NOTEMPTY(pScreen,&updateRegion) && c->updateRequested)
    {
	    for (i = 0; i < REGION_NUM_RECTS(&updateRegion); i++)
	    {
		    int x = REGION_RECTS(&updateRegion)[i].x1;
		    int y = REGION_RECTS(&updateRegion)[i].y1;
		    int w = REGION_RECTS(&updateRegion)[i].x2 - x;
		    int h = REGION_RECTS(&updateRegion)[i].y2 - y;
		    
		    shmsize += (sz_rfbFramebufferUpdateRectHeader
			     + w * (c->cl->format.bitsPerPixel / 8) * h);
	    }

	    shmsize += sizeof(CARD16);

	    if ((shmid = shmget(IPC_PRIVATE, shmsize, IPC_CREAT | 0600)) < 0)
	    {
		    perror("shmget (rfbRootless)");
		    fprintf(stderr, "FIXME: shmget failed to allocate %d bytes (errno=%d) for %d updates\n",
				  shmsize,errno,nUpdateRegionRects) ;
		    shmid = 0;
		    c->cl->preferredEncoding = rfbEncodingRaw ;
	    }

	    if (shmid > 0 && (shm = shmat(shmid, NULL, 0)) == (char *)(-1))
	    {
		perror("shmat (rfbRootless)");
		if (shmctl(shmid, IPC_RMID, 0)==-1) perror("shmctl, IPC_RMID (rfbRootless)");
		shmid = 0;
		c->cl->preferredEncoding = rfbEncodingRaw ;
	    }

	    if (shmid > 0)
	    {
		    fu->nRects = Swap16IfLE(
			    !!sendCursorShape + !!sendCursorPos +
			    !!sendWindowShape);
		    fu->shmid = Swap32IfLE(shmid);
		    
		    c->cl->rfbRectanglesSent[rfbEncodingRawShm]
			    += nUpdateRegionRects;
		    c->cl->rfbBytesSent[rfbEncodingRawShm] += shmsize;
		    c->cl->rfbRawBytesEquivalent += shmsize;
	    }
    }

    ublen = sz_rfbFramebufferUpdateMsg;

    if (shmid > 0)
    {
	    char *s = shm;
	    rfbFramebufferUpdateRectHeader rect;

	    // fprintf(stderr, "shmid: %d nbrects: %d\n",shmid,nUpdateRegionRects) ;

	    CARD16 tmp = Swap16IfLE(nUpdateRegionRects);
	    memmove(s, &tmp, sizeof(CARD16));
	    s += sizeof(CARD16);
	    
	    for (i = 0; i < REGION_NUM_RECTS(&updateRegion); i++)
	    {
		    int x = REGION_RECTS(&updateRegion)[i].x1;
		    int y = REGION_RECTS(&updateRegion)[i].y1;
		    int w = REGION_RECTS(&updateRegion)[i].x2 - x;
		    int h = REGION_RECTS(&updateRegion)[i].y2 - y;
		    int bytesPerLine = w * (c->cl->format.bitsPerPixel / 8);
		    char *fbptr =
			    (rw->pixelData + (rw->bytesPerRow * y) +
			     (x * (rfbScreen.bitsPerPixel / 8)));

		    // fprintf(stderr,"sending update: %i %i %ix%i\n", x,y,w,h);

		    rect.r.x = Swap16IfLE(x);
		    rect.r.y = Swap16IfLE(y);
		    rect.r.w = Swap16IfLE(w);
		    rect.r.h = Swap16IfLE(h);
		    rect.encoding = Swap32IfLE(rfbEncodingRawShm);

		    memmove(s, (char *)&rect, sz_rfbFramebufferUpdateRectHeader);

		    s += sz_rfbFramebufferUpdateRectHeader;

		    (*c->cl->translateFn)(
			    c->cl->translateLookupTable,
			    &rfbServerFormat,
			    &c->cl->format, fbptr, s, rw->bytesPerRow,
			    w, h, c->cl->alphaShift);
		    s += bytesPerLine * h;
	    }
	    
#ifdef SHM_LOCK
	    /* force shm to be resident (cygwin do not have SHM_LOCK) */
	    if (shmctl(shmid,SHM_LOCK,0)==-1)
	    {
		    /* do not care about this error */
		    /* perror("shmctl, SHM_LOCK (rfbRootless)"); */
	    }
#endif
	    /* now the viewer is responsible of the segment */
	    if (shmdt(shm) == -1)
	    {
		    perror("shmdt (rfbRootless)");
	    }
    }

    /* send the NoOp before the first Rect is potentially flushed out */
    if (!rfbSendNoOp(c->cl))
	rfbCloseSock(c->cl->sock);

    if (sendCursorShape) {
	c->cl->cursorWasChanged = FALSE;
	if (!rfbSendCursorShape(c->cl, pScreen))
	    return FALSE;
    }

    if (sendCursorPos) {
	    c->cl->cursorWasMoved = FALSE;
	    if (!rfbSendCursorPos(c->cl, pScreen))
		    return FALSE;
    }

    if (sendWindowShape) {
	    c->shapeChanged = 0;
	    window_MakeWindowShapeUpdate(c->cl, pScreen, rw);
    }

#if TRACE_RFBROOTLESS
    fprintf(stderr, "Send 0x%lx (%lu) / %p (%u)\n",
	    rw->frame->win->drawable.id, REGION_NUM_RECTS(&updateRegion),
	    rw->pixelData, rw->bytesPerRow);
#endif

    if (!(REGION_NOTEMPTY(pScreen,&updateRegion) && c->updateRequested))
    {
	    REGION_UNINIT(pScreen,&updateRegion);
	    if (!rfbSendUpdateBuf(c->cl))
		    return FALSE;
	    return TRUE;
    }
    
    c->updateRequested = FALSE;
    REGION_SUBTRACT(
	    pScreen, &c->modifiedRegion, &c->modifiedRegion, &updateRegion);

    if (shmid > 0)
    {
	    REGION_UNINIT(pScreen,&updateRegion);
	    if (!rfbSendUpdateBuf(c->cl))
		    return FALSE;
	    return TRUE;
    }

    for (i = 0; i < REGION_NUM_RECTS(&updateRegion); i++) {
	int x = REGION_RECTS(&updateRegion)[i].x1;
	int y = REGION_RECTS(&updateRegion)[i].y1;
	int w = REGION_RECTS(&updateRegion)[i].x2 - x;
	int h = REGION_RECTS(&updateRegion)[i].y2 - y;

	c->cl->rfbRawBytesEquivalent +=
		(sz_rfbFramebufferUpdateRectHeader
		 + w * (c->cl->format.bitsPerPixel / 8) * h);

	switch (c->cl->preferredEncoding) {
	case rfbEncodingRawShm:
	    /* send shm data over the wire if shm*() failed earlier,
	     * should not happen if shm works */
	case rfbEncodingRaw:
		/* fprintf(stderr,"Send Rect Encoding ..."); */
	    if (!metisseSendRectEncodingRaw(
			c->cl, rw->pixelData, rfbScreen.bitsPerPixel,
			rw->bytesPerRow, x, y, w, h)) {
		REGION_UNINIT(pScreen,&updateRegion);
		/* fprintf(stderr,"ERROR\n"); */
		return FALSE;
	    }
	    /* fprintf(stderr,"done\n"); */
	    break;
	case rfbEncodingCoRRE:
	    if (!rfbSendRectEncodingCoRRE(
			c->cl, rw->pixelData, rfbScreen.bitsPerPixel,
			rw->bytesPerRow, x, y, w, h)) {
		REGION_UNINIT(pScreen,&updateRegion);
		return FALSE;
	    }
	    break;
	default:
		/* must not happen */
		break;
	}
    }

    REGION_UNINIT(pScreen,&updateRegion);

    if (nUpdateRegionRects == 0xFFFF && !rfbSendLastRectMarker(c->cl)) {
	    fprintf(stderr, "NUMBER TO BIG\n");
	    return FALSE;
    }

    if (!rfbSendUpdateBuf(c->cl))
	return FALSE;

    return TRUE;
}


Bool window_rfbSendFramebufferUpdate(rfbClientPtr cl) {
	rfbWindowRec *w;
	WindowClients *c;

	for(w = Windows; w != NULL; w = w->next) {
		if ((c = find_client(w, cl)) != NULL)
		{
			if (FB_UPDATE_PENDING(c))
			{
				return my_rfbSendFramebufferUpdate(c, w);
			}
		}
	}
	return FALSE;
}

void window_schedule_fb_update_all(rfbClientPtr cl)
{
	rfbWindowRec *w;
	WindowClients *c;

	for(w = Windows; w != NULL; w = w->next) {
		if ((c = find_client(w, cl)) != NULL)
		{
			if (c->firstTime)
			{
				BoxRec box;
				RegionRec reg;

				box.x1 = 0;
				box.y1 = 0;
				box.x2 = w->frame->width;
				box.y2 = w->frame->height;
				REGION_INIT(pScreen,&reg,&box,0);
				REGION_UNION(
					pScreen,&c->modifiedRegion,
					&c->modifiedRegion, &reg);
				rfbSendConfigureWindow(
					w->frame->win->drawable.id,
					w->frame->x, w->frame->y,
					w->frame->width,
					w->frame->height,
					IsRoot(w->frame->win));
				if (REGION_NOTEMPTY(pScreen,&w->bShape))
				{
					c->shapeChanged = 1;
				}
				my_rfbSendFramebufferUpdate(c, w);
				if (w->frame->win->mapped)
				{
					/* FIXME should send in stacking
					 * order */
					unsigned long flags;
					Window tf = None;
					Window uf = None;
					Window gw = None;
					Window dw = None;
					Window cc = None;
					unsigned long fr;
					rfb_facades_prop_t rfp;

					rfp.size = 0;
					flags = getrfbFlags(
						w->frame->win, &tf, &uf, &gw,
						&dw, &cc, &fr, &rfp);
					rfbSendRestackWindow(
						w->frame->win->drawable.id, 0,
						tf, uf, gw, (cc)? cc:dw, fr, 
						flags);
					if (0 && (flags & rfbWindowFlagsFacade))
					{
						
						window_sendFacadesUpdate(
							cl, w, &rfp);
					}
					if (rfp.size > 0 && rfp.data)
					{
						free(rfp.data);
					}
				}
				c->firstTime = 0;
			}
			else if (FB_UPDATE_PENDING(c))
			{
				my_rfbSendFramebufferUpdate(c, w);
			}
		}
	}
}

void window_sendWindowUpdate(rfbClientPtr cl, Window win, BoxRec box, int inc)
{
	rfbWindowRec *w;
	WindowClients *c;
	RegionRec reg;

	for(w = Windows; w != NULL; w = w->next)
	{
		if (w->frame->win->drawable.id == win)
		{
			break;
		}
	}

	if (w == NULL)
	{
		return;
	}

	c = find_client(w, cl);
	if (c == NULL)
	{
		return;
	}


	if (box.x1 == box.x2 && box.y1 == box.y2)
	{
		box.x1 = 0;
		box.y1 = 0;
		box.x2 = w->frame->width;
		box.y2 = w->frame->height;
	}
	
	REGION_INIT(pScreen,&reg,&box,0);
	REGION_UNION(pScreen,&c->modifiedRegion,&c->modifiedRegion, &reg);
	my_rfbSendFramebufferUpdate(c, w);
}

/* ****************************************************************************
 *
 * Rootless Call Back
 *
 * ****************************************************************************/

#define DEBUG_rfbCreateFrame 0
static
Bool rfbCreateFrame(
	RootlessWindowPtr pFrame, ScreenPtr pScreen,
	int newX, int newY, RegionPtr pShape)
{
	rfbWindowRec *rw;

	rw = add_window(pFrame);

	/* Store the implementation private frame ID */
	pFrame->wid = (RootlessFrameID *) rw;

	rw->bytesPerRow = PixmapBytePad(pFrame->width, rfbScreen.depth);
	rw->pixelData = (char *)xcalloc(1, rw->bytesPerRow * pFrame->height);

#if TRACE_RFBROOTLESS || DEBUG_rfbCreateFrame
	fprintf(
		stderr,"rfbCreateFrame for 0x%lx %i %i %ix%i %i %ix%i 0x%lx\n",
		pFrame->win->drawable.id, pFrame->x, pFrame->y,
		pFrame->width, pFrame->height, pFrame->borderWidth,
		pFrame->win->drawable.width, pFrame->win->drawable.height,
		(rw->pixelData)? rw->pixelData:0);
#endif

	rfbSendConfigureWindow(
		pFrame->win->drawable.id, newX, newY, pFrame->width,
		pFrame->height, IsRoot(pFrame->win));

	REGION_INIT(pScreen,&rw->bShape,NullBox,0);
	if (pShape)
	{
		REGION_UNION(pScreen, &rw->bShape, pShape, pShape);
		window_shapeChanged(rw);
	}

	return TRUE;
}

#define DEBUG_rfbDestroyFrame 0
static
void rfbDestroyFrame(
	RootlessFrameID wid)
{
	rfbWindowRec *rw = (rfbWindowRec *)wid;

#if TRACE_RFBROOTLESS || DEBUG_rfbDestroyFrame
	fprintf(stderr,"rfbDestroyFrame for 0x%lx\n",
		rw->frame->win->drawable.id);
#endif

	if (rw->pixelData) {
		xfree(rw->pixelData);
		rw->pixelData = NULL;
	}

	REGION_UNINIT(pScreen,&rw->bShape);

	rfbSendDestroyWindow(rw->frame->win->drawable.id);
	remove_window(rw);
}

#define DEBUG_rfbMoveFrame 0
static
void rfbMoveFrame(RootlessFrameID wid, ScreenPtr pScreen, int newX, int newY)
{

	rfbWindowRec *rw = (rfbWindowRec *)wid;

#if TRACE_RFBROOTLESS || DEBUG_rfbMoveFrame
	fprintf(
		stderr,"rfbMoveFrame for 0x%lx: %i,%i (%ix%i)\n",
		rw->frame->win->drawable.id,
		newX, newY, rw->frame->width, rw->frame->height);
#endif

	rfbSendConfigureWindow(
		rw->frame->win->drawable.id, newX, newY, rw->frame->width,
		rw->frame->height, IsRoot(rw->frame->win));

}

#define DEBUG_rfbRestackFrame 0
static
void rfbRestackFrame(RootlessFrameID wid, RootlessFrameID nextWid)
{
	unsigned long flags = 0;
	rfbWindowRec *rw = (rfbWindowRec *)wid;
	rfbWindowRec *nextRw = (rfbWindowRec *)nextWid;
	Window tf = None;
	Window uf = None;
	Window gw = None;
	Window dw = None;
	Window cc = None;
	unsigned long fr;
	rfb_facades_prop_t rfp;

#if TRACE_RFBROOTLESS || DEBUG_rfbRestackFrame
	fprintf(
		stderr,"rfbRestackFrame for 0x%lx (above 0x%lx)\n",
		rw->frame->win->drawable.id,
		(nextRw)? nextRw->frame->win->drawable.id : 0);
#endif

	if (rw->frame->win == 0)
	{
		/* FIXME: it seems that this can happen */
		return;
	}

	rfp.size = 0;
	flags = getrfbFlags(rw->frame->win, &tf, &uf, &gw, &dw, &cc, &fr, &rfp);
	rfbSendRestackWindow(
		rw->frame->win->drawable.id,
		(nextRw)? nextRw->frame->win->drawable.id : 0,
		tf, uf, gw, (cc)? cc:dw, fr, flags);

	if (flags & rfbWindowFlagsFacade)
	{
		rfbClientPtr cl;
		for (cl = rfbClientHead; cl; cl = cl->next)
		{
			if (cl->enableFacadesEncoding)
			{
				window_sendFacadesUpdate(cl, rw, &rfp);
				#if 0
				int j;
				fprintf(stderr,
					"sended rfbFacadesDescription\n");
				for (j = 0; j < rfp.size; j++)
				{
					fprintf(stderr,"%lu, ", rfp.data[j]);
				}
				fprintf(stderr,"\n");
				#endif
			}
		}
	}
	if (rfp.size && rfp.data)
	{
		free(rfp.data);
	}

	if (dw != None)
	{
		rfbWindowRec *w;
		WindowClients *c;
		rfbClientPtr cl;

		for(w = Windows; w != NULL; w = w->next)
		{
			if (w->frame->win->drawable.id != dw)
			{
				continue;
			}
			for (cl = rfbClientHead; cl; cl = cl->next)
			{
				if ((c = find_client(w, cl)) != NULL)
				{
					BoxRec box;
					RegionRec reg;

					box.x1 = 0;
					box.y1 = 0;
					box.x2 = w->frame->width;
					box.y2 = w->frame->height;
					REGION_INIT(pScreen,&reg,&box,0);
					REGION_UNION(
						pScreen,&c->modifiedRegion,
						&c->modifiedRegion, &reg);
					my_rfbSendFramebufferUpdate(c, w);
				}
			}
		}
	}

	{
		WindowClients *c;
		rfbClientPtr cl;

		for (cl = rfbClientHead; cl; cl = cl->next)
		{
			if ((c = find_client(rw, cl)) != NULL)
			{
				c->firstTime = 0;
			}
		}
	}
}


#define DEBUG_rfbResizeFrame 0
static
void rfbResizeFrame(
	RootlessFrameID wid, ScreenPtr pScreen,
	int newX, int newY, unsigned int newW, unsigned int newH,
	unsigned int gravity)
{
	rfbClientPtr cl;
	rfbWindowRec *rw = (rfbWindowRec *)wid;

	if (rw->pixelData) {
		xfree(rw->pixelData);
	}

	rw->bytesPerRow = PixmapBytePad(newW, rfbScreen.depth);
	rw->pixelData = (char *)xcalloc(1, rw->bytesPerRow * newH);

#if TRACE_RFBROOTLESS || DEBUG_rfbResizeFrame
	fprintf(
		stderr,"rfbResizeFrame for 0x%lx: %i,%i %ix%i %i %i %i,%i "
		"  %i %i %i 0x%lx\n",
		rw->frame->win->drawable.id, newX, newY, newW, newH, gravity,
		rw->frame->borderWidth,
		rw->frame->win->drawable.width, rw->frame->win->drawable.height,
		rw->frame->win->drawable.depth,
		rw->frame->win->drawable.bitsPerPixel, rw->bytesPerRow,
		(long unsigned)rw->pixelData);
#endif
	rfbSendConfigureWindow(
		rw->frame->win->drawable.id, newX, newY, newW, newH,
		IsRoot(rw->frame->win));

	for (cl = rfbClientHead; cl; cl = cl->next)
	{
		WindowClients *c;
		if ((c = find_client(rw, cl)) == NULL)
		{
			continue;
		}
		c->updateRequested = FALSE;
		REGION_SUBTRACT(
			pScreen, &c->modifiedRegion, &c->modifiedRegion,
			&c->modifiedRegion);
	}
}

#define DEBUG_rfbReshapeFrame 0
void
rfbReshapeFrame(RootlessFrameID wid, RegionPtr pShape)
{
	rfbWindowRec *rw = (rfbWindowRec *)wid;

#if TRACE_RFBROOTLESS || DEBUG_rfbReshapeFrame
	fprintf(
		stderr,"rfbReshapeFrame for 0x%lx\n",
		rw->frame->win->drawable.id);
#endif

	REGION_UNINIT(pScreen,&rw->bShape);
	REGION_INIT(pScreen,&rw->bShape,NullBox,0);

	if (pShape && REGION_NUM_RECTS(pShape) > 0)
	{
		REGION_UNION(pScreen, &rw->bShape, pShape, pShape);
	}
	window_shapeChanged(rw);

}

#define DEBUG_rfbUnmapFrame 0
static
void rfbUnmapFrame(RootlessFrameID wid)
{
	rfbClientPtr cl;
	rfbWindowRec *rw = (rfbWindowRec *)wid;

#if TRACE_RFBROOTLESS || DEBUG_rfbUnmapFrame
	fprintf(
		stderr,"rfbUnmapFrame for 0x%lx\n",
		rw->frame->win->drawable.id);
#endif
	rfbSendUnmapWindow(rw->frame->win->drawable.id);

	for (cl = rfbClientHead; cl; cl = cl->next)
	{
		WindowClients *c;
		if ((c = find_client(rw, cl)) == NULL)
		{
			continue;
		}
		c->updateRequested = FALSE;
		REGION_SUBTRACT(
			pScreen, &c->modifiedRegion, &c->modifiedRegion,
			&c->modifiedRegion);
	}
}


#define DEBUG_rfbUpdateRegion 0

static
void rfbUpdateRegion(
	RootlessFrameID wid, RegionPtr pDamage)
{
	rfbWindowRec *rw = (rfbWindowRec *)wid;
	rfbClientPtr cl;

#if DEBUG_rfbUpdateRegion || TRACE_RFBROOTLESS
	fprintf(
		stderr,"rfbUpdateRegion for 0x%lx with damage (%i)",
		rw->frame->win->drawable.id, rw->frame->is_drawing);
#if !defined(ROOTLESS_TRACK_DAMAGE) || !DEBUG_rfbUpdateRegion
	fprintf(stderr,"\n");
#endif
#endif

#ifdef ROOTLESS_TRACK_DAMAGE
	if (!pDamage)
	{
		return;
	}
#endif /* ROOTLESS_TRACK_DAMAGE */

	for (cl = rfbClientHead; cl; cl = cl->next) {
		WindowClients *c;
		if ((c = find_client(rw, cl)) == NULL)
		{
			continue;
		}
#ifdef ROOTLESS_TRACK_DAMAGE
		REGION_UNION(
			pScreen,&c->modifiedRegion,&c->modifiedRegion, pDamage);
#endif
		if (!rw->frame->is_drawing && FB_UPDATE_PENDING(c))
		{
			my_rfbSendFramebufferUpdate(c, rw);
		}
	}
}

#ifndef ROOTLESS_TRACK_DAMAGE
#define DEBUG_rfbDamageRects 0
#define DEBUG_Limits 0

#define USE_APPEND 0

#if DEBUG_Limits
static void debug_print_limits(
	char *str, rfbWindowRec *rw, int shift_x, int shift_y, BoxRec sr)
{
	fprintf(stderr,
		"%s for 0x%lx with "
		"(%i, %i):\n   ww: %i, wh: %i //  x1: %i y1: %i x2: %i y2: %i\n",
		str, rw->frame->win->drawable.id, shift_x, shift_y,
		rw->frame->win->drawable.width, rw->frame->win->drawable.height,
		sr.x1,sr.y1,sr.x2,sr.y2);
}
#endif

static
void rfbDamageRects(
	RootlessFrameID wid, int count, const BoxRec *rects,
	int shift_x, int shift_y)
{
	rfbWindowRec *rw = (rfbWindowRec *)wid;
	int i;
	RegionRec pDamage;
	rfbClientPtr cl;

	REGION_INIT(pScreen, &pDamage, NullBox, 0);

#if DEBUG_rfbDamageRects || TRACE_RFBROOTLESS
	fprintf(
		stderr,"rfbDamageRects for 0x%lx with rectangles (%i, %i,%i): ",
		rw->frame->win->drawable.id, count, shift_x, shift_y);
#endif

	for (i = 0; i < count; i++)
	{
		RegionRec reg;
		BoxRec sr;

		sr.x1 = rects->x1 + shift_x;
		sr.y1 = rects->y1 + shift_y;
		sr.x2 = rects->x2 + shift_x;
		sr.y2 = rects->y2 + shift_y;
		/* It seems that this may happen (thanks to Tony J. White) */
		if ((int)sr.x1 < 0)
		{
#if DEBUG_Limits
			debug_print_limits(
				"x1 negative", rw, shift_x, shift_y, sr);
#endif
			sr.x1 = 0;
			sr.x2 = (int)rw->frame->width;
		}
		if ((int)sr.y1 < 0)
		{
#if DEBUG_Limits
			debug_print_limits(
				"y1 negative", rw, shift_x, shift_y, sr);
#endif
			sr.y1 = 0;
			sr.y2 = (int)rw->frame->height;
		}
		/* safty check */
		if ((int)sr.x2 > (int)rw->frame->width)
		{
#if DEBUG_Limits
			debug_print_limits(
				"x2 to big", rw, shift_x, shift_y, sr);
#endif
			sr.x1 = 0;
			sr.x2 = (int)rw->frame->width;
		}
		if ((int)sr.y2 > (int)rw->frame->height)
		{
#if DEBUG_Limits
			debug_print_limits(
				"y2 to big", rw, shift_x, shift_y, sr);
#endif
			sr.y1 = 0;
			sr.y2 = (int)rw->frame->height;
		}
		if ((int)sr.x1 >= (int)sr.x2)
		{
#if DEBUG_Limits
			debug_print_limits(
				"x1 >= x2", rw, shift_x, shift_y, sr);
#endif
			/*rects++; continue;*/
			sr.x1 = 0;
			sr.x2 = (int)rw->frame->width;
		}
		if ((int)sr.y1 >= (int)sr.y2)
		{
#if DEBUG_Limits
			debug_print_limits(
				"y1 >= y2", rw, shift_x, shift_y, sr);
#endif
			/*rects++; continue;*/
			sr.y1 = 0;
			sr.y2 = (int)rw->frame->height;
		}
		REGION_INIT(pScreen, &reg, &sr, 0);
#if USE_APPEND
		if (count > 1)
			REGION_APPEND(pScreen,&pDamage,&reg);
		else
#endif
			REGION_UNION(pScreen,&pDamage,&pDamage,&reg);

		REGION_UNINIT(pScreen, &reg);
#if DEBUG_rfbDamageRects
		{
			int x = sr.x1;
			int y = sr.y1;
			int w = sr.x2 - x;
			int h = sr.y2 - y;
			fprintf(stderr, " r %i %i %i %i /",
				sr.x1,sr.y1,sr.x2,sr.y2);
			fprintf(stderr, " d %i,%i %ix%i /",x,y,w,h);
		}
#endif
		rects++;
	}

#if  DEBUG_rfbDamageRects
	fprintf(stderr, "\n");
#endif

	for (cl = rfbClientHead; cl; cl = cl->next) {
		WindowClients *c;
		int reduce = 0;

		if ((c = find_client(rw, cl)) == NULL)
		{
			continue;
		}

		if (REGION_NUM_RECTS(&c->modifiedRegion) +
		    REGION_NUM_RECTS(&pDamage)>= MAX_NUMBER_OF_RECTANGLE)
		{
			reduce = 1;
		}
		if (reduce)
		{
			BoxRec box;
			BoxPtr pb;
	    
			pb = REGION_EXTENTS(pScreen, &c->modifiedRegion);
			box.x1 = pb->x1;
			box.y1 = pb->y1;
			box.x2 = pb->x2;
			box.y2 = pb->y2;
			REGION_UNINIT(pScreen,&c->modifiedRegion);
			REGION_INIT(pScreen,&c->modifiedRegion,&box,0);
		}
		if (reduce)
		{
			BoxRec box;
			BoxPtr pb;
	    
			pb = REGION_EXTENTS(pScreen, &pDamage);
			box.x1 = pb->x1;
			box.y1 = pb->y1;
			box.x2 = pb->x2;
			box.y2 = pb->y2;
			REGION_UNINIT(pScreen,&pDamage);
			REGION_INIT(pScreen,&pDamage,&box,0);
		}
#if USE_APPEND
		if (reduce || count <= 1)
#endif
		{
			REGION_UNION(
				pScreen,&c->modifiedRegion,&c->modifiedRegion,
				&pDamage);
		}
#if USE_APPEND
		else
		{
			REGION_APPEND(
				pScreen,&c->modifiedRegion,&pDamage);
			Bool dummy;
			REGION_VALIDATE(pScreen,&c->modifiedRegion,&dummy);
		}
#endif
	}
	REGION_UNINIT(pScreen, &pDamage);
}
#endif /* ndef ROOTLESS_TRACK_DAMAGE */

#define DEBUG_rfbStartStopDrawing 0

static
void rfbStartDrawing(
	RootlessFrameID wid, char **pixelData, int *bytesPerRow)
{
	rfbWindowRec *rw = (rfbWindowRec *)wid;
	
#if TRACE_RFBROOTLESS || DEBUG_rfbStartStopDrawing
	fprintf(
		stderr,"rfbStartDrawing for 0x%lx %i %i 0x%lx\n",
		rw->frame->win->drawable.id, rw->frame->win->drawable.depth,
		rw->bytesPerRow,
		(rw->pixelData)? rw->pixelData:0);
#endif

	*pixelData = rw->pixelData;
	*bytesPerRow = rw->bytesPerRow;
}

static
void rfbStopDrawing(RootlessFrameID wid, Bool flush)
{
	rfbWindowRec *rw = (rfbWindowRec *)wid;
	rfbClientPtr cl;

#if TRACE_RFBROOTLESS ||  DEBUG_rfbStartStopDrawing
	fprintf(
		stderr,"rfbStopDrawing for 0x%lx 0x%lx flush: %i\n",
		rw->frame->win->drawable.id,
		(rw->pixelData)? (long unsigned)rw->pixelData:0,
		flush);
#endif

	if (!flush)
	{
		return;
	}

	for (cl = rfbClientHead; cl; cl = cl->next)
	{
		WindowClients *c;

		if ((c = find_client(rw, cl)) == NULL)
		{
			continue;
		}
		
		if (FB_UPDATE_PENDING(c))
		{
			my_rfbSendFramebufferUpdate(c, rw);
		}
	}
}

#define DEBUG_rfbSwitchWindow 0

static
void rfbSwitchWindow(RootlessWindowPtr pFrame, WindowPtr oldWin)
{
#if TRACE_RFBROOTLESS || DEBUG_rfbSwitchWindow
	fprintf(stderr, "rfbSwitchWindow 0x%lx, 0x%lx\n",
		pFrame->win->drawable.id, oldWin->drawable.id);
#endif
}

/*
 * Function pointer table for rootless support.
 */

static RootlessFrameProcsRec rfbRootlessProcs = {
	rfbCreateFrame,
	rfbDestroyFrame,

	rfbMoveFrame,
	rfbResizeFrame,
	rfbRestackFrame,
	rfbReshapeFrame,
	rfbUnmapFrame,

	rfbStartDrawing,
	rfbStopDrawing,
	rfbUpdateRegion,

#ifndef ROOTLESS_TRACK_DAMAGE
	rfbDamageRects,
#endif

	rfbSwitchWindow,
	/* Optional acceleration functions */
	NULL, // CopyBytes
	NULL, // FillBytes
	NULL, // CompositePixels
	NULL, // CopyWindow
};

/* ***************************************************************************
 *
 * *************************************************************************** */

Bool
rfbRootlessSetupScreen (int index, ScreenPtr pScreen)
{
	/* do not use acceleration functions */
	rootless_CopyBytes_threshold = 0;
	rootless_FillBytes_threshold = 0;
	rootless_CompositePixels_threshold = 0;
	rootless_CopyWindow_threshold = 0;

	rootless_HaveRoot = 1;

	/* Setup generic rooless support. */
	return RootlessInit (pScreen, &rfbRootlessProcs);
}
