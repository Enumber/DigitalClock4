#!/bin/bash
# ============================================================================
#  Digital Clock 4 · Linux 一键安装器 (install.sh)
#  --------------------------------------------------------------------------
#  说明：用户可见的帮助文本在下面的 usage() 里（中英双语）。本注释块是给
#  阅读源码的人看的，不再被 --help 打印。
#
#  两种形态自动识别：
#    · 发布包（本目录已有编译好的 digital_clock）→ 直接安装
#    · 源码检出（无二进制）→ 先编译（需要 Qt5 开发包，见 README），再安装
#
#  交互模式：在终端里直接运行（stdin 是 TTY 且不带任何参数）时进入问答：
#     ① 安装位置：用户目录（默认）/ 自定义路径 / 系统目录 /opt
#        （没有写权限的目录会走 sudo，密码由 sudo 自己在终端提示）
#     ② 是否放桌面图标（免「信任」确认，双击即可启动）
#  开机自启动不在安装时询问（本项目非常驻类）：程序设置里自带「开机启动」
#  开关，装完在 设置 → 杂项 里开即可；脚本化场景仍可用 --autostart。
#
#  普通用法：      bash install.sh
#  高级用法(可选)：
#     bash install.sh --system            系统级安装(需管理员，装到 /opt + /usr/share)
#     bash install.sh --prefix ~/apps     指定安装位置(把程序复制到 该目录/digitalclock4 再建图标)
#     bash install.sh --no-desktop-icon   只进应用菜单，不在桌面放图标
#     bash install.sh --uninstall         卸载(移除桌面/菜单图标；--prefix/--system 同时给则一并删程序副本)
#     bash install.sh --help              查看帮助
# ============================================================================
set -e

# ── --enum-version：只给 EnumSetup 内部用，探测某个目录里这份程序的版本号，
# 不联网、不动锁、不问任何问题，用完立刻退出。必须在其它一切之前处理掉，
# 且不能依赖后面才定义的 SRC——这里自己算一次目录。
# 版本号来源是 digital_clock/core/fork_version.h 的 DIGITALCLOCK4_FORK_VERSION
# ——跟 main.cpp 里 QApplication::setApplicationVersion 设的上游版本（4.7.9，
# 只在 fork 时同步过一次）是两回事，那个不代表这个 fork 自己改了多少版。
if [ "${1:-}" = "--enum-version" ]; then
  _ev_dir="${2:-$(cd "$(dirname "$0")" && pwd)}"
  _ev_version=""
  _ev_repo=""
  if [ -f "$_ev_dir/digital_clock/core/fork_version.h" ]; then
    _ev_version="$(sed -n 's/^const char DIGITALCLOCK4_FORK_VERSION\[\] = "\([^"]*\)".*/\1/p' "$_ev_dir/digital_clock/core/fork_version.h" 2>/dev/null | head -n1)"
    _ev_repo="$(sed -n 's#^const char DIGITALCLOCK4_FORK_API_LATEST\[\] = "https://api\.github\.com/repos/\([^/]*/[^/"]*\)/releases.*#\1#p' "$_ev_dir/digital_clock/core/fork_version.h" 2>/dev/null | head -n1)"
  fi
  printf 'VERSION=%s\n' "$_ev_version"
  printf 'REPO=%s\n' "$_ev_repo"
  exit 0
fi

# 程序真实配置在 ~/.config/Nick Korotysh/Digital Clock.conf（Qt 的
# organizationName 是上游作者名，含空格）——所以这个列表用换行分隔、
# 卸载循环里用换行 IFS，空格路径才不会被拆碎。后两个目录是历史猜测值，保留兜底。
CONFIG_PATHS="$HOME/.config/Nick Korotysh/Digital Clock.conf
$HOME/.config/DigitalClock4
$HOME/.config/digitalclock4"
DATA_PATHS=""

# ── 配置区（换项目时只改这里）───────────────────────────────────────────────
APP_ID="digitalclock4"                   # 唯一标识（.desktop 文件名，用英文小写连字符）
APP_WMCLASS="Digital Clock"              # 窗口 WM_CLASS 的 Class 段（实测值，别改）
APP_NAME="Digital Clock 4"               # 菜单/桌面显示名称（固定英文，不跟随语言）
APP_COMMENT="Customizable desktop clock with skins and plugins"
APP_COMMENT_ZH="可换皮肤、带插件的桌面时钟"
APP_ICON="clock-icon.png"                # 便携目录里的图标（真·Digital Clock 4 图标，用于「程序列表」）
APP_ICON_RAINBOW="clock-icon-rainbow.png" # 彩虹色盘图标，用于桌面快捷方式+托盘（跟程序列表分开）
EXEC_REL="digital_clock"                 # 便携目录里的主程序
EXEC_ARGS=""
RUN_IN_TERMINAL="false"
CATEGORIES="Utility;Clock;"
NEEDS_VENV="0"
VENV_DIR=".venv"
PYDEPS=""
VENV_SYSTEM_SITE="1"
ASK_AUTOSTART="0"                        # 不询问（自启动仅限常驻类项目）；设置里的「开机启动」开关或 --autostart 可用
AUTOSTART_ARGS=""
POST_INSTALL_NOTE="开机自启动、皮肤、插件等都在程序的右键菜单 → 设置 里。"
POST_INSTALL_NOTE_EN="Autostart, skins and plugins live in the app's right-click menu -> Settings."

# 装完就能用：把「时间格式」问出来直接写进程序的配置文件，
# 免得用户装完还要自己进设置翻一遍。
CUSTOMIZE_HOOK() {
  CFG_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/Nick Korotysh"
  CFG="$CFG_DIR/Digital Clock.conf"

  if [ "$INTERACTIVE" = "1" ]; then
    if [ "${USE_GUI:-0}" = "1" ]; then
      if enum_gui_eval list "$(tr_ "时间格式" "Time format")" \
          "$(tr_ "选择 Digital Clock 4 的时间格式" "Choose a time format for Digital Clock 4")" \
          "1|$(tr_ "跟随系统（默认）" "Follow the system (default)")" \
          "2|$(tr_ "24 小时，如 21:05" "24-hour, e.g. 21:05")" \
          "3|$(tr_ "24 小时带秒，如 21:05:30" "24-hour with seconds")" \
          "4|$(tr_ "12 小时带 AM/PM" "12-hour with AM/PM")"; then
        _tf="${CHOICE:-1}"
      else
        _tf="1"
      fi
    else
      say "时间格式：" "Time format:"
      say "  1) 跟随系统（默认）" "  1) Follow the system (default)"
      say "  2) 24 小时，如 21:05" "  2) 24-hour, e.g. 21:05"
      say "  3) 24 小时带秒，如 21:05:30" "  3) 24-hour with seconds, e.g. 21:05:30"
      say "  4) 12 小时带 AM/PM，如 9:05PM" "  4) 12-hour with AM/PM, e.g. 9:05PM"
      printf '%s' "$(tr_ "请选择 [1/2/3/4]（回车=1）: " "Choose [1/2/3/4] (Enter=1): ")"
      read -r _tf || _tf=""
    fi
    case "$_tf" in
      2) TIME_FORMAT="HH:mm" ;;
      3) TIME_FORMAT="HH:mm:ss" ;;
      4) TIME_FORMAT="h:mmA" ;;
      *) TIME_FORMAT="" ;;
    esac
  fi

  # 只在用户选了非默认格式时写配置，避免平白生成一个配置文件
  if [ -n "${TIME_FORMAT:-}" ]; then
    mkdir -p "$CFG_DIR"
    if [ -f "$CFG" ] && grep -q "^time_format=" "$CFG"; then
      sed -i "s|^time_format=.*|time_format=$TIME_FORMAT|" "$CFG"
    elif [ -f "$CFG" ] && grep -q "^\[clock\]" "$CFG"; then
      sed -i "/^\[clock\]/a time_format=$TIME_FORMAT" "$CFG"
    else
      printf '[clock]\ntime_format=%s\n' "$TIME_FORMAT" >> "$CFG"
    fi
    say "▶ 时间格式已设为 $TIME_FORMAT" "▶ Time format set to $TIME_FORMAT"
  fi
}
# ────────────────────────────────────────────────────────────────────────────

# ── 界面语言：LC_ALL > LC_MESSAGES > LANG；zh* → 中文，其余 → 英文 ─────────
_loc="${LC_ALL:-${LC_MESSAGES:-${LANG:-}}}"
case "$_loc" in zh*) UI_LANG="zh" ;; *) UI_LANG="en" ;; esac
tr_() { if [ "$UI_LANG" = "zh" ]; then printf '%s' "$1"; else printf '%s' "$2"; fi; }
say() { printf '%s\n' "$(tr_ "$1" "$2")"; }

# ── 帮助文本（中英双语，跟随系统语言）──────────────────────────────────────
usage() {
if [ "$UI_LANG" = "zh" ]; then
cat <<'EOF'
Digital Clock 4 · Linux 一键安装器

用法：
  bash install.sh                    交互安装（推荐）

交互安装会依次问你：装到哪、要不要桌面图标。
桌面图标会被标记为可信，双击直接启动，不弹「是否允许启动」。
开机自启动不在安装时询问：装完在程序的「设置 → 杂项」里打开「开机启动」
即可（脚本化场景可用 --autostart，写的是程序自己认的那个 autostart 文件，
和设置里的开关是同一件事，两边保持一致，不会重复启动）。

本目录若已有编译好的程序就直接安装；只有源码时会先自动编译
（需要 Qt5 开发包，缺什么会提示，见 README）。

不提问的用法（脚本/自动化）：
  bash install.sh --prefix 目录      装到指定目录（会复制到 目录/digitalclock4）
  bash install.sh --system           系统级安装到 /opt，所有用户可用，需管理员密码
  bash install.sh --no-desktop-icon  只进应用菜单，不放桌面图标
  bash install.sh --autostart        同时设置开机自启动
  bash install.sh --uninstall        卸载（同时给 --prefix/--system 会一并删除程序）
  bash install.sh --help             显示本帮助

装到没有写权限的目录（如 /opt）时会自动请求管理员密码，
密码由 sudo 自己提示，本脚本不经手。
EOF
else
cat <<'EOF'
Digital Clock 4 - one-command installer for Linux

Usage:
  bash install.sh                    interactive install (recommended)

The interactive install asks where to put the program and whether you want a
desktop icon. The desktop icon is marked trusted, so double-clicking it starts
the clock directly with no "allow launching" prompt. Start-at-login is not
asked during installation: turn it on afterwards in the program's own
Settings -> Misc (or pass --autostart in scripts; both write the same
autostart file, so the two always agree and nothing starts twice).

If a compiled program is already present it is installed as is; from a source
checkout it is built first (needs the Qt5 development packages - see README).

Non-interactive use (scripts, automation):
  bash install.sh --prefix DIR       install into DIR/digitalclock4
  bash install.sh --system           install to /opt for all users (needs admin)
  bash install.sh --no-desktop-icon  application menu only, no desktop icon
  bash install.sh --autostart        also enable start at login
  bash install.sh --uninstall        uninstall (also removes the program when
                                     --prefix/--system is given as well)
  bash install.sh --help             show this help

Installing into a directory you cannot write to (such as /opt) asks for your
password via sudo; the script never handles the password itself.
EOF
fi
}

# ── 解析参数 ────────────────────────────────────────────────────────────────
NARGS=$#
MODE="user"
PREFIX=""
WANT_DESKTOP_ICON="1"
WANT_AUTOSTART="0"
DO_UNINSTALL="0"
WANT_YES="0"
WANT_KEEP_CONFIG="0"
WANT_INPLACE="0"
while [ $# -gt 0 ]; do
  case "$1" in
    --system) MODE="system" ;;
    --prefix)
      # 作为最后一个参数出现时 $2 不存在，set -e 会让脚本无提示退出——先查再取
      [ $# -ge 2 ] || { say "--prefix 需要一个目录参数（--help 查看用法）" "--prefix needs a directory argument (see --help)"; exit 1; }
      PREFIX="$2"; shift ;;
    --prefix=*) PREFIX="${1#*=}" ;;
    --no-desktop-icon) WANT_DESKTOP_ICON="0" ;;
    --autostart) WANT_AUTOSTART="1" ;;
    --uninstall) DO_UNINSTALL="1" ;;
    --keep-config) WANT_KEEP_CONFIG="1" ;;
    --inplace) WANT_INPLACE="1" ;;
    --yes|-y) WANT_YES="1" ;;
    --cli) ENUM_FORCE_CLI=1 ;;
    --help|-h) usage; exit 0 ;;
    *) say "未知参数: $1（--help 查看用法）" "Unknown option: $1 (see --help)"; exit 1 ;;
  esac
  shift
done

SRC="$(cd "$(dirname "$0")" && pwd)"

# ── 图形界面：单文件安装器，GUI 代码内嵌在这个脚本里（不再是并排的
# enum-gui-ask.py），运行时落一份临时文件再用 python3 跑。内嵌的是这个安装器
# 专属的精简版：只有 install/uninstall/list/info 四种对话框，装的也只有
# Digital Clock 4 这一个应用——不带 EnumSetup 那个多应用选择器（那个装几十个
# 应用的场景跟这里完全不沾边，带着只会误导）。视觉上跟 ENum Setup 统一：
# 同一个 GLib.set_prgname("enum-setup")（任务栏/窗口图标靠它跟 ENum Setup
# 共用 WM_CLASS，两边窗口互认，见下面 enum_activate_running_instance 的锁
# 逻辑）+ 同一份 enum-setup.svg 图标。
_ENUM_GUI_PY="$(cat <<'ENUM_GUI_PY_EOF'
#!/usr/bin/env python3
"""ENum installer/uninstaller GUI (Gtk3), embedded single-app edition.

Prints KEY=value lines for bash to eval. Only the modes this installer
actually drives are here (install/uninstall/list/info) -- no multi-app
picker, this installer only ever handles this one app.

Modes (argv[1]):
  install   -- location + option checkboxes
  uninstall -- confirm + optional keep-config
  list      -- single-choice list (title, text, then choices as remaining args)
  info      -- info dialog; always exit 0
"""
from __future__ import annotations

import os
import sys

import gi

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GLib  # noqa: E402


def ui_lang() -> str:
    loc = os.environ.get("LC_ALL") or os.environ.get("LC_MESSAGES") or os.environ.get("LANG") or ""
    return "zh" if loc.lower().startswith("zh") else "en"


def T(zh: str, en: str) -> str:
    return zh if ui_lang() == "zh" else en


def emit(**kwargs) -> None:
    """Print KEY=value lines for the installer shell to parse (never eval).

    Values are written literally after the first ``=``. The companion shell
    helper assigns ``${line#*=}`` wholesale -- do not shell-quote here, or the
    quotes become part of the path.
    """
    for k, v in kwargs.items():
        text = str(v).replace(chr(13), " ").replace(chr(10), " ")
        print(f"{k}={text}")


def dialog_install(app_name: str, ask_autostart: bool, ask_auto_update: bool) -> int:
    win = Gtk.Window(title=T(f"安装 {app_name}", f"Install {app_name}"))
    win.set_default_size(480, 360)
    win.set_border_width(14)
    box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
    win.add(box)
    box.pack_start(
        Gtk.Label(
            label=T(
                "程序默认装到 ~/.local/share/enum/，配置在 ~/.config（分开存放）。",
                "Programs go under ~/.local/share/enum/; config stays in ~/.config.",
            ),
            wrap=True,
            xalign=0,
        ),
        False,
        False,
        0,
    )

    box.pack_start(Gtk.Label(label=T("安装位置", "Install location"), xalign=0), False, False, 0)
    loc_store = [
        ("user", T("用户目录 ~/.local/share/enum（默认）", "User ~/.local/share/enum (default)")),
        ("custom", T("自定义路径", "Custom path")),
        ("system", T("系统 /opt/enum（需管理员）", "System /opt/enum (admin)")),
        ("inplace", T("开发树原地（不复制）", "In-place from source (no copy)")),
    ]
    radios = []
    group = None
    for key, label in loc_store:
        r = Gtk.RadioButton.new_with_label_from_widget(group, label)
        if group is None:
            group = r
        r._enum_key = key  # type: ignore[attr-defined]
        radios.append(r)
        box.pack_start(r, False, False, 0)

    path_entry = Gtk.Entry()
    path_entry.set_placeholder_text(T("自定义安装根路径…", "Custom install root…"))
    path_entry.set_sensitive(False)
    box.pack_start(path_entry, False, False, 0)

    def on_loc_toggled(btn):
        if btn.get_active():
            path_entry.set_sensitive(btn._enum_key == "custom")  # type: ignore[attr-defined]

    for r in radios:
        r.connect("toggled", on_loc_toggled)

    chk_desk = Gtk.CheckButton(label=T("在桌面放图标", "Desktop icon"))
    chk_desk.set_active(True)
    box.pack_start(chk_desk, False, False, 0)

    chk_auto = Gtk.CheckButton(label=T("开机自启动", "Start at login"))
    chk_auto.set_active(False)
    if ask_autostart:
        box.pack_start(chk_auto, False, False, 0)

    chk_upd = Gtk.CheckButton(label=T("启动时自动检查更新", "Check for updates at startup"))
    chk_upd.set_active(True)
    if ask_auto_update:
        box.pack_start(chk_upd, False, False, 0)

    btn_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
    btn_row.set_halign(Gtk.Align.END)
    box.pack_end(btn_row, False, False, 0)
    btn_cancel = Gtk.Button(label=T("取消", "Cancel"))
    btn_ok = Gtk.Button(label=T("安装", "Install"))
    btn_ok.get_style_context().add_class("suggested-action")
    btn_row.pack_start(btn_cancel, False, False, 0)
    btn_row.pack_start(btn_ok, False, False, 0)

    state = {"ok": False}

    def finish(ok: bool):
        state["ok"] = ok
        win.destroy()

    btn_cancel.connect("clicked", lambda *_: finish(False))
    btn_ok.connect("clicked", lambda *_: finish(True))
    win.connect("delete-event", lambda *_: (finish(False), True)[1])
    win.connect("destroy", lambda *_: Gtk.main_quit())

    win.show_all()
    Gtk.main()
    if not state["ok"]:
        emit(CANCELLED=1)
        return 1

    mode = "user"
    for r in radios:
        if r.get_active():
            mode = r._enum_key  # type: ignore[attr-defined]
            break
    prefix = path_entry.get_text().strip() if mode == "custom" else ""
    emit(
        CANCELLED=0,
        MODE=("system" if mode == "system" else "user"),
        WANT_INPLACE=(1 if mode == "inplace" else 0),
        PREFIX=prefix,
        WANT_DESKTOP_ICON=(1 if chk_desk.get_active() else 0),
        WANT_AUTOSTART=(1 if (ask_autostart and chk_auto.get_active()) else 0),
        WANT_AUTO_UPDATE=(1 if (not ask_auto_update or chk_upd.get_active()) else 0),
    )
    return 0


def dialog_uninstall(app_name: str, show_keep_config: bool) -> int:
    win = Gtk.Window(title=T(f"卸载 {app_name}", f"Uninstall {app_name}"))
    win.set_default_size(440, 220)
    win.set_border_width(14)
    box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
    win.add(box)
    box.pack_start(
        Gtk.Label(
            label=T(
                f"确定卸载 {app_name}？\n对外安装会删除程序与配置；开发树只移除图标。",
                f"Uninstall {app_name}?\nRelease installs remove program+config; "
                "dev-tree uninstall only removes icons.",
            ),
            wrap=True,
            xalign=0,
        ),
        False,
        False,
        0,
    )
    chk_keep = Gtk.CheckButton(label=T("保留配置与数据", "Keep config and data"))
    chk_keep.set_active(False)
    if show_keep_config:
        box.pack_start(chk_keep, False, False, 0)

    btn_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
    btn_row.set_halign(Gtk.Align.END)
    box.pack_end(btn_row, False, False, 0)
    btn_cancel = Gtk.Button(label=T("取消", "Cancel"))
    btn_ok = Gtk.Button(label=T("卸载", "Uninstall"))
    btn_ok.get_style_context().add_class("destructive-action")
    btn_row.pack_start(btn_cancel, False, False, 0)
    btn_row.pack_start(btn_ok, False, False, 0)

    state = {"ok": False}

    def finish(ok: bool):
        state["ok"] = ok
        win.destroy()

    btn_cancel.connect("clicked", lambda *_: finish(False))
    btn_ok.connect("clicked", lambda *_: finish(True))
    win.connect("delete-event", lambda *_: (finish(False), True)[1])
    win.connect("destroy", lambda *_: Gtk.main_quit())
    win.show_all()
    Gtk.main()
    if not state["ok"]:
        emit(CANCELLED=1)
        return 1
    emit(CANCELLED=0, WANT_KEEP_CONFIG=(1 if chk_keep.get_active() else 0))
    return 0


def dialog_list(title: str, text: str, choices: list[str]) -> int:
    """choices are 'id|label' or plain label (id=1-based index)."""
    win = Gtk.Window(title=title)
    win.set_default_size(420, 280)
    win.set_border_width(14)
    box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
    win.add(box)
    if text:
        box.pack_start(Gtk.Label(label=text, wrap=True, xalign=0), False, False, 0)
    radios = []
    group = None
    parsed = []
    for i, c in enumerate(choices, 1):
        if "|" in c:
            cid, lab = c.split("|", 1)
        else:
            cid, lab = str(i), c
        parsed.append((cid, lab))
        r = Gtk.RadioButton.new_with_label_from_widget(group, lab)
        if group is None:
            group = r
        r._enum_id = cid  # type: ignore[attr-defined]
        radios.append(r)
        box.pack_start(r, False, False, 0)
    btn_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
    btn_row.set_halign(Gtk.Align.END)
    box.pack_end(btn_row, False, False, 0)
    btn_cancel = Gtk.Button(label=T("取消", "Cancel"))
    btn_ok = Gtk.Button(label=T("确定", "OK"))
    btn_ok.get_style_context().add_class("suggested-action")
    btn_row.pack_start(btn_cancel, False, False, 0)
    btn_row.pack_start(btn_ok, False, False, 0)
    state = {"ok": False}

    def finish(ok: bool):
        state["ok"] = ok
        win.destroy()

    btn_cancel.connect("clicked", lambda *_: finish(False))
    btn_ok.connect("clicked", lambda *_: finish(True))
    win.connect("delete-event", lambda *_: (finish(False), True)[1])
    win.connect("destroy", lambda *_: Gtk.main_quit())
    win.show_all()
    Gtk.main()
    if not state["ok"]:
        emit(CANCELLED=1)
        return 1
    chosen = parsed[0][0]
    for r in radios:
        if r.get_active():
            chosen = r._enum_id  # type: ignore[attr-defined]
            break
    emit(CANCELLED=0, CHOICE=chosen)
    return 0


def dialog_info(title: str, text: str) -> int:
    dlg = Gtk.MessageDialog(
        message_type=Gtk.MessageType.INFO,
        buttons=Gtk.ButtonsType.OK,
        text=title,
        secondary_text=text,
    )
    dlg.run()
    dlg.destroy()
    return 0


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: enum-gui-ask.py <mode> ...", file=sys.stderr)
        return 2
    mode = sys.argv[1]
    if mode == "install":
        app = sys.argv[2] if len(sys.argv) > 2 else "App"
        ask_as = sys.argv[3] == "1" if len(sys.argv) > 3 else False
        ask_upd = sys.argv[4] == "1" if len(sys.argv) > 4 else False
        return dialog_install(app, ask_as, ask_upd)
    if mode == "uninstall":
        app = sys.argv[2] if len(sys.argv) > 2 else "App"
        keep = sys.argv[3] == "1" if len(sys.argv) > 3 else True
        return dialog_uninstall(app, keep)
    if mode == "list":
        title = sys.argv[2] if len(sys.argv) > 2 else "Choose"
        text = sys.argv[3] if len(sys.argv) > 3 else ""
        return dialog_list(title, text, sys.argv[4:])
    if mode == "info":
        return dialog_info(sys.argv[2] if len(sys.argv) > 2 else "", sys.argv[3] if len(sys.argv) > 3 else "")
    print(f"unknown mode: {mode}", file=sys.stderr)
    return 2


if __name__ == "__main__":
    GLib.set_prgname("enum-setup")
    _icon_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "enum-setup.svg")
    if os.path.isfile(_icon_path):
        try:
            Gtk.Window.set_default_icon_from_file(_icon_path)
        except Exception:
            pass
    sys.exit(main())
ENUM_GUI_PY_EOF
)"

_ENUM_GUI_SVG="$(cat <<'ENUM_GUI_SVG_EOF'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256" width="256" height="256">
  <rect x="24" y="24" width="176" height="176" rx="36" fill="#1a2a56"/>
  <rect x="40" y="40" width="144" height="144" rx="24" fill="#2979ff"/>
  <rect x="54" y="64" width="34" height="34" rx="9" fill="#f5f8fc" opacity="0.92"/>
  <rect x="97" y="64" width="34" height="34" rx="9" fill="#f5f8fc" opacity="0.62"/>
  <rect x="140" y="64" width="34" height="34" rx="9" fill="#f5f8fc" opacity="0.38"/>
  <circle cx="200" cy="200" r="52" fill="#f5f8fc"/>
  <circle cx="200" cy="200" r="44" fill="#00bed6"/>
  <path d="M200 178 L200 214 M182 198 L200 216 L218 198"
        fill="none" stroke="#f5f8fc" stroke-width="12"
        stroke-linecap="round" stroke-linejoin="round"/>
</svg>
ENUM_GUI_SVG_EOF
)"

# ── 图形界面（有 DISPLAY 时默认走 GUI；ENUM_FORCE_CLI=1 或 --cli 强制终端）──
enum_gui_available() {
  [ "${ENUM_FORCE_CLI:-0}" = "1" ] && return 1
  [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ] || return 1
  command -v python3 >/dev/null 2>&1 || return 1
  # Importing GTK is not enough — DISPLAY may be set but unusable (ssh without
  # X, stale xauth). Gtk.init_check() is the real probe.
  python3 -c "import gi; gi.require_version('Gtk','3.0'); from gi.repository import Gtk; import sys; sys.exit(0 if Gtk.init_check()[0] else 1)" 2>/dev/null
}

# GUI 代码内嵌成字符串，不再是并排的文件——每次弹窗前落进一个 mktemp -d 建的
# 临时目录（随机名、0700 权限，不是可预测的 /tmp 路径），图标 svg 落在同一
# 目录，用完立刻整个目录删掉。
enum_gui_eval() {
  # Must not eval: the install-path field is free text. Spaces used to truncate
  # PREFIX; a semicolon ran as a shell command. Parse KEY=rest-of-line instead.
  local _out _line _key _tmpdir _pyfile _rc
  local _keys=" CANCELLED MODE PREFIX WANT_INPLACE WANT_DESKTOP_ICON WANT_AUTOSTART WANT_AUTO_UPDATE DO_UNINSTALL KEEP_CONFIG WANT_KEEP_CONFIG APPS SELECTED CHOICE VALUE "
  _tmpdir="$(mktemp -d)" || return 1
  _pyfile="$_tmpdir/enum-gui-ask.py"
  printf '%s\n' "$_ENUM_GUI_PY" > "$_pyfile"
  printf '%s\n' "$_ENUM_GUI_SVG" > "$_tmpdir/enum-setup.svg"
  _out="$(python3 "$_pyfile" "$@")"; _rc=$?
  rm -rf "$_tmpdir"
  [ "$_rc" = "0" ] || return "$_rc"
  while IFS= read -r _line; do
    _line="${_line%$'
'}"
    case "$_line" in *=*) ;; *) continue ;; esac
    _key="${_line%%=*}"
    case "$_key" in ''|*[!A-Za-z0-9_]*) continue ;; esac
    case "$_keys" in
      *" $_key "*) printf -v "$_key" '%s' "${_line#*=}" ;;
    esac
  done <<< "$_out"
  return 0
}


# ── ENum Setup 单实例锁（与 EnumSetup / 各 install.sh 共用）────────────────
_ENUM_LOCK_DIR="${XDG_RUNTIME_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/enum}"
_ENUM_LOCK_FILE="$_ENUM_LOCK_DIR/enum-setup-${UID:-$(id -u)}.lock"
# 锁被别人占着时，别只甩一句「自己去找那个窗口关掉」——试着直接把它拉到前台。
# 所有 enum-gui-ask.py（EnumSetup 那份 + 各应用自己这份）都用同一个
# GLib.set_prgname "enum-setup"，不管当前弹的是总安装中心还是这个应用自己的
# 对话框，WM_CLASS 都一样，靠它找、不用管标题文字。wmctrl/xdotool 都是可选的
# X11 工具，两个都没有或者压根没找到窗口（比如另一个实例是纯终端问答，没有
# 窗口）就照旧退回文字提示。
enum_activate_running_instance() {
  [ -n "${DISPLAY:-}" ] || return 1
  if command -v wmctrl >/dev/null 2>&1; then
    wmctrl -x -a enum-setup 2>/dev/null && return 0
  fi
  if command -v xdotool >/dev/null 2>&1; then
    xdotool search --class enum-setup windowactivate 2>/dev/null && return 0
  fi
  return 1
}
# run_one() 真开始装一个应用时一定会多出一个 bash 子进程（bash "$app/install.sh"
# ...）——这是"已经在装"和"还停在选择界面"之间唯一靠谱的分界。GUI 选择器本身
# 是 python 不是 bash；纯终端问答连子进程都没有。查不清楚（没有 ps）就当作
# "不确定"，什么都不做，宁可不动手也不能误杀真在干活的实例。
enum_lock_holder_is_idle() {
  local pids="$1" p line
  command -v ps >/dev/null 2>&1 || return 1
  for p in $pids; do
    while IFS= read -r line; do
      [ -n "$line" ] || continue
      case "$line" in *bash*|*/sh|sh) return 1 ;; esac
    done < <(ps -o comm= --ppid "$p" 2>/dev/null)
  done
  return 0
}

# 拉不到窗口的最后一步：确认对方真的没在干活（见上）才会走到这——先礼后兵，
# TERM 给它几秒钟自己正常退出、自己放开 flock，仍不放手才 KILL。fuser/ps
# 缺一个都直接放弃，不摸黑硬来。
enum_reclaim_stale_lock() {
  local pids p other_pids="" i=0
  command -v fuser >/dev/null 2>&1 || return 1
  command -v ps >/dev/null 2>&1 || return 1
  pids="$(fuser "$_ENUM_LOCK_FILE" 2>/dev/null)"
  [ -n "$pids" ] || return 1
  # fuser 报的是"谁对这个文件开着 fd"——本进程自己刚 exec 9> 过它，也会被算
  # 进去，绝不能把自己也 kill 掉。
  for p in $pids; do
    [ "$p" = "$$" ] && continue
    other_pids="$other_pids $p"
  done
  [ -n "$other_pids" ] || return 1
  enum_lock_holder_is_idle "$other_pids" || return 1
  for p in $other_pids; do kill -TERM "$p" 2>/dev/null; done
  while [ "$i" -lt 20 ]; do
    flock -n 9 2>/dev/null && return 0
    sleep 0.2
    i=$((i + 1))
  done
  for p in $other_pids; do kill -KILL "$p" 2>/dev/null; done
  sleep 0.3
  flock -n 9 2>/dev/null
}

enum_setup_acquire_lock() {
  if [ "${ENUM_SETUP_NESTED:-0}" = "1" ]; then return 0; fi
  mkdir -p "$_ENUM_LOCK_DIR"
  exec 9>"$_ENUM_LOCK_FILE"
  flock -n 9 && return 0

  if enum_activate_running_instance; then
    say "已经有一个安装器在运行，已经帮你切过去了。" \
        "An installer is already running; switched you to that window."
    exit 1
  fi

  if enum_reclaim_stale_lock && flock -n 9 2>/dev/null; then
    say "另一个安装器实例卡住了（拉不到前台，也没有在装任何东西），已经帮你结束并接管。" \
        "The other installer instance was stuck (no window to switch to, and it wasn't installing anything); ended it and took over."
    return 0
  fi

  say "已有安装器在运行，请先关掉另一个窗口。" \
      "Another installer is already running; close it first."
  exit 1
}
enum_is_dev_tree() {
  case "$SRC" in
    */Enum\ Code/*) return 0 ;;
  esac
  [ "${ENUM_DEV_TREE:-0}" = "1" ] && return 0
  return 1
}
enum_scan_remove_desktop() {
  local desk
  for desk in \
      "$(as_desk_user env HOME="$DESK_HOME" xdg-user-dir DESKTOP 2>/dev/null || true)" \
      "$DESK_HOME/Desktop" "$DESK_HOME/桌面"; do
    [ -n "$desk" ] && [ -d "$desk" ] || continue
    rm -f "$desk/$APP_ID.desktop" 2>/dev/null || true
    for f in "$desk"/*.desktop; do
      [ -f "$f" ] || continue
      if grep -qE "Exec=.*$APP_ID|/enum/$APP_ID/" "$f" 2>/dev/null; then
        rm -f "$f" 2>/dev/null || true
      fi
    done
  done
}

enum_setup_acquire_lock

# ── 交互模式：无参数时进入；有图形界面优先 GUI，否则终端问答 ───────────────
INTERACTIVE=0
USE_GUI=0
if enum_gui_available; then USE_GUI=1; fi
if [ "$NARGS" -eq 0 ]; then
  if [ "$USE_GUI" = "1" ] || [ -t 0 ]; then INTERACTIVE=1; fi
fi
if [ "$DO_UNINSTALL" = "1" ] && [ "$USE_GUI" = "1" ] && [ "${ENUM_SETUP_NESTED:-0}" != "1" ] \
   && [ "${ENUM_UNINSTALL_CONFIRMED:-0}" != "1" ] && [ "${WANT_YES:-0}" != "1" ]; then
  INTERACTIVE=1
fi

if [ "$INTERACTIVE" = "1" ] && [ "$USE_GUI" = "1" ]; then
  if [ "$DO_UNINSTALL" = "1" ]; then
    _keep_ui=1
    case "$SRC" in */Enum\ Code/*) _keep_ui=0 ;; esac
    [ "${ENUM_DEV_TREE:-0}" = "1" ] && _keep_ui=0
    if ! enum_gui_eval uninstall "$APP_NAME" "$_keep_ui"; then
      say "已取消。" "Cancelled."; exit 0
    fi
    [ "${CANCELLED:-0}" = "1" ] && { say "已取消。" "Cancelled."; exit 0; }
    ENUM_UNINSTALL_CONFIRMED=1
  else
    if ! enum_gui_eval install "$APP_NAME" "${ASK_AUTOSTART:-0}" "${ASK_AUTO_UPDATE:-0}"; then
      say "已取消。" "Cancelled."; exit 0
    fi
    [ "${CANCELLED:-0}" = "1" ] && { say "已取消。" "Cancelled."; exit 0; }
    if [ -n "${PREFIX:-}" ]; then
      case "$PREFIX" in "~"|"~/"*) PREFIX="$HOME${PREFIX#\~}" ;; esac
    fi
  fi
elif [ "$INTERACTIVE" = "1" ]; then
  say "▶ 安装 $APP_NAME" "▶ Installing $APP_NAME"
  echo ""
  say "安装位置：" "Install location:"
  say "  1) 用户程序目录 ~/.local/share/enum（默认）" "  1) User ~/.local/share/enum (default)"
  say "  2) 自定义路径" "  2) Custom path"
  say "  3) 系统目录 /opt/enum" "  3) System /opt/enum"
  say "  4) 开发树原地" "  4) In-place from source"
  printf '%s' "$(tr_ "请选择 [1/2/3/4]（回车=1）: " "Choose [1/2/3/4] (Enter=1): ")"
  read -r _choice || _choice=""
  case "$_choice" in
    2)
      printf '%s' "$(tr_ "请输入安装路径: " "Install path: ")"
      read -r _p || _p=""
      case "$_p" in "~"|"~/"*) _p="$HOME${_p#\~}" ;; esac
      if [ -n "$_p" ]; then PREFIX="$_p"; else say "未输入路径，改用用户目录。" "No path given, using user directory."; fi
      ;;
    3) MODE="system" ;;
    4) WANT_INPLACE="1" ;;
    *) : ;;
  esac
  if [ "${ASK_AUTOSTART:-0}" = "1" ]; then
    printf '%s' "$(tr_ "开机自启动 $APP_NAME？[y/N]: " "Start $APP_NAME at login? [y/N]: ")"
    read -r _a || _a=""
    case "$_a" in y|Y|yes|YES) WANT_AUTOSTART="1" ;; *) WANT_AUTOSTART="0" ;; esac
  fi
  if [ "${ASK_AUTO_UPDATE:-0}" = "1" ]; then
    printf '%s' "$(tr_ "启动时自动检查更新？[Y/n]: " "Check for updates at startup? [Y/n]: ")"
    read -r _u || _u=""
    case "$_u" in n|N|no|NO) WANT_AUTO_UPDATE="0" ;; *) WANT_AUTO_UPDATE="1" ;; esac
  fi
  printf '%s' "$(tr_ "在桌面放一个图标？[Y/n]: " "Desktop icon? [Y/n]: ")"
  read -r _d || _d=""
  case "$_d" in n|N|no|NO) WANT_DESKTOP_ICON="0" ;; *) WANT_DESKTOP_ICON="1" ;; esac
  echo ""
fi

# 需要 root 权限时的包装：系统级用 sudo，用户级为空（永不弹认证）
# 密码永远由 sudo 自己在终端提示，本脚本不读取、不保存任何密码。
SUDO=""
if [ "$MODE" = "system" ]; then
  if command -v sudo >/dev/null 2>&1; then SUDO="sudo"; else
    say "⚠ --system 需要 root，但没有 sudo；请用 root 运行，或改用默认的用户级安装。" \
        "⚠ --system needs root but sudo is missing; run as root or use the default user-level install."
    exit 1
  fi
fi

# 桌面用户：即使整个脚本被 root 运行，桌面图标仍落到真正的桌面用户身上
if [ "$(id -u)" = "0" ] && [ -n "${SUDO_USER:-}" ]; then
  DESK_USER="$SUDO_USER"
  DESK_HOME="$(getent passwd "$DESK_USER" | cut -d: -f6)"
  [ -z "$DESK_HOME" ] && DESK_HOME="$HOME"
else
  DESK_USER="$(id -un)"
  DESK_HOME="$HOME"
fi
as_desk_user() {
  if [ "$(id -u)" = "0" ] && [ -n "${SUDO_USER:-}" ]; then
    sudo -u "$DESK_USER" DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$(id -u "$DESK_USER")/bus" "$@"
  else
    "$@"
  fi
}

# 卸载配套：把安装阶段放进图标主题的 PNG 撤掉。装的时候进的是哪个主题目录
# 取决于当时的模式，卸载不一定带同样的参数，所以用户级、系统级两处都清。
remove_icon() {
  _icon_id="$1"
  for _thd in "${XDG_DATA_HOME:-$DESK_HOME/.local/share}/icons/hicolor" /usr/share/icons/hicolor; do
    for _sz in 256 128 64 48 32 24 16; do
      _f="$_thd/${_sz}x${_sz}/apps/$_icon_id.png"
      [ -e "$_f" ] || continue
      as_desk_user rm -f "$_f" 2>/dev/null || $SUDO rm -f "$_f" 2>/dev/null || true
    done
  done
}
refresh_icon_cache() {
  command -v gtk-update-icon-cache >/dev/null 2>&1 || return 0
  for _thd in "${XDG_DATA_HOME:-$DESK_HOME/.local/share}/icons/hicolor" /usr/share/icons/hicolor; do
    [ -d "$_thd" ] || continue
    as_desk_user gtk-update-icon-cache -f -t "$_thd" 2>/dev/null \
      || $SUDO gtk-update-icon-cache -f -t "$_thd" 2>/dev/null || true
  done
}

if [ "$MODE" = "system" ]; then
  APPS_DIR="/usr/share/applications"
  DEFAULT_PREFIX="/opt/enum"
else
  APPS_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
  DEFAULT_PREFIX="$SRC"
fi

DESKTOP_DIR="$(as_desk_user env HOME="$DESK_HOME" xdg-user-dir DESKTOP 2>/dev/null || true)"
[ -z "$DESKTOP_DIR" ] || [ ! -d "$DESKTOP_DIR" ] && DESKTOP_DIR="$DESK_HOME/Desktop"

AUTOSTART_DIR="$DESK_HOME/.config/autostart"
[ "$DESK_HOME" = "$HOME" ] && AUTOSTART_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/autostart"

# 强制默认程序目录与配置分离（覆盖脚本前面可能把 DEFAULT_PREFIX 设成 $SRC 的旧逻辑）
if [ "${WANT_INPLACE:-0}" = "1" ]; then
  INSTALL_DIR="$SRC"
elif [ -n "$PREFIX" ]; then
  INSTALL_DIR="$PREFIX/$APP_ID"
elif [ "$MODE" = "system" ]; then
  INSTALL_DIR="/opt/enum/$APP_ID"
else
  INSTALL_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/enum/$APP_ID"
fi

# 自定义路径落在没有写权限的目录（如 /opt）时同样走 sudo——
# 「想装进管理员的文件夹，输入密码就行」，密码仍由 sudo 自己提示。
if [ -z "$SUDO" ] && [ "$INSTALL_DIR" != "$SRC" ]; then
  _probe="$(dirname "$INSTALL_DIR")"
  while [ ! -e "$_probe" ]; do _probe="$(dirname "$_probe")"; done
  if [ ! -w "$_probe" ]; then
    if command -v sudo >/dev/null 2>&1; then
      say "目标目录需要管理员权限，sudo 可能提示输入密码。" \
          "Target directory needs admin rights; sudo may prompt for your password."
      SUDO="sudo"
    else
      say "⚠ 目标目录不可写且没有 sudo，请换一个路径。" \
          "⚠ Target directory is not writable and sudo is missing; choose another path."
      exit 1
    fi
  fi
fi

# ── 卸载 ────────────────────────────────────────────────────────────────────
if [ "$DO_UNINSTALL" = "1" ]; then
  $SUDO rm -f "$APPS_DIR/$APP_ID.desktop"
  rm -f "$AUTOSTART_DIR/$APP_ID.desktop" 2>/dev/null || true
  enum_scan_remove_desktop
  command -v update-desktop-database >/dev/null 2>&1 && $SUDO update-desktop-database "$APPS_DIR" 2>/dev/null || true
  remove_icon "$APP_ID"
  remove_icon "$APP_ID-rainbow"
  for _pair in ${EXTRA_ICONS:-}; do remove_icon "${_pair%%:*}"; done
  refresh_icon_cache
  _dev=0
  enum_is_dev_tree && _dev=1
  if [ "$_dev" = "1" ]; then
    say "检测到 Enum 开发树：不删除源码与配置（只移除菜单/桌面/自启）。" \
        "Dev tree detected: source and config kept (menu/desktop/autostart only)."
  else
    if [ "$INSTALL_DIR" != "$SRC" ] && [ -d "$INSTALL_DIR" ]; then
      $SUDO rm -rf "$INSTALL_DIR"
      say "已删除程序目录：$INSTALL_DIR" "Removed program directory: $INSTALL_DIR"
    fi
    if [ "${WANT_KEEP_CONFIG:-0}" != "1" ]; then
      _oldifs="$IFS"; IFS='
'
      for _c in $CONFIG_PATHS; do
        case "$_c" in "~"|"~/"*) _c="$HOME${_c#\~}" ;; esac
        [ -e "$_c" ] && rm -rf "$_c" && say "已删除配置：$_c" "Removed config: $_c"
      done
      for _d in $DATA_PATHS; do
        case "$_d" in "~"|"~/"*) _d="$HOME${_d#\~}" ;; esac
        [ -e "$_d" ] && rm -rf "$_d" && say "已删除数据：$_d" "Removed data: $_d"
      done
      IFS="$_oldifs"
      # 组织目录是上游作者名，可能被他的其他程序共用——只在空了之后才收掉
      rmdir "$HOME/.config/Nick Korotysh" 2>/dev/null || true
    else
      say "已按 --keep-config 保留配置与数据。" "Kept config/data per --keep-config."
    fi
  fi
  say "✅ 已卸载。" "✅ Uninstalled."
  # 仅交互安装弹窗：被 ENum 安装中心批量调用时 INTERACTIVE=0，否则每卸一个就点一次确定。
  if [ "${USE_GUI:-0}" = "1" ] && [ "${INTERACTIVE:-0}" = "1" ]; then
    enum_gui_eval info "$(tr_ "卸载完成" "Uninstalled")" "$(tr_ "$APP_NAME 已卸载。" "$APP_NAME has been uninstalled.")" || true
  fi
  exit 0
fi

[ "$INTERACTIVE" = "1" ] || say "▶ 安装 $APP_NAME（模式：$MODE）" "▶ Installing $APP_NAME (mode: $MODE)"

# ── Digital Clock 4 专属：识别发布包 / 源码，源码就先编译出便携目录 ─────────
#  便携目录布局（rpath $ORIGIN 已编入，无需 LD_LIBRARY_PATH）：
#    digital_clock  lib/*.so.1  plugins/*.so  clock-icon.png  + 许可证/说明
# 必须同时是普通文件——源码树里有个同名的 digital_clock/ 目录，
# 而 [ -x 目录 ] 恒为真，只用 -x 会把整个源码树当成发布包复制走。
is_exe() { [ -f "$1" ] && [ -x "$1" ]; }

PAYLOAD="$SRC"
if ! is_exe "$SRC/digital_clock"; then
  PAYLOAD="$SRC/dist/$APP_ID"
  if ! is_exe "$PAYLOAD/digital_clock"; then
    say "▶ 未发现编译好的程序，从源码编译（首次需要几分钟）..." \
        "▶ No prebuilt binary found, building from source (takes a few minutes)..."
    _deps="build-essential qtbase5-dev qtbase5-dev-tools qttools5-dev-tools libqt5svg5-dev libqt5x11extras5-dev qtmultimedia5-dev libqt5texttospeech5-dev libxi-dev"
    if ! command -v qmake >/dev/null 2>&1 || ! command -v g++ >/dev/null 2>&1; then
      say "⚠ 缺少编译工具。请先安装依赖后重试：" "⚠ Build tools missing. Install dependencies first, then retry:"
      echo "    sudo apt-get install -y $_deps"
      exit 1
    fi
    BUILD_DIR="$SRC/build-install"
    mkdir -p "$BUILD_DIR"
    if ! (cd "$BUILD_DIR" && qmake -r "$SRC/DigitalClock.pro" && make -j"$(nproc)") >"$BUILD_DIR/build.log" 2>&1; then
      say "⚠ 编译失败，日志：$BUILD_DIR/build.log；若缺 Qt 模块请安装依赖：" \
          "⚠ Build failed, log: $BUILD_DIR/build.log; if a Qt module is missing install:"
      echo "    sudo apt-get install -y $_deps"
      tail -20 "$BUILD_DIR/build.log"
      exit 1
    fi
    mkdir -p "$PAYLOAD/lib" "$PAYLOAD/plugins"
    cp "$BUILD_DIR/digital_clock/digital_clock" "$PAYLOAD/"
    for _l in clock_common skin_draw plugin_core; do
      cp "$(readlink -f "$BUILD_DIR/$_l/lib$_l.so.1")" "$PAYLOAD/lib/lib$_l.so.1"
    done
    cp "$BUILD_DIR"/plugins/*/*.so "$PAYLOAD/plugins/"
    # Qt 图标引擎插件，必须放在 iconengines/ 下 Qt 才会加载。
    # 它渲染所有 .svg.p 图标（右键菜单、插件图标），让它们跟随主题配色——
    # Windows 版就是这么带的（plugins/iconengines/paletteicon.dll）。
    mkdir -p "$PAYLOAD/iconengines"
    cp "$BUILD_DIR/paletteicon/libpaletteicon.so" "$PAYLOAD/iconengines/"
    # Qt 自己的界面文字（对话框按钮、字体/颜色选择器）。Windows 版把它们放在
    # translations/ 里随包分发，这里照做，否则中文系统上会出现「确定/取消」
    # 仍是 OK/Cancel 的中英混排。
    _qt_tr="$(qmake -query QT_INSTALL_TRANSLATIONS 2>/dev/null)"
    if [ -d "$_qt_tr" ]; then
      mkdir -p "$PAYLOAD/translations"
      cp "$_qt_tr"/qt_*.qm "$_qt_tr"/qtbase_*.qm "$PAYLOAD/translations/" 2>/dev/null || true
    fi
    # 程序图标：原作者藏在「关于」里的彩蛋插画，只用在「程序列表」这一处身份上
    # （按明确要求）；彩虹色盘 clock_icon.png 另外单独打包一份，专供桌面快捷
    # 方式和托盘图标使用——这两处不该跟着「程序列表」一起变成彩蛋图。
    cp "$SRC/digital_clock/resources/images/if_clock-c_750020.png" "$PAYLOAD/clock-icon.png"
    cp "$SRC/digital_clock/resources/images/clock_icon.png" "$PAYLOAD/clock-icon-rainbow.png"
    # 皮肤与纹理：和 Windows 版一致，程序按 <程序目录>/skins、/textures 查找
    # 连同各自的来源/授权说明一起复制——这些素材是第三方的，
    # 真正产生授权风险的是「分发」这一步，说明必须跟着发布包走。
    [ -d "$SRC/skins" ] && cp -r "$SRC/skins" "$PAYLOAD/"
    [ -d "$SRC/textures" ] && cp -r "$SRC/textures" "$PAYLOAD/"
    for _f in LICENSE.txt MODIFICATIONS.md changelog.txt README.md README.zh-CN.md; do
      [ -f "$SRC/$_f" ] && cp "$SRC/$_f" "$PAYLOAD/"
    done
    say "▶ 编译完成" "▶ Build finished"
  fi
  # 数据文件每次都重新同步：dist/ 可能是上一次构建留下的，
  # 二进制在就跳过编译，但皮肤/纹理/说明文件/图标可能已经更新过。
  for _d in skins textures; do
    [ -d "$SRC/$_d" ] && { rm -rf "$PAYLOAD/$_d"; cp -r "$SRC/$_d" "$PAYLOAD/"; }
  done
  [ -f "$SRC/digital_clock/resources/images/if_clock-c_750020.png" ] && \
    cp "$SRC/digital_clock/resources/images/if_clock-c_750020.png" "$PAYLOAD/clock-icon.png"
  [ -f "$SRC/digital_clock/resources/images/clock_icon.png" ] && \
    cp "$SRC/digital_clock/resources/images/clock_icon.png" "$PAYLOAD/clock-icon-rainbow.png"
  for _f in LICENSE.txt MODIFICATIONS.md changelog.txt README.md README.zh-CN.md; do
    [ -f "$SRC/$_f" ] && cp "$SRC/$_f" "$PAYLOAD/"
  done
  # 两份 README 里引用的文档和第三方许可证也要进包，否则包内全是断链——
  # 尤其 QHotkey/SingleApplication 那两条：README 声称内置了这两个库并给出
  # 许可证链接，包里却找不到文件，等于分发时缺了物证。
  mkdir -p "$PAYLOAD/docs" "$PAYLOAD/3rdparty/QHotkey" "$PAYLOAD/3rdparty/SingleApplication"
  for _f in why-linux-still-works.md why-linux-still-works.zh-CN.md README.upstream.md; do
    [ -f "$SRC/docs/$_f" ] && cp "$SRC/docs/$_f" "$PAYLOAD/docs/"
  done
  for _s in screenshot-zh.png screenshot-en.png; do
    [ -f "$SRC/docs/$_s" ] && cp "$SRC/docs/$_s" "$PAYLOAD/docs/"
  done
  cp "$SRC/3rdparty/QHotkey/LICENSE" "$PAYLOAD/3rdparty/QHotkey/" 2>/dev/null || true
  cp "$SRC/3rdparty/SingleApplication/LICENSE" "$PAYLOAD/3rdparty/SingleApplication/" 2>/dev/null || true
fi
# 原地安装且是源码检出时，入口和图标都在 dist 子目录里
if [ "$INSTALL_DIR" = "$SRC" ] && [ "$PAYLOAD" != "$SRC" ]; then
  EXEC_REL="dist/$APP_ID/digital_clock"
  APP_ICON="dist/$APP_ID/clock-icon.png"
  APP_ICON_RAINBOW="dist/$APP_ID/clock-icon-rainbow.png"
fi
# 开发树兜底：只有 build/ 二进制、没有 dist/ 时也能装图标（别再写不存在的
# digital_clock/resources/clock-icon.png —— 真文件是 images/if_clock-c_750020.png）
# 先查 PAYLOAD 再查 INSTALL_DIR：--prefix 拷贝式安装走到这里时目标目录还没
# 复制，只查 INSTALL_DIR 会误判成「没图标」，把 APP_ICON 换成源码树相对路径，
# 结果图标主题装不上、.desktop 的 Icon= 是解析不了的相对路径。彩虹图标同理，
# 直接落回源码树里的 clock_icon.png（不需要单独打包一份）。
if [ ! -f "$PAYLOAD/$APP_ICON" ] && [ ! -f "$INSTALL_DIR/$APP_ICON" ]; then
  if [ -f "$SRC/digital_clock/resources/images/if_clock-c_750020.png" ]; then
    APP_ICON="digital_clock/resources/images/if_clock-c_750020.png"
  fi
fi
if [ ! -f "$PAYLOAD/$APP_ICON_RAINBOW" ] && [ ! -f "$INSTALL_DIR/$APP_ICON_RAINBOW" ]; then
  if [ -f "$SRC/digital_clock/resources/images/clock_icon.png" ]; then
    APP_ICON_RAINBOW="digital_clock/resources/images/clock_icon.png"
  fi
fi
if [ "$INSTALL_DIR" = "$SRC" ] && ! is_exe "$INSTALL_DIR/$EXEC_REL"; then
  if is_exe "$SRC/build/digital_clock/digital_clock"; then
    EXEC_REL="build/digital_clock/digital_clock"
  fi
fi

# 覆盖安装时旧版本可能正在运行——正在执行的文件不能被写，cp 会以
# 「Text file busy」失败。先请它退出（升级重装是常态，不是异常）。
if [ -x "$INSTALL_DIR/digital_clock" ]; then
  # 按可执行文件的真实路径匹配，不拿用户给的 --prefix 去当正则
  # （路径里的 . + 等字符会让 pkill -f 误伤别的进程）。
  _running=""
  for _p in $(pgrep -x digital_clock 2>/dev/null); do
    [ "$(readlink -f "/proc/$_p/exe" 2>/dev/null)" = "$(readlink -f "$INSTALL_DIR/digital_clock")" ] \
      && _running="$_running $_p"
  done
  if [ -n "$_running" ]; then
    say "▶ 检测到 $APP_NAME 正在运行，先退出旧版本 ..." "▶ $APP_NAME is running, stopping the old version first ..."
    kill $_running 2>/dev/null || true
    sleep 1
    WAS_RUNNING=1        # 装完再拉起来，免得用户以为程序被装没了
  fi
fi

# ── 复制程序到目标位置（仅当不是原地运行；只复制便携目录，不带源码/中间产物）──
if [ "$INSTALL_DIR" != "$SRC" ]; then
  say "▶ 复制程序到 $INSTALL_DIR ..." "▶ Copying program to $INSTALL_DIR ..."
  $SUDO mkdir -p "$INSTALL_DIR"
  $SUDO cp -a "$PAYLOAD/." "$INSTALL_DIR/"
  # cp -a 保留属主：系统级安装后 /opt 里的二进制会归发起安装的普通用户所有，
  # 而其他用户（含 root）会执行它——那是一条本地提权路径。目录本身是 root 属主，
  # 光看 ls -ld 发现不了。系统级安装一律把程序收归 root。
  if [ "$MODE" = "system" ] || [ "$(id -u)" = "0" ]; then
    $SUDO chown -R root:root "$INSTALL_DIR"
    $SUDO chmod -R go-w "$INSTALL_DIR"
  fi
fi
RUN_DIR="$INSTALL_DIR"

$SUDO chmod +x "$RUN_DIR/$EXEC_REL" 2>/dev/null || true

# ── 把图标装进图标主题 ──────────────────────────────────────────────────────
# 托盘图标必须走这条路：StatusNotifierItem 的 IconName 属性是按图标主题查名字的，
# 给绝对路径的话 indicator-application（Ubuntu/GNOME）解析不了——托盘项注册成功
# 但永远画不出来。装进 hicolor 后程序按主题名取用。
#
# install_theme_icon <源文件（相对RUN_DIR）> <主题图标名>
# 成功则把结果放进 $_THEME_ICON_RESULT（主题名，可直接写进 .desktop 的 Icon=），
# 失败（没装进主题）则放绝对路径兜底。装两份不同身份的图标（程序列表用彩蛋、
# 桌面快捷方式+托盘用彩虹），因为 GNOME 的 StatusNotifierItem host 和 .desktop
# 的 Icon= 都是按图标名查找的，两个身份必须是两个不同的主题名，不能共用一个。
install_theme_icon() {
  local _src_rel="$1" _theme_name="$2"
  _THEME_ICON_RESULT="$_src_rel"
  [ -f "$RUN_DIR/$_src_rel" ] || return
  if [ "$MODE" = "system" ]; then local _theme_dir="/usr/share/icons/hicolor"
  else local _theme_dir="${XDG_DATA_HOME:-$DESK_HOME/.local/share}/icons/hicolor"; fi
  local _installed=0
  for _sz in 256 128 64 48 32 24 16; do
    local _d="$_theme_dir/${_sz}x${_sz}/apps"
    as_desk_user mkdir -p "$_d" 2>/dev/null || $SUDO mkdir -p "$_d" 2>/dev/null || continue
    if command -v convert >/dev/null 2>&1; then
      # mktemp（而不是可预测的 /tmp 固定名）：固定名可被本地其他用户先放一个
      # 符号链接，convert 会顺着链接写到别处去。
      local _tmpicon="$(mktemp --suffix=.png)"
      convert "$RUN_DIR/$_src_rel" -resize ${_sz}x${_sz} "$_tmpicon" 2>/dev/null \
        && chmod 644 "$_tmpicon" 2>/dev/null \
        && { as_desk_user cp "$_tmpicon" "$_d/$_theme_name.png" 2>/dev/null \
             || $SUDO cp "$_tmpicon" "$_d/$_theme_name.png" 2>/dev/null; } \
        && _installed=1
      rm -f "$_tmpicon"
    elif [ "$_sz" = "256" ]; then
      # 没有 convert 就只放原图到 256 档
      { as_desk_user cp "$RUN_DIR/$_src_rel" "$_d/$_theme_name.png" 2>/dev/null \
        || $SUDO cp "$RUN_DIR/$_src_rel" "$_d/$_theme_name.png" 2>/dev/null; } && _installed=1
    fi
  done
  if [ "$_installed" = "1" ]; then
    # 一个图标主题目录必须有 index.theme 才算合法主题，否则部分消费方
    # （包括 GNOME Shell 的托盘扩展）会直接跳过整个目录——表现就是
    # D-Bus 上图标名一切正常，屏幕上却什么都不显示。
    if [ ! -f "$_theme_dir/index.theme" ]; then
      local _idx="$(mktemp)"
      cat > "$_idx" <<'IDXEOF'
[Icon Theme]
Name=Hicolor
Comment=Fallback icon theme
Hidden=true
Directories=16x16/apps,24x24/apps,32x32/apps,48x48/apps,64x64/apps,128x128/apps,256x256/apps
IDXEOF
      for _s in 16 24 32 48 64 128 256; do
        printf '\n[%sx%s/apps]\nSize=%s\nContext=Applications\nType=Threshold\n' "$_s" "$_s" "$_s" >> "$_idx"
      done
      chmod 644 "$_idx" 2>/dev/null || true   # mktemp 默认 0600，cp 到系统主题目录后别人会读不到
      as_desk_user cp "$_idx" "$_theme_dir/index.theme" 2>/dev/null \
        || $SUDO cp "$_idx" "$_theme_dir/index.theme" 2>/dev/null || true
      rm -f "$_idx"
    fi
    command -v gtk-update-icon-cache >/dev/null 2>&1 && \
      { as_desk_user gtk-update-icon-cache -f -t "$_theme_dir" 2>/dev/null \
        || $SUDO gtk-update-icon-cache -f -t "$_theme_dir" 2>/dev/null; } || true
    _THEME_ICON_RESULT="$_theme_name"   # .desktop 也用主题名，比绝对路径更规范
  else
    _THEME_ICON_RESULT="$RUN_DIR/$_src_rel"
  fi
}

install_theme_icon "$APP_ICON" "$APP_ID"
ICON_VALUE="$_THEME_ICON_RESULT"
[ "$ICON_VALUE" = "$APP_ID" ] && \
  say "▶ 图标已装入图标主题（程序列表/关于对话框用的彩蛋图，托盘图标依赖这一步的机制）" \
      "▶ Icon installed into the icon theme (app list; the tray icon relies on this same mechanism)"

install_theme_icon "$APP_ICON_RAINBOW" "$APP_ID-rainbow"
ICON_VALUE_RAINBOW="$_THEME_ICON_RESULT"

EXEC_LINE="\"$RUN_DIR/$EXEC_REL\""
[ -n "$EXEC_ARGS" ] && EXEC_LINE="$EXEC_LINE $EXEC_ARGS"

# ── 项目自定义安装钩子 ──────────────────────────────────────────────────────
CUSTOMIZE_HOOK

# make_desktop [额外Exec参数] [图标覆盖，默认 $ICON_VALUE]
make_desktop() {
local extra="${1:-}"
local icon="${2:-$ICON_VALUE}"
local exec_line="$EXEC_LINE"
[ -n "$extra" ] && exec_line="$exec_line $extra"
cat <<EOF
[Desktop Entry]
Type=Application
Version=1.0
Name=$APP_NAME
Comment=$APP_COMMENT
EOF
# 应用列表和任务栏里一律用英文名（上面的 Name=），不给 Name[zh_CN]：
# 界面语言由程序自己在启动时按 locale 适配，中文系统打开后仍然是中文界面。
[ -n "$APP_COMMENT_ZH" ] && printf 'Comment[zh_CN]=%s\nComment[zh_TW]=%s\n' "$APP_COMMENT_ZH" "$APP_COMMENT_ZH"
cat <<EOF
Exec=$exec_line
Icon=$icon
Terminal=$RUN_IN_TERMINAL
Categories=$CATEGORIES
StartupNotify=false
StartupWMClass=$APP_WMCLASS
EOF
}

# ── 写进【应用程序列表】────────────────────────────────────────────────────
$SUDO mkdir -p "$APPS_DIR"
make_desktop | $SUDO tee "$APPS_DIR/$APP_ID.desktop" >/dev/null
$SUDO chmod +x "$APPS_DIR/$APP_ID.desktop"
command -v update-desktop-database >/dev/null 2>&1 && $SUDO update-desktop-database "$APPS_DIR" 2>/dev/null || true
say "▶ 已添加到应用程序列表" "▶ Added to the application menu"

# ── 写到【桌面】并标记为「已允许运行」──────────────────────────────────────
# 桌面快捷方式用彩虹图标，不跟「程序列表」共用彩蛋图——两者是各自独立的
# .desktop 文件，图标身份可以分开指定。
if [ "$WANT_DESKTOP_ICON" = "1" ]; then
  as_desk_user mkdir -p "$DESKTOP_DIR"
  DESKTOP_FILE="$DESKTOP_DIR/$APP_ID.desktop"
  make_desktop "" "$ICON_VALUE_RAINBOW" | as_desk_user tee "$DESKTOP_FILE" >/dev/null
  as_desk_user chmod +x "$DESKTOP_FILE"
  # GNOME/Nautilus 下标记可信，双击直接运行不弹「允许启动」确认。
  as_desk_user gio set "$DESKTOP_FILE" metadata::trusted true 2>/dev/null || true
  say "▶ 已在桌面添加图标：$DESKTOP_FILE" "▶ Desktop icon added: $DESKTOP_FILE"
else
  say "▶ 已跳过桌面图标" "▶ Skipped desktop icon"
fi

# ── 开机自启动（本项目默认不问；--autostart 显式给了才设置）─────────────────
if [ "$WANT_AUTOSTART" = "1" ]; then
  # 程序自己的「开机自启」开关写的是 <可执行文件名>.desktop，Exec 形如
  # "<路径>" --autostart。这里必须写成完全一样的文件名和 Exec，否则会留下
  # 两条自启动项（开机启动两次），而且程序设置里的复选框会显示成「未启用」。
  as_desk_user mkdir -p "$AUTOSTART_DIR"
  AUTOSTART_FILE="$AUTOSTART_DIR/$(basename "$EXEC_REL").desktop"
  { make_desktop "--autostart"; printf 'X-GNOME-Autostart-enabled=true\n'; } | as_desk_user tee "$AUTOSTART_FILE" >/dev/null
  rm -f "$AUTOSTART_DIR/$APP_ID.desktop" 2>/dev/null || true   # 清掉旧版可能留下的重复项
  say "▶ 已设置开机自启动：$AUTOSTART_FILE" "▶ Autostart enabled: $AUTOSTART_FILE"
fi

echo ""
if [ "$WANT_DESKTOP_ICON" = "1" ]; then
  say "✅ 安装完成。可在应用列表搜索「$APP_NAME」或双击桌面图标启动。" \
      "✅ Installed. Search \"$APP_NAME\" in the app menu or double-click the desktop icon."
else
  say "✅ 安装完成。可在应用列表搜索「$APP_NAME」启动。" \
      "✅ Installed. Search \"$APP_NAME\" in the app menu to launch."
fi
[ -n "$POST_INSTALL_NOTE" ] && say "   $POST_INSTALL_NOTE" "   ${POST_INSTALL_NOTE_EN:-$POST_INSTALL_NOTE}"
# 仅交互安装弹窗：批量/带参安装只写终端，避免装几个应用就弹几个「完成」。
if [ "${USE_GUI:-0}" = "1" ] && [ "${INTERACTIVE:-0}" = "1" ]; then
  _msg="$(tr_ "✅ $APP_NAME 安装完成。" "✅ $APP_NAME installed.")"
  [ -n "$POST_INSTALL_NOTE" ] && _msg="$_msg\n$(tr_ "$POST_INSTALL_NOTE" "${POST_INSTALL_NOTE_EN:-$POST_INSTALL_NOTE}")"
  enum_gui_eval info "$(tr_ "安装完成" "Installed")" "$_msg" || true
fi

# 安装前它就在运行，说明用户正在用——装完自动拉起新版本，
# 不然用户看到的是「程序不见了」，还得自己去应用列表里重开一次。
if [ "${WAS_RUNNING:-0}" = "1" ] && [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ]; then
  say "▶ 正在重新启动 $APP_NAME ..." "▶ Restarting $APP_NAME ..."
  ( cd "$RUN_DIR" && setsid "$RUN_DIR/$EXEC_REL" >/dev/null 2>&1 < /dev/null & ) || true
fi
exit 0
