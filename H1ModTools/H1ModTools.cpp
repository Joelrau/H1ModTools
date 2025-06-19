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

namespace Funcs
{
    namespace H1
    {
        bool isMapLoad(const QString& name)
        {
            return name.endsWith("_load");
        }

        bool isMap(const QString& name)
        {
            const QDir baseDir = QDir(Globals.pathH1).filePath("zonetool/" + name);
            if (!baseDir.exists())
                return false;

            const QString spPath = baseDir.filePath(QStringLiteral("maps/%1.d3dbsp.ents").arg(name));
            const QString mpPath = baseDir.filePath(QStringLiteral("maps/mp/%1.d3dbsp.ents").arg(name));

            return QFileInfo::exists(spPath) || QFileInfo::exists(mpPath);
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

            if (Funcs::H1::isMapLoad(zone))
            {
                const QString folder = zone.left(zone.length() - 5); // remove "_load"
                if (tryMoveFile(folder, zone, ".ff"))
                    return true;
            }

            if (tryMoveFile(zone, zone, ".ff") || tryMoveFile(zone, zone, ".pak"))
                return true;

            return false;
        }

        bool moveReflectionProbes(const QString& zone)
        {
            const QString sourceFolder = Globals.pathH1 + "/dump/" + zone + "/images/";
            const QString targetFolder = Globals.pathH1 + "/zonetool/" + zone + "/images/";

            QDir sourceDir(sourceFolder);
            QDir targetDir(targetFolder);

            // Both source and target directories must exist
            if (!sourceDir.exists() || !targetDir.exists()) {
                qWarning() << "Required folder does not exist:"
                    << (!sourceDir.exists() ? sourceFolder : targetFolder);
                return false;
            }

            // Get _reflection_probe*.dds files and ensure at least one exists
            const QStringList ddsFiles = sourceDir.entryList(QStringList() << "_reflection_probe*.dds", QDir::Files);
            if (ddsFiles.isEmpty()) {
                qWarning() << "No _reflection_probe*.dds files found in" << sourceFolder;
                return false;
            }

            // Move the DDS files
            for (const QString& fileName : ddsFiles) {
                const QString srcPath = sourceFolder + fileName;
                const QString dstPath = targetFolder + fileName;

                // If the file exists at destination, remove it
                if (QFile::exists(dstPath) && !QFile::remove(dstPath)) {
                    qWarning() << "Failed to remove existing destination file:" << dstPath;
                    return false;
                }

                // Move the file
                if (!QFile::rename(srcPath, dstPath)) {
                    qWarning() << "Failed to move file:" << srcPath << "->" << dstPath;
                    return false;
                }
            }

            // Delete any matching .h1Image files
            const QStringList h1ImageFiles = sourceDir.entryList(QStringList() << "_reflection_probe*.h1Image", QDir::Files);
            for (const QString& fileName : h1ImageFiles) {
                const QString filePath = sourceFolder + fileName;

                if (!QFile::remove(filePath)) {
                    qWarning() << "Failed to delete file:" << filePath;
                    return false;
                }
            }

            return true;
        }
    }
    namespace Shared
    {
        void readOutputFromProcess(QProcess* process)
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
        }
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

void H1ModTools::setupListWidgets() {
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

            const bool isMap = Funcs::H1::isMap(name);
            ui.compileReflectionsButton->setEnabled(isMap);
            ui.runMapButton->setEnabled(isMap);
            ui.cheatsCheckBox->setEnabled(isMap);
            ui.developerCheckBox->setEnabled(isMap);
        }
    });
}

void H1ModTools::populateListH1(QTreeWidget* tree, const QString& path) {
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

void H1ModTools::populateListIW(QTreeWidget* tree, const QString& path) {
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

void H1ModTools::populateLists() {
    populateListH1(treeWidgetH1, Globals.pathH1);
    populateListIW(treeWidgetIW3, Globals.pathIW3);
    populateListIW(treeWidgetIW4, Globals.pathIW4);
    populateListIW(treeWidgetIW5, Globals.pathIW5);
}

void H1ModTools::updateVisibility() {
	const auto gameType = getCurrentGameType();

    // Check if the current tab is H1
    bool isH1Selected = gameType == GameType::H1;

    // Enable export only if H1 tab is inactive and compile if it is active
    const bool canExport = !isH1Selected;
    const bool canCompile = isH1Selected;

    ui.exportButton->setEnabled(canExport);
    ui.generateCsvCheckBox->setEnabled(canExport);
    ui.convertGscCheckBox->setEnabled(canExport);

    ui.buildZoneButton->setEnabled(canCompile);
    ui.compileReflectionsButton->setEnabled(false);
    ui.runMapButton->setEnabled(false);
    ui.cheatsCheckBox->setEnabled(false);
    ui.developerCheckBox->setEnabled(false);

    // Refresh list based on selected tab
    if (gameType == GameType::H1)
        populateListH1(treeWidgetH1, Globals.pathH1);
    else if (gameType == GameType::IW3)
        populateListIW(treeWidgetIW3, Globals.pathIW3);
    else if (gameType == GameType::IW4)
        populateListIW(treeWidgetIW4, Globals.pathIW4);
    else if (gameType == GameType::IW5)
        populateListIW(treeWidgetIW5, Globals.pathIW5);
}

GameType H1ModTools::getCurrentGameType() {
    if (ui.tabWidget->currentWidget() == ui.tabH1) return GameType::H1;
    if (ui.tabWidget->currentWidget() == ui.tabIW3) return GameType::IW3;
    if (ui.tabWidget->currentWidget() == ui.tabIW4) return GameType::IW4;
    if (ui.tabWidget->currentWidget() == ui.tabIW5) return GameType::IW5;
    return GameType::H1; // Default to H1
}

void H1ModTools::on_exportButton_clicked() {
    const auto gameType = getCurrentGameType();

    const auto getExecutableName = [gameType]() -> QString {
        switch (gameType) {
        case GameType::IW3: return "zonetool_iw3.exe";
        case GameType::IW4: return "zonetool_iw4.exe";
        case GameType::IW5: return "zonetool_iw5.exe";
        }
        __debugbreak();
        return "";
    };

    const auto getGamePath = [gameType]() -> QString {
        switch (gameType) {
        case GameType::IW3: return Globals.pathIW3;
        case GameType::IW4: return Globals.pathIW4;
        case GameType::IW5: return Globals.pathIW5;
        }
        __debugbreak();
        return "";
    };

    const QString executable = getExecutableName();
    const QString pathStr = getGamePath() + "/" + executable;
    QFileInfo file(pathStr);

    if (!file.exists() || !file.isExecutable()) {
        qWarning() << executable << "not found or not executable at" << pathStr;
        return;
    }

    const auto getCurrentTreeWidget = [this, gameType]() {
        switch (gameType) {
        case GameType::IW3: return treeWidgetIW3;
        case GameType::IW4: return treeWidgetIW4;
        case GameType::IW5: return treeWidgetIW5;
        }
        return static_cast<QTreeWidget*>(nullptr);
    };

    auto* widget = getCurrentTreeWidget();
    if (!widget || !widget->currentItem() || !widget->currentItem()->parent())
        return;

    const QString currentSelectedText = widget->currentItem()->text(0);
    const QString currentSelectedPath = widget->currentItem()->data(0, Qt::UserRole).toString();
    const bool isUserMap = currentSelectedPath.contains("usermaps");

    if (isUserMap)
        qInfo() << "Exporting map" << currentSelectedText;
    else
        qInfo() << "Exporting zone" << currentSelectedText;

    const auto showH1WidgetForZone = [this](const QString& zone) {
        const QString sourceFile = Globals.pathH1 + "/zone_source/" + zone + ".csv";
        if (QFile(sourceFile).exists()) {
            populateListH1(treeWidgetH1, Globals.pathH1);
            ui.tabWidget->setCurrentWidget(ui.tabH1);
            auto items = treeWidgetH1->findItems(zone + ".csv", Qt::MatchExactly | Qt::MatchRecursive);
            if (!items.isEmpty()) {
                treeWidgetH1->setCurrentItem(items.first());
                treeWidgetH1->scrollToItem(treeWidgetH1->currentItem(), QAbstractItemView::PositionAtCenter);
                treeWidgetH1->setFocus();
            }
            updateVisibility();
        }
	};

    const auto dumpZone = [=](const QString& zone, const bool showH1Widget = false) {
        disableUiAndStoreState();

        QStringList arguments;
        arguments << "-silent" << "-dumpzone" << zone;

        QProcess* process = new QProcess(this);
        process->setProgram(pathStr);
        process->setArguments(arguments);
        process->setWorkingDirectory(file.absolutePath());
        process->setProcessChannelMode(QProcess::MergedChannels);

        connect(process, &QProcess::readyRead, [process]() {
            Funcs::Shared::readOutputFromProcess(process);
        });

        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int exitCode, QProcess::ExitStatus status) {
            Funcs::Shared::readOutputFromProcess(process);
            process->deleteLater();

            if (status != QProcess::NormalExit || exitCode != 0) {
                qCritical() << executable << "exited with error during zone export.";
                restoreUiState();
                return;
            }

            const QString dumpFolder = getGamePath() + "/dump/" + zone;
            const QString destFolder = Globals.pathH1 + "/zonetool/" + zone;
            QtUtils::moveDirectory(dumpFolder, destFolder);

            if (ui.convertGscCheckBox->isChecked()) {
                // fixup GSC...
            }

            if (ui.generateCsvCheckBox->isChecked()) {
                // generate CSV...
            }

            restoreUiState();

            if(showH1Widget)
                showH1WidgetForZone(zone);

            return;
        });

        process->start();
        if (!process->waitForStarted()) {
            qWarning() << "Failed to start process:" << executable;
            process->deleteLater();
            restoreUiState();
            return;
        }
    };

    if (!isUserMap) {
        const QString zone = QFileInfo(currentSelectedText).completeBaseName();
        dumpZone(zone, true);
    }
    else {
        // TODO: Handle usermap export logic
    }
}

void H1ModTools::on_buildZoneButton_clicked() {
    const QString executable = "zonetool.exe";
    const QString pathStr = Globals.pathH1 + "/" + executable;
    QFileInfo file(pathStr);

    if (!file.exists() || !file.isExecutable()) {
        qWarning() << executable << "not found or not executable at" << pathStr;
        return;
    }

    if (treeWidgetH1->currentItem() == nullptr || treeWidgetH1->currentItem()->parent() == nullptr)
        return;

    const QString currentSelectedText = treeWidgetH1->currentItem()->text(0);
    const QString currentSelectedZone = QFileInfo(currentSelectedText).completeBaseName();

    const auto isMap = Funcs::H1::isMap(currentSelectedZone) || Funcs::H1::isMapLoad(currentSelectedZone);

    QStringList arguments;
    arguments << "-buildzone" << currentSelectedZone;

    qDebug() << "Building zone" << currentSelectedZone;

    disableUiAndStoreState();

    QProcess* process = new QProcess(this);
    process->setProgram(pathStr);
    process->setArguments(arguments);
    process->setWorkingDirectory(file.absolutePath());
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, &QProcess::readyRead, [process]() {
        Funcs::Shared::readOutputFromProcess(process);
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [this, process, executable, isMap, currentSelectedZone](int exitCode, QProcess::ExitStatus status) {
        // Flush any remaining output
        Funcs::Shared::readOutputFromProcess(process);

        qDebug() << executable << "exited with code" << exitCode
            << (status == QProcess::NormalExit ? "(NormalExit)" : "(CrashExit)");
        process->deleteLater();

        if (exitCode != QProcess::NormalExit)
        {
            qCritical() << "Failed to build zone" << currentSelectedZone;
            restoreUiState();
            return;
        }

        // Move map files to usermaps
        if (isMap && !Funcs::H1::moveToUsermaps(currentSelectedZone)) {
            qWarning() << "Failed to move" << currentSelectedZone << "to usermaps";
        }
        else if (isMap) {
            qDebug() << "Moved" << currentSelectedZone << "to usermaps";
        }

        qDebug() << "Compiled zone" << currentSelectedZone;

        restoreUiState();
    });

    process->start();
    if (!process->waitForStarted()) {
        qWarning() << "Failed to start" << executable;
        process->deleteLater();

        restoreUiState();
        return;
    }
}

void H1ModTools::on_compileReflectionsButton_clicked() {
    const QString executable = "h1-mod_dev.exe";
    const QString pathStr = Globals.pathH1 + "/" + executable;
    QFileInfo file(pathStr);

    if (!file.exists() || !file.isExecutable()) {
        qWarning() << executable << "not found or not executable at" << pathStr;
        return;
    }

    if (treeWidgetH1->currentItem() == nullptr || treeWidgetH1->currentItem()->parent() == nullptr)
        return;

    const QString currentSelectedText = treeWidgetH1->currentItem()->text(0);
    const QString currentSelectedZone = QFileInfo(currentSelectedText).completeBaseName();

    // Compile reflections
    QStringList arguments;
    arguments << "-multiplayer"
        << "+set" << "r_reflectionProbeGenerate" << "1"
        << "+set" << "r_reflectionProbeGenerateExit" << "1"
        << "+map" << currentSelectedZone;

    qDebug() << "Generating reflection probes for zone" << currentSelectedZone;

    disableUiAndStoreState();

    QProcess* process = new QProcess(this);
    process->setProgram(pathStr);
    process->setArguments(arguments);
    process->setWorkingDirectory(file.absolutePath());
    //process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [this, process, executable, currentSelectedZone](int exitCode, QProcess::ExitStatus status) {
        qDebug() << executable << "exited with code" << exitCode
            << (status == QProcess::NormalExit ? "(NormalExit)" : "(CrashExit)");
        process->deleteLater();

        if (exitCode != QProcess::NormalExit)
        {
            qCritical() << "Failed to generate reflection probes for zone" << currentSelectedZone;
            restoreUiState();
            return;
        }

        if (Funcs::H1::moveReflectionProbes(currentSelectedZone))
            qDebug() << "Reflection probes successfully generated";

        restoreUiState();

        qDebug() << "Generated reflection probes for map" << currentSelectedZone;
        qInfo() << "Build your zone again to see the generated reflection probes";
    });

    process->start();
    if (!process->waitForStarted()) {
        qWarning() << "Failed to start" << executable;
        process->deleteLater();

        restoreUiState();
    }
}

void H1ModTools::on_runMapButton_clicked() {
    const QString executable = "h1-mod_dev.exe";
    const QString pathStr = Globals.pathH1 + "/" + executable;
    QFileInfo file(pathStr);

    if (!file.exists() || !file.isExecutable()) {
        qWarning() << executable << "not found or not executable at" << pathStr;
        return;
    }

    if (treeWidgetH1->currentItem() == nullptr || treeWidgetH1->currentItem()->parent() == nullptr)
        return;

    const QString currentSelectedText = treeWidgetH1->currentItem()->text(0);
    const QString currentSelectedZone = QFileInfo(currentSelectedText).completeBaseName();

    const bool isMP = currentSelectedZone.startsWith("mp_");

    // Run map
    QStringList arguments;
    arguments << (isMP ? "-multiplayer" : "-singleplayer");
    if (ui.cheatsCheckBox->isChecked())
        arguments << "+set" << "sv_cheats" << "1";
    if (ui.developerCheckBox->isChecked()) {
        arguments << "+set" << "developer" << "1";
        arguments << "+set" << "developer_script" << "1";
    }
    arguments << (ui.cheatsCheckBox->isChecked() ? "+devmap" : "+map") << currentSelectedZone;

    qDebug() << "Launching map" << currentSelectedZone;

    // Launch detached
    QString workingDir = file.absolutePath();
    if (!QProcess::startDetached(pathStr, arguments, workingDir)) {
        qWarning() << "Failed to launch detached process:" << pathStr;
    }
}

void H1ModTools::onOutputBufferContextMenu(const QPoint& pos) {
    QMenu* menu = ui.outputBuffer->createStandardContextMenu();
    menu->addSeparator();

    QAction* refreshAction = menu->addAction("Clear");
    QAction* selectedAction = menu->exec(ui.outputBuffer->mapToGlobal(pos));

    if (selectedAction == refreshAction) {
        ui.outputBuffer->clear();
    }

    delete menu;  // clean up
}

void H1ModTools::onTreeContextMenuRequested(const QPoint& pos) {
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

void H1ModTools::disableUiAndStoreState() {
    QList<QWidget*> widgets = {
        ui.buildZoneButton,
        ui.exportButton,
        ui.compileReflectionsButton,
        ui.runMapButton,
        ui.settingsButton,
        ui.tabWidget,
        ui.cheatsCheckBox,
        ui.developerCheckBox,
        ui.generateCsvCheckBox,
        ui.convertGscCheckBox,
    };

    m_uiEnabledStates.clear();
    for (QWidget* widget : widgets)
    {
        m_uiEnabledStates[widget] = widget->isEnabled();
        widget->setEnabled(false);
    }
}

void H1ModTools::restoreUiState() {
    for (auto it = m_uiEnabledStates.begin(); it != m_uiEnabledStates.end(); ++it)
    {
        it.key()->setEnabled(it.value());
    }
    m_uiEnabledStates.clear();
}