#include "H1ModTools.h"
#include "COD4ModTools.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationDomain("auroramod.dev");
    QCoreApplication::setOrganizationName("Aurora");
    QCoreApplication::setApplicationName("H1ModTools");

    H1ModTools w;
    w.show();
    return a.exec();
}
