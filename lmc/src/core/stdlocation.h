/****************************************************************************
**
** This file is part of LAN Messenger.
** 
** Copyright (c) 2010 - 2012 Qualia Digital Solutions.
** 
** Contact:  qualiatech@gmail.com
** 
** LAN Messenger is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** LAN Messenger is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with LAN Messenger.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/


#ifndef STDLOCATION_H
#define STDLOCATION_H

#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>
#include <QDateTime>
#include <QStandardPaths>
#include <QCoreApplication>
#include "settings.h"

#define SL_TRANSFERHISTORY "transfers.lst"
#define SL_FILESTORAGEDIR "Received Files"
#define SL_CACHEDIR "cache"
#define SL_RESOURCE "lmc.rcc"
#define SL_LANGDIR "lang"
#define SL_THEMEDIR "themes"
#define SL_GROUPFILE "group.cfg"
#define SL_AVATARFILE "avt_local.png"
#define SL_LOGDIR "logs"
#define SL_TEMPCONFIG "lmctmpconf.ini"

class StdLocation {
public:
  static QString transferHistory(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::AppLocalDataLocation) +
                                    "/" SL_TRANSFERHISTORY);
  }

  static QString fileStorageDir(void) {
    lmcSettings settings;

    QString path = QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                              QStandardPaths::DocumentsLocation) +
                                            "/" SL_FILESTORAGEDIR);
    path = settings.value(IDS_FILESTORAGEPATH, path).toString();
    return path;
  }

  static QString cacheDir(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::AppLocalDataLocation) +
                                    "/" SL_CACHEDIR);
  }

  static QString libDir(void) {
    return QDir::toNativeSeparators(QDir::currentPath());
  }

  static QString resourceFile(void) {
    QStringList candidates = {
      QDir::current().absoluteFilePath(SL_RESOURCE),
      QCoreApplication::applicationDirPath() + "/" SL_RESOURCE,
      QCoreApplication::applicationDirPath() + "/runtime/" SL_RESOURCE,
      QCoreApplication::applicationDirPath() + "/../release/" SL_RESOURCE,
      "/usr/lib/lmc/" SL_RESOURCE
    };
    for(const QString& c : candidates) {
      if(QFile::exists(c))
        return QDir::toNativeSeparators(c);
    }
    return QDir::toNativeSeparators(candidates.first());
  }

  static QString resLangDir(void) {
    return ":/" SL_LANGDIR;
  }

  static QString sysLangDir(void) {
    QStringList candidates = {
      QDir::currentPath() + "/" SL_LANGDIR,
      QCoreApplication::applicationDirPath() + "/" SL_LANGDIR,
      QCoreApplication::applicationDirPath() + "/runtime/" SL_LANGDIR,
      QCoreApplication::applicationDirPath() + "/../release/" SL_LANGDIR,
      "/usr/lib/lmc/" SL_LANGDIR
    };
    for(const QString& c : candidates) {
      if(QDir(c).exists())
        return QDir::toNativeSeparators(c);
    }
    return QDir::toNativeSeparators(candidates.first());
  }

  static QString userLangDir(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::AppLocalDataLocation) +
                                    "/" SL_LANGDIR);
  }

  static QString resThemeDir(void) {
    return ":/" SL_THEMEDIR;
  }

  static QString sysThemeDir(void) {
    QStringList candidates = {
      QDir::currentPath() + "/" SL_THEMEDIR,
      QCoreApplication::applicationDirPath() + "/" SL_THEMEDIR,
      QCoreApplication::applicationDirPath() + "/../release/" SL_THEMEDIR,
      "/usr/lib/lmc/" SL_THEMEDIR
    };
    for(const QString& c : candidates) {
      if(QDir(c).exists())
        return QDir::toNativeSeparators(c);
    }
    return QDir::toNativeSeparators(candidates.first());
  }

  static QString userThemeDir(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::AppLocalDataLocation) +
                                    "/" SL_THEMEDIR);
  }

  static QString groupFile(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::AppLocalDataLocation) +
                                    "/" SL_GROUPFILE);
  }

  static QString avatarFile(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::AppLocalDataLocation) +
                                    "/" SL_AVATARFILE);
  }

  static QString logDir(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::AppLocalDataLocation) +
                                    "/" SL_LOGDIR);
  }

  static QString freeLogFile(void) {
    QString fileName = "lmc_" +
                       QString::number(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()) + ".log";
    return QDir::toNativeSeparators(logDir() + "/" + fileName);
  }

  static QString tempConfigFile(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::TempLocation) +
                                    "/" SL_TEMPCONFIG);
  }
};

#endif // STDLOCATION_H
