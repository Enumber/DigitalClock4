/*
 * Headless regression harness for DigitalClock4 multi-monitor placement.
 */

#include "../digital_clock/core/screen_placement_policy.h"

#include <QDebug>
#include <QMap>
#include <QVector>

#include <algorithm>
#include <cstdlib>

using digital_clock::core::placement::SavedPlacement;
using digital_clock::core::placement::Screen;

namespace {

void expect(bool condition, const char* message)
{
  if (condition) return;
  qCritical() << "FAIL:" << message;
  std::exit(EXIT_FAILURE);
}

QVector<Screen> twoScreens()
{
  return {
    {"primary", QRect(0, 0, 1920, 1080), QRect(0, 32, 1920, 1048), true},
    {"secondary", QRect(1920, 0, 2560, 1440), QRect(1920, 40, 2560, 1400), false}
  };
}

void firstRunPrefersNonPrimaryScreen()
{
  const QVector<Screen> screens = twoScreens();
  expect(digital_clock::core::placement::chooseInitialScreen(screens, QString()) == "secondary",
         "first run with multiple displays must prefer a non-primary display");
}

void persistedPreferenceSurvivesOrderAndTemporaryAbsence()
{
  QVector<Screen> screens = twoScreens();
  std::reverse(screens.begin(), screens.end());
  expect(digital_clock::core::placement::chooseInitialScreen(screens, "secondary") == "secondary",
         "screen ordering changes must not replace a persisted identity");

  screens.remove(0); // secondary is temporarily disconnected
  expect(digital_clock::core::placement::chooseInitialScreen(screens, "secondary") == "primary",
         "an absent preferred display must fall back to a remaining visible display");

  screens = twoScreens(); // the original display returns after an app/system restart
  expect(digital_clock::core::placement::chooseInitialScreen(screens, "secondary") == "secondary",
         "a reconnected preferred display must automatically become the target again");
}

void singleScreenBehaviorIsPreserved()
{
  QVector<Screen> screens = twoScreens();
  screens.remove(1);
  expect(digital_clock::core::placement::chooseInitialScreen(screens, QString()) == "primary",
         "a single display must remain the initial target");
  expect(digital_clock::core::placement::usableRect(screens[0], false) == screens[0].availableGeometry,
         "a single primary display must continue to respect its panel work area");
}

void legacyPositionPreservesItsExistingDisplay()
{
  const QVector<Screen> screens = twoScreens();
  expect(digital_clock::core::placement::screenContainingPoint(screens, QPoint(400, 100)) == "primary",
         "a legacy position on the primary display must stay on the primary display");
  expect(digital_clock::core::placement::screenContainingPoint(screens, QPoint(4100, 100)) == "secondary",
         "a legacy position on a secondary display must stay on that display");
}

void primaryDisconnectAlsoFallsBack()
{
  QVector<Screen> remaining = twoScreens();
  remaining.remove(0);
  expect(digital_clock::core::placement::chooseFallbackScreen(remaining, "primary") == "secondary",
         "disconnecting the primary display must select a visible replacement");
}

void nonPrimaryMayOverlapPanels()
{
  const QVector<Screen> screens = twoScreens();
  expect(digital_clock::core::placement::usableRect(screens[0], false) == screens[0].availableGeometry,
         "primary display must respect its available work area by default");
  expect(digital_clock::core::placement::usableRect(screens[0], true) == screens[0].geometry,
         "explicit panel overlap must use full primary display geometry");
  expect(digital_clock::core::placement::usableRect(screens[1], false) == screens[1].geometry,
         "non-primary displays must allow panel overlap automatically");
}

void savedGeometryIsPerStableDisplayIdentity()
{
  QMap<QString, SavedPlacement> saved;
  saved.insert("primary", {QRect(QPoint(100, 80), QSize(320, 120)), QRect(0, 0, 1920, 1080)});
  saved.insert("secondary", {QRect(QPoint(4100, 100), QSize(320, 120)), QRect(1920, 0, 2560, 1440)});

  const QVector<Screen> screens = twoScreens();
  expect(digital_clock::core::placement::restoredTopLeft(screens[1], saved, QSize(320, 120), QPoint()) ==
           QPoint(4100, 100),
         "reconnected display must restore its own geometry rather than fallback geometry");
  expect(digital_clock::core::placement::restoredTopLeft(screens[0], saved, QSize(320, 120), QPoint()) ==
           QPoint(100, 80),
         "each display identity must retain an independent geometry");
}

void savedGeometryTracksDisplayAcrossDesktopReordering()
{
  QMap<QString, SavedPlacement> saved;
  saved.insert("secondary", {
    QRect(QPoint(4100, 100), QSize(320, 120)), QRect(1920, 0, 2560, 1440)
  });
  const Screen moved = {
    "secondary", QRect(-2560, 0, 2560, 1440), QRect(-2560, 40, 2560, 1400), false
  };

  expect(digital_clock::core::placement::restoredTopLeft(
           moved, saved, QSize(320, 120), QPoint()) == QPoint(-380, 100),
         "saved display geometry must keep the same monitor-relative position after reordering");
}

void fallbackMoveDoesNotOverwriteDisconnectedGeometry()
{
  QMap<QString, SavedPlacement> saved;
  const SavedPlacement disconnected = {
    QRect(QPoint(4100, 100), QSize(320, 120)), QRect(1920, 0, 2560, 1440)
  };
  saved.insert("secondary", disconnected);

  const Screen primary = twoScreens()[0];
  const QPoint fallback = digital_clock::core::placement::restoredTopLeft(
    primary, saved, QSize(320, 120), QPoint(4100, 100), disconnected.screenGeometry);

  expect(primary.geometry.contains(fallback),
         "fallback position must be moved onto the remaining visible display");
  expect(saved.value("secondary").windowGeometry.topLeft() == QPoint(4100, 100),
         "temporary fallback must not overwrite the disconnected display geometry");
}

void stableIdentityPrefersHardwareSerial()
{
  const QString a = digital_clock::core::placement::screenIdentity("Dell", "U2720Q", "ABC123", "DP-1");
  const QString b = digital_clock::core::placement::screenIdentity("Dell", "U2720Q", "ABC123", "HDMI-2");
  expect(a == b, "connector renames must not change an identity backed by a hardware serial");

  const QString noSerialA = digital_clock::core::placement::screenIdentity("Dell", "U2720Q", "", "DP-1");
  const QString noSerialB = digital_clock::core::placement::screenIdentity("Dell", "U2720Q", "", "DP-2");
  expect(noSerialA != noSerialB, "connector name must distinguish identical displays without serials");
}

} // namespace

int main()
{
  firstRunPrefersNonPrimaryScreen();
  persistedPreferenceSurvivesOrderAndTemporaryAbsence();
  singleScreenBehaviorIsPreserved();
  legacyPositionPreservesItsExistingDisplay();
  primaryDisconnectAlsoFallsBack();
  nonPrimaryMayOverlapPanels();
  savedGeometryIsPerStableDisplayIdentity();
  savedGeometryTracksDisplayAcrossDesktopReordering();
  fallbackMoveDoesNotOverwriteDisconnectedGeometry();
  stableIdentityPrefersHardwareSerial();
  qInfo() << "screen placement policy: all tests passed";
  return EXIT_SUCCESS;
}
