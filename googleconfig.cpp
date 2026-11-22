#include "googleconfig.h"

#include "../Lib/iniconfig.h"

#include <QApplication>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>

QString googleFetchIniFile()
{
    const QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) ;
    const QStringList folders = QStringList()
        << home + "/ContactManager"
        << home + "/AccessibleApps"
        << QApplication::applicationDirPath() ;

    // googlefetch.ini in every folder before ContactManager.ini is used
    const QStringList names = QStringList() << "googlefetch.ini" << "ContactManager.ini" ;

    for (const QString &name : names) {
        for (const QString &folder : folders) {
            const QString path = folder + "/" + name ;
            if (QFileInfo::exists(path)) return path ;
        }
    }
    return QString() ;
}

bool loadGoogleClient(const QString &inifile, QString &clientid, QString &secret)
{
    clientid = "" ;
    secret = "" ;
    if (inifile.isEmpty()) return false ;

    IniConfig ini ;
    if (!ini.init(inifile)) return false ;

    clientid = ini.get("google", "clientid").trimmed() ;
    secret = ini.get("google", "secret").trimmed() ;
    return !clientid.isEmpty() && !secret.isEmpty() ;
}
