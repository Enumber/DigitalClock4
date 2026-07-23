/*
    Digital Clock: window on top plugin
    Copyright (C) 2014-2020  Nick Korotysh <nick.korotysh@gmail.com>

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

#include "win_on_top.h"

#include <QCoreApplication>

// Q_INIT_RESOURCE must be called from global scope (inside a namespace the
// symbol would be mangled into it) and with a name unique across the process:
// every plugin used to call its qrc "lang", so the shared symbol
// qInitResources_lang bound to whichever library loaded first.
// It must be called explicitly at all, because a plugin's compiled-in
// resources are not yet registered while its constructor runs, so tr() would
// silently return the untranslated source strings.
static void init_win_on_top_resources() { Q_INIT_RESOURCE(win_on_top_lang); }

namespace win_on_top {

typedef void (*pSetItemName)(LPCWSTR);
typedef void (*pStartFunc)(void);
typedef void (*pStopFunc)(void);

WinOnTop::WinOnTop() : lib_(0)
{
  init_win_on_top_resources();
  InitTranslator(QLatin1String(":/win_on_top/lang/win_on_top_"));
  info_.display_name = tr("\"Always on top\" for any window");
  info_.description = tr("Adds \"Always on Top\" menu item to system menu for any window.");
}

void WinOnTop::Start()
{
  std::wstring lib_path = QCoreApplication::applicationDirPath().toStdWString();
  lib_path += L"/plugins/special/win_on_top_hook.dll";
  lib_ = LoadLibrary(lib_path.c_str());
  if (!lib_) return;

  WCHAR item_name[32];
  memset(item_name, 0, 64);
  tr("Always on Top").toWCharArray(item_name);
  pSetItemName set_name = (pSetItemName)GetProcAddress(lib_, "SetItemName");
  set_name(item_name);

  pStartFunc start = (pStartFunc)GetProcAddress(lib_, "Start");
  start();
}

void WinOnTop::Stop()
{
  if (!lib_) return;
  pStopFunc stop = (pStopFunc)GetProcAddress(lib_, "Stop");
  stop();
  FreeLibrary(lib_);
}

} // namespace win_on_top
