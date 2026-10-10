# Metisse

Metisse is an X window system built around a 3D compositor: windows are
rendered off-screen and drawn with OpenGL, so they can be rotated, scaled and
folded in perspective while remaining fully usable. It was created by the
**In Situ** research group (Université Paris-Sud / INRIA / LRI) and released
under the GNU GPL.

This repository **continues the original project**. Its history starts from
the last upstream release and preserves it unchanged; everything that follows
is a patch on top, so `git log -p` shows exactly what was changed and why.

## Original authors

- **Olivier Chapuis**
- **Nicolas Roussel**
- **Rémi Cardona** (maintenance, including the 0.4.1 version bump)

By count of `ChangeLog` entries they are, in order, Olivier Chapuis (262),
Nicolas Roussel (30) and Rémi Cardona (26), with one entry from Loïc Dachary.

`AUTHORS` is the original file ("A lot, see the source") and `ChangeLog` is the
original developers' own change history (325 entries, 2003-10-26 to 2008-12-09). Metisse includes
modified versions of third-party code: the X.Org server (`xserver/`) and FVWM
(`fvwm-insitu/`), each under its own licence. Every upstream licence file is
kept in place. Metisse itself is GPL (see `COPYING`).

## Where this copy came from

The project's home (`insitu.lri.fr/metisse`) and its CVS server
(`cvs.lri.fr`) are no longer reachable, and no source repository is
known to survive. The first commit is the unmodified **0.4.1** release tarball
(the last release, around 2013), recovered from the Internet Archive's
Wayback Machine:

    https://web.archive.org/web/2013id_/http://insitu.lri.fr/metisse/download/metisse/metisse-0.4.1.tar.bz2

The original development history is not available: the tarball is a clean
export with no CVS metadata, so the `ChangeLog` is the best record of it.
Metisse needs the **nucleo** library, also by Nicolas Roussel (LGPL), which is
a separate project and is not included here.

## Changes since 0.4.1

Commits after the import port the code to a modern toolchain (Ubuntu 26.04,
GCC, libpng 1.6) and fix a stack buffer overflow in
`FvwmCompositor/desktop/MetisseSource.cxx` (6 encodings were written into a
5-slot buffer, aborting the compositor at startup under a stack protector).

Some commits also contain generated build files (`configure` output,
Makefiles) recorded with local install paths; they are historical, not
sources.

## Direction

The aim is to rework Metisse's 3D window rendering on top of a conventional
window manager (Openbox, with Xfce) using X Composite and OpenGL
texture-from-pixmap, without a second X server. Large parts of the original
tree are expected to be removed or rewritten along the way. The history
remains here, so the original work stays credited and recoverable.

## Documentation

The original HTML documentation is in `doc/docs/` (`building.html`,
`running.html`, `using.html`).
