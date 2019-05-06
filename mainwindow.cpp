#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QJsonDocument>
#include <QDomDocument>
#include "version.h"
#include "../Lib/supportfunctions.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ini = new QSettings("trumpton.uk", "GoogleFetcher");
    ui->setupUi(this);
    refreshtoken = ini->value("refreshtoken").toString() ;
    username = ini->value("username").toString() ;

    if (username.isEmpty())
       ui->labelUsername->setText("Not Registered");
    else
       ui->labelUsername->setText(username) ;
       gc.setupRToken(refreshtoken);

    ui->comboBox->addItem("GET www.google.com/m8/feeds/contacts/default/full", "get://www.google.com/m8/feeds/contacts/default/full?v=3.0");
    ui->comboBox->addItem("GET www.google.com/m8/feeds/contacts/default/full/{contactId}", "get://www.google.com/m8/feeds/contacts/default/full/{}?v=3.0") ;
    ui->comboBox->addItem("GET www.google.com/m8/feeds/groups/default/full", "get://www.google.com/m8/feeds/groups/default/full?v=3.0") ;
    ui->comboBox->addItem("GET www.google.com/m8/feeds/{userEmail}/full/{groupId}", "get://www.google.com/m8/feeds/{userEmail}/full/{}?v=3.0") ;
    ui->comboBox->addItem("GET www.googleapis.com/calendar/v3", "get://www.googleapis.com/calendar/v3") ;
    ui->comboBox->addItem("POST www.google.com/m8/feeds/contacts/default/full", "post://www.google.com/m8/feeds/contacts/default/full") ;
    ui->comboBox->addItem("PUT www.google.com/m8/feeds/contacts/default/full/{contactId}", "put://www.google.com/m8/feeds/contacts/default/full/{}") ;
    ui->comboBox->addItem("GET https://people.googleapis.com/v1/people/me/connections?personFields=all", "get://people.googleapis.com/v1/people/me/connections?personFields=addresses,ageRanges,biographies,birthdays,braggingRights,coverPhotos,emailAddresses,events,genders,imClients,interests,locales,memberships,metadata,names,nicknames,occupations,organizations,phoneNumbers,photos,relations,relationshipInterests,relationshipStatuses,residences,sipAddresses,skills,taglines,urls,userDefined") ;
    ui->comboBox->addItem("POST https://people.googleapis.com/v1/people:createContact", "post://people.googleapis.com/v1/people:createContact") ;
    ui->comboBox->addItem("POST https://people.googleapis.com/v1/{personId}:updateContact", "post://people.googleapis.com/v1/{}:updateContact") ;
    ui->comboBox->addItem("GET https://people.googleapis.com/v1/contactGroups", "get://people.googleapis.com/v1/contactGroups") ;
}

MainWindow::~MainWindow()
{
    delete ini ;
    delete ui;
}

void MainWindow::on_action_Register_triggered()
{
    refreshtoken = gc.Authorise() ;
    username = gc.getUsername() ;
    ini->setValue("refreshtoken", refreshtoken) ;
    ini->setValue("username", username) ;
    if (username.isEmpty())
        ui->labelUsername->setText("Not Registered");
    else
        ui->labelUsername->setText(username) ;
}

void MainWindow::on_pushButton_clicked()
{
    bool asjson=false ;
    QString url ;
    bool get ;
    bool put ;
    QString response ;

    ui->plainTextOutput->clear() ;

    asjson = ui->checkBox->isChecked() ;

    url = ui->comboBox->currentData().toString() ;
    url = url.replace("{}", ui->lineEdit->text()) ;
    if (asjson) url = url + QString("&alt=json") ;

    if (url.contains(QString("get://"))) {
        get=true ;
        put=false ;
        url = url.replace("get://", "https://") ;
    } else if (url.contains(QString("put://"))) {
        get=false ;
        put=true ;
        url = url.replace("put://", "https://") ;
    } else {
        get=false ;
        put=false ;
        url = url.replace("post://", "https://") ;
    }

    if (get) {
        response = gc.googleGet(url) ;
    } else if (put){
        response = gc.googlePutPostDelete(url, GoogleConnection::Put, ui->plainTextInput->toPlainText(), asjson, true) ;
    } else {
        response = gc.googlePutPostDelete(url, GoogleConnection::Post, ui->plainTextInput->toPlainText(), asjson, true) ;
    }

    if (!gc.getNetworkError().isEmpty()) {
        ui->plainTextOutput->setPlainText(gc.getNetworkError());
    } else if (asjson) {
        ui->plainTextOutput->setPlainText(jsonBeautifier(response));
    } else {
        ui->plainTextOutput->setPlainText(xmlBeautifier(response));
    }
}


QString MainWindow::jsonBeautifier(QString src)
{
    QJsonDocument jdoc ;
    jdoc = jdoc.fromJson(QByteArray(src.toLatin1())) ;
    return QString(jdoc.toJson(QJsonDocument::Indented)) ;
}


QString MainWindow::xmlBeautifier(QString src)
{
    QDomDocument xdoc ;
    xdoc.setContent(src) ;
    return xdoc.toString(4) ;
}

void MainWindow::on_action_About_triggered()
{
    QString text =
    QString("GoogleFetch Release %1.\n").arg(BUILDVERSION) +
    QString("It was built on: %1.\n").arg(buildDate()) +
    QString("\GoogleFetch Repository Version: %1.\n").arg(appHash()) +
    QString("Library Repository Version: %1.\n").arg(libVersion()) ;

    warningOkDialog(this, QString("About GoogleFetch"), text)  ;
}
