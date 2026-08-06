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

#include "skin_manager.h"

#include <QDir>
#include <QSettings>
#include <QApplication>

#include "skin/clock_raster_skin.h"
#include "skin/clock_vector_skin.h"
#include "skin/clock_text_skin.h"

namespace digital_clock {
namespace core {

namespace {

// Whether skin_root looks like a valid skin (same test CreateSkin() uses),
// without actually constructing a skin object. Mirrors CreateSkin()'s logic:
// a directory is a valid skin iff it has skin.ini AND at least one *.svg or
// *.png file.
bool IsValidSkinDir(const QDir& skin_root)
{
  if (!skin_root.exists("skin.ini")) return false;
  bool has_svg = !skin_root.entryList(QStringList("*.svg"), QDir::Files).isEmpty();
  bool has_png = !skin_root.entryList(QStringList("*.png"), QDir::Files).isEmpty();
  return has_svg || has_png;
}

// Reads only the display name out of skin.ini. Used by ListSkins() to build
// the skin picker: it must NOT go through CreateSkin(), because for raster
// (PNG) skins that eagerly decodes all 10 digits + separators + am/pm into
// QPixmap objects (RasterSkin's ctor) just to read one string out of an ini
// file. With ~20 bundled skins (8 of them PNG-based, ~8.6MB of PNG source
// data) that meant every app start decoded every raster skin's images into
// memory, one skin at a time, purely to populate the "Skin" dropdown.
QString ReadSkinName(const QDir& skin_root)
{
  QSettings config(skin_root.filePath("skin.ini"), QSettings::IniFormat);
  return config.value("info/name").toString();
}

} // namespace

ClockSkinPtr CreateSkin(const QDir& skin_root)
{
  QStringList images = skin_root.entryList(QStringList("*.svg"), QDir::Files);
  bool skinini = skin_root.exists("skin.ini");
  ClockSkinPtr skin;
  if (!images.empty() && skinini) skin.reset(new VectorSkin(skin_root));
  images = skin_root.entryList(QStringList("*.png"), QDir::Files);
  if (!images.empty() && skinini) skin.reset(new RasterSkin(skin_root));
  return skin;
}

ClockSkinPtr CreateSkin(const QFont& font)
{
  return ClockSkinPtr(new TextSkin(font));
}


SkinManager::SkinManager(QObject* parent) : QObject(parent)
{
  search_paths_.append(":/clock/default_skins");
#ifdef Q_OS_MACOS
  search_paths_.append(qApp->applicationDirPath() + "/../Resources/skins");
#else
  search_paths_.append(qApp->applicationDirPath() + "/skins");
#endif
#ifdef Q_OS_LINUX
  search_paths_.append("/usr/share/digitalclock4/skins");
  search_paths_.append("/usr/local/share/digitalclock4/skins");
  search_paths_.append(QDir::homePath() + "/.local/share/digitalclock4/skins");
#endif
}

ClockSkinPtr SkinManager::CurrentSkin() const
{
  return current_skin_;
}

void SkinManager::ListSkins()
{
  skins_.clear();
  for (auto& s_path : qAsConst(search_paths_)) {
    QDir s_dir(s_path);
    for (auto& f_dir : s_dir.entryList(QDir::NoDotAndDotDot | QDir::AllDirs)) {
      QDir skin_root(s_dir.filePath(f_dir));
      // Just read the name from skin.ini here — do NOT call CreateSkin(),
      // it would decode every image of every raster skin only to throw the
      // result away a moment later. Actual image loading happens lazily in
      // LoadSkin(), once, for whichever single skin the user has selected.
      if (!IsValidSkinDir(skin_root)) continue;
      skins_[ReadSkinName(skin_root)] = skin_root;
    }
  }
  emit SearchFinished(skins_.keys());
}

void SkinManager::LoadSkin(const QString& skin_name)
{
  ClockSkinPtr skin;
  if (skin_name == "Text Skin") {
    skin = CreateSkin(font_);
  } else {
    skin = CreateSkin(skins_[skin_name]);
  }
  if (!skin && !fallback_skin_.isEmpty()) {
    emit ErrorMessage(tr("Skin '%1' is not loaded, using default skin.").arg(skin_name));
    skin = CreateSkin(skins_[fallback_skin_]);
  }
  current_skin_ = skin;
  SetSeparators(seps_);
  emit SkinLoaded(skin.dynamicCast<skin_draw::ISkin>());
  // get skin info
  BaseSkin::TSkinInfo info;
  if (skin) info = skin->GetInfo();
  emit SkinInfoLoaded(info);
}

void SkinManager::SetFont(const QFont& font)
{
  font_ = font;
  // update text skin if needed
  if (current_skin_.dynamicCast<TextSkin>()) LoadSkin("Text Skin");
}

void SkinManager::SetSeparators(const QString& seps)
{
  seps_ = seps;
  if (current_skin_) current_skin_->SetSeparators(seps);
}

void SkinManager::SetFallbackSkin(const QString& skin_name)
{
  fallback_skin_ = skin_name;
}

} // namespace core
} // namespace digital_clock
