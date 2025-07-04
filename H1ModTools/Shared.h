#pragma once

#include "Globals.h"

enum GameType {
    H1,
    IW3,
    IW4,
    IW5
};

namespace Funcs
{
    namespace Shared
    {
        QString getGamePath(GameType gameType);

        bool isMapLoad(const QString& name);
        bool isMap(const QString& name, GameType gameType);
        bool isMpMap(const QString& name, GameType gameType);
    }
}