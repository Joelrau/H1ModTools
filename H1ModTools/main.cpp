#include "H1ModTools.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    H1ModTools w;
    w.show();
    return a.exec();
}
