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

#ifndef DIGITAL_CLOCK_GUI_ADVANCED_SETTINGS_DIALOG_H
#define DIGITAL_CLOCK_GUI_ADVANCED_SETTINGS_DIALOG_H

#include <QDialog>

#include "settings_keys.h"

namespace digital_clock {

namespace core {
class ClockSettings;
}

namespace gui {

namespace Ui {
class AdvancedSettingsDialog;
}

// The handful of Misc-tab options a normal user rarely touches, pulled out
// behind their own "Advanced Settings..." button — Appearance is untouched
// and matches the original author's tab exactly, this dialog only ever holds
// Misc-tab overflow.
class AdvancedSettingsDialog : public QDialog
{
  Q_OBJECT

public:
  AdvancedSettingsDialog(core::ClockSettings* config, QWidget* parent = nullptr);
  ~AdvancedSettingsDialog();

signals:
  void OptionChanged(Option opt, const QVariant& value);

private slots:
  void on_show_on_all_monitors_clicked(bool checked);
  void on_show_in_fullscreen_clicked(bool checked);
  void on_snap_to_edges_clicked(bool checked);
  void on_snap_threshold_valueChanged(int value);
  void on_refresh_interval_valueChanged(int value);
  void on_clock_url_enabled_clicked(bool checked);
  void on_clock_url_edit_textEdited(const QString& arg1);
  void on_browse_url_file_btn_clicked();
  void on_show_hide_enable_clicked(bool checked);
  void on_export_state_clicked(bool checked);

private:
  void InitControls();

  Ui::AdvancedSettingsDialog* ui;

  core::ClockSettings* config_;
};

} // namespace gui
} // namespace digital_clock

#endif // DIGITAL_CLOCK_GUI_ADVANCED_SETTINGS_DIALOG_H
