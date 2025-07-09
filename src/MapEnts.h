#pragma once

#include "Globals.h"

class MapEnts
{
public:
    MapEnts(const QString& mapEntsPath);
    ~MapEnts();

    QString path;

    struct MapEntVar
    {
        QString key;
        QString value;
    };

    class MapEntity
    {
    public:
        void clear() {
            vars.clear();
        }

        void add_var(const MapEntVar& var)
        {
            vars.append(var);
        }

        QString get(const QString& key) const
        {
            for (const auto& var : this->vars)
            {
                if (var.key == key)
                {
                    return var.value;
                }
            }

            return "";
        }

        QList<MapEntVar> vars;
    };

    QList<MapEntity> ents;

    void readVars();
};

class MapEntsReader
{
public:
    struct DestructibleData
    {
        QString name;   // destructible_type
        QString model;

        bool operator==(const DestructibleData& other) const {
            return name == other.name && model == other.model;
        }
    };

    struct AnimatedModelData
    {
        QString precacheScript;
        QString model;

        bool operator==(const AnimatedModelData& other) const {
            return precacheScript == other.precacheScript && model == other.model;
        }
    };

    MapEntsReader(const QString& mapEntsPath);
    ~MapEntsReader();

    QSet<DestructibleData> getDestructibles() const { return destructibles; }
    QSet<AnimatedModelData> getAnimatedModels() const { return animatedModels; }
    QStringList getAllModels() const { return models; }

    bool globalIntermissionExists;
    QList<MapEnts::MapEntity> mapEnts;

private:
    QSet<DestructibleData> destructibles;
    QSet<AnimatedModelData> animatedModels;
    QStringList models;
};

// Hash functions for QSet usage
inline size_t qHash(const MapEntsReader::DestructibleData& key, uint seed = 0)
{
    return qHash(key.name, seed) ^ qHash(key.model, seed << 1);
}

inline size_t qHash(const MapEntsReader::AnimatedModelData& key, uint seed = 0)
{
    return qHash(key.precacheScript, seed << 1)
        ^ qHash(key.model, seed << 2);
}