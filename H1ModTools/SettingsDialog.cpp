#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

SettingsDialog::SettingsDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::loadSettings()
{
    ui->lineEditH1->setText(Globals.pathH1);
    ui->lineEditIW3->setText(Globals.pathIW3);
    ui->lineEditIW4->setText(Globals.pathIW4);
    ui->lineEditIW5->setText(Globals.pathIW5);
    ui->lineEditH1exe->setText(Globals.h1_mod_exe);
}

void SettingsDialog::on_buttonBox_accepted() {
    // Collect path values
    QString h1Path = ui->lineEditH1->text().trimmed();
    QString iw3Path = ui->lineEditIW3->text().trimmed();
    QString iw4Path = ui->lineEditIW4->text().trimmed();
    QString iw5Path = ui->lineEditIW5->text().trimmed();
    QString h1_mod_exe = ui->lineEditH1exe->text().trimmed();

    struct PathInfo {
        QString label;
        QString value;
    };

    QList<PathInfo> pathsToValidate = {
        {"H1",  h1Path},
        {"IW3", iw3Path},
        {"IW4", iw4Path},
        {"IW5", iw5Path},
    };

    for (const PathInfo& p : pathsToValidate) {
        if (!p.value.isEmpty() && !QDir(p.value).exists()) {
            QMessageBox::warning(this, "Invalid Path", QString("%1 path does not exist:\n%2").arg(p.label, p.value));
            return;
        }
    }

    Globals.pathH1 = h1Path;
    Globals.pathIW3 = iw3Path;
    Globals.pathIW4 = iw4Path;
    Globals.pathIW5 = iw5Path;
    Globals.h1_mod_exe = h1_mod_exe;

    saveGlobalsToJson(this);

    accept();
}

void SettingsDialog::on_buttonBox_rejected()
{
    reject();
}

void SettingsDialog::on_buttonBrowseH1_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Call of Duty - Modern Warfare Remastered (H1) Directory");
    if (!dir.isEmpty()) ui->lineEditH1->setText(dir);
}

void SettingsDialog::on_buttonBrowseIW3_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Call of Duty 4 - Modern Warfare (IW3) Directory");
    if (!dir.isEmpty()) ui->lineEditIW3->setText(dir);
}

void SettingsDialog::on_buttonBrowseIW4_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Call of Duty - Modern Warfare 2 (IW4) Directory");
    if (!dir.isEmpty()) ui->lineEditIW4->setText(dir);
}

void SettingsDialog::on_buttonBrowseIW5_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Call of Duty - Modern Warfare 3 (IW5) Directory");
    if (!dir.isEmpty()) ui->lineEditIW5->setText(dir);
}