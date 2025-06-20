#include "Globals.h"

Globals_e Globals{};

void saveGlobalsToJson(QWidget* parent)
{
    QJsonObject settings{};

    QJsonObject paths{};
    paths["h1"] = Globals.pathH1;
    paths["iw3"] = Globals.pathIW3;
    paths["iw4"] = Globals.pathIW4;
    paths["iw5"] = Globals.pathIW5;
    
    settings["paths"] = paths;
    settings["h1_mod_exe"] = Globals.h1_mod_exe;

    QFile file("settings.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(parent, "Error", "Failed to open settings.json for writing.");
        return;
    }

    file.write(QJsonDocument(settings).toJson());
    file.close();
}

bool loadGlobalsFromJson()
{
    QFile file("settings.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false; // File doesn't exist or can't be opened
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse settings.json:" << error.errorString();
        return false;
    }

    QJsonObject obj = doc.object();
    QJsonObject paths = obj["paths"].toObject();

    Globals.pathH1 = paths["h1"].toString();
    Globals.pathIW3 = paths["iw3"].toString();
    Globals.pathIW4 = paths["iw4"].toString();
    Globals.pathIW5 = paths["iw5"].toString();
    Globals.h1_mod_exe = obj["h1_mod_exe"].toString();

    return true;
}