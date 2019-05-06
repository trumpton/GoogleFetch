#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include "googleconnection.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_action_Register_triggered();

    void on_pushButton_clicked();

    void on_action_About_triggered();

private:
    QString xmlBeautifier(QString src) ;
    QString jsonBeautifier(QString src) ;

    Ui::MainWindow *ui;
    GoogleConnection gc ;
    QString refreshtoken ;
    QString username ;
    QSettings *ini ;
};

#endif // MAINWINDOW_H
