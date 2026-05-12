#include "MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("StrobeCam");
    app.setOrganizationName("BYU FLOW Lab");

    MainWindow window;
    window.show();

    return app.exec();
}