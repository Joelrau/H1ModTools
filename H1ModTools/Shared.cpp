#include "Shared.h"

namespace Funcs
{
    namespace Shared
    {
        QString getGamePath(GameType gameType)
        {
            switch (gameType) {
            case GameType::IW3: return Globals.pathIW3;
            case GameType::IW4: return Globals.pathIW4;
            case GameType::IW5: return Globals.pathIW5;
            case GameType::H1: return Globals.pathH1;
            default:
                __debugbreak();
                return "";
            }
        };

        bool isMapLoad(const QString& name)
        {
            return name.endsWith("_load");
        }

        bool isMap(const QString& name, GameType gameType)
        {
            const QDir baseDir = QDir(getGamePath(gameType)).filePath("zonetool/" + name);
            if (!baseDir.exists())
                return false;

            const QString spPath = baseDir.filePath(QStringLiteral("maps/%1.d3dbsp.ents").arg(name));
            const QString mpPath = baseDir.filePath(QStringLiteral("maps/mp/%1.d3dbsp.ents").arg(name));

            return QFileInfo::exists(spPath) || QFileInfo::exists(mpPath);
        }

        bool isMpMap(const QString& name, GameType gameType)
        {
            const QDir baseDir = QDir(getGamePath(gameType)).filePath("zonetool/" + name);
            if (!baseDir.exists())
                return false;

            const QString mpPath = baseDir.filePath(QStringLiteral("maps/mp/%1.d3dbsp.ents").arg(name));
            return QFileInfo::exists(mpPath);
        }
    }
}