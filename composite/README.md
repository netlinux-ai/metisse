# composite: Composite-based rework of Metisse's rendering

Phase 1 of rebuilding Metisse's 3D window rendering on a conventional X
server, without Xmetisse's second X server or its VNC-style pixel copy.

`composite.c` redirects every top-level window off-screen (XComposite), binds
each window's pixmap as a GL texture with `GLX_EXT_texture_from_pixmap`
(zero-copy), and draws the windows bottom-to-top on the Composite Overlay
Window. Phase 1 draws windows flat. Phase 2 adds per-window 3D transforms.

    make
    ./composite [-display :4] [-shot out.ppm]    # SIGUSR1 writes a frame to out.ppm

Run it under any window manager. It was developed against Openbox on Xephyr.
Needs libx11, libxcomposite (>= 0.4), libxdamage, libxfixes and libgl.

Status: tested on Xephyr (software GL, llvmpipe) only. Not yet tested on a
hardware GL driver.
