#ifndef DISPCUR_H
#define DISPCUR_H

CursorPtr rfbSpriteGetCursorPtr(ScreenPtr pScreen);
void rfbSpriteGetCursorPos(ScreenPtr pScreen, int *px, int *py);

Bool rfbInitCursor (ScreenPtr pScreen);

#endif
