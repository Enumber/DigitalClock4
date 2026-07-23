/*
    Digital Clock: tray color plugin
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

#include "tray_color.h"

#include <QSystemTrayIcon>
#include <QColorDialog>
#include <QPainter>

#include "plugin_settings.h"
#include "tray_color_settings.h"

// Q_INIT_RESOURCE must be called from global scope (inside a namespace the
// symbol would be mangled into it) and with a name unique across the process:
// every plugin used to call its qrc "lang", so the shared symbol
// qInitResources_lang bound to whichever library loaded first.
// It must be called explicitly at all, because a plugin's compiled-in
// resources are not yet registered while its constructor runs, so tr() would
// silently return the untranslated source strings.
static void init_tray_color_resources() { Q_INIT_RESOURCE(tray_color_lang); }

namespace tray_color {

TrayColor::TrayColor() : tray_icon_(nullptr)
{
  is_enabled_ = false;

  init_tray_color_resources();

  InitTranslator(QLatin1String(":/tray_color/lang/tray_color_"));
  info_.display_name = tr("Tray icon color");
  info_.description = tr("Allows to change tray icon color.");
  InitIcon(":/tray_color/icon.png");
}

void TrayColor::Init(QSystemTrayIcon* tray_icon)
{
  tray_icon_ = tray_icon;
  old_icon_ = tray_icon->icon();

  QSettings::SettingsMap defaults;
  InitDefaults(&defaults);
  settings_->SetDefaultValues(defaults);
  settings_->Load();
}

void TrayColor::Start()
{
  is_enabled_ = true;
  RedrawTrayIcon(settings_->GetOption(OPT_TRAY_COLOR).value<QColor>());
}

void TrayColor::Stop()
{
  tray_icon_->setIcon(old_icon_);
  is_enabled_ = false;
}

void TrayColor::Configure()
{
  QColor color = QColorDialog::getColor(settings_->GetOption(OPT_TRAY_COLOR).value<QColor>(),
                                        DialogParent());
  if (color.isValid()) {
    settings_->SetOption(OPT_TRAY_COLOR, color);
    if (is_enabled_) RedrawTrayIcon(color);
  }
  settings_->Save();
  emit configured();
}

void TrayColor::RedrawTrayIcon(const QColor& color)
{
  QSize size = tray_icon_->icon().actualSize(QSize(48, 48));

  QImage result(size, QImage::Format_ARGB32_Premultiplied);
  QPainter painter(&result);
  painter.setCompositionMode(QPainter::CompositionMode_Source);
  painter.fillRect(result.rect(), Qt::transparent);
  painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

  painter.drawPixmap(0, 0, tray_icon_->icon().pixmap(size));

  QPixmap texture(2, 2);
  texture.fill(color);
  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
  painter.drawTiledPixmap(result.rect(), texture);
  painter.end();

  tray_icon_->setIcon(QPixmap::fromImage(result));
}

} // namespace tray_color
