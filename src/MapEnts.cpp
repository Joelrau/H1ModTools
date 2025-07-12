#include "MapEnts.h"

MapEnts::MapEnts(const QString& mapEntsPath)
{
    this->setPath(mapEntsPath);
}

MapEnts::~MapEnts()
{
    this->path.clear();
}

void MapEnts::setPath(const QString& mapEntsPath)
{
    this->path = mapEntsPath;
}

void MapEnts::readEnts()
{
    if (this->path.isEmpty()) {
        return;
    }

    QFile file(this->path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << this->path;
        return;
    }

    QTextStream in(&file);
    QString line;
    unsigned int lineNum = 0;
    bool inBlock = false;

    MapEntity entity{};

    while (!in.atEnd()) {
        line = in.readLine().trimmed();
        lineNum++;

        if (line.startsWith("//")) {
            continue;
        }

        if (line[0] == "{") {
            inBlock = true;
        }
        else if (line[0] == "}") {
            ents.append(entity);
            entity.clear();
            inBlock = false;
        }
        else if (inBlock) {
            QRegularExpression expr("^\"([^\"]+)\"\\s+\"([^\"]+)\"$");
            QRegularExpressionMatch match = expr.match(line);

            if (!match.hasMatch()) {
                qWarning().noquote() << QString("Failed to parse line %1 (%2)").arg(lineNum).arg(line);
                continue;
            }

            MapEntVar var{};
            var.key = match.captured(1).toLower();
            var.value = match.captured(2);
            entity.addVar(var);
        }
    }
}

void MapEnts::writeEnts()
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << path;
        return;
    }

    QTextStream stream(&file);

    for (const auto& entity : ents) {
        stream << "{\n";
        for (const auto& var : entity.vars) {
            stream << QString("\"%1\" \"%2\"\n").arg(var.key, var.value);
        }
        stream << "}\n";
    }
}

MapEntsReader::MapEntsReader(const QString& mapEntsPath)
{
    this->mapEnts.setPath(mapEntsPath);
    this->mapEnts.readEnts();

    if (this->mapEnts.ents.isEmpty()) {
        return;
    }

    this->destructibles.clear();
    this->animatedModels.clear();
    this->globalIntermissionExists = false;

    for (auto& ent : this->mapEnts.ents)
    {
        auto targetname = ent.get("targetname");
        auto classname = ent.get("classname");
        auto model = ent.get("model");

        if (classname == "mp_global_intermission")
            this->globalIntermissionExists = true;

        if (!model.isEmpty() && classname == "script_model")
            this->models.append(model);

        auto destructible_type = ent.get("destructible_type");
        if (!destructible_type.isEmpty()) {
            DestructibleData data{};
            data.name = destructible_type;
            data.model = model;
            this->destructibles.insert(data);
        }

        if (!targetname.isEmpty() && targetname == "animated_model") {
            AnimatedModelData data{};
            data.model = model;
            data.precacheScript = ent.get("precache_script");
            this->animatedModels.insert(data);
        }
    }

    this->models.sort();
    this->models.removeDuplicates();
}

MapEntsReader::~MapEntsReader()
{
    this->destructibles.clear();
    this->animatedModels.clear();
    this->models.clear();
}
