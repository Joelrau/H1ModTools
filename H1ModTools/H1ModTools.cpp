#include "H1ModTools.h"

#include "LogRedirector.h"
#include "SettingsDialog.h"

H1ModTools::H1ModTools(QWidget *parent)
    : QMainWindow(parent)
{
    loadGlobals();

    ui.setupUi(this);

    LogRedirector* logger = new LogRedirector(ui.outputBuffer);
    logger->installQtMessageHandler();

    connect(ui.tabWidget, &QTabWidget::currentChanged, this, &H1ModTools::updateVisibility);

    setupListWidgets(); // Once at init
    populateLists();
    updateVisibility();
}

H1ModTools::~H1ModTools()
{}

void H1ModTools::on_settings_button_clicked() {
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        populateLists();
    }
}

QString findGameFolderInAllDrives(const QString& folderName) {
    const QStringList potentialSteamPaths = {
        "Program Files (x86)/Steam/steamapps/common/",
        "SteamLibrary/steamapps/common/",
    };

    for (const QStorageInfo& drive : QStorageInfo::mountedVolumes()) {
        if (!drive.isValid() || !drive.isReady())
            continue;

        const QString rootPath = drive.rootPath();

        for (const QString& steamPath : potentialSteamPaths) {
            const QString fullPath = QDir::cleanPath(rootPath + "/" + steamPath + folderName);
            if (QDir(fullPath).exists()) {
                return fullPath;
            }
        }
    }

    return {};
}

void generateSettings(QWidget* parent) {
    Globals.pathH1 = findGameFolderInAllDrives("Call of Duty Modern Warfare Remastered");
    Globals.pathIW3 = findGameFolderInAllDrives("Call of Duty 4");
    Globals.pathIW4 = findGameFolderInAllDrives("Call of Duty Modern Warfare 2");
    Globals.pathIW5 = findGameFolderInAllDrives("Call of Duty Modern Warfare 3");

    saveGlobalsToJson(parent);

    qDebug() << "Generated settings.json with auto-discovered paths.";
}

void H1ModTools::loadGlobals() {
    if (!loadGlobalsFromJson()) {
        // No settings found, let's generate!
        generateSettings(this);
    }
}

// Store list widgets for later use
QListWidget* listWidgetH1 = nullptr;
QListWidget* listWidgetIW3 = nullptr;
QListWidget* listWidgetIW4 = nullptr;
QListWidget* listWidgetIW5 = nullptr;

void H1ModTools::setupListWidgets()
{
    // Create and layout each QListWidget inside the corresponding tab
    auto setupTabList = [](QWidget* tab, QListWidget*& listWidget) {
        listWidget = new QListWidget(tab);
        auto* layout = new QVBoxLayout(tab);
        layout->addWidget(listWidget);
        layout->setContentsMargins(4, 4, 4, 4);  // Optional
        tab->setLayout(layout);
    };

    setupTabList(ui.tabH1, listWidgetH1);
    setupTabList(ui.tabIW3, listWidgetIW3);
    setupTabList(ui.tabIW4, listWidgetIW4);
    setupTabList(ui.tabIW5, listWidgetIW5);
}

void H1ModTools::populateListIW3(QListWidget* list, QString path)
{
    list->clear();

    QDir dir(path);
    if (!dir.exists()) return;

    // zone
    {

    }

    // usermaps
}

void H1ModTools::populateLists()
{
    populateListIW3(listWidgetIW3, Globals.pathIW3);
}

void H1ModTools::updateVisibility()
{
    // Check if the current tab is H1
    bool isH1Selected = ui.tabWidget->currentWidget() == ui.tabH1;

    // Example logic: enable export only if H1 tab is active
    const bool canExport = !isH1Selected;
    const bool canCompile = isH1Selected;

    ui.exportButton->setEnabled(canExport);
    ui.generateCSVButton->setEnabled(canCompile);
    ui.compileReflectionsButton->setEnabled(canCompile);
    ui.buildZoneButton->setEnabled(canCompile);
    ui.RunMapButton->setEnabled(canCompile);
    ui.RunMapButton->setEnabled(canCompile);
    ui.cheatsCheckBox->setEnabled(canCompile);
    ui.developerCheckBox->setEnabled(canCompile);
}

void H1ModTools::update()
{
    updateVisibility();
}