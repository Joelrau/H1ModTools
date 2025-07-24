#pragma once

#include <QtWidgets/QtWidgets>

#include "../Shared.h"

void generateCSV(const QString& zone, const QString& destFolder, const bool isMpMap, GameType sourceGameType, GameType targetGameType);