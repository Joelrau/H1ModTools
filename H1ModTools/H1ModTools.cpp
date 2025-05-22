#include "H1ModTools.h"

#include "SettingsDialog.h"

H1ModTools::H1ModTools(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    logger = std::make_unique<LogRedirector>(ui.outputBuffer);
    logger->installQtMessageHandler();

    loadGlobals();

    setupListWidgets(); // Once at init
    //populateLists();

    connect(ui.tabWidget, &QTabWidget::currentChanged, this, &H1ModTools::updateVisibility);
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

bool H1_isMap(const QString& name)
{
    const QDir baseDir = QDir(Globals.pathH1).filePath("zonetool/" + name);
    if (!baseDir.exists())
        return false;

    const QString spPath = baseDir.filePath(QStringLiteral("maps/%1.d3dbsp.ents").arg(name));
    const QString mpPath = baseDir.filePath(QStringLiteral("maps/mp/%1.d3dbsp.ents").arg(name));

    return QFileInfo::exists(spPath) || QFileInfo::exists(mpPath);
}

void H1ModTools::setupListWidgets()
{
    // Create and layout each QTreeWidget inside the corresponding tab
    auto setupTabTree = [](QWidget* tab, QTreeWidget*& treeWidget) {
        treeWidget = new QTreeWidget(tab);
        treeWidget->setHeaderHidden(true); // Hide the header
        auto* layout = new QVBoxLayout(tab);
        layout->addWidget(treeWidget);
        layout->setContentsMargins(4, 4, 4, 4);
        tab->setLayout(layout);
    };

    setupTabTree(ui.tabH1, treeWidgetH1);
    setupTabTree(ui.tabIW3, treeWidgetIW3);
    setupTabTree(ui.tabIW4, treeWidgetIW4);
    setupTabTree(ui.tabIW5, treeWidgetIW5);

    // Connect context menu signals only once
    auto connectContextMenu = [this](QTreeWidget* tree) {
        tree->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(tree, &QTreeWidget::customContextMenuRequested, this, &H1ModTools::onTreeContextMenuRequested);
    };
    connectContextMenu(treeWidgetH1);
    connectContextMenu(treeWidgetIW3);
    connectContextMenu(treeWidgetIW4);
    connectContextMenu(treeWidgetIW5);

    connect(treeWidgetH1, &QTreeWidget::currentItemChanged, this,
        [this](QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/) {
        if (current && current->parent() != nullptr) {  // Ignore root items
            qDebug() << "Current item:" << current->text(0);

            const bool isMap = H1_isMap(current->text(0));
            ui.compileReflectionsButton->setEnabled(isMap);
            ui.RunMapButton->setEnabled(isMap);
            ui.cheatsCheckBox->setEnabled(isMap);
            ui.developerCheckBox->setEnabled(isMap);
        }
    });
}

void H1ModTools::populateListH1(QTreeWidget* tree, const QString& path)
{
    tree->clear();

    QDir zonetoolDir(path + "/zonetool");
    if (zonetoolDir.exists()) {
        QTreeWidgetItem* root = new QTreeWidgetItem(tree);
        root->setText(0, QString("zonetool"));
        QFont boldFont = root->font(0);
        boldFont.setBold(true);
        root->setFont(0, boldFont);
        root->setFlags(Qt::ItemIsEnabled);

        QFileInfoList dirs = zonetoolDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo& dirInfo : dirs) {
            QTreeWidgetItem* item = new QTreeWidgetItem(root);
            item->setText(0, dirInfo.fileName());
            item->setData(0, Qt::UserRole, dirInfo.absoluteFilePath()); // Use folder directly
        }
    }

    tree->expandAll();
}

void H1ModTools::populateListIW(QTreeWidget* tree, const QString& path)
{
    tree->clear();

    const QStringList languageFolders = {
        "english", "french", "german", "spanish",
        "japanese", "russian", "italian", "dlc"
    };

    // zone/{language}
    QDir zoneBaseDir(path + "/zone");
    if (zoneBaseDir.exists()) {
        for (const QString& lang : languageFolders) {
            QDir langDir(zoneBaseDir.filePath(lang));
            if (!langDir.exists())
                continue;

            QTreeWidgetItem* langRoot = new QTreeWidgetItem(tree);
            langRoot->setText(0, QString("zone/%1").arg(lang));
            QFont boldFont = langRoot->font(0);
            boldFont.setBold(true);
            langRoot->setFont(0, boldFont);
            langRoot->setFlags(Qt::ItemIsEnabled);

            QFileInfoList ffFiles = langDir.entryInfoList({ "*.ff" }, QDir::Files);
            for (const QFileInfo& fileInfo : ffFiles) {
                QTreeWidgetItem* item = new QTreeWidgetItem(langRoot);
                item->setText(0, fileInfo.fileName());
                item->setData(0, Qt::UserRole, fileInfo.absolutePath()); // Just store folder
            }
        }
    }

    // usermaps
    QDir usermapsDir(path + "/usermaps");
    if (usermapsDir.exists()) {
        QTreeWidgetItem* mapsRoot = new QTreeWidgetItem(tree);
        mapsRoot->setText(0, "usermaps");
        QFont boldFont = mapsRoot->font(0);
        boldFont.setBold(true);
        mapsRoot->setFont(0, boldFont);
        mapsRoot->setFlags(Qt::ItemIsEnabled);

        QFileInfoList mapDirs = usermapsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo& dirInfo : mapDirs) {
            QTreeWidgetItem* item = new QTreeWidgetItem(mapsRoot);
            item->setText(0, dirInfo.fileName());
            item->setData(0, Qt::UserRole, dirInfo.absoluteFilePath()); // Use folder directly
        }
    }

    tree->expandAll();
}

void H1ModTools::populateLists()
{
    populateListH1(treeWidgetH1, Globals.pathH1);
    populateListIW(treeWidgetIW3, Globals.pathIW3);
    populateListIW(treeWidgetIW4, Globals.pathIW4);
    populateListIW(treeWidgetIW5, Globals.pathIW5);
}

void H1ModTools::updateVisibility()
{
    int index = ui.tabWidget->currentIndex();
    QWidget* currentTab = ui.tabWidget->widget(index);

    // Check if the current tab is H1
    bool isH1Selected = ui.tabWidget->currentWidget() == ui.tabH1;

    // Example logic: enable export only if H1 tab is active
    const bool canExport = !isH1Selected;
    const bool canCompile = isH1Selected;

    ui.exportButton->setEnabled(canExport);
    ui.generateCsvCheckBox->setEnabled(canExport);

    ui.buildZoneButton->setEnabled(canCompile);
    ui.compileReflectionsButton->setEnabled(false);
    ui.RunMapButton->setEnabled(false);
    ui.cheatsCheckBox->setEnabled(false);
    ui.developerCheckBox->setEnabled(false);

    // Refresh list based on selected tab
    if (currentTab == ui.tabH1)
        populateListH1(treeWidgetH1, Globals.pathH1);
    else if (currentTab == ui.tabIW3)
        populateListIW(treeWidgetIW3, Globals.pathIW3);
    else if (currentTab == ui.tabIW4)
        populateListIW(treeWidgetIW4, Globals.pathIW4);
    else if (currentTab == ui.tabIW5)
        populateListIW(treeWidgetIW5, Globals.pathIW5);
}

void H1ModTools::onTreeContextMenuRequested(const QPoint& pos)
{
    QTreeWidget* tree = qobject_cast<QTreeWidget*>(sender());
    if (!tree) return;

    QTreeWidgetItem* item = tree->itemAt(pos);
    if (!item) return;

    const QVariant pathVar = item->data(0, Qt::UserRole);
    if (!pathVar.isValid()) return;

    const QString path = pathVar.toString();
    if (path.isEmpty()) return;

    QMenu contextMenu(tree);
    QAction* openExplorerAction = contextMenu.addAction("Open in File Explorer");

    QAction* selectedAction = contextMenu.exec(tree->viewport()->mapToGlobal(pos));
    if (selectedAction == openExplorerAction) {
#if defined(Q_OS_WIN)
        QProcess::startDetached("explorer", { QDir::toNativeSeparators(path) });
#elif defined(Q_OS_MAC)
        QProcess::startDetached("open", { path });
#else // Linux
        QProcess::startDetached("xdg-open", { path });
#endif
    }
}