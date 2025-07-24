#pragma once

#include <QtWidgets/QtWidgets>

#include "Shared.h"

#include "Utils/QTUtils.h"

struct Globals_e
{
    QString pathH1;
    QString pathIW3;
    QString pathIW4;
    QString pathIW5;
    QString h1Executable;
};

extern Globals_e Globals;

extern void saveGlobalsToJson(QWidget* parent);
extern bool loadGlobalsFromJson();