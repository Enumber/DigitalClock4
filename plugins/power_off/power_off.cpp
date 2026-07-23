/*
    Digital Clock: power off plugin
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

#include "power_off.h"

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <QProcess>
#endif

#include "plugin_settings.h"
#include "message_box.h"

#include "power_off_settings.h"

#include "gui/settings_dialog.h"

// Q_INIT_RESOURCE must be called from global scope (inside a namespace the
// symbol would be mangled into it) and with a name unique across the process:
// every plugin used to call its qrc "lang", so the shared symbol
// qInitResources_lang bound to whichever library loaded first.
// It must be called explicitly at all, because a plugin's compiled-in
// resources are not yet registered while its constructor runs, so tr() would
// silently return the untranslated source strings.
static void init_power_off_resources() { Q_INIT_RESOURCE(power_off_lang); }

namespace power_off {

PowerOff::PowerOff() : active_(false)
{
  init_power_off_resources();
  InitTranslator(QLatin1String(":/power_off/lang/power_off_"));
  info_.display_name = tr("Auto power off");
  info_.description = tr("Shutdown system at specified time.");
  InitIcon(":/power_off/icon.svg.p");
}

void PowerOff::Start()
{
  QSettings::SettingsMap defaults;
  InitDefaults(&defaults);
  settings_->SetDefaultValues(defaults);
  settings_->Load();

#ifdef Q_OS_WIN
  // Windows needs SE_SHUTDOWN_NAME enabled on the process token before
  // ExitWindowsEx will work. On Linux logind grants shutdown to the active
  // local session, so there is nothing to acquire up front.
  HANDLE hToken;
  TOKEN_PRIVILEGES* NewState;
  OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
  NewState = (TOKEN_PRIVILEGES*)malloc(sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES));
  NewState->PrivilegeCount = 1;
  LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &NewState->Privileges[0].Luid);
  NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  AdjustTokenPrivileges(hToken, FALSE, NewState, 0, NULL, NULL);
  free(NewState);
  CloseHandle(hToken);
#endif
}

void PowerOff::Configure()
{
  SettingsDialog* dialog = new SettingsDialog(DialogParent());
  connect(dialog, &SettingsDialog::destroyed, this, &PowerOff::configured);
  // load current settings to dialog
  connect(settings_, SIGNAL(OptionChanged(QString,QVariant)),
          dialog, SLOT(SettingsListener(QString,QVariant)));
  QSettings::SettingsMap defaults;
  InitDefaults(&defaults);
  settings_->SetDefaultValues(defaults);
  settings_->TrackChanges(true);
  settings_->Load();
  settings_->TrackChanges(false);
  // connect main signals/slots
  connect(dialog, SIGNAL(OptionChanged(QString,QVariant)),
          settings_, SLOT(SetOption(QString,QVariant)));
  connect(dialog, SIGNAL(accepted()), settings_, SLOT(Save()));
  connect(dialog, SIGNAL(rejected()), settings_, SLOT(Load()));
  dialog->show();
}

void PowerOff::TimeUpdateListener()
{
  QString off_time = settings_->GetOption(OPT_TIME).value<QTime>().toString();
  QString curr_time = QTime::currentTime().toString();
  if (off_time != curr_time || active_) return;
  active_ = true;
  TMessageBox msg;
  msg.setIcon(QMessageBox::Warning);
  msg.setWindowTitle(tr("System shutdown"));
  msg.setText(tr("Warning! Shutdown will happen after a few seconds."));
  msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
  msg.setDefaultButton(QMessageBox::Ok);
  msg.setAutoClose(true);
  msg.setTimeout(20);
  if (msg.exec() != QMessageBox::Ok) return;
  const bool force = settings_->GetOption(OPT_FORCE).toBool();
#ifdef Q_OS_WIN
  UINT flags = EWX_SHUTDOWN;
  force ? flags |= EWX_FORCE : flags |= EWX_FORCEIFHUNG;
  ExitWindowsEx(flags, 0);
#else
  // logind lets the active local session power off without root; --force skips
  // asking the running applications to close, matching EWX_FORCE on Windows.
  QStringList args;
  if (force) args << QLatin1String("--force");
  args << QLatin1String("poweroff");
  if (!QProcess::startDetached(QLatin1String("systemctl"), args))
    QProcess::startDetached(QLatin1String("shutdown"), {QLatin1String("-h"), QLatin1String("now")});
#endif
}

} // namespace power_off
