#include "CSVGenerator.h"
#include "MapEnts.h"
#include "CSV.h"

QSet<QString> parseCreateFxGsc(const QString& data)
{
    QSet<QString> sounds;

    QRegularExpression regex("\\w+\\.v\\[\\s*\"soundalias\"\\s*\\]\\s*=\\s*\"([\\w\\s]+?)\"",
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatchIterator it = regex.globalMatch(data);
    while (it.hasNext())
    {
        QRegularExpressionMatch match = it.next();
        sounds.insert(match.captured(1));
    }

    return sounds;
}

QSet<QString> parseFxGsc(const QString& data)
{
    QSet<QString> effects;

    QRegularExpression regex("loadfx\\s*\\(\\s*\"([^\"]+)\"\\s*\\);",
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatchIterator it = regex.globalMatch(data);
    while (it.hasNext())
    {
        QRegularExpressionMatch match = it.next();
        effects.insert(match.captured(1));
    }

    return effects;
}

QMap<QString, QString> parseAnimatedModelScript(const QString& scriptText)
{
    QMap<QString, QString> result;
    QMap<QString, QString> variableMap;

    // Match: variableName = "modelName";
    QRegularExpression varAssignRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*=\\s*\"([^\"]+)\"");
    QRegularExpressionMatchIterator varIter = varAssignRegex.globalMatch(scriptText);
    while (varIter.hasNext())
    {
        QRegularExpressionMatch match = varIter.next();
        variableMap.insert(match.captured(1), match.captured(2));
    }

    // Match: level.anim_prop_models[variableName]["..."] = "animationName";
    QRegularExpression animPropUseRegex("level\\.anim_prop_models\\[\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\]\\s*\\[\\s*\"[^\"]+\"\\s*\\]\\s*=\\s*\"([^\"]+)\"");
    QRegularExpressionMatchIterator useIter = animPropUseRegex.globalMatch(scriptText);
    while (useIter.hasNext())
    {
        QRegularExpressionMatch match = useIter.next();
        QString varName = match.captured(1);
        QString variation = match.captured(2);

        if (variableMap.contains(varName))
        {
            QString modelName = variableMap.value(varName);
            result.insert(modelName, variation);
        }
    }

    return result;
}

void generateCSV(const QString& zone, const QString& destFolder, const bool isMpMap, GameType sourceGameType, GameType targetGameType)
{
    if (!Funcs::Shared::isMap(zone, targetGameType) && !Funcs::Shared::isMapLoad(zone)) {
        qInfo() << "Could not generate csv for zone" << zone << "since it's not map/map load";
        return;
    }

    const auto rootDir = destFolder;

    CSV csv{};
    const auto save = [&]()
    {
        const auto csvFilePath = Funcs::Shared::getGamePath(targetGameType) + "/zone_source/" + zone + ".csv";
		csv.writeFile(csvFilePath);
    };

    const QString mapPrefix = isMpMap
        ? "maps/mp"
        : "maps";
    const QString mapPrefixFull = destFolder + "/" + mapPrefix + "/";
    const QString mapentsPath = mapPrefixFull + zone + ".d3dbsp.ents";

    const auto addRow = [&](const CSV::Row row)
    {
		csv.addRow(row);
    };

    const auto addComment = [&](const QString& str)
    {
        addRow({"// " + str});
	};

    const auto addEmptyLine = [&]()
    {
        addRow({});
	};

    const auto addAsset = [&](const QString& type, const QString& name)
    {
        qInfo() << "Adding" << type.toUtf8().data() << name.toUtf8().data();
        addRow({type, name});
    };

    addComment("Generated by H1ModTools");
    addEmptyLine();

    // add assets path...
    static const auto getAssetsPath = [sourceGameType]() -> QString {
        QString assetsFolder = "zonetool_assets/";
        switch (sourceGameType)
        {
        case IW3: return assetsFolder + "iw3";
        case IW4: return assetsFolder + "iw4";
        case IW5: return assetsFolder + "iw5";
        default:
            __debugbreak();
            return "";
        }
    };
    addComment("Searches for game assets in the specified paths. If the third parameter is true, assets from these paths will be prioritized.");
    addRow({ "addpaths", getAssetsPath(), "false"});
    addEmptyLine();

    if (Funcs::Shared::isMapLoad(zone)) {
        addAsset("techset", ",2d");
        addAsset("material", ",$victorybackdrop");
        addAsset("material", ",$defeatbackdrop");
        addAsset("material", "$levelbriefing");
        addAsset("material", "$levelbriefingcrossfade");

        save();
        return;
    }

    const auto& map = zone;

    const auto isMpZone = isMpMap ? true : zone.contains("_mp") || zone.contains("mp_") ? true : false;
    addComment("Ignore list: this will make zonetool not add assets that already exist in the following zones to the built zone.");
    if (isMpZone)
    {
        addRow({ "ignore", "assetlist/code_post_gfx_mp" });
        addRow({ "ignore", "assetlist/common_mp" });
        addRow({ "ignore", "assetlist/techsets_common_mp" });
        addEmptyLine();
    }
    else
    {
        addRow({ "ignore", "assetlist/code_post_gfx" });
        addRow({ "ignore", "assetlist/common" });
        addRow({ "ignore", "assetlist/techsets_common" });
        addEmptyLine();
    }

    if (isMpMap)
    {
        addComment("netconststrings");
        addAsset("netconststrings", QString("ncs_%1_level").arg("mdl"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("mat"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("rmb"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("veh"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("vfx"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("loc"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("snd"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("sbx"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("snl"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("shk"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("mnu"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("tag"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("hic"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("nps"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("mic"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("sel"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("wep"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("att"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("hnt"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("anm"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("fxt"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("acl"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("lui"));
        addAsset("netconststrings", QString("ncs_%1_level").arg("lsr"));
        addEmptyLine();
    }

    auto mapEntsRead = MapEntsReader(mapentsPath);

    auto models = mapEntsRead.getAllModels();
    if (!models.isEmpty()) {
        addComment("models");
        for (const auto& model : models) {
            addAsset("xmodel", model);
        }
        addEmptyLine();
    }

    QString createFxName = QString("maps/createfx/%1_fx.gsc").arg(map);
    QString createFxSoundsName = QString("maps/createfx/%1_sound.gsc").arg(map);

    auto addSounds = [&](const QString& file) {
        QString createFxPath = QString("%1/%2").arg(rootDir, file);

        QFile f(createFxPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        QTextStream in(&f);
        QString data = in.readAll();

        qDebug() << "Parsing createfx gsc...";

        QSet<QString> sounds = parseCreateFxGsc(data);
        if (!sounds.isEmpty()) {
            addComment("sounds");
            for (const auto& sound : sounds) {
                addAsset("sound", sound);
            }
            addEmptyLine();
        }
    };

    QString fxName = QString("%1/%2_fx.gsc").arg(mapPrefix, map);
    auto addEffects = [&]() {
        QString fxPath = QString("%1/%2").arg(rootDir, fxName);

        QFile f(fxPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        QTextStream in(&f);
        QString data = in.readAll();

        qDebug() << "Parsing fx gsc...";

        QSet<QString> effects = parseFxGsc(data);
        if (!effects.isEmpty()) {
            addComment("effects");
            for (const auto& effect : effects) {
                addAsset("fx", effect);
            }
            addEmptyLine();
        }
    };

    addSounds(createFxName);
    addSounds(createFxSoundsName);
    addEffects();

    auto addMapAsset = [&](const QString& type, const QString& ext)
    {
        QString name = QString("%1/%2.d3dbsp").arg(mapPrefix, map);

        QString path = rootDir + "/" + name + ext;
        QString pathJson = path + ".json";
        if (!QFile::exists(path) && !QFile::exists(pathJson)) {
            addAsset("#" + type, name);
        }
        else {
            addAsset(type, name);
        }
    };

    auto addBotPathsIfExists = [&]()
    {
        QString name = QString("%1/%2.csv").arg(mapPrefix, map);
        QString path = rootDir + "/" + name;
        if (QFile::exists(path))
        {
            addAsset("aipaths", name);
        }
    };

    auto addIterator = [&](const QString& type, const QString& folder,
        const QString& extension, const QString& comment, bool usePath = true)
    {
        QDir dir(rootDir + "/" + folder);
        if (!dir.exists())
            return;

        bool addedComment = false;
        QStringList files = dir.entryList(QDir::Files | QDir::NoSymLinks);
        for (const QString& file : files)
        {
            if (!file.endsWith(extension, Qt::CaseInsensitive))
                continue;

            if (!addedComment)
            {
                addedComment = true;
                addComment(comment);
            }

            if (!usePath)
            {
                QString name = file.left(file.length() - extension.length());
                addAsset(type, name);
            }
            else
            {
                QString name = folder + file;
                addAsset(type, name);
            }
        }

        if (addedComment)
        {
            addEmptyLine();
        }
    };

    auto addIfExists = [&](const CSV::Row row, const QString& path) -> bool
    {
        if (!QFile::exists(rootDir + "/" + path))
            return false;

        addRow(row);
        return true;
    };

    {
        QString compassName = QString("compass_map_%1").arg(map);
        QString compassPath = QString("materials/%1.json").arg(compassName);
        addIfExists({ "// compass" }, compassPath);
        addIfExists({ "material", compassName }, compassPath);
    }

    addIterator("stringtable", "maps/createart/", ".csv", "lightsets");
    addIterator("clut", "clut/", ".clut", "color lookup tables", false);
    addIterator("rawfile", "vision/", ".vision", "visions");
    addIterator("rawfile", "sun/", ".sun", "sun");

    auto addGsc = [&](const QString& path)
    {
        QString gscPath = rootDir + "/" + path;
        if (!QFile::exists(gscPath)) {
            addAsset("#rawfile", path);
        }
        else {
            addAsset("rawfile", path);
        }
    };

    auto addGscIfExists = [&](const QString& path) -> bool
    {
        QString gscPath = rootDir + "/" + path;
        if (!QFile::exists(gscPath))
            return false;

        addAsset("rawfile", path);
        return true;
    };

    addComment("gsc");
    addGsc(QString("%1/%2.gsc").arg(mapPrefix, map));
    addGsc(fxName);
    addGsc(createFxName);
    addGscIfExists(createFxSoundsName);
    addGscIfExists(QString("%1/%2_precache.gsc").arg(mapPrefix, map));
    addGscIfExists(QString("%1/%2_lighting.gsc").arg(mapPrefix, map));
    addGscIfExists(QString("%1/%2_aud.gsc").arg(mapPrefix, map));
    addGsc(QString("maps/createart/%1_art.gsc").arg(map));
    addGsc(QString("maps/createart/%1_fog.gsc").arg(map));
    addGsc(QString("maps/createart/%1_fog_hdr.gsc").arg(map));
    addEmptyLine();

    auto animated_models = mapEntsRead.getAnimatedModels();
    if (!animated_models.isEmpty()) {
        addGsc(QString("%1/_animatedmodels.gsc").arg(mapPrefix));

        for (auto& animated_model : animated_models) {
            if (!animated_model.precacheScript.isEmpty()) {
                QString precacheScript = animated_model.precacheScript;
                precacheScript = precacheScript.replace(' ', '/') + ".gsc";
                addGsc(precacheScript);
                auto vars = parseAnimatedModelScript(QtUtils::readFile(rootDir + "/" + precacheScript));
                for (auto it = vars.constBegin(); it != vars.constEnd(); ++it) {
                    addAsset("model", it.key());
                    addAsset("xanim", it.value());
                }
            }
        }

        addEmptyLine();
    }

    auto destructible = mapEntsRead.getDestructibles();
    if (!destructible.isEmpty()) {
        addGsc("common_scripts/_destructible.gsc");
        addGsc("common_scripts/_destructible_types.gsc");
        addAsset("rawfile", "animtrees/chicken.atr");
        addEmptyLine();

        // how tf do we add the assets required by destructibles?? just iterating all is wasteful...
    }

    // we need to parse the map gsc for precacheModel and ambientPlay/playSounds?

    qInfo() << "Adding map assets...";

    addComment("map assets");
    addBotPathsIfExists();
    addMapAsset("com_map", ".commap");
    addMapAsset("fx_map", ".fxmap");
    addMapAsset("gfx_map", ".gfxmap");
    addMapAsset("map_ents", ".ents");
    addMapAsset("glass_map", ".glassmap");
    addMapAsset("phys_worldmap", ".physmap");
    addMapAsset("aipaths", ".aipaths");
    addMapAsset(!isMpMap ? "col_map_sp" : "col_map_mp", ".colmap");
    addEmptyLine();

    save();
}