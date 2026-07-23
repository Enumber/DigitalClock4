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
#     ② 是否开机自启动（写的是程序自己认的那个 autostart 文件，与设置里的
#        「开机启动」开关是同一件事——两边保持一致，不会出现重复启动）
#     ③ 是否放桌面图标（免「信任」确认，双击即可启动）
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

# ── 配置区（换项目时只改这里）───────────────────────────────────────────────
APP_ID="digitalclock4"                   # 唯一标识（.desktop 文件名，用英文小写连字符）
APP_WMCLASS="Digital Clock"              # 窗口 WM_CLASS 的 Class 段（实测值，别改）
APP_NAME="Digital Clock"                 # 菜单/桌面显示名称（固定英文，不跟随语言）
APP_COMMENT="Customizable desktop clock with skins and plugins"
APP_COMMENT_ZH="可换皮肤、带插件的桌面时钟"
APP_ICON="clock-icon.png"                # 便携目录里的图标（真·Digital Clock 4 图标）
EXEC_REL="digital_clock"                 # 便携目录里的主程序
EXEC_ARGS=""
RUN_IN_TERMINAL="false"
CATEGORIES="Utility;Clock;"
NEEDS_VENV="0"
VENV_DIR=".venv"
PYDEPS=""
VENV_SYSTEM_SITE="1"
ASK_AUTOSTART="1"                        # 安装时询问；写的是程序自己认的那个 autostart 文件
AUTOSTART_ARGS=""
POST_INSTALL_NOTE="开机自启动、皮肤、插件等都在程序的右键菜单 → 设置 里。"
POST_INSTALL_NOTE_EN="Autostart, skins and plugins live in the app's right-click menu -> Settings."

# 装完就能用：把「时间格式」问出来直接写进程序的配置文件，
# 免得用户装完还要自己进设置翻一遍。
CUSTOMIZE_HOOK() {
  CFG_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/Nick Korotysh"
  CFG="$CFG_DIR/Digital Clock.conf"

  if [ "$INTERACTIVE" = "1" ]; then
    say "时间格式：" "Time format:"
    say "  1) 跟随系统（默认）" "  1) Follow the system (default)"
    say "  2) 24 小时，如 21:05" "  2) 24-hour, e.g. 21:05"
    say "  3) 24 小时带秒，如 21:05:30" "  3) 24-hour with seconds, e.g. 21:05:30"
    say "  4) 12 小时带 AM/PM，如 9:05PM" "  4) 12-hour with AM/PM, e.g. 9:05PM"
    printf '%s' "$(tr_ "请选择 [1/2/3/4]（回车=1）: " "Choose [1/2/3/4] (Enter=1): ")"
    read -r _tf || _tf=""
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

交互安装会依次问你：装到哪、要不要开机自启动、要不要桌面图标。
桌面图标会被标记为可信，双击直接启动，不弹「是否允许启动」。
开机自启动写的是程序自己认的那个 autostart 文件，和「设置 → 杂项」里的
「开机启动」开关是同一件事，两边保持一致，不会重复启动。

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

The interactive install asks where to put the program, whether to start it
automatically at login, and whether you want a desktop icon. The desktop icon
is marked trusted, so double-clicking it starts the clock directly with no
"allow launching" prompt. The autostart entry uses the same file the program's
own Settings -> Misc toggle does, so the two always agree and nothing starts
twice.

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
while [ $# -gt 0 ]; do
  case "$1" in
    --system) MODE="system" ;;
    --prefix) PREFIX="$2"; shift ;;
    --prefix=*) PREFIX="${1#*=}" ;;
    --no-desktop-icon) WANT_DESKTOP_ICON="0" ;;
    --autostart) WANT_AUTOSTART="1" ;;
    --uninstall) DO_UNINSTALL="1" ;;
    --help|-h) usage; exit 0 ;;
    *) say "未知参数: $1（--help 查看用法）" "Unknown option: $1 (see --help)"; exit 1 ;;
  esac
  shift
done

SRC="$(cd "$(dirname "$0")" && pwd)"

# ── 交互模式：stdin 是 TTY 且没带任何参数才进入 ─────────────────────────────
INTERACTIVE=0
if [ -t 0 ] && [ "$NARGS" -eq 0 ]; then INTERACTIVE=1; fi

if [ "$INTERACTIVE" = "1" ]; then
  say "▶ 安装 $APP_NAME" "▶ Installing $APP_NAME"
  echo ""
  say "安装位置：" "Install location:"
  say "  1) 用户目录（默认，无需密码）" "  1) User directory (default, no password)"
  say "  2) 自定义路径" "  2) Custom path"
  say "  3) 系统目录 /opt（所有用户可用，需要管理员密码）" "  3) System directory /opt (all users, admin password required)"
  printf '%s' "$(tr_ "请选择 [1/2/3]（回车=1）: " "Choose [1/2/3] (Enter=1): ")"
  read -r _choice || _choice=""
  case "$_choice" in
    2)
      printf '%s' "$(tr_ "请输入安装路径（程序会复制到 该目录/$APP_ID）: " "Install path (program is copied to <path>/$APP_ID): ")"
      read -r _p || _p=""
      case "$_p" in "~"|"~/"*) _p="$HOME${_p#\~}" ;; esac
      if [ -n "$_p" ]; then PREFIX="$_p"; else say "未输入路径，改用用户目录。" "No path given, using user directory."; fi
      ;;
    3)
      MODE="system"
      say "系统级安装需要管理员权限，接下来 sudo 可能提示输入密码。" "System-wide install needs admin rights; sudo may prompt for your password."
      ;;
    *) : ;;
  esac
  if [ "$ASK_AUTOSTART" = "1" ]; then
    printf '%s' "$(tr_ "开机时自动启动？[y/N]: " "Start automatically at login? [y/N]: ")"
    read -r _a || _a=""
    case "$_a" in y|Y|yes|YES) WANT_AUTOSTART="1" ;; *) WANT_AUTOSTART="0" ;; esac
  fi
  printf '%s' "$(tr_ "在桌面放一个图标？[Y/n]: " "Put an icon on the desktop? [Y/n]: ")"
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

if [ "$MODE" = "system" ]; then
  APPS_DIR="/usr/share/applications"
  DEFAULT_PREFIX="/opt"
else
  APPS_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
  DEFAULT_PREFIX="$SRC"
fi

DESKTOP_DIR="$(as_desk_user env HOME="$DESK_HOME" xdg-user-dir DESKTOP 2>/dev/null || true)"
[ -z "$DESKTOP_DIR" ] || [ ! -d "$DESKTOP_DIR" ] && DESKTOP_DIR="$DESK_HOME/Desktop"

AUTOSTART_DIR="$DESK_HOME/.config/autostart"
[ "$DESK_HOME" = "$HOME" ] && AUTOSTART_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/autostart"

if [ -n "$PREFIX" ]; then
  INSTALL_DIR="$PREFIX/$APP_ID"
elif [ "$MODE" = "system" ]; then
  INSTALL_DIR="$DEFAULT_PREFIX/$APP_ID"
else
  INSTALL_DIR="$SRC"
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
  rm -f "$DESKTOP_DIR/$APP_ID.desktop" 2>/dev/null || true
  rm -f "$AUTOSTART_DIR/$APP_ID.desktop" "$AUTOSTART_DIR/$(basename "$EXEC_REL").desktop" 2>/dev/null || true
  command -v update-desktop-database >/dev/null 2>&1 && $SUDO update-desktop-database "$APPS_DIR" 2>/dev/null || true
  if [ "$INSTALL_DIR" != "$SRC" ] && [ -d "$INSTALL_DIR" ]; then
    $SUDO rm -rf "$INSTALL_DIR"
    say "已删除程序副本：$INSTALL_DIR" "Removed program copy: $INSTALL_DIR"
  fi
  say "✅ 已卸载 $APP_NAME 的图标、菜单项与自启动。" "✅ Uninstalled $APP_NAME icons, menu entry and autostart."
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
    # 真·Digital Clock 4 图标（Windows 版 exe 用的那张，彩虹色盘）。
    # 别用 images/if_clock-c_750020.png——那是作者藏在「关于」里的彩蛋插画。
    cp "$SRC/digital_clock/resources/images/clock_icon.png" "$PAYLOAD/clock-icon.png"
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
  # 二进制在就跳过编译，但皮肤/纹理/说明文件可能已经更新过。
  for _d in skins textures; do
    [ -d "$SRC/$_d" ] && { rm -rf "$PAYLOAD/$_d"; cp -r "$SRC/$_d" "$PAYLOAD/"; }
  done
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
# 但永远画不出来。装进 hicolor 后程序用 QIcon::fromTheme("digitalclock4") 取用。
ICON_VALUE="$APP_ICON"
if [ -f "$RUN_DIR/$APP_ICON" ]; then
  if [ "$MODE" = "system" ]; then ICON_THEME_DIR="/usr/share/icons/hicolor"
  else ICON_THEME_DIR="${XDG_DATA_HOME:-$DESK_HOME/.local/share}/icons/hicolor"; fi
  _installed_theme_icon=0
  for _sz in 256 128 64 48 32 24 16; do
    _d="$ICON_THEME_DIR/${_sz}x${_sz}/apps"
    as_desk_user mkdir -p "$_d" 2>/dev/null || $SUDO mkdir -p "$_d" 2>/dev/null || continue
    if command -v convert >/dev/null 2>&1; then
      convert "$RUN_DIR/$APP_ICON" -resize ${_sz}x${_sz} "/tmp/.dc4icon.$$.png" 2>/dev/null \
        && { as_desk_user cp "/tmp/.dc4icon.$$.png" "$_d/$APP_ID.png" 2>/dev/null \
             || $SUDO cp "/tmp/.dc4icon.$$.png" "$_d/$APP_ID.png" 2>/dev/null; } \
        && _installed_theme_icon=1
      rm -f "/tmp/.dc4icon.$$.png"
    elif [ "$_sz" = "256" ]; then
      # 没有 convert 就只放原图到 256 档
      { as_desk_user cp "$RUN_DIR/$APP_ICON" "$_d/$APP_ID.png" 2>/dev/null \
        || $SUDO cp "$RUN_DIR/$APP_ICON" "$_d/$APP_ID.png" 2>/dev/null; } && _installed_theme_icon=1
    fi
  done
  if [ "$_installed_theme_icon" = "1" ]; then
    # 一个图标主题目录必须有 index.theme 才算合法主题，否则部分消费方
    # （包括 GNOME Shell 的托盘扩展）会直接跳过整个目录——表现就是
    # D-Bus 上图标名一切正常，屏幕上却什么都不显示。
    if [ ! -f "$ICON_THEME_DIR/index.theme" ]; then
      _idx="$(mktemp)"
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
      as_desk_user cp "$_idx" "$ICON_THEME_DIR/index.theme" 2>/dev/null \
        || $SUDO cp "$_idx" "$ICON_THEME_DIR/index.theme" 2>/dev/null || true
      rm -f "$_idx"
    fi
    command -v gtk-update-icon-cache >/dev/null 2>&1 && \
      { as_desk_user gtk-update-icon-cache -f -t "$ICON_THEME_DIR" 2>/dev/null \
        || $SUDO gtk-update-icon-cache -f -t "$ICON_THEME_DIR" 2>/dev/null; } || true
    ICON_VALUE="$APP_ID"          # .desktop 也用主题名，比绝对路径更规范
    say "▶ 图标已装入图标主题（托盘图标依赖这一步）" "▶ Icon installed into the icon theme (the tray icon depends on this)"
  else
    ICON_VALUE="$RUN_DIR/$APP_ICON"
  fi
fi

EXEC_LINE="\"$RUN_DIR/$EXEC_REL\""
[ -n "$EXEC_ARGS" ] && EXEC_LINE="$EXEC_LINE $EXEC_ARGS"

# ── 项目自定义安装钩子 ──────────────────────────────────────────────────────
CUSTOMIZE_HOOK

# make_desktop [额外Exec参数]
make_desktop() {
local extra="${1:-}"
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
Icon=$ICON_VALUE
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
if [ "$WANT_DESKTOP_ICON" = "1" ]; then
  as_desk_user mkdir -p "$DESKTOP_DIR"
  DESKTOP_FILE="$DESKTOP_DIR/$APP_ID.desktop"
  make_desktop | as_desk_user tee "$DESKTOP_FILE" >/dev/null
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

# 安装前它就在运行，说明用户正在用——装完自动拉起新版本，
# 不然用户看到的是「程序不见了」，还得自己去应用列表里重开一次。
if [ "${WAS_RUNNING:-0}" = "1" ] && [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ]; then
  say "▶ 正在重新启动 $APP_NAME ..." "▶ Restarting $APP_NAME ..."
  ( cd "$RUN_DIR" && setsid "$RUN_DIR/$EXEC_REL" >/dev/null 2>&1 < /dev/null & ) || true
fi
exit 0
