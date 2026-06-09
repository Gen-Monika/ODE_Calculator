#include "MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("ODE Calculator");
    QApplication::setOrganizationName("SWUST");

    MainWindow window;
    window.show();

    return app.exec();
}
