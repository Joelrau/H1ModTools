#pragma once
#include "Globals.h"

#include "ui_SettingsDialog.h"

void setupStyle();

namespace Ui {
    class SettingsDialog;
}

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();

private:
    void loadSettings();
    void handleDialogRejected();
    void handleDialogAccepted();
    QVariant savedThemeValue;

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

    void on_buttonBrowseH1_clicked();
    void on_buttonBrowseIW3_clicked();
    void on_buttonBrowseIW4_clicked();
    void on_buttonBrowseIW5_clicked();

    void on_AuroraThemeCheckBox_clicked();

private:
    Ui::SettingsDialog* ui;
};