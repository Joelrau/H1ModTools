#include "H1ModTools.h"
#include "COD4ModTools.h"

#include <QApplication>

#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int argc = 0;
    char** argv = nullptr;

    QApplication a(argc, argv);

    QCoreApplication::setOrganizationDomain("auroramod.dev");
    QCoreApplication::setOrganizationName("Aurora");
    QCoreApplication::setApplicationName("H1ModTools");

    H1ModTools w;
    w.show();

    return a.exec();
}