#pragma once
#include "Globals.h"

class LogRedirector : public QObject
{
    Q_OBJECT

public:
    explicit LogRedirector(QTextEdit* targetEdit, QObject* parent = nullptr);
    ~LogRedirector();

    void installQtMessageHandler();

private slots:
    void readFromPipe();

private:
    void setupPipe();
    void appendColoredText(const QString& msg, const QColor& color);

    int pipeFd[2];
    QSocketNotifier* notifier;
    QTextEdit* outputEdit;

    static LogRedirector* instance;
    static void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
};