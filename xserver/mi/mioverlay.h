/* $Id: mioverlay.h,v 1.1 2003/12/05 08:56:45 roussel Exp $ */

#ifndef __MIOVERLAY_H
#define __MIOVERLAY_H

typedef void (*miOverlayTransFunc)(ScreenPtr, int, BoxPtr);
typedef Bool (*miOverlayInOverlayFunc)(WindowPtr);

Bool
miInitOverlay(
   ScreenPtr pScreen, 
   miOverlayInOverlayFunc inOverlay,
   miOverlayTransFunc trans
);

Bool
miOverlayGetPrivateClips(
    WindowPtr pWin,
    RegionPtr *borderClip,
    RegionPtr *clipList
);

Bool miOverlayCollectUnderlayRegions(WindowPtr, RegionPtr*);
void miOverlayComputeCompositeClip(GCPtr, WindowPtr);
Bool miOverlayCopyUnderlay(ScreenPtr);
void miOverlaySetTransFunction(ScreenPtr, miOverlayTransFunc);
void miOverlaySetRootClip(ScreenPtr, Bool);

#endif /* __MIOVERLAY_H */
