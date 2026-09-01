#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    // Initialize the Qt environment and event loop
    QApplication a(argc, argv);
    
    // Instantiate and display your main window
    MainWindow w;
    w.show();
    
    // Hand control over to Qt's event loop
    return a.exec();
}