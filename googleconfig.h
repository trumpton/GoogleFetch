#ifndef GOOGLECONFIG_H
#define GOOGLECONFIG_H

#include <QString>

// The ini file that GoogleFetch reads its Google client ID and secret from.  It is googlefetch.ini, or failing
// that ContactManager.ini, and is looked for in these folders, in this order, with the first one that exists used:
//
//    ~/ContactManager
//    ~/AccessibleApps
//    the folder of the program
//
// The entries are the [google] section, exactly as in ContactManager.ini:
//
//    [google]
//    clientid=...
//    secret=...

// The path of the ini file, or "" if neither file is found
QString googleFetchIniFile() ;

// Reads clientid and secret from the [google] section of the ini file.  Returns false if the file is "" or cannot be
// read, or if either value is blank.
bool loadGoogleClient(const QString &inifile, QString &clientid, QString &secret) ;

#endif // GOOGLECONFIG_H
