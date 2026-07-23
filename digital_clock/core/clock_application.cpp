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

#include "clock_application.h"

#include <algorithm>
#include <functional>

#include <QAction>
#include <QUrl>
#include <QDesktopServices>
#include <QApplication>

#include "settings_storage.h"

#include "core/clock_settings.h"
#include "core/clock_state.h"
#include "core/plugin_manager.h"
#include "core/screen_placement_policy.h"
#include "core/skin_manager.h"

#include "gui/clock_window.h"
#include "gui/clock_widget.h"
#include "gui/context_menu.h"
#include "gui/about_dialog.h"
#include "gui/settings_dialog.h"
#include "gui/tray_control.h"

#include "platform/mouse_tracker.h"

namespace digital_clock {
namespace core {

namespace {

const char* const S_PREFERRED_SCREEN_KEY = "screen_placement/preferred_screen";

QVector<placement::Screen> currentScreens()
{
  QVector<placement::Screen> result;
  for (QScreen* screen : QGuiApplication::screens()) {
    result.append({
      gui::ClockWindow::ScreenId(screen),
      screen->geometry(),
      screen->availableGeometry(),
      screen == QGuiApplication::primaryScreen()
    });
  }
  return result;
}

QScreen* screenForId(const QString& id)
{
  for (QScreen* screen : QGuiApplication::screens()) {
    if (gui::ClockWindow::ScreenId(screen) == id) return screen;
  }
  return nullptr;
}

} // namespace

ClockApplication::ClockApplication(ClockSettings* config, QObject* parent) :
  QObject(parent),
  config_backend_(config->GetBackend()),
  app_config_(config)
{
  state_ = new core::ClockState(config_backend_);

  CreateWindows();

  skin_manager_ = new core::SkinManager(this);
  skin_manager_->ListSkins();
  skin_manager_->SetFallbackSkin("Electronic (default)");


  tray_control_ = new gui::TrayControl(this);
  connect(tray_control_, &gui::TrayControl::ShowSettingsDlg, this, &ClockApplication::ShowSettingsDialog);
  connect(tray_control_, &gui::TrayControl::ShowAboutDlg, this, &ClockApplication::ShowAboutDialog);
  connect(tray_control_, &gui::TrayControl::AppExit, qApp, &QApplication::quit);

  mouse_tracker_ = new MouseTracker(this);
  mouse_tracker_->start();

  for (auto w : qAsConst(clock_windows_)) ConnectWindow(w);

  ConnectTrayMessages();

  connect(qApp, &QGuiApplication::screenRemoved, this, &ClockApplication::OnScreenRemoved);
  connect(qApp, &QGuiApplication::screenAdded, this, &ClockApplication::OnScreenAdded);
  connect(qApp, &QGuiApplication::primaryScreenChanged,
          this, &ClockApplication::OnPrimaryScreenChanged);

  InitPluginSystem();
  Reset();
  for (auto w : qAsConst(clock_windows_)) w->LoadState();
  UpdateVisibilityAction();

  timer_.setSingleShot(false);
}

ClockApplication::~ClockApplication()
{
  ShutdownPluginSystem();
  timer_.stop();
  mouse_tracker_->stop();
  delete mouse_tracker_;
  for (auto w : qAsConst(clock_windows_)) delete w;
  delete state_;
  delete tray_control_;
}

void ClockApplication::UpdateVisibilityAction()
{
  bool checked = false;
  for (auto w : qAsConst(clock_windows_)) {
    checked = checked || w->isVisible();
    if (checked) break;
  }
  tray_control_->GetShowHideAction()->setChecked(checked);
}

void ClockApplication::Reset()
{
  // widnow settings
  ApplyOption(OPT_OPACITY, app_config_->GetValue(OPT_OPACITY));
  ApplyOption(OPT_FULLSCREEN_DETECT, app_config_->GetValue(OPT_FULLSCREEN_DETECT));
  // must be applied before 'stay on top': both drive the same window manager
  // bypass flag, and this one decides whether the clock may cover panels
  ApplyOption(OPT_ALLOW_OVER_PANELS, app_config_->GetValue(OPT_ALLOW_OVER_PANELS));
  ApplyOption(OPT_STAY_ON_TOP, app_config_->GetValue(OPT_STAY_ON_TOP));
  ApplyOption(OPT_TRANSP_FOR_INPUT, app_config_->GetValue(OPT_TRANSP_FOR_INPUT));
  ApplyOption(OPT_ALIGNMENT, app_config_->GetValue(OPT_ALIGNMENT));
  ApplyOption(OPT_BACKGROUND_COLOR, app_config_->GetValue(OPT_BACKGROUND_COLOR));
  ApplyOption(OPT_BACKGROUND_ENABLED, app_config_->GetValue(OPT_BACKGROUND_ENABLED));
  ApplyOption(OPT_SHOW_WINDOW_BORDER, app_config_->GetValue(OPT_SHOW_WINDOW_BORDER));
  ApplyOption(OPT_SNAP_TO_EDGES, app_config_->GetValue(OPT_SNAP_TO_EDGES));
  ApplyOption(OPT_SNAP_THRESHOLD, app_config_->GetValue(OPT_SNAP_THRESHOLD));
  ApplyOption(OPT_USE_HOVER_BUTTONS, app_config_->GetValue(OPT_USE_HOVER_BUTTONS));
  ApplyOption(OPT_WINDOW_MOVE_STEP, app_config_->GetValue(OPT_WINDOW_MOVE_STEP));

  // load time format first to update separators where it required
  ApplyOption(OPT_TIME_FORMAT, app_config_->GetValue(OPT_TIME_FORMAT));
  for (auto w : qAsConst(clock_windows_)) w->blockSignals(true);
  ApplyOption(OPT_SEPARATOR_FLASH, app_config_->GetValue(OPT_SEPARATOR_FLASH));
  ApplyOption(OPT_TIME_ZONE, app_config_->GetValue(OPT_TIME_ZONE));
  ApplyOption(OPT_DISPLAY_LOCAL_TIME, app_config_->GetValue(OPT_DISPLAY_LOCAL_TIME));
  ApplyOption(OPT_ZOOM, app_config_->GetValue(OPT_ZOOM));
  ApplyOption(OPT_COLOR, app_config_->GetValue(OPT_COLOR));
  ApplyOption(OPT_TEXTURE, app_config_->GetValue(OPT_TEXTURE));
  ApplyOption(OPT_TEXTURE_TYPE, app_config_->GetValue(OPT_TEXTURE_TYPE));
  ApplyOption(OPT_TEXTURE_PER_ELEMENT, app_config_->GetValue(OPT_TEXTURE_PER_ELEMENT));
  ApplyOption(OPT_TEXTURE_DRAW_MODE, app_config_->GetValue(OPT_TEXTURE_DRAW_MODE));
  ApplyOption(OPT_CUSTOMIZATION, app_config_->GetValue(OPT_CUSTOMIZATION));
  ApplyOption(OPT_SPACING, app_config_->GetValue(OPT_SPACING));
  ApplyOption(OPT_COLORIZE_COLOR, app_config_->GetValue(OPT_COLORIZE_COLOR));
  ApplyOption(OPT_COLORIZE_LEVEL, app_config_->GetValue(OPT_COLORIZE_LEVEL));

  ApplyOption(OPT_FONT, app_config_->GetValue(OPT_FONT));
  ApplyOption(OPT_SKIN_NAME, app_config_->GetValue(OPT_SKIN_NAME));
  for (auto w : qAsConst(clock_windows_)) w->blockSignals(false);

  for (auto w : qAsConst(clock_windows_)) w->TimeoutHandler();      // to apply changes


  // misc settings
  ApplyOption(OPT_CLOCK_URL_ENABLED, app_config_->GetValue(OPT_CLOCK_URL_ENABLED));
  ApplyOption(OPT_CLOCK_URL_STRING, app_config_->GetValue(OPT_CLOCK_URL_STRING));
  ApplyOption(OPT_SHOW_HIDE_ENABLED, app_config_->GetValue(OPT_SHOW_HIDE_ENABLED));
  ApplyOption(OPT_EXPORT_STATE, app_config_->GetValue(OPT_EXPORT_STATE));
  ApplyOption(OPT_KEEP_ALWAYS_VISIBLE, app_config_->GetValue(OPT_KEEP_ALWAYS_VISIBLE));
  ApplyOption(OPT_SHOW_ON_ALL_DESKTOPS, app_config_->GetValue(OPT_SHOW_ON_ALL_DESKTOPS));

  plugin_manager_->UnloadPlugins();
  plugin_manager_->LoadPlugins(app_config_->GetValue(OPT_PLUGINS).toStringList());

  // refresh interval
  ApplyOption(OPT_REFRESH_INTERVAL, app_config_->GetValue(OPT_REFRESH_INTERVAL));
}

void ClockApplication::ApplyOption(const Option opt, const QVariant& value)
{
  switch (opt) {
    case OPT_SKIN_NAME:
      skin_manager_->LoadSkin(value.toString());
      break;

    case OPT_FONT:
      skin_manager_->SetFont(value.value<QFont>());
      break;

    case OPT_REFRESH_INTERVAL:
      timer_.start(value.toInt());
      break;

    case OPT_SHOW_HIDE_ENABLED:
      tray_control_->GetShowHideAction()->setVisible(value.toBool());
      Q_FALLTHROUGH();
      // fallthrough

    default:
      for (auto w : qAsConst(clock_windows_)) w->ApplyOption(opt, value);
  }
}

void ClockApplication::ShowSettingsDialog()
{
  clock_windows_.first()->activateWindow();
  clock_windows_.first()->raise();
  static QPointer<gui::SettingsDialog> dlg;
  if (!dlg) {
    dlg = new gui::SettingsDialog(app_config_, state_);
    // main signals/slots: change options, apply and reset
    connect(dlg.data(), &gui::SettingsDialog::OptionChanged, this, &ClockApplication::ApplyOption);
    connect(dlg.data(), &gui::SettingsDialog::OptionChanged, app_config_, &core::ClockSettings::SetValue);
    connect(dlg.data(), &gui::SettingsDialog::accepted, app_config_, &core::ClockSettings::Accept);
    connect(app_config_, &core::ClockSettings::accepted, config_backend_, &SettingsStorage::Accept);
    connect(app_config_, &core::ClockSettings::accepted, state_, &core::ClockState::Accept);
    connect(dlg.data(), &gui::SettingsDialog::rejected, config_backend_, &SettingsStorage::Reject);
    connect(dlg.data(), &gui::SettingsDialog::ResetSettings, config_backend_, &SettingsStorage::Reset);
    connect(app_config_, &core::ClockSettings::rejected, this, &ClockApplication::Reset);
    connect(app_config_, &core::ClockSettings::rejected, state_, &core::ClockState::Reject);
    // TODO: connect common (not related to SettingsDialog) signal/slots in constructor
    // export/import
    connect(dlg.data(), &gui::SettingsDialog::ExportSettings, config_backend_, &SettingsStorage::Export);
    connect(dlg.data(), &gui::SettingsDialog::ImportSettings, config_backend_, &SettingsStorage::Import);
    // check for updates
    // skins list
    connect(skin_manager_, &core::SkinManager::SearchFinished, dlg.data(), &gui::SettingsDialog::SetSkinList);
    skin_manager_->ListSkins();
    // 'preview mode' support
    tray_control_->GetShowHideAction()->setEnabled(false);
    for (auto w : qAsConst(clock_windows_)) {
      w->EnablePreviewMode();
      w->EnsureVisible();
      connect(dlg.data(), &gui::SettingsDialog::destroyed, w, &gui::ClockWindow::RestoreVisibility);
      connect(dlg.data(), &gui::SettingsDialog::destroyed, w, &gui::ClockWindow::DisablePreviewMode);
    }
    connect(dlg.data(), &gui::SettingsDialog::destroyed, [this] () { tray_control_->GetShowHideAction()->setEnabled(true); });
    // plugins engine
    connect(dlg.data(), &gui::SettingsDialog::OptionChanged, plugin_manager_, &core::PluginManager::UpdateSettings);
    // plugins list
    connect(plugin_manager_, &core::PluginManager::SearchFinished, dlg.data(), &gui::SettingsDialog::SetPluginsList);
    plugin_manager_->ListAvailable();
    // enable/disable plugin, configure plugin
    connect(dlg.data(), &gui::SettingsDialog::PluginStateChanged, plugin_manager_, &core::PluginManager::EnablePlugin);
    // plugin dialogs must be parented to the settings dialog, otherwise the window
    // manager has no reason to keep such a (application modal) dialog above it
    connect(dlg.data(), &gui::SettingsDialog::PluginConfigureRequest, plugin_manager_,
            [this] (const QString& name) { plugin_manager_->ConfigurePlugin(name, dlg); });

    dlg->show();
  }
  dlg->raise();
  dlg->activateWindow();
}

void ClockApplication::ShowAboutDialog()
{
  static QPointer<gui::AboutDialog> dlg;
  if (!dlg) {
    dlg = new gui::AboutDialog();
    dlg->show();
  }
  dlg->raise();
  dlg->activateWindow();
}

void ClockApplication::OnScreenRemoved(QScreen* screen)
{
  const QString removed_id = gui::ClockWindow::ScreenId(screen);
  auto iter = std::find_if(clock_windows_.begin(), clock_windows_.end(),
                           [&removed_id] (gui::ClockWindow* window) {
    return window->targetScreenId() == removed_id;
  });
  if (iter == clock_windows_.end()) return;

  gui::ClockWindow* window = *iter;
  const QPoint fallback_position = window->savedPosition();
  const QRect removed_geometry = window->targetScreenGeometry().isValid()
                               ? window->targetScreenGeometry() : screen->geometry();
  window->SaveStateBeforeScreenRemoval();

  const QString fallback_id = placement::chooseFallbackScreen(currentScreens(), removed_id);
  QScreen* fallback = screenForId(fallback_id);
  if (!fallback) return;

  const bool show_on_all = app_config_->GetValue(OPT_SHOW_ON_ALL_MONITORS).toBool();
  if (show_on_all && clock_windows_.size() > 1) {
    clock_windows_.erase(iter);
    window->deleteLater();
    UpdatePluginWindowData();
    // Loaded plugins may retain per-window objects/pointers. Reinitialize them
    // against the reduced window list before the removed window is destroyed.
    Reset();
  } else {
    window->RestoreOnScreen(fallback, fallback_position, removed_geometry);
  }
  UpdateVisibilityAction();
}

void ClockApplication::OnScreenAdded(QScreen* screen)
{
  if (!screen) return;
  const QString added_id = gui::ClockWindow::ScreenId(screen);
  const bool show_on_all = app_config_->GetValue(OPT_SHOW_ON_ALL_MONITORS).toBool();

  if (show_on_all) {
    const auto existing = std::find_if(clock_windows_.cbegin(), clock_windows_.cend(),
                                       [&added_id] (gui::ClockWindow* window) {
      return window->targetScreenId() == added_id;
    });
    if (existing != clock_windows_.cend()) return;

    gui::ClockWindow* window = new gui::ClockWindow(app_config_, screen,
                                                     next_legacy_window_id_++);
    clock_windows_.append(window);
    ConnectWindow(window);
    UpdatePluginWindowData();
    Reset();
    window->LoadState();
    UpdateVisibilityAction();
    return;
  }

  if (clock_windows_.isEmpty() || added_id != preferred_screen_id_) return;
  gui::ClockWindow* window = clock_windows_.first();
  if (window->targetScreenId() == added_id) return;
  window->RestoreOnScreen(screen, window->savedPosition(), window->targetScreenGeometry());
}

void ClockApplication::OnPrimaryScreenChanged(QScreen* screen)
{
  Q_UNUSED(screen)
  for (auto window : qAsConst(clock_windows_)) window->RefreshScreenPolicy();
}

void ClockApplication::OnWindowTargetScreenChanged(const QString& screenId)
{
  if (app_config_->GetValue(OPT_SHOW_ON_ALL_MONITORS).toBool()) return;
  preferred_screen_id_ = screenId;
  state_->SetVariable(S_PREFERRED_SCREEN_KEY, preferred_screen_id_);
}

void ClockApplication::InitPluginSystem()
{
  plugin_manager_ = new core::PluginManager(this);
  plugin_manager_->ListAvailable();
  UpdatePluginWindowData();
}

void ClockApplication::ShutdownPluginSystem()
{
  plugin_manager_->UnloadPlugins();
}

void ClockApplication::CreateWindows()
{
  const QList<QScreen*> screens = QGuiApplication::screens();
  if (screens.isEmpty()) return;

  preferred_screen_id_ = state_->GetVariable(S_PREFERRED_SCREEN_KEY).toString();
  const bool had_persisted_preference = !preferred_screen_id_.isEmpty();
  if (app_config_->GetValue(OPT_SHOW_ON_ALL_MONITORS).toBool()) {
    for (int i = 0; i < screens.size(); ++i)
      clock_windows_.append(new gui::ClockWindow(app_config_, screens[i],
                                                  next_legacy_window_id_++));
    return;
  }

  // Existing installations only have the old position key. Treat the screen
  // containing that anchor as their preference rather than moving them to the
  // new multi-monitor default.
  if (!had_persisted_preference) {
    QVariant legacy_position = state_->GetVariable("1/position");
    if (!legacy_position.isValid())
      legacy_position = state_->GetVariable("clock_position");
    if (legacy_position.isValid())
      preferred_screen_id_ = placement::screenContainingPoint(
        currentScreens(), legacy_position.toPoint());
  }

  const QString selected_id = placement::chooseInitialScreen(currentScreens(), preferred_screen_id_);
  QScreen* selected = screenForId(selected_id);
  if (!selected) selected = QGuiApplication::primaryScreen();
  if (!selected) selected = screens.first();

  if (!had_persisted_preference) {
    preferred_screen_id_ = gui::ClockWindow::ScreenId(selected);
    state_->SetVariable(S_PREFERRED_SCREEN_KEY, preferred_screen_id_);
  }
  clock_windows_.append(new gui::ClockWindow(app_config_, selected,
                                              next_legacy_window_id_++));
}

void ClockApplication::ConnectWindow(gui::ClockWindow* window)
{
  connect(tray_control_, &gui::TrayControl::VisibilityChanged,
          window, &gui::ClockWindow::ChangeVisibility);
  connect(tray_control_, &gui::TrayControl::PositionChanged,
          window, &gui::ClockWindow::MoveWindow);
  connect(&timer_, &QTimer::timeout, window, &gui::ClockWindow::TimeoutHandler);
  connect(skin_manager_, &SkinManager::SkinLoaded, window, &gui::ClockWindow::ApplySkin);
  connect(window->clockWidget(), &gui::ClockWidget::SeparatorsChanged,
          skin_manager_, &SkinManager::SetSeparators);
  connect(window->contextMenu(), &gui::ContextMenu::VisibilityChanged,
          this, &ClockApplication::UpdateVisibilityAction);
  connect(window->contextMenu(), &gui::ContextMenu::ShowSettingsDlg,
          this, &ClockApplication::ShowSettingsDialog);
  connect(window->contextMenu(), &gui::ContextMenu::ShowAboutDlg,
          this, &ClockApplication::ShowAboutDialog);
  connect(window->contextMenu(), &gui::ContextMenu::AppExit, qApp, &QApplication::quit);
  connect(mouse_tracker_, &MouseTracker::mousePositionChanged,
          window, &gui::ClockWindow::HandleMouseMove);
  connect(window, &gui::ClockWindow::TargetScreenChanged,
          this, &ClockApplication::OnWindowTargetScreenChanged);
}

void ClockApplication::UpdatePluginWindowData()
{
  if (!plugin_manager_) return;
  core::TPluginData plugin_data;
  plugin_data.settings = app_config_;
  plugin_data.tray = tray_control_->GetTrayIcon();
  for (auto window : qAsConst(clock_windows_))
    plugin_data.windows.append(window->clockWidget());
  plugin_manager_->SetInitData(plugin_data);
}

void ClockApplication::ConnectTrayMessages()
{
  // skin_manager messages
  connect(skin_manager_, &core::SkinManager::ErrorMessage, [this] (const QString& msg) {
    disconnect(tray_control_->GetTrayIcon(), &QSystemTrayIcon::messageClicked, nullptr, nullptr);
    // *INDENT-OFF*
    tray_control_->GetTrayIcon()->showMessage(
          tr("%1 Error").arg(qApp->applicationName()), msg, QSystemTrayIcon::Warning);
    // *INDENT-ON*
  });
}

} // namespace core
} // namespace digital_clock
