
#include "googleconnection.h"

#include "../Lib/supportfunctions.h"
#include "../Lib/amessagebox.h"
#include "../Lib/dialogfocus.h"

#include <QNetworkReply>
#include <QUrl>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QUrlQuery>
#include <QSslSocket>
#include <QThread>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// The scopes asked for, the same as ContactManager asks for them (googleaccess_oauth2.cpp)
static const char *GOOGLE_SCOPE = "https://www.googleapis.com/auth/calendar https://www.googleapis.com/auth/contacts https://www.google.com/m8/feeds/ profile email" ;


//=====================================================================================================
//
// Helper functions for the responses (copied from ContactManager's googleaccess_network.cpp)
//

// Google's reason for a failed request, and its message, from the error body it sends
static void googleReason(const QByteArray &body, QString &reason, QString &message)
{
    reason.clear() ;
    message.clear() ;
    QJsonObject error = QJsonDocument::fromJson(body).object().value("error").toObject() ;
    QJsonArray errors = error.value("errors").toArray() ;
    if (!errors.isEmpty()) {
        reason = errors.at(0).toObject().value("reason").toString() ;
        message = errors.at(0).toObject().value("message").toString() ;
    }
    if (message.isEmpty()) message = error.value("message").toString() ;
}

// Only these are retried: Google says the limit will clear.  Authentication, permission, not-found and mismatch
// errors are not.
static bool isRateLimit(int status, const QString &reason)
{
    return status==429 || (status==403 && (reason=="rateLimitExceeded" || reason=="userRateLimitExceeded")) ;
}

// Google's server errors (5xx) are retried in the same way as a rate limit: they usually clear
static bool isServerError(int status)
{
    return status>=500 && status<=599 ;
}

// Errors that are retried: a rate limit, or a server error
static bool isRetryable(int status, const QString &reason)
{
    return isRateLimit(status, reason) || isServerError(status) ;
}

// The wait before retry number attempt (1 to 4): 500 ms, then 1 s, 2 s and 4 s, with a random fraction added, up to 4 s
static int rateLimitWait(int attempt)
{
    int base = 500 << (attempt-1) ;
    int wait = base + QRandomGenerator::global()->bounded(base/2 + 1) ;
    return qMin(wait, 4000) ;
}

// The status and reason, as the report gives them
static QString failureText(int status, const QString &reason, const QString &message)
{
    QString text = QString("Google answered HTTP %1").arg(status) ;
    if (!reason.isEmpty()) text += QString(" %1").arg(reason) ;
    if (!message.isEmpty()) text += QString(": %1").arg(message) ;
    return text ;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Constructors
//

GoogleConnection::GoogleConnection()
{
    refreshtoken="" ;
    errorstatus="" ;
    accesstoken="" ;
    username="" ;
    refreshtokenandusername = "" ;
    logsequencenumber = 1 ;
    connectionerror = false ;
    errorcode = 0 ;
}

GoogleConnection::GoogleConnection(const QString& rt)
{
    refreshtoken="" ;
    errorstatus="" ;
    accesstoken="" ;
    username="" ;
    refreshtokenandusername = "" ;
    logsequencenumber = 1 ;
    connectionerror = false ;
    errorcode = 0 ;
    setupRToken(rt);
}


//=====================================================================================================
//
// Public: setClient
//
// Sets the Google OAuth client ID and secret.  They come from the ini file, and are sent with every request.
//
void GoogleConnection::setClient(const QString &id, const QString &secret)
{
    clientid = id ;
    clientsecret = secret ;
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
// Perform a Put/Post/Patch or Delete Operation, and return the corresponding message
//
QString GoogleConnection::googlePutPostDelete(QString link, enum GoogleConnection::Action action, QString data, bool isjson, bool includeheaderinresponse, QString logfile)
{
    googlePutPostResponse="" ;
    errorcode=0 ;
    errorstatus="" ;
    errorrecovery="" ;

    // The pacing set after Google's rate limits, before each upload
    int pacingdelay = pacer.delayBeforeNext() ;
    if (pacingdelay>0) QThread::msleep(pacingdelay) ;

    bool hadratelimit=false ;
    int ratelimitretries=0 ;
    int ratelimitwait=0 ;
    int retries=1 ;
    bool complete=false ;
    bool readsuccess=false ;
    bool timeoutretry=false ;
    QString header ;
    QString QueryChar="?" ;
    if (link.contains("?")) QueryChar="&" ;

    if (!logfile.isEmpty()) writeToFile(logfile, "") ;

    do {
        timeoutretry=false ;

        if (accesstoken.isEmpty()) {

            readsuccess=false ;
            googlePutPostResponse = "No Access Token" ;

        } else {

            QNetworkReply *reply = NULL ;
            QUrl url(link + QueryChar + "access_token=" + accesstoken) ;
            QNetworkRequest request(url) ;
            QEventLoop eventLoop ;

            // get the page
            QObject::connect(&netmanager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
            QByteArray submitdata = data.toUtf8() ;

            if (isjson) {
                request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                header = "Content-Type: application/json\n" ;
            } else {
                request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
                header = "Content-Type: application/xml\n" ;
            }

            request.setHeader(QNetworkRequest::ContentLengthHeader, submitdata.length());
            header = header + "Content-Length: " + QString::number(submitdata.length()) + "\n" ;

            request.setRawHeader("charset", "UTF-8") ;
            header = header + "charset: UTF-8\n" ;

            QString auth = QString("Bearer ") + accesstoken ;
            request.setRawHeader("Authorization", auth.toLatin1()) ;
            header = header + "Authorization: Bearer " + accesstoken + "\n" ;

            // Any version of the item may be changed (the ETag is not used here)
            request.setRawHeader("If-Match", "*") ;
            header = header + "If-Match: *\n" ;

            QString actionname ;
            switch (action) {
            case GoogleConnection::Post:
                actionname="POST" ;
                break ;
            case GoogleConnection::Put:
                actionname="PUT" ;
                break ;
            case GoogleConnection::Delete:
                actionname="DELETE" ;
                break ;
            case GoogleConnection::Patch:
                actionname="PATCH" ;
                break ;
            }

            if (!logfile.isEmpty())
                writeToFile(logfile,
                            QString("URL: ") + link + QueryChar + "access_token=" + accesstoken +
                            QString("\n\nHEADER >>\n") + header +
                            QString("\n\n") + actionname + QString(">>\n") + data +
                            QString("\n\n"), true) ;

            switch (action) {
            case GoogleConnection::Post:
                reply = netmanager.post(request, submitdata) ;
                break ;
            case GoogleConnection::Put:
                reply = netmanager.put(request, submitdata) ;
                break ;
            case GoogleConnection::Delete:
                // From PHP - $req->setRequestHeaders(array('content-type' => 'application/atom+xml; charset=UTF-8; type=feed'));
                // From stackexchange: "GData-Version": "3.0", "Authorization":"Bearer " + token.accesstoken, "if-match":"*"
                reply = netmanager.deleteResource(request) ;
                break ;
            case GoogleConnection::Patch:
                reply = netmanager.sendCustomRequest(request, "PATCH", submitdata) ;
                break ;
            }

            eventLoop.exec() ;
            QVariant replycode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) ;
            errorcode = replycode.toInt() ;
            int status = replycode.toInt() ;

            // The body is read once, for the result and the reason Google gives
            QByteArray body = reply->readAll() ;
            QString reason, message ;
            googleReason(body, reason, message) ;

            switch (reply->error()) {
            case QNetworkReply::ConnectionRefusedError:
                connectionerror = true ;
                errorstatus="Connection Refused." ;
                errorrecovery="Could not reach Google. Sync again." ;
                break ;
            case QNetworkReply::RemoteHostClosedError:
                connectionerror = true ;
                errorstatus="Connection closed by Google (Remote Host Closed Connection)." ;
                errorrecovery="Google closed the connection. Sync again." ;
                break ;
            case QNetworkReply::HostNotFoundError:
                connectionerror = true ;
                errorstatus="Host accounts.google.com Not Found." ;
                errorrecovery="Could not find Google. Check the connection, then sync again." ;
                break ;
            case QNetworkReply::UnknownServerError:
                connectionerror = true ;
                errorstatus="Unknown Server Error." ;
                errorrecovery="Google reported a server error. Sync again." ;
                break ;
            default:

                connectionerror = false ;
                if (status>=200 && status<=299) {
                  errorstatus = "" ;
                  googlePutPostResponse = body ;
                  readsuccess=true ;

                } else if (status==412) {
                  // Precondition Failed: Google's copy has changed since the ETag sent.  It is not retried here
                  errorstatus = "Network Error 412" ;
                  googlePutPostResponse = body ;
                  readsuccess=true ;

                } else if (isRetryable(status, reason) && ratelimitretries<4) {
                  // Google's rate limit: wait, then ask again.  Four retries, then the request is a failure
                  ratelimitretries++ ;
                  ratelimitwait = rateLimitWait(ratelimitretries) ;
                  hadratelimit = isRateLimit(status, reason) ;
                  timeoutretry = true ;

                } else if (isRetryable(status, reason)) {
                  googlePutPostResponse = body ;
                  hadratelimit = isRateLimit(status, reason) ;
                  errorstatus = failureText(status, reason, message) + " (still failing after 4 retries)" ;
                  errorrecovery = "Google did not clear the error after four retries. Wait five minutes, then sync again." ;
                  readsuccess=true ;

                } else if (status==0) {
                  // No answer from Google at all
                  errorstatus = "Network Error " + reply->errorString() ;
                  errorrecovery = "Could not reach Google. Sync again." ;
                  connectionerror = true ;
                  readsuccess=false ;

                } else if (status==401) {
                  // Not signed in: the sign-in is refreshed below
                  errorstatus = failureText(status, reason, message) ;
                  errorrecovery = "Sign in to Google again, then sync." ;
                  readsuccess=false ;

                } else {
                  // Anything else is not retried.  It is reported with Google's reason
                  googlePutPostResponse = body ;
                  errorstatus = failureText(status, reason, message) ;
                  errorrecovery = "Correct the item named above, then sync again." ;
                  readsuccess=true ;
                }
                break ;
            }
            // The reply has been read, so free it (the manager is shared)
            if (reply) reply->deleteLater() ;
        }

        if (timeoutretry==true) {
            // A rate limit: wait, then try again
            QThread::msleep(ratelimitwait) ;
            complete=false ;

        } else if (readsuccess || retries==0) {
            complete=true ;

        } else {
            // If there is an error refresh the access token and retry once
            getAccessToken() ;
            retries-- ;
        }

        if (!logfile.isEmpty())
            writeToFile(logfile,
                        QString("RESPONSE>>\n") + googlePutPostResponse +
                        QString("\n\nSTATUS>>\n") + QString::number(errorcode) + QString(" ") + errorstatus +
                        QString("\n\n--------------------------\n\n"), true) ;

    } while (!complete) ;

    // The pacing counts each transaction, and whether it needed a retry
    pacer.finished(hadratelimit) ;
    if (!errorstatus.isEmpty()) {
        lastfailure = errorstatus ;
        lastrecovery = errorrecovery ;
    }

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
    bool complete=false ;
    bool readsuccess=false ;
    QString logseq = QString("%1").arg(logsequencenumber++,4,10,QChar('0')) ;

    errorcode=0 ;
    errorstatus="" ;
    errorrecovery="" ;
    googleGetResponse="" ;

    // Rate limits are retried as for writes
    int ratelimitretries=0 ;
    int ratelimitwait=0 ;

    if (!logfile.isEmpty()) writeToFile(logfile, "") ;

    do {
        if (accesstoken.isEmpty()) {

            googleGetResponse = "" ;
            readsuccess=false ;

        } else {

            QString QueryChar="?" ;
            if (link.contains("?")) QueryChar="&" ;
            QNetworkReply *reply = NULL ;
            QUrl url(link + QueryChar + "access_token=" + accesstoken) ;
            QNetworkRequest request(url) ;
            QEventLoop eventLoop ;

            if (!logfile.isEmpty())
                writeToFile(logfile,
                            QString("URL: ") + link + QueryChar + "access_token=" + accesstoken +
                            QString("\n\nGET>>") +
                            QString("\n\n"), true) ;

            // get the page
            QObject::connect(&netmanager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
            reply = netmanager.get(request) ;
            eventLoop.exec() ;
            QVariant replycode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) ;
            errorcode = replycode.toInt() ;
            int status = replycode.toInt() ;

            // The body gives Google's reason for a failure
            QByteArray body = reply->readAll() ;
            QString reason, message ;
            googleReason(body, reason, message) ;

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
                if (status>=200 && status<=299) {
                  googleGetResponse = body ;
                  errorstatus="" ;
                  readsuccess=true ;

                } else if (isRetryable(status, reason) && ratelimitretries<4) {
                  // Google's rate limit: wait, then ask again
                  ratelimitretries++ ;
                  ratelimitwait = rateLimitWait(ratelimitretries) ;
                  errorstatus = failureText(status, reason, message) ;
                  readsuccess=false ;

                } else if (isRetryable(status, reason)) {
                  googleGetResponse = "" ;
                  errorstatus = failureText(status, reason, message) + " (still failing after 4 retries)" ;
                  errorrecovery = "Google did not clear the error after four retries. Wait five minutes, then sync again." ;
                  readsuccess=true ;

                } else if (status==401) {
                  // Not signed in: the sign-in is refreshed once below
                  googleGetResponse = "" ;
                  errorstatus = failureText(status, reason, message) ;
                  errorrecovery = "Sign in to Google again, then sync." ;
                  readsuccess=false ;

                } else if (status==0) {
                  // No answer from Google: tried once more
                  googleGetResponse = "" ;
                  errorstatus = "Network Error " + reply->errorString() ;
                  errorrecovery = "Could not reach Google. Sync again." ;
                  readsuccess=false ;

                } else {
                  // Anything else is not retried: it is reported with Google's reason
                  googleGetResponse = body ;
                  errorstatus = failureText(status, reason, message) ;
                  errorrecovery = "Sync again once the cause in the report is put right." ;
                  readsuccess=true ;
                }
                break ;
            }
            // The reply has been read, so free it (the manager is shared)
            if (reply) reply->deleteLater() ;
        }

        // Add the log file name to the error status
        if (!errorstatus.isEmpty() && !logfile.isEmpty()) {
            errorstatus = logseq + QString(": ") + errorstatus ;
        }

        // A rate limit is waited out before the request is sent again.  Otherwise an error refreshes the access token
        // and retries once
        if (ratelimitwait>0) {
            QThread::msleep(ratelimitwait) ;
            ratelimitwait=0 ;
            complete=false ;
        } else if (readsuccess || retries==0) {
            complete=true ;
        } else {
            getAccessToken() ;
            retries-- ;
        }

        if (!logfile.isEmpty())
            writeToFile(logfile,
                        QString("RESPONSE>>\n") + googleGetResponse +
                        QString("\n\nSTATUS>>\n") + QString::number(errorcode) + QString(" ") + errorstatus +
                        QString("\n\n--------------------------\n\n"), true) ;

    } while (!complete) ;

    if (!errorstatus.isEmpty()) {
        lastfailure = errorstatus ;
        lastrecovery = errorrecovery ;
    }
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


//=====================================================================================================
//
// Public - GoogleConnection - setupRToken
//
// Sets the refresh token, and the username, which were generated with Authorise and saved
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
    QString errstring ;

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
      params.addQueryItem("client_id", clientid);
      params.addQueryItem("scope", GOOGLE_SCOPE);
      request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
      QObject::connect(&manager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
      reply = manager.post(request, params.query(QUrl::FullyEncoded).toUtf8());

      eventLoop.exec() ;

      errstring = reply->errorString() ;
      resultstring = reply->readAll() ;
      device_code = ExtractParameter(resultstring, "device_code") ;
      user_code = ExtractParameter(resultstring, "user_code") ;
      verification_url = ExtractParameter(resultstring, "verification_url") ;
      expires_in = ExtractParameter(resultstring, "expires_in") ;
      interval = ExtractParameter(resultstring, "interval") ;
    }

    if (user_code.isEmpty()) {
        QString errmsg = ExtractParameter(resultstring, "error_description") ;
        QString detail = errmsg.isEmpty() ? errstring : errmsg ;
        QString tls = QSslSocket::supportsSsl() ? QString("TLS is available (%1)").arg(QSslSocket::sslLibraryVersionString()) : QString("TLS is not available") ;
        addLog(QString("GoogleConnection::Authorise: Unable to authorise with Google.  The reply was: ") + detail + ". " + tls + ". Check the network connection, and the clientid and secret in the [google] section of the ini file.") ;
        errorOkDialog(NULL, "GoogleFetch Error", QString( "Unable to connect. : ") + detail) ;
        return refreshtokenandusername ;
    }

    // Prompt the user to authenticate, using the code

    AMessageBox mb ;
    mb.setTextFormat(Qt::RichText) ;
    mb.setTextInteractionFlags(Qt::TextBrowserInteraction) ;
    QString str =
                  QString("<p>1. Copy the following code to the clipboard:</p>") +
                  QString("<p align=\"center\"><font size=\"+2\" color=\"blue\"><b>") + user_code + QString("</b></font></p>") +
                  QString("<p>2. Connect to <a href=\"") + verification_url + QString("\"><font size=\"+1\">") + verification_url + QString("</font></p>") +
                  QString("<p>3. Paste the code when prompted</p>") +
                  QString("<p>4. Select a Google account / login</p>") +
                  QString("<p>5. Select <i>Allow</i> to enable GoogleFetch to access your contacts / calendars</p>") +
                  QString("<p>6. Acknowledge and authorise the application</p>") +
                  QString("<p>7. And then press OK <i>below</i> to continue when complete</p>") ;
    mb.setText(str) ;
    if (!execFocused(&mb)) {
        return refreshtokenandusername ;
    }

    // Get the refresh and access tokens
    {
      QNetworkReply *reply ;
      QEventLoop eventLoop ;
      QNetworkAccessManager manager ;
      QUrlQuery params ;
      QUrl url("https://www.googleapis.com/oauth2/v4/token") ;
      QNetworkRequest request(url) ;
      params.addQueryItem("client_id", clientid);
      params.addQueryItem("client_secret", clientsecret);
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

    // Get the username (i.e. the login email address)
    {
        QString result ;
        result = googleGet("https://www.googleapis.com/oauth2/v3/userinfo") ;
        username = ExtractParameter(result, "email") ;
    }

    if (username.isEmpty()) {
        addLog("GoogleConnection::Authorise: Unable to authorise with Google.  Network error, or TLS is not available") ;
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
// Private: GoogleConnection - getAccessToken
//
//  Updates the access token, based on the refresh token
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

    if (clientid.isEmpty() || clientsecret.isEmpty()) {
        errorstatus="The Google client ID and secret are not set: give clientid and secret in the [google] section of googlefetch.ini or ContactManager.ini" ;
        return ;
    }

    QUrl url("https://accounts.google.com/o/oauth2/token") ;
    QNetworkRequest request(url) ;
    params.addQueryItem("client_id", clientid);
    params.addQueryItem("client_secret", clientsecret);
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
        if (replycode.toInt()>=200 && replycode.toInt()<=299) {
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
    QRegularExpression rx ;
    QRegularExpressionMatch rem ;
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
    pattern = "\"" + QRegularExpression::escape(Parameter) + "\" *: *\"(.*?)\"" ;
    rx.setPattern(pattern) ;
    rem = rx.match(Response) ;
    if (rem.hasMatch()) extracttokenresult = rem.captured(1) ;
    return extracttokenresult ;
}
