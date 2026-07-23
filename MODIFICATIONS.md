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
