/*
    Digital Clock - beautiful customizable clock with plugins
    Copyright (C) 2013-2020  Nick Korotysh <nick.korotysh@gmail.com>

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

#include "tray_control.h"

#include <QIcon>
#ifdef Q_OS_WIN
#include <QSysInfo>
#include <QVersionNumber>
#endif
#include <QApplication>

#include "gui/context_menu.h"
#ifdef Q_OS_WIN
#include "platform/system_theme_tracker.h"
#endif

namespace digital_clock {
namespace gui {
#ifdef Q_OS_WIN
static QIcon pick_win10_tray_icon(bool is_light_theme)
{
  return is_light_theme ? QIcon(":/clock/icons/tray/clock-alt-b.svg") : QIcon(":/clock/icons/tray/clock-alt-w.svg");
}
#endif

QIcon TrayControl::platform_tray_icon() const
{
  QIcon tray_icon(":/clock/icons/tray/clock.svg");            // default tray icon
#if defined(Q_OS_WIN)
  if (QVersionNumber::fromString(QSysInfo::productVersion()) >= QVersionNumber(10))
    tray_icon = pick_win10_tray_icon(sys_theme_tracker_->isLightTheme());
#endif
#if defined(Q_OS_MACOS) && !defined(HAVE_NATIVE_TRAY_ICON)
  tray_icon = QIcon(":/clock/icons/tray/clock-smaller.svg");  // Qt is NOT patched
#endif
#if defined(Q_OS_LINUX)
  // Prefer a themed icon. StatusNotifierItem hosts resolve the IconName
  // property through the icon theme; when the icon is not a themed one Qt
  // falls back to writing a temporary PNG and publishing its absolute path,
  // which indicator-application (Ubuntu/GNOME) cannot resolve — the item
  // registers on the bus and nothing is ever drawn. The installer puts our
  // icon into hicolor as "digitalclock4"; the resource stays as the fallback
  // for running straight out of a build tree.
  tray_icon = QIcon::fromTheme(QStringLiteral("digitalclock4"),
                               QIcon(":/clock/icons/tray/clock-smaller.svg.p"));
#endif
  return tray_icon;
}

TrayControl::TrayControl(QObject* parent) : QObject(parent)
{
#ifdef Q_OS_WIN
  sys_theme_tracker_ = nullptr;
  if (QVersionNumber::fromString(QSysInfo::productVersion()) >= QVersionNumber(10)) {
    sys_theme_tracker_ = new SystemThemeTracker();
    connect(sys_theme_tracker_, &SystemThemeTracker::themeChanged, this, &TrayControl::SysThemeChangedHandler);
    sys_theme_tracker_->start();
  }
#endif
  tray_menu_ = new ContextMenu();
  connect(tray_menu_, &ContextMenu::VisibilityChanged, this, &TrayControl::VisibilityChanged);
  connect(tray_menu_, &ContextMenu::PositionChanged, this, &TrayControl::PositionChanged);
  connect(tray_menu_, &ContextMenu::ShowSettingsDlg, this, &TrayControl::ShowSettingsDlg);
  connect(tray_menu_, &ContextMenu::ShowAboutDlg, this, &TrayControl::ShowAboutDlg);
  connect(tray_menu_, &ContextMenu::AppExit, this, &TrayControl::AppExit);

  QIcon tray_icon = platform_tray_icon();
#ifndef Q_OS_LINUX
  // A mask icon is what macOS and the Windows 10 dark theme want. On Linux the
  // StatusNotifierItem host renders it as a monochrome template, which turns
  // our colour icon into a blank square — the item is there and you cannot see
  // it. Ship the real icon instead.
  tray_icon.setIsMask(true);
#endif
  tray_icon_ = new QSystemTrayIcon(tray_icon, this);
  tray_icon_->setVisible(true);
  tray_icon_->setContextMenu(tray_menu_->menu());
  tray_icon_->setToolTip(QApplication::applicationDisplayName() + " " + QApplication::applicationVersion());
  connect(tray_icon_, &QSystemTrayIcon::activated, this, &TrayControl::TrayEventHandler);
}

TrayControl::~TrayControl()
{
#ifdef Q_OS_WIN
  if (sys_theme_tracker_) {
    sys_theme_tracker_->stop();
    while (!sys_theme_tracker_->isFinished())
      QThread::yieldCurrentThread();
    delete sys_theme_tracker_;
  }
#endif
  delete tray_menu_;
}

QSystemTrayIcon* TrayControl::GetTrayIcon() const Q_DECL_NOEXCEPT
{
  return tray_icon_;
}

QAction* TrayControl::GetShowHideAction() const Q_DECL_NOEXCEPT
{
  return tray_menu_->visibilityAction();
}

void TrayControl::TrayEventHandler(QSystemTrayIcon::ActivationReason reason)
{
  if (reason == QSystemTrayIcon::DoubleClick) emit ShowSettingsDlg();
}

#ifdef Q_OS_WIN
void TrayControl::SysThemeChangedHandler(bool is_light_theme)
{
  tray_icon_->setIcon(pick_win10_tray_icon(is_light_theme));
}
#endif
} // namespace gui
} // namespace digital_clock
