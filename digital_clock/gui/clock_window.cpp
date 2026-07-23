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

#include "clock_window.h"

#include <functional>

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QMenu>
#include <QUrl>
#if (QT_VERSION < QT_VERSION_CHECK(5, 10, 0))
#include <QDesktopWidget>
#endif

#include "core/clock_settings.h"
#include "core/clock_state.h"
#include "core/screen_placement_policy.h"

#include "gui/card_layout.h"
#include "gui/clock_widget.h"
#include "gui/context_menu.h"
#include "gui/hover_buttons.h"


static const char* const S_OPT_POSITION_KEY = "position";
static const char* const S_OPT_VISIBLE_KEY = "visible";
static const char* const S_SCREEN_PLACEMENT_PREFIX = "screen_placement/screens";
static const char* const S_SCREEN_IDENTITY_KEY = "identity";
static const char* const S_SCREEN_WINDOW_GEOMETRY_KEY = "window_geometry";
static const char* const S_SCREEN_GEOMETRY_KEY = "screen_geometry";

#define S_OPT_POSITION_ID(X) QString("%1/%2").arg(X).arg(S_OPT_POSITION_KEY)

#define S_OPT_POSITION S_OPT_POSITION_ID(legacy_id_)
#define S_OPT_VISIBLE QString("%1/%2").arg(legacy_id_).arg(S_OPT_VISIBLE_KEY)


namespace digital_clock {
namespace gui {

namespace {

QScreen* findScreen(QWidget* w)
{
  QRect r = w->frameGeometry();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
  QScreen* screen = QGuiApplication::screenAt(r.topLeft());
#else
  QScreen* screen = QGuiApplication::screens()[QApplication::desktop()->screenNumber(w)];
#endif
  if (!screen) {
    int max_intersected = 0;
    for (QScreen* s : QGuiApplication::screens()) {
      QRect ir = r.intersected(s->availableGeometry());
      int sq = ir.width() * ir.height();
      if (sq > max_intersected) {
        max_intersected = sq;
        screen = s;
      }
    }
  }

  if (!screen)
    screen = QGuiApplication::primaryScreen();

  return screen;
}

QScreen* findScreenAt(const QPoint& point)
{
  // QGuiApplication::screenAt() was added in Qt 5.10, while Digital Clock
  // still supports older Qt 5 releases. Geometry lookup is equivalent here
  // and keeps legacy-position migration buildable on those releases.
  for (QScreen* screen : QGuiApplication::screens()) {
    if (screen->geometry().contains(point)) return screen;
  }
  return nullptr;
}

QString screenStateKey(const QString& screenId, const char* leaf)
{
  const QString encoded = QString::fromLatin1(QUrl::toPercentEncoding(screenId));
  return QString("%1/%2/%3").arg(S_SCREEN_PLACEMENT_PREFIX, encoded, QLatin1String(leaf));
}

core::placement::Screen screenDescriptor(const QScreen* screen)
{
  return {
    ClockWindow::ScreenId(screen),
    screen->geometry(),
    screen->availableGeometry(),
    screen == QGuiApplication::primaryScreen()
  };
}

}

ClockWindow::ClockWindow(core::ClockSettings* app_config, QScreen* screen,
                         int legacyId, QWidget* parent) :
  QWidget(parent, Qt::Window),
  app_config_(app_config),
  legacy_id_(legacyId)
{
  // A clock never needs keyboard focus. Without this the window maps as
  // "demands attention", and GNOME answers that with a "… is ready"
  // notification every single time the clock starts.
  setWindowFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
#ifdef Q_OS_MACOS
  setWindowFlag(Qt::NoDropShadowWindowHint);
#else
  if (!app_config->GetValue(OPT_SHOW_TASKBAR_ICON).toBool()) setWindowFlag(Qt::Tool);
#endif
  setAttribute(Qt::WA_TranslucentBackground);

  c_menu_ = new ContextMenu(this);
  connect(c_menu_, &ContextMenu::VisibilityChanged, this, &ClockWindow::ChangeVisibility);
  connect(c_menu_, &ContextMenu::PositionChanged, this, &ClockWindow::MoveWindow);

  state_ = new core::ClockState(app_config_->GetBackend(), this);
  connect(state_, &core::ClockState::accepted, this, &ClockWindow::SaveState);
  connect(state_, &core::ClockState::rejected, this, &ClockWindow::LoadState);
  connect(state_, &core::ClockState::rejected, this, &ClockWindow::CorrectPosition);

  connect(app_config_, &core::ClockSettings::accepted, state_, &core::ClockState::Accept);
  connect(app_config_, &core::ClockSettings::rejected, state_, &core::ClockState::Reject);

  clock_widget_ = new gui::ClockWidget(this);

  HoverButtons* hb = new HoverButtons(this);
  connect(hb, &HoverButtons::buttonClicked, this, &ClockWindow::onHoverButtonClicked);

  CardLayout* main_layout = new CardLayout(this);
  main_layout->setSizeConstraint(QLayout::SetFixedSize);
  main_layout->setSpacing(0);
  main_layout->setMargin(2);
  main_layout->addWidget(hb);
  main_layout->addWidget(clock_widget_);
  setLayout(main_layout);

  dragging_ = false;
  last_visibility_ = true;
  fullscreen_detect_enabled_ = false;
  keep_always_visible_ = true;

  SetTargetScreen(screen);
}

QString ClockWindow::ScreenId(const QScreen* screen)
{
  if (!screen) return QString();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
  return core::placement::screenIdentity(screen->manufacturer(), screen->model(),
                                         screen->serialNumber(), screen->name());
#else
  // Hardware metadata was added to QScreen in Qt 5.9. Connector/display name
  // is the strongest stable identifier available on the older supported Qt 5.
  return core::placement::screenIdentity(QString(), QString(), QString(), screen->name());
#endif
}

bool ClockWindow::previewMode() const
{
  return clock_widget_->preview();
}

QPoint ClockWindow::savedPosition() const
{
  return TopLeftFromAnchor(saved_position_, cur_alignment_);
}

void ClockWindow::showEvent(QShowEvent* event)
{
  CorrectPositionImpl();
  c_menu_->visibilityAction()->setChecked(true);
  QWidget::showEvent(event);
}

void ClockWindow::hideEvent(QHideEvent* event)
{
  c_menu_->visibilityAction()->setChecked(false);
  QWidget::hideEvent(event);
}

void ClockWindow::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton) {
    drag_position_ = event->globalPos() - frameGeometry().topLeft();
    dragging_ = true;
    this->layout()->itemAt(0)->widget()->setDisabled(true);
    event->accept();
  }
}

void ClockWindow::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons() & Qt::LeftButton) {
    QPoint target_pos = event->globalPos() - drag_position_;
    if (snap_to_edges_) {
      QRect screen = UsableScreenRect(findScreen(this));
      QRect widget = frameGeometry();
      if (qAbs(target_pos.x() - screen.left()) <= snap_threshold_)
        target_pos.setX(screen.left());
      if (qAbs(target_pos.y() - screen.top()) <= snap_threshold_)
        target_pos.setY(screen.top());
      if (qAbs(screen.right() - (target_pos.x() + widget.width())) <= snap_threshold_)
        target_pos.setX(screen.right() - widget.width());
      if (qAbs(screen.bottom() - (target_pos.y() + widget.height())) <= snap_threshold_)
        target_pos.setY(screen.bottom() - widget.height());
    }
    move(target_pos);
    event->accept();
  }
}

void ClockWindow::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton) {
    AdoptScreenFromCurrentPosition();
    SaveState();
    this->layout()->itemAt(0)->widget()->setEnabled(app_config_->GetValue(OPT_USE_HOVER_BUTTONS).toBool());
    event->accept();
    dragging_ = false;
  }
}

void ClockWindow::paintEvent(QPaintEvent* /*event*/)
{
  QPainter p(this);
  p.setCompositionMode(QPainter::CompositionMode_Clear);
  p.fillRect(this->rect(), Qt::transparent);
  if (!testAttribute(Qt::WA_TranslucentBackground)) {
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(this->rect(), bg_color_);
  }
  if (clock_widget_->preview() && show_border_) {
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    p.setPen(QPen(Qt::red, 2, Qt::DotLine, Qt::SquareCap, Qt::MiterJoin));
    p.drawRect(rect().adjusted(1, 1, -1, -1));
  }
}

void ClockWindow::resizeEvent(QResizeEvent* event)
{
  // ignore spontaneous resize events sent by system
  // they often have incorrect window geometry, which leads to strange
  // effects when right or center alignment is used, especially on Linux
  if (event->spontaneous()) {
    event->ignore();
    return;
  }

  if (cur_alignment_ != CAlignment::A_LEFT && event->oldSize().isValid()) {
    QPoint cur_pos = this->pos();
    if (cur_alignment_ == CAlignment::A_RIGHT) {
      cur_pos.setX(cur_pos.x() + event->oldSize().width() - event->size().width());
    } else {
      Q_ASSERT(cur_alignment_ == CAlignment::A_CENTER);
      cur_pos.setX(cur_pos.x() + (event->oldSize().width() - event->size().width()) / 2);
      cur_pos.setY(cur_pos.y() + (event->oldSize().height() - event->size().height()) / 2);
    }
    this->move(cur_pos);
  }
  CorrectPosition();
  event->accept();
}

void ClockWindow::contextMenuEvent(QContextMenuEvent* event)
{
  c_menu_->menu()->popup(event->globalPos());
  event->accept();
}

bool ClockWindow::event(QEvent* event)
{
  if (event->type() == QEvent::WindowDeactivate &&
      !previewMode() &&
      !(windowFlags() & Qt::WindowStaysOnTopHint))
    window()->lower();
  return QWidget::event(event);
}

void ClockWindow::ApplyOption(const Option opt, const QVariant& value)
{
  switch (opt) {
    case OPT_OPACITY:
      setWindowOpacity(value.toReal());
      break;

    case OPT_FULLSCREEN_DETECT:
      fullscreen_detect_enabled_ = value.toBool();
      break;

    case OPT_STAY_ON_TOP:
      stay_on_top_ = value.toBool();
      SetWindowFlag(Qt::WindowStaysOnTopHint, value.toBool());
      UpdateBypassWindowManager();
#ifdef Q_OS_WIN
      WinOnTopWorkaround();
#endif
#ifdef Q_OS_LINUX
      X11FullscreenWorkaround();
#endif
      break;

    case OPT_TRANSP_FOR_INPUT:
      SetWindowFlag(Qt::WindowTransparentForInput, value.toBool());
      break;

    case OPT_ALIGNMENT:
      cur_alignment_ = static_cast<CAlignment>(value.toInt());
      clock_widget_->ApplyOption(opt, value);
      break;

    case OPT_BACKGROUND_ENABLED:
      this->setAttribute(Qt::WA_TranslucentBackground, !value.toBool());
      this->repaint();
      break;

    case OPT_BACKGROUND_COLOR:
      bg_color_ = value.value<QColor>();
      this->repaint();
      break;

    case OPT_SHOW_WINDOW_BORDER:
      show_border_ = value.toBool();
      break;

    case OPT_SNAP_TO_EDGES:
      snap_to_edges_ = value.toBool();
      break;

    case OPT_SNAP_THRESHOLD:
      snap_threshold_ = value.toInt();
      break;

    case OPT_EXPORT_STATE:
      state_->SetExportable(value.toBool());
      break;

    case OPT_KEEP_ALWAYS_VISIBLE:
      keep_always_visible_ = value.toBool();
      this->CorrectPosition();
      break;

    case OPT_ALLOW_OVER_PANELS:
      allow_over_panels_ = value.toBool();
      UpdateBypassWindowManager();
      this->CorrectPosition();
      break;

    case OPT_SHOW_HIDE_ENABLED:
      c_menu_->visibilityAction()->setVisible(value.toBool());
      break;

    case OPT_SHOW_ON_ALL_DESKTOPS:
      SetVisibleOnAllDesktops(value.toBool());
      break;

    case OPT_USE_HOVER_BUTTONS:
      this->layout()->itemAt(0)->widget()->setVisible(value.toBool());
      break;

    default:
      clock_widget_->ApplyOption(opt, value);
  }
}

void ClockWindow::ApplySkin(skin_draw::ISkin::SkinPtr skin)
{
  clock_widget_->ApplySkin(skin);
}

void ClockWindow::ChangeVisibility(bool visible)
{
  c_menu_->visibilityAction()->setChecked(visible);
  this->setVisible(visible);
  state_->SetVariable(S_OPT_VISIBLE, visible);
}

void ClockWindow::EnsureVisible()
{
  last_visibility_ = this->isVisible();
  if (!this->isVisible()) this->setVisible(true);
  c_menu_->visibilityAction()->setDisabled(true);
}

void ClockWindow::RestoreVisibility()
{
  this->setVisible(last_visibility_);
  c_menu_->visibilityAction()->setEnabled(true);
}

void ClockWindow::EnablePreviewMode()
{
  setPreviewMode(true);
}

void ClockWindow::DisablePreviewMode()
{
  setPreviewMode(false);
}

void ClockWindow::setPreviewMode(bool enable)
{
  if (enable)
    clock_widget_->EnablePreviewMode();
  else
    clock_widget_->DisablePreviewMode();
  this->update();
}

void ClockWindow::TimeoutHandler()
{
  clock_widget_->TimeoutHandler();
#ifdef Q_OS_WIN
  WinOnTopWorkaround();
#endif
#ifdef Q_OS_LINUX
  X11FullscreenWorkaround();
#endif
}

void ClockWindow::LoadState()
{
  LoadStateImpl();
}

void ClockWindow::LoadStateImpl(const QPoint& fallbackPosition,
                                const QRect& fallbackScreenGeometry,
                                bool hasFallback)
{
  QScreen* screen = target_screen_.data();
  if (!screen) return;

  const CAlignment last_align = static_cast<CAlignment>(app_config_->GetValue(OPT_ALIGNMENT).toInt());
  cur_alignment_ = last_align;

  const QSize window_size = PlacementSize();

  const core::placement::Screen target = screenDescriptor(screen);
  QMap<QString, core::placement::SavedPlacement> saved;
  const QVariant window_geometry = state_->GetVariable(
    screenStateKey(target_screen_id_, S_SCREEN_WINDOW_GEOMETRY_KEY));
  const QVariant saved_screen_geometry = state_->GetVariable(
    screenStateKey(target_screen_id_, S_SCREEN_GEOMETRY_KEY));
  if (window_geometry.isValid()) {
    saved.insert(target_screen_id_, {
      window_geometry.toRect(), saved_screen_geometry.toRect()
    });
  }

  QPoint top_left;
  if (!saved.isEmpty() || hasFallback) {
    top_left = core::placement::restoredTopLeft(
      target, saved, window_size, fallbackPosition, fallbackScreenGeometry);
  } else {
    // Migrate the old index-based position once. Screen indexes are not stable,
    // so all subsequent saves use the QScreen hardware identity above.
    QVariant legacy = state_->GetVariable(S_OPT_POSITION);
    if (!legacy.isValid()) legacy = state_->GetVariable(S_OPT_POSITION_ID(1));
    if (!legacy.isValid()) legacy = state_->GetVariable("clock_position");

    if (legacy.isValid()) {
      const QPoint legacy_top_left = TopLeftFromAnchor(legacy.toPoint(), last_align);
      QScreen* source = findScreenAt(legacy_top_left);
      if (!source) source = QGuiApplication::primaryScreen();
      top_left = core::placement::restoredTopLeft(
        target, saved, window_size, legacy_top_left,
        source ? source->geometry() : QRect());
    } else {
      const QRect available = UsableScreenRect(screen);
      top_left = QPoint(available.right() - window_size.width() + 1 - 20,
                        available.top() + 20);
      top_left = core::placement::clampTopLeft(top_left, window_size, available);
    }
  }

  saved_position_ = AlignmentAnchor(top_left);
  this->move(top_left);
  target_screen_geometry_ = screen->geometry();
  PersistScreenPlacement(top_left, target_screen_geometry_);

  this->setVisible(state_->GetVariable(S_OPT_VISIBLE, true).toBool());
  last_visibility_ = this->isVisible();
#ifdef Q_OS_WIN
  if (this->isVisible() && ((windowFlags() & Qt::Tool) == Qt::Tool)) KeepOnDesktop();
#endif
}

void ClockWindow::SaveState()
{
  const QPoint top_left = this->pos();
  saved_position_ = AlignmentAnchor(top_left);
  const bool commit = !clock_widget_->preview();
  state_->SetVariable(S_OPT_POSITION, saved_position_, commit);

  QRect screen_geometry = target_screen_geometry_;
  if (target_screen_) screen_geometry = target_screen_->geometry();
  PersistScreenPlacement(top_left, screen_geometry, commit);
  state_->SetVariable(S_OPT_VISIBLE, last_visibility_, commit);
}

void ClockWindow::SaveStateBeforeScreenRemoval()
{
  // screenRemoved is emitted after Qt has already teleported native windows to
  // another screen. saved_position_ still describes the last intentional move.
  PersistScreenPlacement(savedPosition(), target_screen_geometry_);
}

void ClockWindow::RestoreOnScreen(QScreen* screen, const QPoint& fallbackPosition,
                                  const QRect& fallbackScreenGeometry)
{
  SetTargetScreen(screen);
  LoadStateImpl(fallbackPosition, fallbackScreenGeometry, true);
  UpdateBypassWindowManager();
  CorrectPosition();
}

void ClockWindow::RefreshScreenPolicy()
{
  UpdateBypassWindowManager();
  CorrectPosition();
}

void ClockWindow::PersistScreenPlacement(const QPoint& topLeft, const QRect& screenGeometry,
                                         bool commit)
{
  if (target_screen_id_.isEmpty()) return;
  const QSize window_size = PlacementSize();
  state_->SetVariable(screenStateKey(target_screen_id_, S_SCREEN_IDENTITY_KEY),
                      target_screen_id_, commit);
  state_->SetVariable(screenStateKey(target_screen_id_, S_SCREEN_WINDOW_GEOMETRY_KEY),
                      QRect(topLeft, window_size), commit);
  state_->SetVariable(screenStateKey(target_screen_id_, S_SCREEN_GEOMETRY_KEY),
                      screenGeometry, commit);
}

QSize ClockWindow::PlacementSize() const
{
  // Before the fixed layout is shown QWidget::size() is still Qt's generic
  // 640x480 default. The layout hint is the clock's real eventual size and is
  // therefore the safe size for startup restoration and persistence.
  const QSize hint = sizeHint();
  return (hint.isValid() && !hint.isEmpty()) ? hint : size();
}

QPoint ClockWindow::AlignmentAnchor(const QPoint& topLeft) const
{
  const QSize window_size = PlacementSize();
  QPoint anchor = topLeft;
  if (cur_alignment_ == CAlignment::A_RIGHT) {
    anchor.rx() += window_size.width() - 1;
  } else if (cur_alignment_ == CAlignment::A_CENTER) {
    anchor.rx() += window_size.width() / 2 - 1;
    anchor.ry() += window_size.height() / 2 - 1;
  }
  return anchor;
}

QPoint ClockWindow::TopLeftFromAnchor(const QPoint& anchor, CAlignment alignment) const
{
  const QSize window_size = PlacementSize();
  QPoint top_left = anchor;
  if (alignment == CAlignment::A_RIGHT) {
    top_left.rx() -= window_size.width() - 1;
  } else if (alignment == CAlignment::A_CENTER) {
    top_left.rx() -= window_size.width() / 2 - 1;
    top_left.ry() -= window_size.height() / 2 - 1;
  }
  return top_left;
}

void ClockWindow::MoveWindow(Qt::Alignment align)
{
  QScreen* scr = target_screen_ ? target_screen_.data() : findScreen(this);
  if (!scr) return;
  QRect screen = UsableScreenRect(scr);
  QRect window = this->frameGeometry();
  QPoint curr_pos = this->pos();
  if (align & Qt::AlignLeft) curr_pos.setX(screen.left());
  if (align & Qt::AlignHCenter) curr_pos.setX(screen.center().x() - window.width() / 2);
  if (align & Qt::AlignRight) curr_pos.setX(screen.right() - window.width());
  if (align & Qt::AlignTop) curr_pos.setY(screen.top());
  if (align & Qt::AlignVCenter) curr_pos.setY(screen.center().y() - window.height() / 2);
  if (align & Qt::AlignBottom) curr_pos.setY(screen.bottom() - window.height());
  if (curr_pos != this->pos()) this->move(curr_pos);
  SaveState();
}

void ClockWindow::HandleMouseMove(const QPoint& global_pos)
{
  if (!app_config_->GetValue(OPT_TRANSPARENT_ON_HOVER).toBool())
    return;

  bool entered = property("dc_mouse_entered").toBool();

  QRect rect = frameGeometry();
#ifndef Q_OS_MACOS
  QTransform t;
  t.scale(devicePixelRatioF(), devicePixelRatioF());
  rect = t.mapRect(rect);
#endif
  if (rect.contains(global_pos) && !entered) {
    entered = true;
    setWindowOpacity(app_config_->GetValue(OPT_OPACITY_ON_HOVER).toReal());
  }

  if (!rect.contains(global_pos) && entered) {
    entered = false;
    setWindowOpacity(app_config_->GetValue(OPT_OPACITY).toReal());
  }

  setProperty("dc_mouse_entered", entered);
}

void ClockWindow::onHoverButtonClicked(HoverButtons::Direction direction)
{
  QPoint p = this->pos();
  const int step = app_config_->GetValue(OPT_WINDOW_MOVE_STEP).toInt();

  switch (direction) {
    case HoverButtons::Left:
      p.rx() -= step;
      break;

    case HoverButtons::Right:
      p.rx() += step;
      break;

    case HoverButtons::Top:
      p.ry() -= step;
      break;

    case HoverButtons::Bottom:
      p.ry() += step;
      break;

    default:
      break;
  }

  this->move(p);
  CorrectPositionImpl();
  SaveState();
}

void ClockWindow::CorrectPosition()
{
  if (keep_always_visible_) CorrectPositionImpl();
}

void ClockWindow::SetTargetScreen(QScreen* screen)
{
  if (!screen) return;
  if (target_screen_ == screen) {
    target_screen_geometry_ = screen->geometry();
    UpdateBypassWindowManager();
    return;
  }

  if (target_screen_) disconnect(target_screen_, nullptr, this, nullptr);
  target_screen_ = screen;
  target_screen_id_ = ScreenId(screen);
  target_screen_geometry_ = screen->geometry();

  connect(screen, &QScreen::geometryChanged, this, [this] () {
    if (!target_screen_) return;
    target_screen_geometry_ = target_screen_->geometry();
    LoadState();
    CorrectPosition();
  });
  connect(screen, &QScreen::availableGeometryChanged, this, &ClockWindow::CorrectPosition);
  UpdateBypassWindowManager();
}

void ClockWindow::AdoptScreenFromCurrentPosition()
{
  QScreen* screen = findScreen(this);
  if (!screen || ScreenId(screen) == target_screen_id_) return;
  SetTargetScreen(screen);
  emit TargetScreenChanged(target_screen_id_);
}

QRect ClockWindow::UsableScreenRect(const QScreen* screen) const
{
  if (!screen) return QRect();
  return core::placement::usableRect(screenDescriptor(screen), allow_over_panels_);
}

void ClockWindow::UpdateBypassWindowManager()
{
#ifdef Q_OS_LINUX
  // "better stay on top" asks for the very same window manager bypass, so the
  // flag may only be dropped when none of the placement/topmost policies wants it.
  // A non-primary display intentionally gets the whole screen, including panel
  // strips; bypassing is required for the clock to be stacked above those docks.
  const bool non_primary = target_screen_ && target_screen_ != QGuiApplication::primaryScreen();
  bool want_bypass = allow_over_panels_ || non_primary ||
                     (stay_on_top_ && app_config_->GetValue(OPT_BETTER_STAY_ON_TOP).toBool());
  if (want_bypass == bool(windowFlags() & Qt::X11BypassWindowManagerHint)) return;
  SetWindowFlag(Qt::X11BypassWindowManagerHint, want_bypass);
#endif
}

void ClockWindow::CorrectPositionImpl()
{
  QPoint curr_pos = this->pos();
  QScreen* scr = target_screen_ ? target_screen_.data() : findScreen(this);
  if (!scr) return;
  QRect screen = UsableScreenRect(scr);
  curr_pos.setX(std::max(curr_pos.x(), screen.left()));
  curr_pos.setX(std::min(curr_pos.x(), screen.left() + screen.width() - this->width()));
  curr_pos.setY(std::max(curr_pos.y(), screen.top()));
  curr_pos.setY(std::min(curr_pos.y(), screen.top() + screen.height() - this->height()));
  if (curr_pos != this->pos()) this->move(curr_pos);
}

void ClockWindow::SetWindowFlag(Qt::WindowType flag, bool set)
{
  QWidget* aw = QApplication::activeWindow();
  bool last_visible = isVisible();
  setWindowFlag(flag, set);
  if (last_visible != isVisible()) show();
  if (aw) aw->activateWindow();
}
#if !defined(Q_OS_MACOS) && !defined(Q_OS_LINUX)
void ClockWindow::SetVisibleOnAllDesktops(bool set)
{
  Q_UNUSED(set)
}
#endif
} // namespace gui
} // namespace digital_clock
