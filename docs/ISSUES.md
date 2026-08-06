**English** · [中文](ISSUES.zh-CN.md)

# Fixed issues log (2026-07-22, reported on real hardware by the user)

Source: found by the user on a real GNOME desktop (Ubuntu 24.04, X11).
**None of these show up under Xvfb** — they involve the real window manager,
tray host, and panel interactions, which only a real machine can confirm.
Kept on file so the same class of bug doesn't get reintroduced.

## Fixed

### Tray icon not showing up
Not actually a bug in this program (one of the false leads): the root cause
was `indicator-application` claiming `org.kde.StatusNotifierWatcher` on this
system, and GNOME's `ubuntu-appindicators` extension only scans the bus once,
at its own startup — so **any program started after the extension loads never
gets its tray icon drawn**. Reloading the extension makes it appear. The real
fix is disabling the redundant `indicator-application` (that's the user's own
system configuration, left untouched here).

Two genuine program-side bugs were found and fixed alongside this (see
`MODIFICATIONS.md`): `setIsMask(true)` renders the colour icon as an invisible
blank template on Linux; and `IconName` used to be published as an absolute
path to a temp file, which some tray hosts only resolve by theme name.

### Icon appearing in the taskbar for no reason
A regression introduced during this round of fixes:
`X11ApplyTaskbarVisibility`'s "disable" branch overwrote the window type,
turning the `UTILITY` type Qt sets for `Qt::Tool` into `NORMAL`. The whole
function was removed, restoring the upstream behaviour.

### "Ready" notification popping up
Caused by the window picking up `_NET_WM_STATE_DEMANDS_ATTENTION`. Fixed by
adding `Qt::WindowDoesNotAcceptFocus` (the clock never needed keyboard focus
anyway).

### Clicking a plugin's settings freezes the whole Settings window
Actually two separate problems stacked together, which the user distinguished
clearly:
1. **Genuine lag** (the primary one): the first click on an enabled-but-not-yet
   loaded plugin's settings triggers a synchronous `dlopen` + init on the spot;
   for something like the talking-clock (TTS) plugin that can take several
   seconds, during which the UI is unresponsive.
2. **Stacking order** (secondary, a "nice to have"): the plugin's config
   dialog had no parent window, so to the window manager it was only
   "transient for group" — it popped up behind the always-on-top main
   Settings window instead of in front of it.

Fix: a `SetDialogParent()` mechanism plus a `PluginConfigureRequest` signal
chain that correctly docks each plugin dialog above the Settings window;
plugins with nothing to configure (e.g. "variable transparency", "random
position") now grey out their gear button instead of leaving it clickable
with no effect.

Verified: the talking-clock plugin (the slowest) pops up correctly within 8
seconds and stays above the Settings window; the date plugin takes about 2
seconds.

### Clock can't cover the top panel / tray area
A new setting, `OPT_ALLOW_OVER_PANELS` (off by default, exposed on the
"Experimental" tab), switched window placement from the panel-excluding
`availableGeometry()` to the full-screen `geometry()` when enabled.

> **Follow-up (2026-08-05)**: the "Experimental" tab mentioned here has since
> been retired entirely, and `OPT_ALLOW_OVER_PANELS` itself has been deleted
> outright — covering panels is now Linux's unconditional default, with
> nothing left to toggle. The one thing it still can't cover is GNOME Shell's
> own top panel (a limitation of that desktop environment, not something this
> program can fix). See the "'Cover Panels' removed as a setting" and
> "diagnosed: GNOME Shell limitation" sections in `MODIFICATIONS.md`. This
> historical entry is kept as-is; it does not mean today's settings UI still
> has this option.

### Tray/context-menu "Quit" doesn't actually quit
The root cause had nothing to do with the suspected culprit
(`SingleApplication`): `qApp->quit()` always succeeded — what hung was the
destruction that followed. `mouse_tracker_linux.cpp`'s tracking thread was
blocked inside `XNextEvent()`, and `stop()` only set a flag and waited
forever; the thread would never reach the point where it checks that flag
because it was stuck waiting for the next X event. Fixed by switching to an
`XPending()` + `poll(200ms)` loop with a real `wait()`/join. Verified: "Quit"
from the context menu exited the process cleanly 5 times in a row.

### Re-checked first-run defaults
Tested on a brand-new config: position (1531, 20) on a 1600px-wide screen
with a 50px-wide window (top-right corner), time format `09 25` (24-hour
`HH:mm`, no AM/PM), 20% zoom, 60% opacity, single-instance enforced (a second
launch is refused). All matched expectations.

## Testing notes (lessons learned the hard way, for future reference)

- Use `Xvfb` for an isolated display — **never `DISPLAY=:1`** (the user's real
  desktop).
- Always wrap the launched program in `dbus-run-session`, or its tray icon
  leaks into the user's real taskbar.
- Always use an **isolated `$HOME`**, or you will corrupt the user's real
  config (this actually happened once: `transp_for_input=true` got written
  into the user's real config, making the clock completely unclickable).
- **Tray, notification, and panel-stacking behaviour simply cannot be tested
  under Xvfb** — only a real machine, confirmed by the user, settles these.
- When multiple agents edit the same source tree concurrently, intermediate
  build artifacts can conflict with each other (one side adds a symbol the
  other side's not-yet-rebuilt library doesn't have yet) — that's not
  evidence the change itself is broken; rebuild and re-verify once everything
  has landed.
