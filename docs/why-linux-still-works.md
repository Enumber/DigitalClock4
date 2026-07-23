**English** · [中文](why-linux-still-works.zh-CN.md)

Why this fork exists, and what the Wayland argument actually says
================================================================

[中文](why-linux-still-works.zh-CN.md)

When the upstream author retired Digital Clock 5 he gave one technical reason
for dropping Linux (release notes for 5.2.2):

> "Linux is not supported anymore (even technically it is possible to build),
> as clock is totally useless in Wayland environments just because it simply
> can't be moved"

That sentence is easy to misread as "this program does not work on Linux". It
does not say that. Here is what it actually means, and what it means for you.

What the limitation really is
-----------------------------

The clock is a frameless top-level window (`Qt::FramelessWindowHint`,
`digital_clock/gui/clock_window.cpp:88`). It implements dragging itself: the
mouse handler computes a new position and calls `move()`
(`clock_window.cpp:177-193`), and the saved position is restored the same way
at startup (`clock_window.cpp:452`).

Wayland deliberately does not let a client know or choose where its own window
sits on screen — there are no global window coordinates in the protocol, by
design. So on Wayland `move()` on a top-level window does nothing. The clock
lands wherever the compositor decides, you cannot drag it, and it cannot
restore where you left it. That part of the author's statement is accurate.

Where it does not apply
-----------------------

**On X11, none of this is true.** `move()` works, dragging works, and the saved
position is restored on the next start. X11 is still the default session on
plenty of desktops, and remains available as a login option on GNOME and KDE.
You can check which one you are in:

```sh
echo $XDG_SESSION_TYPE     # x11  or  wayland
```

If it prints `x11`, the clock behaves exactly as it does on Windows, including
drag-to-position. This was verified on the build in this repository: the window
drags with the mouse and remembers its position across restarts.

And on Wayland it is degraded, not useless
------------------------------------------

Even where the limitation does apply, "cannot be moved" is not the same as
"useless". A desktop clock that sits in a fixed spot is still a clock — it
still shows the time, still takes skins, still runs the alarm, timer and
scheduler plugins. This fork therefore keeps Linux support and documents the
limitation here.

If you are on Wayland and want the position to stick, the practical options are
to log into an X11 session, or to run this clock under XWayland (most Wayland
compositors still run X11 clients, and the X11 positioning path works there).

What this fork adds on top
--------------------------

Upstream did publish a Linux build of 4.7.9 — but only as a bare `.tar.xz`
with no installer, no desktop integration, and no updates since December 2020.
This fork adds the parts that were missing: a one-command installer with
desktop and application-menu integration, CI, the same skin and texture set the
Windows build ships with, a completed Chinese translation, and fixes for
several long-standing bugs (see `../MODIFICATIONS.md`).
