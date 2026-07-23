/*
    Digital Clock - beautiful customizable clock with plugins
    Copyright (C) 2017-2020  Nick Korotysh <nick.korotysh@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gui/clock_window.h"

// Qt (and our own Qt-using headers) must come first: X11 headers #define Bool,
// None and Status as macros, which breaks any Qt header parsed after them.
#include <QX11Info>

#include "core/clock_settings.h"
#include "platform/fullscreen_detect.h"
#include "settings_keys.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

namespace digital_clock {
namespace gui {

static unsigned long GetCurrentDesktop(Display* display)
{
  Atom actual_type_return;
  int actual_format_return = 0;
  unsigned long nitems_return = 0;
  unsigned long bytes_after_return = 0;
  unsigned long* desktop = nullptr;
  unsigned long ret;

  if (XGetWindowProperty(display, DefaultRootWindow(display),
                         XInternAtom(display, "_NET_CURRENT_DESKTOP", False),
                         0, 1, False, XA_CARDINAL,
                         &actual_type_return, &actual_format_return,
                         &nitems_return, &bytes_after_return,
                         reinterpret_cast<unsigned char**>(&desktop)) != Success) {
    return 0;
  }
  if (actual_type_return != XA_CARDINAL || nitems_return == 0) {
    return 0;
  }

  ret = desktop[0];
  XFree(desktop);
  return ret;
}

void ClockWindow::SetVisibleOnAllDesktops(bool set)
{
  if (!QX11Info::isPlatformX11()) return;

  unsigned long desktop = GetCurrentDesktop(QX11Info::display());
  // http://stackoverflow.com/questions/16775352/keep-a-application-window-always-on-current-desktop-on-linux-and-mac/
  unsigned long data = set ? 0xFFFFFFFF : desktop;
  XChangeProperty(QX11Info::display(),
                  winId(),
                  XInternAtom(QX11Info::display(), "_NET_WM_DESKTOP", False),
                  XA_CARDINAL,
                  32,
                  PropModeReplace,
                  reinterpret_cast<unsigned char*>(&data),  // all desktop
                  1);
}

void ClockWindow::X11FullscreenWorkaround()
{
  // winId() on a window that was never shown forces creation and fires a spurious
  // move event, which would overwrite the saved position before it is restored
  if (!this->isVisible()) return;
  if (!app_config_->GetValue(OPT_STAY_ON_TOP).toBool()) return;

  // Do nothing at all while the feature is off — which is the default. Touching
  // the stay-on-top flag here regardless meant recreating the native window
  // (SetWindowFlag destroys and re-maps it on X11) and fighting the separate
  // X11BypassWindowManagerHint path that "better stay on top" uses, so
  // always-on-top misbehaved even for users who never enabled this.
  if (!fullscreen_detect_enabled_) return;

  // whatever was already fullscreen when the clock started is not what the user
  // means by "something went fullscreen" — remember it once and ignore it after
  if (!window_ignore_list_captured_) {
    window_ignore_list_captured_ = true;
    const QStringList user_list = app_config_->GetValue(OPT_FULLSCREEN_IGNORE_LST).toStringList();
    window_ignore_list_ = GetFullscreenWindowsOnSameMonitor(this->winId());
    for (const QString& cls : user_list) window_ignore_list_.insert(cls);
  }

  const bool covered = IsFullscreenWndOnSameMonitor(this->winId(), window_ignore_list_);

  if (covered) {
    if (this->windowFlags() & Qt::WindowStaysOnTopHint) {
      this->SetWindowFlag(Qt::WindowStaysOnTopHint, false);
      this->lower();
    }
  } else {
    if (!(this->windowFlags() & Qt::WindowStaysOnTopHint))
      this->SetWindowFlag(Qt::WindowStaysOnTopHint, true);
  }
}

} // namespace gui
} // namespace digital_clock
