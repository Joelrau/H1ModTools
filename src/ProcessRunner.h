#pragma once

#include "Globals.h"

#include <functional>

class ProcessRunner : public QObject
{
    Q_OBJECT

public:
    explicit ProcessRunner(QObject* parent = nullptr);

    static void readOutputFromProcess(QProcess* process);

    void run(
        const QFileInfo& exe,
        const QStringList& args,
        std::function<void(int)> onFinish = {}
    );
};