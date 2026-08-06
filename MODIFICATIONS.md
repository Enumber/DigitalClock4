**English** · [中文](MODIFICATIONS.zh-CN.md)

# Modifications (Linux fork)

This is a Linux-focused fork of [Digital Clock 4](https://github.com/Kolcha/DigitalClock4)
by Nick Korotysh (GPL-3.0-or-later). Upstream supported Linux but never packaged it —
the last Linux release (4.7.9, December 2020) is a bare tarball with no installer —
and the project was archived in 2026. This fork makes the program buildable, installable
and usable on Linux.

**Maintainer of this fork:** **ENum** (GitHub: [Enumber](https://github.com/Enumber)).

All changes are licensed under the same GPL-3.0-or-later terms as upstream.
Original copyright notices are preserved.

## Build fixes

- `qm_gen.pri`: quote paths for `lrelease` (trees with spaces).
- Added stub English `*_en.ts` sources required by `lang.qrc`.
- `plugin_core/plugin_base.cpp`: include `<QWidget>`.
- Restored qmake support for vendored QHotkey; vendored QHotkey + SingleApplication.

## Translations

Fixed plugin translation loading (`Q_INIT_RESOURCE`, unique qrc names, resource prefixes),
completed Simplified Chinese strings, fixed Qt mnemonics and mistranslations.
Language selection still follows `QLocale::system()`.

## Parity with the official Windows build

- Real application icon for `.desktop` / window icon.
- Bundled skins and textures from the official Windows package (see `skins/README.md`).
- `power_off` ported via `systemctl`/`shutdown`.
- `talking_clock` builds when Qt TextToSpeech is present.
- X11 fullscreen detection; taskbar-icon toggle exposed on Linux.

## Bug fixes

- Autostart `Exec`/`Icon` pointed at real installed paths.
- Removed network update checker entirely (this build does not contact the network on its own;
  IP plugin remains optional and off by default).

## Linux packaging

- `$ORIGIN` RUNPATH for portable layout.
- Interactive `install.sh`.
- Documented build dependencies including `libxi-dev`.

## Installer (2026-07-23 to 2026-07-29)

- Graphical install/uninstall dialogs (GTK), documented as the primary way to
  run the installer; the original interactive-only flow is now the fallback
  when no display is available.
- Program and config directories are separated; uninstall defaults to removing
  both unless told otherwise.
- Desktop icon path and display name fixed for in-place installs.
- Installer no longer asks about autostart during install — the in-app
  Settings toggle is the only place that controls it now.
- Hardened GUI parsing: no `eval` of free-text paths.
- Icon conversion uses `mktemp` instead of a predictable `/tmp` filename.
- GUI result dialogs only show up in interactive runs; a stale dev-tree glob
  pattern was dropped.
- Fixed a self-kill bug in the installer's own second-instance handling, and
  added window-raise + stale-lock reclaim so a second launch switches to the
  already-open installer window instead of doing nothing or getting stuck.

## Settings and UI (2026-08-04)

- **New "Cover Panels" quick toggle**, in both the tray icon's context menu and the
  clock window's own right-click menu (right under Position). It mirrors the existing
  Settings → Misc checkbox, stays in sync with it in both directions, and takes effect
  and saves immediately — no need to open Settings or restart the clock.
- **All options that used to require a restart now apply live**: fullscreen detection,
  multi-workspace display, keep-always-visible, click-to-move, the Linux "better stay on
  top" mode, showing the clock on all monitors, and the taskbar-icon toggle all take
  effect immediately. The old "Experimental" tab — which existed only because some of
  these used to require a restart — is gone entirely; nothing left in Settings needs
  one (the sole remaining holdout, "only one instance," stopped being a user setting at
  all, see below).
- **Single clock instance is now enforced, not a toggle** — the "allow only one
  instance" checkbox is gone; a second launch is simply blocked and exits, leaving the
  already-running one untouched, same as the previous default.
- Removed a leftover "enable autoupdate" checkbox that had been force-hidden since the
  network update checker itself was removed (see Bug fixes below) — it did nothing and
  contradicted the "never phones home" guarantee.
- Tray icon: the Linux fallback icon (used when the desktop's icon theme can't resolve
  the themed one — e.g. running straight from a build tree) is now the real colour
  application icon instead of a plain black outline glyph.
- About dialog: clicking the logo used to cycle through two hidden images the original
  author left in there; that easter egg is gone, the logo now always shows the real
  application icon. The Donate tab now says explicitly that the BTC address belongs to
  the original author, Nick Korotysh, and that donations do not pass through or get
  handled by ENum. The Links tab drops one dead link (`showroom.qt.io`, which now
  redirects elsewhere), notes that the upstream SourceForge ticket tracker restricts
  who can file new tickets, and adds a new section pointing at this fork's own GitHub
  repository and issue tracker — previously nothing in the dialog linked back here.

## Settings dialog decluttered, one duplicate menu separator fixed (2026-08-05)

- The tray icon's (and the clock window's own) right-click menu had two consecutive
  separators between "About" and "Quit" — a leftover from when the upstream "Check for
  Updates" item that used to sit between them was removed. It rendered as a stray blank
  gap in the menu; the duplicate is gone.
- **Removed the "better 'stay on top' for Linux" checkbox.** It only controlled whether
  the plain `Qt::WindowStaysOnTopHint` request (which not every window manager honours)
  got reinforced with a full window-manager bypass; both it and the ordinary "stay on
  top" option already defaulted to on, so the checkbox's only real effect was letting
  someone turn the reinforcement *off* (at the cost of losing alt-tab and normal window
  management for that window) — an implementation detail with no reason to be
  user-facing. "Stay on top" now always uses the stronger, WM-bypassing method on Linux;
  there is nothing left to configure.
- **Appearance and Misc regrouped, still just two tabs (plus Plugins)** — the two
  tabs had accumulated ~35 controls between them (many dropped in wholesale when the
  old "Experimental" tab was retired) with no consistent theme, and everything sat in
  one long flat checkbox list.
  - **Appearance**: purely visual — skin/texture/background/colorize, zoom, opacity,
    separator flash, time format, alignment, digit spacing. Unchanged in spirit, just
    no longer sharing space with window-behaviour checkboxes.
  - **Misc**: now organised into two group boxes instead of one flat list — "Window"
    (stay on top, cover panels, multi-monitor/multi-workspace display, fullscreen
    visibility, autostart) and "Mouse and Interaction" (mouse transparency, hover
    effects, click-to-move) — followed by the smaller leftover items: edge snapping,
    refresh interval, clock URL, Show/Hide menu item
    toggle, another-timezone display, state export.
- **Removed two more checkboxes whose only real effect was letting someone opt into
  behaviour nobody would actually want**, same reasoning as the "better stay on top"
  removal above:
  - "Show app icon in the taskbar (and Alt+Tab)" — a desktop clock has no reason to
    show up there; it already defaulted to off, so the checkbox only let someone turn
    it *on*. The window now always carries `Qt::Tool`, set once at construction —
    there is nothing left to configure.
  - "Always keep clock visible on screen, prevent out of screen position" — keeping
    the clock reachable is not optional behaviour, it is just correct. `ClockWindow::
    CorrectPosition()` now always clamps instead of being gated by a config value that
    defaulted to true anyway.
- The main dialog keeps the original author-designed window size (the `.ui`'s declared
  613×447) instead of auto-fitting to content, so it does not look shrunk compared to
  the classic Settings window.
- **Appearance rebuilt to match this repo's very first commit exactly**, `QGridLayout`
  and all — `git show` on the fork's first commit was the only reliable way to settle
  what "the original layout" actually was, since by the time this round of cleanup
  started the tab had already absorbed the since-dissolved "Experimental" tab's
  contents in an earlier session. The true original Appearance only ever held: stay on
  top, mouse transparency, separator flash, opacity, zoom, skin/font pick, and the
  texture/colour/background customization — laid out as a 4-row grid (checkbox column
  beside the opacity/zoom sliders in row 0, skin spanning row 1, texture customization
  as three side-by-side cells in row 2, background/colorize in row 3), not the stacked
  `QVBoxLayout` this fork's recent edits had been using. Hover-transparency and
  hide-completely were never part of Appearance; they came from the old Experimental
  tab and now live in Misc, next to click-to-move (all three are "mouse behaviour on
  the clock"). Switching back to the original grid recovered most of the height that
  had crept in from vertical stacking, though the window still renders a bit taller
  than the declared 613×447 on this Linux/Chinese-font setup — very likely because 447
  was measured against Windows font metrics, not something a layout change can close
  further. Misc keeps its "Advanced Settings..." button (inside the Misc tab, not the
  shared button row) for cover panels, multi-monitor/fullscreen display, edge snapping,
  refresh interval, Clock URL, the Show/Hide menu toggle, and state export.

## Application icon reverted to the original author's easter-egg artwork (2026-08-05)

By explicit request, this reverses part of the earlier "About dialog" cleanup above:
the click-the-logo easter egg itself (cycling between two hidden images) stays removed,
but one of those two images — `images/if_clock-c_750020.png`, restored from this repo's
first commit — is now used in exactly two places: the About dialog's logo (its own
`icons.qrc` alias, `about-logo.png`) and the installed `.desktop`/icon-theme entry that
shows up in the application launcher (`install.sh` packages it as `clock-icon.png`,
same name as always). Everywhere else — the taskbar/window icon and the tray icon
fallback — still uses the rainbow-dial artwork that matches the Windows build's
`clock_icon_win.ico` (`images/clock_icon.png`, the `app-icon.png` alias, unchanged from
before this easter-egg detour). The second restored image, `images/if_favorite_c_750016.png`,
is in the repository but not wired into anything yet.

## "Cover Panels" removed as a setting, its behaviour is now always on (2026-08-05)

Following the diagnosis below — the option does nothing on GNOME Shell no matter what,
because the shell's own panel is always composited on top — keeping it as a checkbox
the user has to find and enable was pure friction with no corresponding benefit on the
desktop most Linux users actually run. Same call already made for "better stay on top
for Linux", the taskbar-icon toggle, and "keep always visible": if turning an option
off only opts into a worse default, it should not be a user-facing option at all.

- `OPT_ALLOW_OVER_PANELS` is gone from `settings_keys.h` and `clock_settings.cpp`
  entirely — no config key, no default value.
- `ClockWindow::UpdateBypassWindowManager()` now unconditionally sets
  `Qt::X11BypassWindowManagerHint` on Linux; `UsableScreenRect()` always treats the
  panel strip as claimable. On window managers that do not special-case their own
  panel (i.e. everything except the GNOME Shell case documented below), the clock
  now covers panels by default, with nothing to configure.
- The "Cover Panels" entries in both the tray icon's context menu and the clock
  window's own right-click menu are gone (`ContextMenu`/`TrayControl` no longer carry
  this action or its signal at all).
- The Advanced Settings dialog's checkbox and its explanatory label are gone from the
  "Window" group box; the group still holds "show on all monitors" and "show in
  fullscreen", which are unrelated settings and were left untouched.
- Ran `lupdate` across all five `.ts` files so the removed strings are correctly
  marked `vanished` rather than left dangling as unreferenced source text.

## "Cover panels" diagnosed: GNOME Shell limitation, not a bug here (2026-08-05)

A user report of "Cover Panels" not visibly doing anything on their real desktop (GNOME
Shell) led to a full re-read of `ClockWindow::UpdateBypassWindowManager()` /
`core::placement::usableRect()` — both are correct: with the option on, the window gets
`Qt::X11BypassWindowManagerHint` and the clamp bounds include the panel strip. The
gap is external: GNOME Shell's own panel is composited above every client window by
design, with no X11 hint (override-redirect, always-on-top, or otherwise) able to
change that from the application side — this is a widely reported limitation (see e.g.
the dash-to-panel project's ["Panel is above 'always on top' windows"](https://github.com/home-sweet-gnome/dash-to-panel/issues/838)
issue and [mutter#587](https://gitlab.gnome.org/GNOME/mutter/-/work_items/587)), not
something fixable from a regular application. Re-verified the option's logic against a
plain X11 window manager with a docked strut window — it works correctly there; it's
specifically GNOME Shell's own panel that can't be covered by any app. Added a code
comment at the bypass logic and expanded both the checkbox's tooltip and its
description label to say so plainly, so this doesn't read as a bug report again.

## Icon identity split into two icon themes (2026-08-05)

Restoring the easter-egg artwork as the app-list icon (see the section above) meant
every icon surface that resolved through the single `"digitalclock4"` hicolor theme
name — app-menu entry, desktop shortcut, and tray icon fallback alike — switched to it
together, since they all shared one theme name. That was not the intent: only the
app-list identity was meant to carry the easter-egg image; the desktop shortcut and
tray icon should keep showing the rainbow-dial artwork that matches the Windows build's
icon.

Two hicolor theme names now exist instead of one:
- `digitalclock4` — app-menu (`.desktop` in `applications/`) entry only. Still
  `if_clock-c_750020.png`, the easter-egg artwork.
- `digitalclock4-rainbow` — desktop shortcut and the Linux tray-icon fallback. Points
  at `clock_icon.png`, the rainbow-dial artwork also used for the taskbar/window icon
  and matching the Windows build's `clock_icon_win.ico`.

The About dialog logo is untouched by this split — it loads its own `icons.qrc` alias
(`about-logo.png`) directly and never goes through either theme name.

- `install.sh`: added an `APP_ICON_RAINBOW` variable (`clock_icon.png`, packaged as
  `clock-icon-rainbow.png`) alongside the existing `APP_ICON` (the easter egg,
  `clock-icon.png`). `install_theme_icon()` — previously one hardcoded block that
  installed a single icon under `$APP_ID` — is now a reusable function taking a source
  file and a theme name, called once per identity. `make_desktop()` gained an
  icon-override parameter so the desktop-shortcut `.desktop` entry can request
  `digitalclock4-rainbow` while the app-menu entry keeps the default. Uninstall removes
  both theme icons (`remove_icon "$APP_ID"` and the new `remove_icon "$APP_ID-rainbow"`).
- `digital_clock/gui/tray_control.cpp`: `QIcon::fromTheme("digitalclock4", ...)` →
  `QIcon::fromTheme("digitalclock4-rainbow", ...)`, so the tray icon — and its
  build-tree/unthemed-fallback image — stays the rainbow dial regardless of what the
  app-menu icon looks like.

## About dialog logo click easter egg restored, cycle changed to two images (2026-08-05)

Upstream's original behaviour: triple-clicking the About dialog logo within 250ms
(`ClickableLabel::requiredClicksCount()`/`clickTimeout()`) cycled it through three
states — the rainbow dial (default) → hidden image A → hidden image B → back to the
rainbow dial. An earlier round of this fork removed the click handling entirely (the
logo was static). By request, the click handling is back — but because this fork's
About logo is now permanently the easter-egg artwork rather than the rainbow dial (see
the two sections above), there is no rainbow-dial state to cycle back to. The restored
cycle only toggles between the two easter-egg images (`about-logo.png` ⇄
`about-logo-alt.png`); the rainbow dial never appears here. Trigger is unchanged:
triple-click within 250ms.

- `digital_clock/resources/icons.qrc`: new `about-logo-alt.png` alias pointing at
  `images/if_favorite_c_750016.png` (the second of the two easter-egg images restored
  from this repo's first commit; previously in the repository but unwired).
- `digital_clock/gui/about_dialog.h`/`.cpp`: added `on_logo_lbl_clicked()` (Qt's
  auto-connect slot for `logo_lbl`'s `clicked()` signal) and a `logo_showing_alt_` bool
  tracking which of the two images is current.

## Bug fix: browsing for a Clock URL file never actually saved it (2026-08-05)

Pre-existing bug, inherited unchanged from `settings_dialog.cpp` when the Advanced
Settings dialog was split out — not introduced by any of the changes above, caught
during this round's final review. `on_browse_url_file_btn_clicked()` picked a file
and called `QLineEdit::setText()` to show it, but `setText()` does not emit
`textEdited()` — the only signal `on_clock_url_edit_textEdited()` listens for to
actually persist the value. Picking a file via the "..." button updated the visible
text field but never saved the option; typing the same path by hand would have
worked. Fixed by explicitly emitting `OptionChanged(OPT_CLOCK_URL_STRING, ...)`
after the file picker returns, same as the manual-typing path already does.

## Installer merged into a single file; fork now has its own version number (2026-08-05)

- **`install.sh` + `enum-gui-ask.py` merged into one file.** The GUI (Gtk3) code used
  to live in a sibling `enum-gui-ask.py` shipped next to `install.sh`; it is now
  embedded directly inside `install.sh` as a heredoc and written to a private
  `mktemp -d` directory right before each dialog is shown, then deleted. Trimmed to
  only the four dialog modes this installer actually drives (install/uninstall/list/
  info) — the multi-app picker mode from the shared template never applied here (this
  installer only ever handles this one program) and was dropped rather than carried
  along as dead code. Behaviour is unchanged; verified with real, simulated mouse
  clicks (not just direct widget calls) through the full X11 event path.
- **Installer window/taskbar icon** now matches ENum Setup's own icon (same
  `GLib.set_prgname("enum-setup")`, same SVG artwork embedded alongside the GUI code)
  instead of the generic system icon — visually this installer now reads as part of
  the same ENum Setup family. Opening this installer while ENum Setup (or another
  ENum installer) is already open switches you to that window instead of showing a
  second one, via the single-instance lock the two already share; unaffected by this
  change, just confirmed still correct with the merged file.
- **New `digital_clock/core/fork_version.h`**, `DIGITALCLOCK4_FORK_VERSION = "1.0"` —
  this fork previously had no version number of its own at all; the only version
  string anywhere (`QApplication::setApplicationVersion("4.7.9")` in `main.cpp`) is
  upstream's, frozen at whatever it was when this fork branched off, and does not
  reflect any of the fork-side history in this document. `1.0` is the starting point
  for this fork's own line, tracked independently from here on (bump this file +
  tag releases to match, same convention as the other ENum forks that already have
  one). `install.sh --enum-version` now reports it instead of always returning empty.
