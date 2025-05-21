#include "LogRedirector.h"

#include <QMetaObject>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextEdit>
#include <QSocketNotifier>

#include <io.h>
#include <fcntl.h>
#include <stdio.h>

LogRedirector* LogRedirector::instance = nullptr;

LogRedirector::LogRedirector(QTextEdit* targetEdit, QObject* parent)
    : QObject(parent), outputEdit(targetEdit)
{
    Q_ASSERT(outputEdit);
    outputEdit->setReadOnly(true);
    setupPipe();
}

LogRedirector::~LogRedirector()
{
    if (notifier)
        delete notifier;
}

void LogRedirector::setupPipe()
{
    if (_pipe(pipeFd, 1024, _O_TEXT) == -1) {
        outputEdit->append("Failed to create pipe.");
        return;
    }

    _dup2(pipeFd[1], _fileno(stdout));
    _dup2(pipeFd[1], _fileno(stderr));

    notifier = new QSocketNotifier(pipeFd[0], QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, &LogRedirector::readFromPipe);
}

void LogRedirector::readFromPipe()
{
    char buffer[1024];
    int bytes = _read(pipeFd[0], buffer, sizeof(buffer) - 1);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        QString text = QString::fromLocal8Bit(buffer).trimmed();

        QColor color = Qt::white;
        if (text.contains("[ Debug ]", Qt::CaseInsensitive))
            color = Qt::cyan;
        else if (text.contains("[ Warning ]", Qt::CaseInsensitive))
            color = Qt::yellow;
        else if (text.contains("[ Critical ]", Qt::CaseInsensitive) || text.contains("[ Error ]", Qt::CaseInsensitive))
            color = Qt::red;
        else if (text.contains("[ Fatal ]", Qt::CaseInsensitive))
            color = Qt::darkRed;

        appendColoredText(text, color);
    }
}

void LogRedirector::appendColoredText(const QString& msg, const QColor& color)
{
    QMetaObject::invokeMethod(outputEdit, [=]() {
        QTextCharFormat fmt;
        fmt.setForeground(color);
        QTextCursor cursor = outputEdit->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(msg + "\n", fmt);
        outputEdit->setTextCursor(cursor);
    });
}

void LogRedirector::installQtMessageHandler()
{
    instance = this;
    qInstallMessageHandler(LogRedirector::qtMessageHandler);
}

void LogRedirector::qtMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    QString prefix;
    switch (type) {
    case QtDebugMsg:    prefix = "[ Debug ] "; break;
    case QtWarningMsg:  prefix = "[ Warning ] "; break;
    case QtCriticalMsg: prefix = "[ Critical ] "; break;
    case QtFatalMsg:    prefix = "[ Fatal ] "; break;
    }

    QString full = prefix + msg;
    fprintf(stdout, "%s\n", full.toUtf8().constData());
    fflush(stdout);

    if (instance && instance->outputEdit) {
        QColor color = Qt::white;
        switch (type) {
        case QtDebugMsg:    color = Qt::cyan; break;
        case QtWarningMsg:  color = Qt::yellow; break;
        case QtCriticalMsg: color = Qt::red; break;
        case QtFatalMsg:    color = Qt::darkRed; break;
        }

        instance->appendColoredText(full, color);
    }
}