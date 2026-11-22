#ifndef GOOGLECONNECTION_H
#define GOOGLECONNECTION_H

#include <QString>
#include <QNetworkAccessManager>
#include "ratepacer.h"

// The Google sign-in and requests that GoogleFetch makes.  These are copies of the ones ContactManager makes (its
// googleaccess_oauth2.cpp and googleaccess_network.cpp), so a request sent from here is sent in the same way.  The
// bench fault injection and the ContactManager settings are left out.
class GoogleConnection
{

public:
    enum Action {
        Post = 0,
        Create = 0,
        Put = 1,
        Update = 1,
        Delete = 2,
        Patch = 3
    };

private:

    // Details received following google authorisation, either from the authorisation, or retrieved from
    // storage and passed to setupRToken
    QString username, refreshtoken, refreshtokenandusername ;
    QString accesstoken ;

    // The Google OAuth client ID and secret, from the ini file (see googleconfig.h)
    QString clientid, clientsecret ;

    // The last error, and what to do next about it
    QString errorstatus ;
    QString errorrecovery ;
    bool connectionerror ;
    int errorcode ;

    // The last request that failed, and its recovery step
    QString lastfailure, lastrecovery ;

    // Pacing of the uploads after Google's rate limits
    RatePacer pacer ;

    // Number for the log file of each request
    int logsequencenumber ;

    // One network manager is shared by the requests
    QNetworkAccessManager netmanager ;

    // Responses
    QString googleGetResponse ;
    QString googlePutPostResponse ;

    // Gets an access token, using the refresh token
    void getAccessToken() ;

    // Extract a parameter from a JSON response
    QString ExtractParameter(QString Response, QString Parameter, int Occurrence=1) ;

public:

    // Constructor.  The refresh token, previously generated with Authorise, is given to setupRToken.
    GoogleConnection();
    GoogleConnection(const QString &rt) ;
    void setupRToken(const QString& rt) ;

    // Sets the Google OAuth client ID and secret, which every request sends
    void setClient(const QString &id, const QString &secret) ;

    // Pop up authorisation dialog, and return the refresh token and username, which must be saved for future use.
    QString Authorise(QString logfile=QString("")) ;

    // Return the username associated with the current google account
    QString getUsername() ;

    // Fetch information via a http get
    QString googleGet(QString link, QString logfile = QString("")) ;

    // Fetch information via an http put, post or delete.  isjson sets the content type, and includeheaderinresponse
    // adds the request's URL and headers to the answer, for the debug display.
    QString googlePutPostDelete(QString link, enum GoogleConnection::Action action, QString data = QString(""), bool isjson=false, bool includeheaderinresponse=false, QString logfile = QString("")) ;

    // Return the last network error string, or "" if OK
    QString getNetworkError() ;
    int getNetworkErrorCode() ;

    // What to do next, when the last request failed
    QString getRecoveryStep() { return errorrecovery ; }

    // Returns true if last network error was network-connection related
    bool isConnectionError() ;

};

#endif // GOOGLECONNECTION_H
