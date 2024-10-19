#include "mainwindow.h"
#include <QApplication>
#include <iostream>
#include "../Lib/supportfunctions.h"

int main(int argc, char *argv[])
{
        QApplication a(argc, argv);

        a.setApplicationName("Google Fetch") ;
		a.setApplicationVersion(GITHASH) ;

        MainWindow w;
        w.show();

        return a.exec();
}
