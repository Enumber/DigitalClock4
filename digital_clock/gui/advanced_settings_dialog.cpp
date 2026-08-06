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

#include "advanced_settings_dialog.h"
#include "ui_advanced_settings_dialog.h"

#include <QFileDialog>
#include <QUrl>

#include "core/clock_settings.h"

namespace digital_clock {
namespace gui {

AdvancedSettingsDialog::AdvancedSettingsDialog(core::ClockSettings* config, QWidget* parent) :
  QDialog(parent),
  ui(new Ui::AdvancedSettingsDialog),
  config_(config)
{
  ui->setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose);
#if !defined(Q_OS_WIN) && !defined(Q_OS_LINUX)
  ui->show_in_fullscreen->setVisible(false);   // needs per-platform window enumeration
#endif
  InitControls();
  resize(minimumSizeHint());
}

AdvancedSettingsDialog::~AdvancedSettingsDialog()
{
  delete ui;
}

void AdvancedSettingsDialog::InitControls()
{
  Q_ASSERT(config_);

  ui->show_on_all_monitors->setChecked(config_->GetValue(OPT_SHOW_ON_ALL_MONITORS).toBool());
  ui->show_in_fullscreen->setChecked(!config_->GetValue(OPT_FULLSCREEN_DETECT).toBool());

  ui->snap_to_edges->setChecked(config_->GetValue(OPT_SNAP_TO_EDGES).toBool());
  ui->snap_threshold->setValue(config_->GetValue(OPT_SNAP_THRESHOLD).toInt());
  ui->refresh_interval->setValue(config_->GetValue(OPT_REFRESH_INTERVAL).toInt());

  ui->clock_url_enabled->setChecked(config_->GetValue(OPT_CLOCK_URL_ENABLED).toBool());
  ui->clock_url_edit->setText(config_->GetValue(OPT_CLOCK_URL_STRING).toString());

  ui->show_hide_enable->setChecked(config_->GetValue(OPT_SHOW_HIDE_ENABLED).toBool());
  ui->export_state->setChecked(config_->GetValue(OPT_EXPORT_STATE).toBool());
}

void AdvancedSettingsDialog::on_show_on_all_monitors_clicked(bool checked)
{
  emit OptionChanged(OPT_SHOW_ON_ALL_MONITORS, checked);
}

void AdvancedSettingsDialog::on_show_in_fullscreen_clicked(bool checked)
{
  emit OptionChanged(OPT_FULLSCREEN_DETECT, !checked);
}

void AdvancedSettingsDialog::on_snap_to_edges_clicked(bool checked)
{
  emit OptionChanged(OPT_SNAP_TO_EDGES, checked);
}

void AdvancedSettingsDialog::on_snap_threshold_valueChanged(int value)
{
  emit OptionChanged(OPT_SNAP_THRESHOLD, value);
}

void AdvancedSettingsDialog::on_refresh_interval_valueChanged(int value)
{
  emit OptionChanged(OPT_REFRESH_INTERVAL, value);
}

void AdvancedSettingsDialog::on_clock_url_enabled_clicked(bool checked)
{
  emit OptionChanged(OPT_CLOCK_URL_ENABLED, checked);
}

void AdvancedSettingsDialog::on_clock_url_edit_textEdited(const QString& arg1)
{
  emit OptionChanged(OPT_CLOCK_URL_STRING, arg1);
}

void AdvancedSettingsDialog::on_browse_url_file_btn_clicked()
{
  QUrl url = QFileDialog::getOpenFileUrl(this);
  if (!url.isValid()) return;
  ui->clock_url_edit->setText(url.toString());
  // setText() doesn't emit textEdited(), which is what actually persists the
  // value (see on_clock_url_edit_textEdited() below) — without this, picking
  // a file here only ever updated the visible text, never the saved option.
  emit OptionChanged(OPT_CLOCK_URL_STRING, url.toString());
}

void AdvancedSettingsDialog::on_show_hide_enable_clicked(bool checked)
{
  emit OptionChanged(OPT_SHOW_HIDE_ENABLED, checked);
}

void AdvancedSettingsDialog::on_export_state_clicked(bool checked)
{
  emit OptionChanged(OPT_EXPORT_STATE, checked);
}

} // namespace gui
} // namespace digital_clock
