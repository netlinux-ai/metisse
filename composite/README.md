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
it removes the transform.

`spin.sh WINDOW [SECONDS]` swings a window through 3D as a demo.

With a patched Openbox (branch `feature/metisse-tilt` of the NetLinux Openbox
fork), Alt+Shift+drag tilts a window and Alt+Shift+middle-click removes the
tilt; Openbox only publishes the property, this program draws it.

## Pointer input on tilted windows (phase 4)

While any window is tilted, the compositor keeps a *virtual* pointer (where
the cursor is drawn) and warps the *real* X pointer to the matching point on
the flat window under it, so clicks, drags, the wheel and enter/leave events
all arrive as genuine input, not synthetic events. The real cursor is hidden
(XFixes) and the current cursor image is drawn in GL at the virtual position.
Drawing and hit-testing share the same camera and model matrices, so they
cannot disagree. A button press keeps its window until released, so drags
continue outside the window. With no tilted window nothing is remapped and
the normal cursor is used.

Options:

    -absolute   pointer device reports absolute positions (Xephyr); the
                default assumes a relative device (a real mouse)
    -verbose    log each remapped position

While Openbox holds the pointer (move, resize, tilt, menus) it sets
`_METISSE_GRAB` on the root window and the compositor stands down, because
Openbox reads raw pointer coordinates then.

Known limits: a transform lives on the frame window, so it is lost if the
window manager restarts; a window turned edge-on cannot be hit; only the
pointer is remapped (keyboard focus is unaffected); one screen only.

Run it under any window manager. It was developed against Openbox on Xephyr.
Needs libx11, libxcomposite (>= 0.4), libxdamage, libxfixes and libgl.

Status: pointer remapping tested on Xephyr (software GL) only, in both
absolute and relative modes; rendering also tested on a real X11 desktop
with the NVIDIA proprietary driver (Quadro K620, direct rendering, about 5% of
one CPU core for a 1920x1080 desktop with hundreds of windows).

## Animation (phase 5)

`_METISSE_TRANSFORM` holds the *target*. The drawn rotation, scale and
opacity ease towards it exponentially (time constant 70 ms), so tilts and
untilts glide instead of snapping. Pointer remapping uses the drawn values, so
it always agrees with what is on screen. Newly seen windows start at their
target. `-noanim` applies changes instantly. The loop runs at ~8 ms only
while something is moving or tilted.
