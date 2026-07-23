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

#ifndef DIGITAL_CLOCK_GUI_CLOCK_WINDOW_H
#define DIGITAL_CLOCK_GUI_CLOCK_WINDOW_H

#include <QWidget>

#include <QPointer>
#include <QSet>
#include <QString>

#include "settings_keys.h"
#include "iskin.h"

#include "gui/hover_buttons.h"

class QScreen;

namespace digital_clock {

namespace core {
class ClockSettings;
class ClockState;
}

namespace gui {
class ClockWidget;
class ContextMenu;


class ClockWindow : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(bool previewMode READ previewMode WRITE setPreviewMode)

public:
  explicit ClockWindow(core::ClockSettings* app_config, QScreen* screen,
                       int legacyId = 1, QWidget* parent = nullptr);

  bool previewMode() const;

  static QString ScreenId(const QScreen* screen);
  QString targetScreenId() const Q_DECL_NOEXCEPT { return target_screen_id_; }
  QRect targetScreenGeometry() const Q_DECL_NOEXCEPT { return target_screen_geometry_; }

  gui::ClockWidget* clockWidget() const Q_DECL_NOEXCEPT { return clock_widget_; }
  gui::ContextMenu* contextMenu() const Q_DECL_NOEXCEPT { return c_menu_; }

  QPoint savedPosition() const;

signals:
  void TargetScreenChanged(const QString& screenId);

protected:
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;
  bool event(QEvent* event) override;

public slots:
  void ApplyOption(const Option opt, const QVariant& value);
  void ApplySkin(skin_draw::ISkin::SkinPtr skin);

  void ChangeVisibility(bool visible);
  void EnsureVisible();
  void RestoreVisibility();

  void EnablePreviewMode();
  void DisablePreviewMode();
  void setPreviewMode(bool enable);

  void TimeoutHandler();

  void LoadState();
  void SaveState();
  void SaveStateBeforeScreenRemoval();
  void RestoreOnScreen(QScreen* screen, const QPoint& fallbackPosition,
                       const QRect& fallbackScreenGeometry);
  void RefreshScreenPolicy();

  void MoveWindow(Qt::Alignment align);

  void HandleMouseMove(const QPoint& global_pos);

private slots:
  void onHoverButtonClicked(HoverButtons::Direction direction);
  void CorrectPosition();

private:
  void CorrectPositionImpl();
  void SetTargetScreen(QScreen* screen);
  void AdoptScreenFromCurrentPosition();
  void LoadStateImpl(const QPoint& fallbackPosition = QPoint(),
                     const QRect& fallbackScreenGeometry = QRect(),
                     bool hasFallback = false);
  void PersistScreenPlacement(const QPoint& topLeft, const QRect& screenGeometry,
                              bool commit = true);
  QSize PlacementSize() const;
  QPoint AlignmentAnchor(const QPoint& topLeft) const;
  QPoint TopLeftFromAnchor(const QPoint& anchor, CAlignment alignment) const;
  // Rectangle the clock is allowed to live in on the given screen: the full
  // screen when it may cover panels/docks, the desktop work area otherwise.
  QRect UsableScreenRect(const QScreen* screen) const;
  // Re-evaluates whether the window must bypass the window manager. Covering a
  // panel needs more than coordinates: panels are dock-layer windows, and a
  // regular managed window is both stacked below them and (in most WMs)
  // constrained to the work area. Only an override-redirect window escapes both.
  void UpdateBypassWindowManager();
#ifdef Q_OS_WIN
  void WinOnTopWorkaround();
  void KeepOnDesktop();
#endif
#ifdef Q_OS_LINUX
  // X11 counterpart of the fullscreen part of WinOnTopWorkaround(): drop the
  // stay-on-top hint while something else is fullscreen on the same screen
  void X11FullscreenWorkaround();
#endif
  void SetWindowFlag(Qt::WindowType flag, bool set);

  void SetVisibleOnAllDesktops(bool set);

  core::ClockSettings* app_config_;
  core::ClockState* state_;

  gui::ClockWidget* clock_widget_;
  gui::ContextMenu* c_menu_;

  const int legacy_id_;
  QPointer<QScreen> target_screen_;
  QString target_screen_id_;
  QRect target_screen_geometry_;

  bool dragging_;
  QPoint drag_position_;
  // Qt sends QGuiApplication::screenRemoved() **after** it moves windows to primary screen,
  // so there is no way to get window position in handler, this variable used as workaround.
  QPoint saved_position_;
  CAlignment cur_alignment_ = A_LEFT;
  bool last_visibility_;

  QColor bg_color_;
  bool show_border_ = true;

  bool fullscreen_detect_enabled_;
  bool keep_always_visible_;
  bool allow_over_panels_ = false;
  bool stay_on_top_ = false;
  QSet<QString> window_ignore_list_;
  // "already captured" must be tracked separately: an empty ignore list is a
  // legitimate result (nothing was fullscreen at startup), and testing the set
  // itself for emptiness re-captures on every tick — which quietly adds the
  // window that just went fullscreen to the list it is supposed to be checked against
  bool window_ignore_list_captured_ = false;

  bool snap_to_edges_ = true;
  int snap_threshold_ = 20;
};

} // namespace gui
} // namespace digital_clock

#endif // DIGITAL_CLOCK_GUI_CLOCK_WINDOW_H
