#pragma once
#include "Globals.h"

#include <QtWidgets/QMainWindow>
#include "ui_H1ModTools.h"

class H1ModTools : public QMainWindow
{
    Q_OBJECT

public:
    H1ModTools(QWidget *parent = nullptr);
    ~H1ModTools();

private:
    void setupListWidgets();

    void populateListIW3(QListWidget* list, QString path);
    void populateLists();

    void updateVisibility();
    void update();

    void loadGlobals();

private slots:
    void on_settings_button_clicked();

private:
    Ui::H1ModToolsClass ui;
};
