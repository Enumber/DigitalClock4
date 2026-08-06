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

#ifndef DIGITAL_CLOCK_GUI_ABOUT_DIALOG_H
#define DIGITAL_CLOCK_GUI_ABOUT_DIALOG_H

#include <QDialog>

namespace digital_clock {
namespace gui {

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
  Q_OBJECT

public:
  explicit AboutDialog(QWidget* parent = nullptr);
  ~AboutDialog();

private slots:
  // The original author's own "surprise": triple-click the logo (see
  // ClickableLabel's requiredClicksCount) to toggle it between the two
  // easter-egg images. Unlike the pristine upstream version, this never
  // reverts to the plain rainbow icon — the About logo is permanently the
  // easter-egg artwork in this fork, by explicit request; the toggle only
  // switches between the two easter-egg images themselves.
  void on_logo_lbl_clicked();

private:
  int logoSize() const Q_DECL_NOEXCEPT;

private:
  Ui::AboutDialog* ui;
  bool logo_showing_alt_ = false;
};

} // namespace gui
} // namespace digital_clock

#endif // DIGITAL_CLOCK_GUI_ABOUT_DIALOG_H
