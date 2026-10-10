# composite: Composite-based rework of Metisse's rendering

Phase 1 of rebuilding Metisse's 3D window rendering on a conventional X
server, without Xmetisse's second X server or its VNC-style pixel copy.

`composite.c` redirects every top-level window off-screen (XComposite), binds
each window's pixmap as a GL texture with `GLX_EXT_texture_from_pixmap`
(zero-copy), and draws the windows bottom-to-top on the Composite Overlay
Window. Phase 2 adds a per-window 3D transform (rotation, scale, opacity) under
a perspective camera calibrated so untransformed windows are pixel-exact.

    make
    ./composite [-display :4] [-shot out.ppm]    # SIGUSR1 writes a frame to out.ppm

## Transforms

Set the `_METISSE_TRANSFORM` property (INTEGER, 32-bit, thousandths) on a
top-level frame window: `rx ry rz scale opacity`, angles in degrees. For
testing, `set-transform.sh WINDOW RX RY RZ [SCALE [OPACITY]]` does this with
`xprop` (it climbs from any client window to its frame); with only a window
it removes the transform. Pointer input is not yet remapped (phase 4): clicks
still hit the real, flat window.

`spin.sh WINDOW [SECONDS]` swings a window through 3D as a demo.

With a patched Openbox (branch `feature/metisse-tilt` of the NetLinux Openbox
fork), Alt+Shift+drag tilts a window and Alt+Shift+middle-click removes the
tilt; Openbox only publishes the property, this program draws it.

Run it under any window manager. It was developed against Openbox on Xephyr.
Needs libx11, libxcomposite (>= 0.4), libxdamage, libxfixes and libgl.

Status: tested on Xephyr (software GL, llvmpipe) and on a real X11 desktop
with the NVIDIA proprietary driver (Quadro K620, direct rendering, about 5% of
one CPU core for a 1920x1080 desktop with hundreds of windows).
