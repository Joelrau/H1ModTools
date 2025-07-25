﻿#include "SettingsDialog.h"

void setupStyle()
{
    QSettings settings;
    auto auroraTheme = settings.value("UseAuroraTheme", true).toBool();

    if (auroraTheme)
    {
        QFile file(":/H1ModTools/Resources/Styles/main.qss");
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            qWarning() << "Failed to open stylesheet resource";

            QApplication::setStyle(QStyleFactory::create("Fusion"));
        }
        else
        {
            QString styleSheet = QString::fromUtf8(file.readAll());
            qApp->setStyleSheet(styleSheet);
        }
    }
    else
    {
        qApp->setStyle("Windows");
        qApp->setStyleSheet("");
    }
}

SettingsDialog::SettingsDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    QSettings settings;
    savedThemeValue = settings.value("UseAuroraTheme", true);

    ui->setupUi(this);

    ui->AuroraThemeCheckBox->setChecked(savedThemeValue.toBool());

    connect(this, &QDialog::rejected, this, &SettingsDialog::handleDialogRejected);
    connect(this, &QDialog::accepted, this, &SettingsDialog::handleDialogAccepted);

    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::handleDialogRejected()
{
    QSettings settings;
    settings.setValue("UseAuroraTheme", savedThemeValue.toBool());
    setupStyle();
}

void SettingsDialog::handleDialogAccepted()
{
    // Collect path values
    QString h1Path = ui->lineEditH1->text().trimmed();
    QString iw3Path = ui->lineEditIW3->text().trimmed();
    QString iw4Path = ui->lineEditIW4->text().trimmed();
    QString iw5Path = ui->lineEditIW5->text().trimmed();
    QString h1Executable = ui->lineEditH1exe->text().trimmed();

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
    Globals.h1Executable = h1Executable;

    saveGlobalsToJson(this);
}

void SettingsDialog::loadSettings()
{
    ui->lineEditH1->setText(Globals.pathH1);
    ui->lineEditIW3->setText(Globals.pathIW3);
    ui->lineEditIW4->setText(Globals.pathIW4);
    ui->lineEditIW5->setText(Globals.pathIW5);
    ui->lineEditH1exe->setText(Globals.h1Executable);
}

void SettingsDialog::on_buttonBox_accepted()
{
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

void SettingsDialog::on_AuroraThemeCheckBox_clicked()
{
    QSettings settings;
    settings.setValue("UseAuroraTheme", ui->AuroraThemeCheckBox->isChecked());
    setupStyle();
}