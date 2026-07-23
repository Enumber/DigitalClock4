/*
    Digital Clock - beautiful customizable clock with plugins
    Copyright (C) 2026  Digital Clock contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
 */

#include "screen_placement_policy.h"

#include <algorithm>

namespace digital_clock {
namespace core {
namespace placement {

namespace {

QString identityPart(const QString& value)
{
  const QString trimmed = value.trimmed();
  return QString("%1:%2").arg(trimmed.size()).arg(trimmed);
}

const Screen* screenById(const QVector<Screen>& screens, const QString& id)
{
  for (const Screen& screen : screens) {
    if (screen.id == id) return &screen;
  }
  return nullptr;
}

} // namespace

QString screenIdentity(const QString& manufacturer, const QString& model,
                       const QString& serialNumber, const QString& name)
{
  const QString serial = serialNumber.trimmed();
  if (!serial.isEmpty()) {
    // A hardware serial follows the monitor if the cable/connector changes.
    return QString("serial/%1/%2/%3")
      .arg(identityPart(manufacturer), identityPart(model), identityPart(serial));
  }

  // Qt/platform plugins do not always expose EDID serials. Keep the connector
  // name in that fallback so two otherwise identical monitors remain distinct.
  return QString("connector/%1/%2/%3")
    .arg(identityPart(manufacturer), identityPart(model), identityPart(name));
}

QString screenContainingPoint(const QVector<Screen>& screens, const QPoint& point)
{
  for (const Screen& screen : screens) {
    if (screen.geometry.contains(point)) return screen.id;
  }
  return QString();
}

QString chooseInitialScreen(const QVector<Screen>& screens, const QString& persistedPreferred)
{
  if (screens.isEmpty()) return QString();

  if (!persistedPreferred.isEmpty() && screenById(screens, persistedPreferred))
    return persistedPreferred;

  if (persistedPreferred.isEmpty()) {
    for (const Screen& screen : screens) {
      if (!screen.primary) return screen.id;
    }
  }

  for (const Screen& screen : screens) {
    if (screen.primary) return screen.id;
  }
  return screens.first().id;
}

QString chooseFallbackScreen(const QVector<Screen>& screens, const QString& unavailableScreen)
{
  for (const Screen& screen : screens) {
    if (screen.id != unavailableScreen && screen.primary) return screen.id;
  }
  for (const Screen& screen : screens) {
    if (screen.id != unavailableScreen) return screen.id;
  }
  return QString();
}

QRect usableRect(const Screen& screen, bool allowOverPanels)
{
  return (allowOverPanels || !screen.primary) ? screen.geometry : screen.availableGeometry;
}

QPoint clampTopLeft(const QPoint& position, const QSize& windowSize, const QRect& bounds)
{
  if (!bounds.isValid()) return position;

  const int maxX = std::max(bounds.left(), bounds.right() - windowSize.width() + 1);
  const int maxY = std::max(bounds.top(), bounds.bottom() - windowSize.height() + 1);
  return QPoint(qBound(bounds.left(), position.x(), maxX),
                qBound(bounds.top(), position.y(), maxY));
}

QPoint restoredTopLeft(const Screen& screen, const QMap<QString, SavedPlacement>& saved,
                       const QSize& windowSize, const QPoint& fallbackPosition,
                       const QRect& fallbackScreenGeometry)
{
  const auto savedIt = saved.constFind(screen.id);
  if (savedIt != saved.cend()) {
    QPoint restored = savedIt->windowGeometry.topLeft();
    if (savedIt->screenGeometry.isValid())
      restored += screen.geometry.topLeft() - savedIt->screenGeometry.topLeft();
    return clampTopLeft(restored, windowSize, screen.geometry);
  }

  QPoint translated = fallbackPosition;
  if (fallbackScreenGeometry.isValid())
    translated += screen.geometry.topLeft() - fallbackScreenGeometry.topLeft();
  return clampTopLeft(translated, windowSize, screen.geometry);
}

} // namespace placement
} // namespace core
} // namespace digital_clock
