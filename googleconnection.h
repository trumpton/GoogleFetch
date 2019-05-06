#ifndef GOOGLECONNECTION_H
#define GOOGLECONNECTION_H

#include <QJsonObject>
#include <QDomElement>
#include <QString>
#include <QList>

class GoogleConnection
{

public:
    enum Action {
        Post = 0,
        Create = 0,
        Put = 1,
        Update = 1,
        Delete = 2
    };

private:

    // Details received following google authorisation
    // either from the authorisation, or retrieved from
    // storage and passed to the constructor. Used by:
    //    GoogleConnection(QString rtoken);
    //    QString googleAuthorise() ;
    //    QString getUsername() ;
    QString username, refreshtoken, refreshtokenandusername ;
    QString errorstatus ;
    bool connectionerror ;
    int errorcode ;

    // Gets an access token, using the refresh token
    void getAccessToken() ;
    QString accesstoken  ;

    // Get / Post Response Data
    QString googleGetResponse ;
    QString googlePutPostResponse ;

    // Extract data from response
    QString ExtractParameter(QString Response, QString Parameter, int Occurrence=1) ;

  public:

    // Constructor, sets the refresh token which has been
    // previously generated with googleAuthorise.
    GoogleConnection();
    GoogleConnection(const QString &rt) ;
    void setupRToken(const QString& rt) ;

    // Pop up authorisation dialog, and return a refresh_token
    // which must be saved for future use.
    QString Authorise(QString logfile=QString("")) ;

    // Return the username associated with the current
    // google account
    QString getUsername() ;

    // Fetch information via a http get
    QString googleGet(QString link, QString logfile = QString("")) ;

    // Fetch information via an http put or post
    QString googlePutPostDelete(QString link, enum GoogleConnection::Action action, QString data = QString(""), bool isjson=false, bool includeheaderinresponse=false, QString logfile = QString("")) ;

    // Return the last network error string, or "" if OK
    QString getNetworkError() ;
    int getNetworkErrorCode() ;

    // Returns true if last network error was network-connection related
    bool isConnectionError() ;

    // Useful debug functions
    QString debugGetDataResponse() ;
    QString debugPutPostDataResponse() ;

};

#endif // GOOGLEACCESS_H
