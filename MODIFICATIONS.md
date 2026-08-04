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

## Settings and UI (2026-08-04)

- **New "Cover Panels" quick toggle**, in both the tray icon's context menu and the
  clock window's own right-click menu (right under Position). It mirrors the existing
  Settings → Misc checkbox, stays in sync with it in both directions, and takes effect
  and saves immediately — no need to open Settings or restart the clock.
- **Most options that used to require a restart now apply live**: fullscreen detection,
  multi-workspace display, keep-always-visible, click-to-move, the Linux "better stay on
  top" mode, showing the clock on all monitors, and the taskbar-icon toggle all take
  effect immediately. The old "Experimental" tab is gone — the couple of settings that
  genuinely still need a restart (see below) now carry a small note saying so, right
  next to the checkbox, instead of one blanket warning covering everything on the tab.
- **Single clock instance is now enforced, not a toggle** — the "allow only one
  instance" checkbox is gone; a second launch always hands off to the already-running
  one, same as the previous default.
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
