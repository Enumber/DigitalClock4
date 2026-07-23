/*
    Digital Clock - beautiful customizable clock with plugins
    Copyright (C) 2013-2020  Nick Korotysh <nick.korotysh@gmail.com>
    X11 implementation (C) 2026 ENum

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

// X11 counterpart of fullscreen_detect.cpp, which is Win32-only. Upstream had
// no implementation here, so "hide the clock when something is fullscreen" was
// hidden from the settings on Linux.
//
// The clock only needs to know whether some *other* window is fullscreen on the
// same screen. Rather than measuring geometry, this asks the window manager:
// EWMH-compliant WMs advertise _NET_WM_STATE_FULLSCREEN on such windows, which
// is what a full-screen video player or game sets. Windows are identified by
// their WM_CLASS class name, mirroring GetWindowClassName() on Windows so the
// user-visible ignore list means the same thing on both platforms.

#include "fullscreen_detect.h"

#include <QGuiApplication>
#include <QRect>
#include <QScreen>
#include <QWindow>

#include <QX11Info>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>   // XClassHint / XGetClassHint

namespace digital_clock {

namespace {

// RAII for the buffers XGetWindowProperty hands back
class XProperty
{
public:
  XProperty(Display* dpy, Window wnd, Atom prop, Atom type)
  {
    Atom actual_type = None;
    int actual_format = 0;
    unsigned long bytes_after = 0;
    if (XGetWindowProperty(dpy, wnd, prop, 0, (~0L), False, type,
                           &actual_type, &actual_format,
                           &count_, &bytes_after, &data_) != Success) {
      data_ = nullptr;
      count_ = 0;
    }
    if (actual_type != type) {
      reset();
    }
  }

  ~XProperty() { reset(); }

  XProperty(const XProperty&) = delete;
  XProperty& operator=(const XProperty&) = delete;

  const unsigned char* data() const { return data_; }
  unsigned long count() const { return count_; }

private:
  void reset()
  {
    if (data_) XFree(data_);
    data_ = nullptr;
    count_ = 0;
  }

  unsigned char* data_ = nullptr;
  unsigned long count_ = 0;
};

QString WindowClassName(Display* dpy, Window wnd)
{
  XClassHint hint;
  hint.res_name = nullptr;
  hint.res_class = nullptr;
  if (!XGetClassHint(dpy, wnd, &hint)) return QString();
  // res_class is the "class" half of WM_CLASS, the stable per-application part
  const QString name = QString::fromLocal8Bit(hint.res_class ? hint.res_class : hint.res_name);
  if (hint.res_name) XFree(hint.res_name);
  if (hint.res_class) XFree(hint.res_class);
  return name;
}

bool IsFullscreen(Display* dpy, Window wnd)
{
  const Atom net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", True);
  const Atom net_fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", True);
  if (net_wm_state == None || net_fullscreen == None) return false;

  XProperty state(dpy, wnd, net_wm_state, XA_ATOM);
  const Atom* atoms = reinterpret_cast<const Atom*>(state.data());
  if (!atoms) return false;
  for (unsigned long i = 0; i < state.count(); ++i)
    if (atoms[i] == net_fullscreen) return true;
  return false;
}

// Which screen a window sits on, by its top-left corner in root coordinates.
QScreen* ScreenOf(Display* dpy, Window wnd)
{
  Window root = None;
  int x = 0, y = 0;
  unsigned int w = 0, h = 0, border = 0, depth = 0;
  if (!XGetGeometry(dpy, wnd, &root, &x, &y, &w, &h, &border, &depth)) return nullptr;

  int root_x = 0, root_y = 0;
  Window child = None;
  // a window's own x/y are relative to its parent, which is usually the WM frame
  if (!XTranslateCoordinates(dpy, wnd, root, 0, 0, &root_x, &root_y, &child)) return nullptr;

  return QGuiApplication::screenAt(QPoint(root_x + int(w) / 2, root_y + int(h) / 2));
}

// Walk _NET_CLIENT_LIST and hand every fullscreen window on our screen to func.
template <typename Func>
void ForEachFullscreenOnSameScreen(WId wid, Func func)
{
  if (!QX11Info::isPlatformX11()) return;
  Display* dpy = QX11Info::display();
  if (!dpy) return;

  const Atom client_list = XInternAtom(dpy, "_NET_CLIENT_LIST", True);
  if (client_list == None) return;   // not an EWMH window manager

  QScreen* own_screen = ScreenOf(dpy, static_cast<Window>(wid));

  XProperty list(dpy, DefaultRootWindow(dpy), client_list, XA_WINDOW);
  const Window* windows = reinterpret_cast<const Window*>(list.data());
  if (!windows) return;

  for (unsigned long i = 0; i < list.count(); ++i) {
    const Window wnd = windows[i];
    if (wnd == static_cast<Window>(wid)) continue;
    if (!IsFullscreen(dpy, wnd)) continue;
    // no own_screen means we could not place ourselves; treat every screen as
    // relevant rather than silently doing nothing
    if (own_screen && ScreenOf(dpy, wnd) != own_screen) continue;
    func(WindowClassName(dpy, wnd));
  }
}

} // namespace

bool IsFullscreenWndOnSameMonitor(WId wid, const QSet<QString>& ignore_list)
{
  bool found = false;
  ForEachFullscreenOnSameScreen(wid, [&] (const QString& cls) {
    if (!found && !ignore_list.contains(cls)) found = true;
  });
  return found;
}

QSet<QString> GetFullscreenWindowsOnSameMonitor(WId wid)
{
  QSet<QString> result;
  ForEachFullscreenOnSameScreen(wid, [&] (const QString& cls) { result.insert(cls); });
  return result;
}

} // namespace digital_clock
