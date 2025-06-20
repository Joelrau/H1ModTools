#pragma once

#include <QStyleFactory>
#include <QString>
#include <QProcess>
#include <QObject>
#include <QTextEdit>
#include <QSocketNotifier>
#include <QStorageInfo>
#include <QDir>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QListWidget>
#include <QTreeWidget>
#include <QMenu>
#include <QDesktopServices>
#include <QProgressDialog>

#include "QTUtils.h"

struct Globals_e
{
    QString pathH1;
    QString pathIW3;
    QString pathIW4;
    QString pathIW5;
    QString h1_mod_exe;
};

extern Globals_e Globals;

extern void saveGlobalsToJson(QWidget* parent);
extern bool loadGlobalsFromJson();