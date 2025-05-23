#include "H1ModTools.h"

#include "SettingsDialog.h"

void setupStyle()
{
    QFile file(":/H1ModTools/Styles/main.qss");
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

H1ModTools::H1ModTools(QWidget *parent)
    : QMainWindow(parent)
{
    setupStyle();

    ui.setupUi(this);

    logger = std::make_unique<LogRedirector>(ui.outputBuffer);
    logger->installQtMessageHandler();

    ui.outputBuffer->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.outputBuffer, &QWidget::customContextMenuRequested,
        this, &H1ModTools::onOutputBufferContextMenu);

    loadGlobals();

    setupListWidgets(); // Once at init
    //populateLists();

    connect(ui.tabWidget, &QTabWidget::currentChanged, this, &H1ModTools::updateVisibility);
    updateVisibility();
}

H1ModTools::~H1ModTools()
{}

void H1ModTools::on_settingsButton_clicked() {
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        populateLists();
    }
}

QString findGameFolderInAllDrives(const QString& folderName) {
    const QStringList potentialSteamPaths = {
        "Program Files/Steam/steamapps/common/",
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

bool H1_isMapLoad(const QString& name)
{
    return name.endsWith("_load");
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
            const auto name = QFileInfo(current->text(0)).completeBaseName();
            qDebug() << "Current item:" << name;

            const bool isMap = H1_isMap(name);
            ui.compileReflectionsButton->setEnabled(isMap);
            ui.runMapButton->setEnabled(isMap);
            ui.cheatsCheckBox->setEnabled(isMap);
            ui.developerCheckBox->setEnabled(isMap);
        }
    });
}

void H1ModTools::populateListH1(QTreeWidget* tree, const QString& path)
{
    tree->clear();

    QDir zoneSourceDir(path + "/zone_source");
    if (!zoneSourceDir.exists()) return;

    QTreeWidgetItem* root = new QTreeWidgetItem(tree);
    root->setText(0, QString("zone_source"));
    QFont boldFont = root->font(0);
    boldFont.setBold(true);
    root->setFont(0, boldFont);
    root->setFlags(Qt::ItemIsEnabled);

    QStringList filters;
    filters << "*.csv";

    QFileInfoList csvFiles = zoneSourceDir.entryInfoList(filters, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& fileInfo : csvFiles) {
        QTreeWidgetItem* fileItem = new QTreeWidgetItem(root);
        fileItem->setText(0, fileInfo.fileName());
        fileItem->setData(0, Qt::UserRole, fileInfo.absoluteFilePath());
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
            QDir langDir(zoneBaseDir.absoluteFilePath(lang));
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
                item->setData(0, Qt::UserRole, fileInfo.absoluteFilePath());
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
            item->setData(0, Qt::UserRole, dirInfo.filePath());
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
    ui.runMapButton->setEnabled(false);
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

bool moveToUsermaps(const QString& zone)
{
    const auto tryMoveFile = [](const QString& folder, const QString& zoneName, const QString& ext = "") -> bool
    {
        QString sourcePath = Globals.pathH1 + "/zone/" + zoneName + ext;
        QString targetFolder = Globals.pathH1 + "/usermaps/" + folder;

        QFile sourceFile(sourcePath);
        if (!sourceFile.exists())
            return false;

        QDir dir;
        if (!dir.exists(targetFolder))
        {
            if (!dir.mkpath(targetFolder))
                return false;
        }

        QString targetPath = targetFolder + "/" + zoneName + ext;
        if (QFile::exists(targetPath))
            QFile::remove(targetPath); // Overwrite if exists

        return sourceFile.rename(targetPath);
    };

    if (H1_isMapLoad(zone))
    {
        const QString folder = zone.left(zone.length() - 5); // remove "_load"
        if (tryMoveFile(folder, zone, ".ff"))
            return true;
    }

    if (tryMoveFile(zone, zone, ".ff") || tryMoveFile(zone, zone, ".pak"))
        return true;

    return false;
}

void H1ModTools::on_buildZoneButton_clicked()
{
    QString ztPathStr = Globals.pathH1 + "/zonetool.exe";
    QFileInfo ztFile(ztPathStr);

    if (!ztFile.exists() || !ztFile.isExecutable()) {
        qWarning() << "zonetool.exe not found or not executable at" << ztPathStr;
        return;
    }

    if (treeWidgetH1->currentItem() == nullptr || treeWidgetH1->currentItem()->parent() == nullptr)
        return;

    const QString currentSelectedText = treeWidgetH1->currentItem()->text(0);
    const QString currentSelectedZone = QFileInfo(currentSelectedText).completeBaseName();

    const auto isMap = H1_isMap(currentSelectedZone) || H1_isMapLoad(currentSelectedZone);

    QStringList arguments;
    arguments << "-buildzone" << currentSelectedZone;

    qDebug() << "Building zone" << currentSelectedZone;

    QProcess* process = new QProcess(this);
    process->setProgram(ztPathStr);
    process->setArguments(arguments);
    process->setWorkingDirectory(ztFile.absolutePath());
    process->setProcessChannelMode(QProcess::MergedChannels);

    auto readOutput = [](QProcess* process)
    {
        auto handleLogLine = [](const QString& line) {
            // We'll extract and remove the prefix before logging
            QString trimmedLine = line.trimmed();

            QString message;

            if (trimmedLine.startsWith("[ DEBUG ]", Qt::CaseInsensitive)) {
                message = trimmedLine.mid(QString("[ DEBUG ]").length()).trimmed();
                qInfo().noquote() << message;
            }
            else if (trimmedLine.startsWith("[ INFO ]", Qt::CaseInsensitive)) {
                message = trimmedLine.mid(QString("[ INFO ]").length()).trimmed();
                qInfo().noquote() << message;
            }
            else if (trimmedLine.startsWith("[ WARNING ]", Qt::CaseInsensitive)) {
                message = trimmedLine.mid(QString("[ WARNING ]").length()).trimmed();
                qWarning().noquote() << message;
            }
            else if (trimmedLine.startsWith("[ ERROR ]", Qt::CaseInsensitive)) {
                message = trimmedLine.mid(QString("[ ERROR ]").length()).trimmed();
                qCritical().noquote() << message;
            }
            else if (trimmedLine.startsWith("[ FATAL ]", Qt::CaseInsensitive)) {
                message = trimmedLine.mid(QString("[ FATAL ]").length()).trimmed();
                qCritical().noquote() << message;  // No app exit
            }
            else {
                // No prefix found, just log the entire line
                qInfo().noquote() << trimmedLine;
            }
        };

        QByteArray output = process->readAll();
        QStringList lines = QString::fromLocal8Bit(output).split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            if (!line.isEmpty()) {
                handleLogLine(line);
            }
        }
    };

    connect(process, &QProcess::readyRead, [process, readOutput]() {
        readOutput(process);
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [process, readOutput, isMap, currentSelectedZone](int exitCode, QProcess::ExitStatus status) {
        // Flush any remaining output
        readOutput(process);

        qDebug() << "zonetool.exe exited with code" << exitCode
            << (status == QProcess::NormalExit ? "(NormalExit)" : "(CrashExit)");
        process->deleteLater();

        if (exitCode != QProcess::NormalExit)
            return;

        // Move map files to usermaps
        if (isMap && !moveToUsermaps(currentSelectedZone)) {
            qWarning() << "Failed to move" << currentSelectedZone << "to usermaps";
        }
        else if (isMap) {
            qDebug() << "Moved" << currentSelectedZone << "to usermaps";
        }
    });

    process->start();
    if (!process->waitForStarted()) {
        qWarning() << "Failed to start zonetool.exe";
        process->deleteLater();
    }
}

void H1ModTools::onOutputBufferContextMenu(const QPoint& pos)
{
    QMenu* menu = ui.outputBuffer->createStandardContextMenu();
    menu->addSeparator();

    QAction* refreshAction = menu->addAction("Refresh");
    QAction* selectedAction = menu->exec(ui.outputBuffer->mapToGlobal(pos));

    if (selectedAction == refreshAction) {
        ui.outputBuffer->clear();
    }

    delete menu;  // clean up
}

void H1ModTools::onTreeContextMenuRequested(const QPoint& pos)
{
    auto* tree = qobject_cast<QTreeWidget*>(sender());
    if (!tree) return;

    auto* item = tree->itemAt(pos);
    if (!item) return;

    const QString path = item->data(0, Qt::UserRole).toString();
    if (path.isEmpty()) return;

    const QFileInfo fileInfo(path);
    const bool isFile = fileInfo.isFile();
    const bool isCsv = isFile && fileInfo.suffix().compare("csv", Qt::CaseInsensitive) == 0;

    QMenu contextMenu(tree);
    QAction* editAction = nullptr;
    QAction* openExplorerAction = contextMenu.addAction("Open in File Explorer");
    QAction* deleteAction = nullptr;

    if (isCsv) {
        contextMenu.addSeparator();
        editAction = contextMenu.addAction("Edit");
        contextMenu.addSeparator();
        deleteAction = contextMenu.addAction("Delete");
    }

    QAction* selectedAction = contextMenu.exec(tree->viewport()->mapToGlobal(pos));
    if (!selectedAction) return;

    if (selectedAction == deleteAction) {
        const QMessageBox::StandardButton reply = QMessageBox::question(
            tree, "Delete CSV",
            QString("Are you sure you want to delete '%1'?").arg(fileInfo.fileName()),
            QMessageBox::Yes | QMessageBox::No
        );
        if (reply == QMessageBox::Yes) {
            if (!QFile::remove(fileInfo.absoluteFilePath())) {
                QMessageBox::warning(tree, "Delete Failed", "Failed to delete the file.");
            }
            else {
                delete item;
            }
        }
        return;
    }

    if (selectedAction == editAction) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
        return;
    }

    // Handle "Open in File Explorer"
    const QString targetPath = isFile ? fileInfo.absoluteFilePath() : fileInfo.absoluteFilePath();

#if defined(Q_OS_WIN)
    QProcess::startDetached("explorer", { "/select,", QDir::toNativeSeparators(targetPath) });
#elif defined(Q_OS_MAC)
    QProcess::startDetached("open", { targetPath });
#else // Linux and others
    QProcess::startDetached("xdg-open", { targetPath });
#endif
}