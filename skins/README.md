**English** · [中文](README.zh-CN.md)

Bundled skins — provenance and attribution
==========================================

These 19 skins are **not** our work and **not** part of the Digital Clock 4
source tree. They are the skin set that Nick Korotysh bundled with the official
Digital Clock 4 binary distributions, taken verbatim from the official Windows
package `digital_clock_4-win_portable.zip` (version 4.7.9, 2020-12-20,
downloaded from the project's SourceForge file area).

They are included here so the Linux build ships the same out-of-the-box
experience as the official Windows build, which is otherwise the only place
these skins are distributed now that the project is EOL.

Each skin is a folder of PNG/SVG artwork plus a `skin.ini` naming its author.
Authorship as declared by the skins themselves:

| Folder | Skin name | Author (from `skin.ini`) |
|---|---|---|
| `bin-clock` | Binary clock | Nick Korotysh |
| `comic` | Comic Sans | Nick Korotysh |
| `electronic-italic` | Electronic Italic | Nick Korotysh |
| `hearts-empty` | Hearts Empty | Nick Korotysh |
| `hearts-simple` | Hearts Simple | Nick Korotysh |
| `qsn_cp_clock_digital_new` | QSN CP clock digital new | Nick Korotysh |
| `flipclock-hd` | Flipclock HD | guy hoffman |
| `numeric-dotted` | Numeric Dotted | jean_victor_balin |
| `steel` | Steel | Chrisdesign |
| `rainbow_numbers` | Rainbow Numbers | zcool.com.cn |
| `christmas_tree_b` | Christmas / New Year (Blue) | not stated |
| `christmas_tree_r` | Christmas / New Year (Red) | not stated |
| `doodled-empty` | Doodled Empty | not stated |
| `floral_digits` | Floral digits | not stated |
| `grass_numbers-hd` | Grass Numbers HD | not stated |
| `liquid_numbers` | Liquid Numbers | not stated |
| `origami_style` | Origami Style | not stated |
| `pattern-numbers` | Pattern numbers | not stated |
| `vintage_digits` | Vintage digits | not stated |

The same applies to the `textures/` folder at the repository root.

**On licensing, plainly:** the skins carry no licence files of their own, and
the upstream project never stated terms for them separately from the program's
GPL-3. We therefore cannot claim they are GPL-3, and we make no licence claim
about them. What we can say is that they were distributed publicly by the
upstream author as part of the official release, and we redistribute them
unmodified with attribution intact.

If you are one of the authors above and would rather this repository not carry
your work, open an issue and we will remove it — the program runs fine with no
`skins/` folder at all, falling back to its built-in skin.
