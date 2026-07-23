/*
    Digital Clock - beautiful customizable clock with plugins
    Copyright (C) 2026  Digital Clock contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
 */

#ifndef DIGITAL_CLOCK_CORE_SCREEN_PLACEMENT_POLICY_H
#define DIGITAL_CLOCK_CORE_SCREEN_PLACEMENT_POLICY_H

#include <QMap>
#include <QRect>
#include <QString>
#include <QVector>

namespace digital_clock {
namespace core {
namespace placement {

struct Screen
{
  QString id;
  QRect geometry;
  QRect availableGeometry;
  bool primary;
};

struct SavedPlacement
{
  QRect windowGeometry;
  QRect screenGeometry;
};

QString screenIdentity(const QString& manufacturer, const QString& model,
                       const QString& serialNumber, const QString& name);
QString screenContainingPoint(const QVector<Screen>& screens, const QPoint& point);

QString chooseInitialScreen(const QVector<Screen>& screens, const QString& persistedPreferred);
QString chooseFallbackScreen(const QVector<Screen>& screens, const QString& unavailableScreen);

QRect usableRect(const Screen& screen, bool allowOverPanels);

QPoint clampTopLeft(const QPoint& position, const QSize& windowSize, const QRect& bounds);
QPoint restoredTopLeft(const Screen& screen, const QMap<QString, SavedPlacement>& saved,
                       const QSize& windowSize, const QPoint& fallbackPosition,
                       const QRect& fallbackScreenGeometry = QRect());

} // namespace placement
} // namespace core
} // namespace digital_clock

#endif // DIGITAL_CLOCK_CORE_SCREEN_PLACEMENT_POLICY_H
