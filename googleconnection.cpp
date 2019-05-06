
#include "googleconnection.h"
#include "google-account.h"

#include "../Lib/supportfunctions.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QMessageBox>
#include <QUrlQuery>


//*****************************************************************************************************

// TODO: IfMatch ETag
//       see: https://developers.google.com/google-apps/contacts/v3/#updating_contacts
//
// TODO: retry in case accesstoken had expired
//
// TODO: merge post and get into same finction
//       googleQuery(QString link, QString xml, bool post)
//       if xml is empty, it is a get (and the content type xml is not set)
//       if post is true, it's a post
//       otherwise it's a put
//
// TODO: applies to all network connections - if the computer does not have an active network
//       interface, the app segfaults


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Constructor
//

//
//=====================================================================================================
// Public: GoogleAccess - Constructor
//
//  Initialises, and saves the refresh token to the internal data structures
//  if it has been provided.  If it is not provided, no function will work, and
//  a refresh token must be acquired by calling the Authorise function.
//
GoogleConnection::GoogleConnection()
{
    refreshtoken="" ;
    errorstatus="" ;
    accesstoken="" ;
    username="" ;
    refreshtokenandusername = "" ;
}

GoogleConnection::GoogleConnection(const QString& rt)
{
    refreshtoken="" ;
    errorstatus="" ;
    accesstoken="" ;
    username="" ;
    refreshtokenandusername = "" ;
    setupRToken(rt);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Network Request / Response Functions
//

//=====================================================================================================
//
// Public: getNetworkError
//
// Return the last network error string, or "" if OK
//
QString GoogleConnection::getNetworkError()
{
    return errorstatus ;
}

int GoogleConnection::getNetworkErrorCode()
{
    return errorcode ;
}

bool GoogleConnection::isConnectionError()
{
    return connectionerror ;
}



//=====================================================================================================
//
// Public: googlePutPostDelete
//
// Perform a Put/Post or Delete Operation, and return the corresponding message
//
QString GoogleConnection::googlePutPostDelete(QString link, enum GoogleConnection::Action action, QString data, bool isjson, bool includeheaderinresponse, QString logfile)
{
    googlePutPostResponse="" ;
    errorcode=0 ;
    errorstatus="" ;

    int retries=1 ;
    int complete=false ;
    int readsuccess=false ;
    QString header ;
    QString QueryChar="?" ;
    if (link.contains("?")) QueryChar="&" ;

    if (!logfile.isEmpty()) writeToFile(logfile, "") ;

    do {
        if (accesstoken.isEmpty()) {

            readsuccess=false ;
            googlePutPostResponse = "No Access Token" ;

        } else {

            QNetworkAccessManager manager ;
            QNetworkReply *reply ;
            QUrl url(link + QueryChar + "access_token=" + accesstoken) ;
            QNetworkRequest request(url) ;
            QEventLoop eventLoop ;

            // get the page
            QObject::connect(&manager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
            QByteArray submitdata = data.toStdString().c_str() ;

            if (isjson) {
                request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                header = "Content-Type: application/json\n" ;
            } else {
                request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
                header = "Content-Type: application/xml\n" ;
            }

            request.setHeader(QNetworkRequest::ContentLengthHeader, submitdata.length());
            header = header + "Content-Length: " + submitdata.length() + "\n" ;

            request.setRawHeader("charset", "UTF-8") ;
            header = header + "charset: UTF-8\n" ;

            QString auth = QString("Bearer ") + accesstoken ;
            request.setRawHeader("Authorization", auth.toLatin1()) ;
            header = header + "Authorization: Bearer " + accesstoken + "\n" ;

            request.setRawHeader("GData-Version", "3.0") ;
            header = header + "GData-Version: 3.0" ;

            request.setRawHeader("If-Match", "*") ;
            header = header + "If-Match: *\n" ;

            QString actionname ;

            switch (action) {
            case GoogleConnection::Post:
                reply = manager.post(request, submitdata) ;
                actionname="POST" ;
                break ;
            case GoogleConnection::Put:
                reply = manager.put(request, submitdata) ;
                actionname="PUT" ;
                break ;
            case GoogleConnection::Delete:
                // From PHP - $req->setRequestHeaders(array('content-type' => 'application/atom+xml; charset=UTF-8; type=feed'));
                // From stackexchange: "GData-Version": "3.0", "Authorization":"Bearer " + token.accesstoken, "if-match":"*"
                reply = manager.deleteResource(request) ;
                actionname="DELETE" ;
                break ;
            }

            if (!logfile.isEmpty())
                writeToFile(logfile,
                            QString("URL: ") + link + QueryChar + "access_token=" + accesstoken +
                            QString("\n\nHEADER >>\n") + header +
                            QString("\n\n") + actionname + QString(">>\n") + data +
                            QString("\n\n"), true) ;

            eventLoop.exec() ;
            QVariant replycode=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) ;
            errorcode=replycode.toInt() ;

            // if page load is OK, get details, else set error string
            switch (reply->error()) {
            case QNetworkReply::ConnectionRefusedError:
                connectionerror = true ;
                errorstatus="Connection Refused." ;
                break ;
            case QNetworkReply::RemoteHostClosedError:
                connectionerror = true ;
                errorstatus="Remote Host Closed Connection." ;
                break ;
            case QNetworkReply::HostNotFoundError:
                connectionerror = true ;
                errorstatus="Host accounts.google.com Not Found." ;
                break ;
            case QNetworkReply::UnknownServerError:
                connectionerror = true ;
                errorstatus="Unknown Server Error." ;
                break ;
            default:
                connectionerror = false ;
                if (replycode>=200 && replycode<=299) {
                  errorstatus = "" ;
                  googlePutPostResponse = reply->readAll() ;
                  readsuccess=true ; ;
                } else {
                  errorstatus = "Network Error " + replycode.toString() ;
                  googlePutPostResponse = errorstatus + " - " + reply->readAll() ;  // TODO: temp
                  readsuccess=false ;
                }
            }
        }

        // if there is an error refresh the access token and retry once
        if (readsuccess || retries==0) {
            complete=true ;
        } else {
            getAccessToken() ;
            retries-- ;
        }

        if (!logfile.isEmpty())
            writeToFile(logfile,
                        QString("RESPONSE>>\n") + googlePutPostResponse +
                        QString("STATUS: ") + errorstatus +
                        QString("\n\n--------------------------\n\n"), true) ;

    } while (!complete) ;

    if (includeheaderinresponse) {
        return  QString("URL: ") + link + QueryChar + "access_token=" + accesstoken +
                QString("\n\nHEADER >>\n") + header +
                QString("\n\n-----------------------------------------\n\n") + googlePutPostResponse ;

    }
    return googlePutPostResponse ;
}


//=====================================================================================================
//
// Public: googleGet
//
// Perform a Get Operation, and return the corresponding message
//
QString GoogleConnection::googleGet(QString link, QString logfile)
{
    int retries=1 ;
    int complete=false ;
    int readsuccess=false ;

    errorcode=0 ;
    errorstatus="" ;
    googleGetResponse="" ;

    if (!logfile.isEmpty()) writeToFile(logfile, "") ;

    do {
        if (accesstoken.isEmpty()) {

            googleGetResponse = "" ;
            readsuccess=false ;

        } else {

            QString QueryChar="?" ;
            if (link.contains("?")) QueryChar="&" ;
            QNetworkAccessManager manager ;
            QNetworkReply *reply ;
            QUrl url(link + QueryChar + "access_token=" + accesstoken) ;
            QNetworkRequest request(url) ;
            QEventLoop eventLoop ;

            if (!logfile.isEmpty())
                writeToFile(logfile,
                            QString("URL: ") + link + QueryChar + "access_token=" + accesstoken +
                            QString("\n\nGET>>") +
                            QString("\n\n"), true) ;

            // get the page
            QObject::connect(&manager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
            reply = manager.get(request);
            eventLoop.exec() ;
            QVariant replycode=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) ;
            errorcode=replycode.toInt() ;

            switch (reply->error()) {
            case QNetworkReply::ConnectionRefusedError:
                connectionerror=true ;
                errorstatus="Connection Refused" ;
                break ;
            case QNetworkReply::RemoteHostClosedError:
                connectionerror=true ;
                errorstatus="Remote Host Closed Connection" ;
                break ;
            case QNetworkReply::HostNotFoundError:
                connectionerror=true ;
                errorstatus="Host accounts.google.com Not Found" ;
                break ;
            case QNetworkReply::UnknownServerError:
                connectionerror=true ;
                errorstatus="Unknown Server Error" ;
                break ;
            default:
                connectionerror=false ;
                // if page load is OK, get details, else set error string
                if (replycode>=200 && replycode<=299) {
                  googleGetResponse = reply->readAll() ;
                  errorstatus="" ;
                  readsuccess=true ;
                } else {
                  googleGetResponse = "" ;
                  errorstatus = "Network Error " + replycode.toString() ;
                  readsuccess=false ;
                }
                break ;

            }
        }

        // if there is an error refresh the access token and retry once
        if (readsuccess || retries==0) {
            complete=true ;
        } else {
            getAccessToken() ;
            retries-- ;
        }

        if (!logfile.isEmpty())
            writeToFile(logfile,
                        QString("RESPONSE>>\n") + googleGetResponse +
                        QString("STATUS: ") + errorstatus +
                        QString("\n\n--------------------------\n\n"), true) ;


    } while (!complete) ;


    return googleGetResponse ;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  OAuth2 Functions
//

//
// Useful Links
//
// The oauth2 playground allows you to walk through all submissions
// accesses manually, and gives a good overview of how the protocols
// actually work:
//
//     https://developers.google.com/oauthplayground/
//
// https://developers.google.com/google-apps/contacts/v3/?hl=ja#updating_contacts
//


// OAuth2 / Google - Network Access Helper Functions
//
// setupRToken          - Sets the refresh token, previously generated with Authorise
// Authorise            - allows user to log in and authorise use.  Returns refresh token/email
// googleGetAccessToken - Retrieve the access token for the session
//
//


//=====================================================================================================
//
// Public - GoogleConnection - setupRToken
//
// Constructor, sets the refresh token which has been
// previously generated with Authorise.
//
void GoogleConnection::setupRToken(const QString& rt)
{
    QStringList rtparts = rt.split(" ") ;
    if (rtparts.size()==2) {
      refreshtoken=rtparts.at(0) ;
      username=rtparts.at(1) ;
    }
}


//=====================================================================================================
//
// Public: GoogleConnection - Authorise
//
//  Get the authorisation token, by popping up a dialog box to prompt the
//  user to visit an authorisation url, and enter the response code.
//
//  Returns a refresh_token, which is valid indefinately, and enables the app
//  to gain access again and again without logging in.
//
QString GoogleConnection::Authorise(QString logfile)
{
    QString resultstring  ;

    QString device_code ;
    QString user_code ;
    QString verification_url ;
    QString expires_in ;
    QString interval ;

    refreshtoken="" ;
    accesstoken="" ;
    username="" ;
    refreshtokenandusername="" ;

    // Get the authorisation url and user code

    {
      QNetworkReply *reply ;
      QEventLoop eventLoop ;
      QNetworkAccessManager manager ;
      QUrlQuery params ;
      QUrl url("https://accounts.google.com/o/oauth2/device/code") ;
      QNetworkRequest request(url) ;
      params.addQueryItem("client_id", CLIENTID);
      params.addQueryItem("scope", SCOPE);
      request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
      QObject::connect(&manager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
      reply = manager.post(request, params.query(QUrl::FullyEncoded).toUtf8());

      eventLoop.exec() ;
      QNetworkReply::NetworkError err = reply->error() ;
      QString errstring = reply->errorString() ;
      resultstring = reply->readAll() ;
      device_code = ExtractParameter(resultstring, "device_code") ;
      user_code = ExtractParameter(resultstring, "user_code") ;
      verification_url = ExtractParameter(resultstring, "verification_url") ;
      expires_in = ExtractParameter(resultstring, "expires_in") ;
      interval = ExtractParameter(resultstring, "interval") ;
    }

    if (user_code.isEmpty()) {
        addLog("Unable to authorise with Google.  This is caused by either a network error (check your connection); incorrectly configured GoogleConnection-account.h (during compilation); or missing OpenSSL files - ssleay32.dll / libeay32.dll (Windows Clients)") ;
        errorOkDialog(NULL, "Contact Manager Error", "Unable to connect.  Network error or invalid configuration.  See Log for details.") ;
        return refreshtokenandusername ;
    }

    // Prompt the user to authenticate, using the code

    // TODO: Check errstring / resultstring and report
    // particularly if SSL DLLs aren't working

    QMessageBox mb ;
    mb.setTextFormat(Qt::RichText) ;
    mb.setTextInteractionFlags(Qt::TextBrowserInteraction) ;
    QString str = QString("<p>1. Connect to <a href=\"") + verification_url + QString("\"><font size=\"+1\">") + verification_url + QString("</font></p>") +
                  QString("<p>2. Sign-in, and Enter the following code when prompted</p>") +
                  QString("<p align=\"center\"><font size=\"+2\" color=\"blue\"><b>") + user_code + QString("</b></font></p>") +
                  QString("<p>3. Select <i>Allow</i> to enable Contact Manager to access your contacts / calendars</p>") +
                  QString("<p>4. And then press OK <i>below</i> to continue when complete</p>") ;
    mb.setText(str) ;
    if (!mb.exec()) {
        return refreshtokenandusername ;
    }

    // TODO: UP TO HERE IN FLOW

    // Get the refresh and access tokens
    {
      QNetworkReply *reply ;
      QEventLoop eventLoop ;
      QNetworkAccessManager manager ;
      QUrlQuery params ;
      QUrl url("https://www.googleapis.com/oauth2/v4/token") ;
      QNetworkRequest request(url) ;
      params.addQueryItem("client_id", CLIENTID);
      params.addQueryItem("client_secret", SECRET);
      params.addQueryItem("code", device_code);
      params.addQueryItem("grant_type", "http://oauth.net/grant_type/device/1.0");
      request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
      QObject::connect(&manager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
      reply = manager.post(request, params.query(QUrl::FullyEncoded).toUtf8());
      eventLoop.exec() ;
      resultstring = reply->readAll() ;

      refreshtoken = ExtractParameter(resultstring, "refresh_token") ;
      accesstoken = ExtractParameter(resultstring, "access_token") ;

    }

    // Above should have been polled ....


    // Get the username (i.e. the login email address)
    {
        QString result ;
        result = googleGet("https://www.googleapis.com/oauth2/v3/userinfo") ;
        username = ExtractParameter(result, "email") ;
    }

    if (username.isEmpty()) {
        addLog("Unable to authorise with Google.  Network error or missing ssleay32.dll and libeay32.dll") ;
    }

    if (!logfile.isEmpty())
        writeToFile(logfile, QString("authentication:\n") + resultstring + QString("\nemail: ") + username) ;

    refreshtokenandusername = refreshtoken + " " + username ;
    return refreshtokenandusername ;
}

QString GoogleConnection::getUsername()
{
    return username ;
}

//=====================================================================================================
//
// Private: GoogleConnection - googleGetAccessToken
//
//  Updates the access token, based on the authorisation token
//

void GoogleConnection::getAccessToken()
{
    QNetworkReply *reply ;
    QEventLoop eventLoop ;
    QNetworkAccessManager manager ;
    QUrlQuery params ;

    accesstoken="" ;

    if (refreshtoken.isEmpty()) {
        errorstatus="Google account not set up in File/Setup (invalid refresh token)" ;
        return ;
    }

    QUrl url("https://accounts.google.com/o/oauth2/token") ;
    QNetworkRequest request(url) ;
    params.addQueryItem("client_id", CLIENTID);
    params.addQueryItem("client_secret", SECRET);
    params.addQueryItem("refresh_token", refreshtoken);
    params.addQueryItem("grant_type", "refresh_token");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QObject::connect(&manager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
    reply = manager.post(request, params.query(QUrl::FullyEncoded).toUtf8());
    eventLoop.exec() ;

    switch (reply->error()) {
    case QNetworkReply::ConnectionRefusedError:
        errorstatus="Connection Refused" ;
        break ;
    case QNetworkReply::RemoteHostClosedError:
        errorstatus="Remote Host Closed Connection" ;
        break ;
    case QNetworkReply::HostNotFoundError:
        errorstatus="Host accounts.google.com Not Found" ;
        break ;
    case QNetworkReply::UnknownServerError:
        errorstatus="Unknown Server Error" ;
        break ;
    default:
        QVariant replycode=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) ;
        if (replycode>=200 && replycode<=299) {
            QString resultstring = reply->readAll() ;
            accesstoken = ExtractParameter(resultstring, "access_token") ;
            if (accesstoken.isEmpty()) {
                errorstatus="Google access token not found." ;
            }
        } else {
            errorstatus="Error " + replycode.toString() ;
        }
        break ;
    }


}

//=====================================================================================================
//
// Private: GoogleConnection - ExtractParameter
//
// Parse the supplied response, and extract the JSON parameter identified
//

QString GoogleConnection::ExtractParameter(QString Response, QString Parameter, int Occurrence)
{
    QRegExp rx ;
    QStringList records;
    QString record ;
    QString pattern ;
    QString extracttokenresult ;

    extracttokenresult="" ;
    if (Response.isEmpty()) return extracttokenresult ;

    // Remove \n and
    // Extract the Occurrenceth set of {}
    records = Response.replace("\n","").split("{") ;
    int numrecords = records.size() ;
    if (Occurrence>=(numrecords) || Occurrence<1) return extracttokenresult ;
    record=records[Occurrence] ;

    // Find "parameter" : "xxxx",
    // Or "parameter" : "xxxx"}
    pattern = "\"" + Parameter + "\" *: *\"(.*)\"" ;
    rx.setPattern(pattern) ;
    rx.setMinimal(true) ;
    if (rx.indexIn(record)>=0) extracttokenresult = rx.cap(1) ;
    return extracttokenresult ;
}

