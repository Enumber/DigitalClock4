#
#   Digital Clock - beautiful customizable clock with plugins
#   Copyright (C) 2013-2020  Nick Korotysh <nick.korotysh@gmail.com>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

TEMPLATE = subdirs

SUBDIRS += \
    alarm \
    any_zoom \
    chime \
    countdown_timer \
    date \
    ip_address \
    quick_note \
    random_position \
    schedule \
    spectrum_clock \
    timetracker \
    tray_color \
    var_translucency

# power_off works on Linux too now (logind instead of ExitWindowsEx);
# win_on_top stays Windows-only, X11 already has "stay on top" built in
SUBDIRS += \
    power_off

windows {
win32-msvc* {
SUBDIRS += \
    win_on_top
}
}

qtHaveModule(texttospeech):SUBDIRS += talking_clock

OTHER_FILES += common.pri
