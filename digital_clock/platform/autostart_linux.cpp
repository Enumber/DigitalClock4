/*
    Digital Clock - beautiful customizable clock with plugins
    Copyright (C) 2015-2020  Nick Korotysh <nick.korotysh@gmail.com>

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

#include "autostart.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

static QString GetInstanceSuffix()
{
#ifdef PORTABLE_VERSION
  QByteArray path = QCoreApplication::applicationFilePath().toUtf8();
  quint16 checksum = qChecksum(path.data(), path.size());
  return QStringLiteral("-") + QString::number(checksum);
#endif
  return QString();
}

static QString GetAppFileName()
{
  QFileInfo fi(QCoreApplication::applicationFilePath());
  return fi.fileName();
}

static QString GetAutoStartDir()
{
  return QDir::home().absoluteFilePath(".config/autostart");
}

static QString GetDesktopFile()
{
  QDir auto_start_dir(GetAutoStartDir());
  return auto_start_dir.absoluteFilePath(GetAppFileName() + GetInstanceSuffix() + ".desktop");
}

static QString GetAutoStartCmd()
{
  // Upstream pointed this at "<binary>.sh", a launcher that only exists inside
  // their bundled-Qt tarball and sets LD_LIBRARY_PATH. It is not part of the
  // program, so on any normal install the autostart entry pointed at a file
  // that did not exist: the checkbox looked enabled and nothing ever started.
  // This build resolves its own libraries through $ORIGIN, so launch the
  // executable directly. --autostart is kept as a marker and ignored by the app.
  return QString("\"%1\" --autostart").arg(QCoreApplication::applicationFilePath());
}

// icon for the autostart entry: the real app icon shipped next to the binary,
// falling back to a freedesktop theme name when running from a build tree
static QString GetAutoStartIcon()
{
  QFileInfo bin(QCoreApplication::applicationFilePath());
  const QString png = bin.absoluteDir().absoluteFilePath(QStringLiteral("clock-icon.png"));
  if (QFile::exists(png)) return png;
  return QStringLiteral("clock");
}

static bool IsAutoStartFileHasCorrectCmd(const QString& path, const QString& cmd)
{
  QFile f(path);

  if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QString data(f.readAll());
    f.close();

    QRegularExpression re(QLatin1String("^Exec=(.*)$"), QRegularExpression::MultilineOption);
    QRegularExpressionMatch m = re.match(data);
    if (m.hasMatch())
      return m.captured(1) == cmd;
  }

  return false;
}


bool IsAutoStartEnabled()
{
  return IsAutoStartFileHasCorrectCmd(GetDesktopFile(), GetAutoStartCmd());
}

void SetAutoStart(bool enable)
{
  QString desktop_file = GetDesktopFile();
  if (enable) {
    QString autostart_cmd = GetAutoStartCmd();
    if (IsAutoStartFileHasCorrectCmd(desktop_file, autostart_cmd)) return;
    QString startup_dir = GetAutoStartDir();
    if (!QFile::exists(startup_dir)) QDir::home().mkpath(startup_dir);

    // *INDENT-OFF*
    QString desktop_data = QString(
          "[Desktop Entry]\n"
          "Name=%1\n"
          "Comment=%2\n"
          "Type=Application\n"
          "Terminal=false\n"
          "Exec=%3\n"
          "Icon=%4\n"
          "StartupNotify=false\n")
        .arg(
          QCoreApplication::applicationName(),
          QCoreApplication::applicationName() + " by " + QCoreApplication::organizationName(),
          autostart_cmd,
          GetAutoStartIcon());
    // *INDENT-ON*
    QFile file(desktop_file);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write(desktop_data.toLocal8Bit());
    file.close();
  } else {
    QFile::remove(desktop_file);
  }
}
