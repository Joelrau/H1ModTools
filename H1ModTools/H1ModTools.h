#pragma once
#include "Globals.h"

#include <QtWidgets/QMainWindow>
#include "ui_H1ModTools.h"

#include "LogRedirector.h"

enum GameType {
    H1,
    IW3,
    IW4,
    IW5
};

class H1ModTools : public QMainWindow
{
    Q_OBJECT

public:
    H1ModTools(QWidget *parent = nullptr);
    ~H1ModTools();

private:
    void setupListWidgets();

    void populateListH1(QTreeWidget* list, const QString& path);
    void populateListIW(QTreeWidget* list, const QString& path);
    void populateLists();

    void updateVisibility();

    void loadGlobals();

    GameType getCurrentGameType();

private slots:
	void on_exportButton_clicked();

    void on_runMapButton_clicked();
    void on_compileReflectionsButton_clicked();
    void on_buildZoneButton_clicked();
    void on_settingsButton_clicked();
    void onTreeContextMenuRequested(const QPoint& pos);
    void onOutputBufferContextMenu(const QPoint& pos);

private:
    Ui::H1ModToolsClass ui;
    std::unique_ptr<LogRedirector> logger;

    QTreeWidget* treeWidgetH1 = nullptr;
    QTreeWidget* treeWidgetIW3 = nullptr;
    QTreeWidget* treeWidgetIW4 = nullptr;
    QTreeWidget* treeWidgetIW5 = nullptr;

    QMap<QWidget*, bool> m_uiEnabledStates;
    void disableUiAndStoreState();
    void restoreUiState();
};
