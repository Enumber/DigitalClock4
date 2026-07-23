/*
    Digital Clock: spectrum clock plugin
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

#include "spectrum_clock.h"

// Q_INIT_RESOURCE must be called from global scope (inside a namespace the
// symbol would be mangled into it) and with a name unique across the process:
// every plugin used to call its qrc "lang", so the shared symbol
// qInitResources_lang bound to whichever library loaded first.
// It must be called explicitly at all, because a plugin's compiled-in
// resources are not yet registered while its constructor runs, so tr() would
// silently return the untranslated source strings.
static void init_spectrum_clock_resources() { Q_INIT_RESOURCE(spectrum_clock_lang); }

namespace spectrum_clock {

SpectrumClock::SpectrumClock()
{
  init_spectrum_clock_resources();
  InitTranslator(QLatin1String(":/spectrum_clock/lang/spectrum_clock_"));
  info_.display_name = QLatin1String("\"Spectrum clock\"");
  info_.description = tr("Changes clock color during time.");
  InitIcon(":/spectrum_clock/icon.png");
}

void SpectrumClock::Init(const QMap<Option, QVariant>& current_settings)
{
  old_color_ = current_settings[OPT_COLOR].value<QColor>();
  old_colorize_color_ = current_settings[OPT_COLORIZE_COLOR].value<QColor>();
  cur_color_ = Qt::red;
}

void SpectrumClock::Start()
{
  emit OptionChanged(OPT_COLOR, cur_color_);
  emit OptionChanged(OPT_COLORIZE_COLOR, cur_color_);
}

void SpectrumClock::Stop()
{
  emit OptionChanged(OPT_COLOR, old_color_);
  emit OptionChanged(OPT_COLORIZE_COLOR, old_colorize_color_);
}

void SpectrumClock::TimeUpdateListener()
{
  int r = cur_color_.red();
  int g = cur_color_.green();
  int b = cur_color_.blue();
  int i = 5;

  if (r == 255 && g  < 255 && b ==  0 ) { g += i; }
  if (r  >  0  && g == 255 && b ==  0 ) { r -= i; }
  if (r ==  0  && g == 255 && b  < 255) { b += i; }
  if (r ==  0  && g  >  0  && b == 255) { g -= i; }
  if (r  < 255 && g ==  0  && b == 255) { r += i; }
  if (r == 255 && g ==  0  && b  >  0 ) { b -= i; }

  cur_color_.setRgb(r, g, b);
  emit OptionChanged(OPT_COLOR, cur_color_);
  emit OptionChanged(OPT_COLORIZE_COLOR, cur_color_);
}

} // namespace spectrum_clock
