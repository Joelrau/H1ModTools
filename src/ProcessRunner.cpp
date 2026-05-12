#include "ProcessRunner.h"
#include <QDir>
#include <QFile>

ProcessRunner::ProcessRunner(QObject* parent)
    : QObject(parent)
{
}

void ProcessRunner::readOutputFromProcess(QProcess* process)
{
    auto handleLogLine = [](const QString& line) 
    {
        struct LevelConfig
        {
            QString prefix;
            QtMsgType type;
        };

        static const LevelConfig configs[] = 
        {
            { "[ DEBUG ]",   QtInfoMsg },     // Mapping Debug to Info to avoid noise
            { "[ INFO ]",    QtInfoMsg },
            { "[ WARNING ]", QtWarningMsg },
            { "[ ERROR ]",   QtCriticalMsg },
            { "[ FATAL ]",   QtCriticalMsg }
        };

        for (const auto& config : configs)
        {
            if (line.startsWith(config.prefix, Qt::CaseInsensitive))
            {
                QString message = line.mid(config.prefix.length()).trimmed();

                switch (config.type)
                {
                case QtDebugMsg:    qDebug().noquote() << message; break;
                case QtInfoMsg:     qInfo().noquote() << message; break;
                case QtWarningMsg:  qWarning().noquote() << message; break;
                case QtCriticalMsg: qCritical().noquote() << message; break;
                case QtFatalMsg:    qFatal("%s", qUtf8Printable(message)); break;
                }
                return;
            }
        }

        // Default fallback
        qInfo().noquote() << line.trimmed();
    };

    QByteArray output = process->readAll();
    QStringList lines = QString::fromLocal8Bit(output).split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines)
    {
        if (!line.isEmpty())
        {
            handleLogLine(line);
        }
    }
}

void ProcessRunner::run(
    const QFileInfo& exe,
    const QStringList& args,
    std::function<void(int)> onFinish)
{
    const QString fullPath = exe.absoluteFilePath();

    if (!exe.exists())
    {
        qCritical() << "Missing executable:" << fullPath;
        if (onFinish) onFinish(-1);
        return;
    }

    auto* proc = new QProcess(this);
    proc->setProgram(fullPath);
    proc->setArguments(args);
    proc->setWorkingDirectory(exe.absolutePath());
    proc->setProcessChannelMode(QProcess::MergedChannels);

    connect(proc, &QProcess::readyRead, [proc]()
    {
        readOutputFromProcess(proc);
    });

    connect(proc, &QProcess::errorOccurred, [onFinish](QProcess::ProcessError error)
    {
        qWarning() << "Process error occurred:" << error;
        if(onFinish)
			onFinish(-1);
    });

    connect(proc, &QProcess::finished, this, [proc, onFinish](int code)
    {
        readOutputFromProcess(proc);

        if (onFinish)
            onFinish(code);

        proc->deleteLater();
    });

    proc->start();
}