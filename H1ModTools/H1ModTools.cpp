#include "H1ModTools.h"

#include "SettingsDialog.h"

#include "GSC.h"
#include "MapEnts.h"
#include "CSVGenerator.h"

const QStringList languageFolders = {
        "english", "french", "german", "spanish",
        "japanese", "russian", "italian", "dlc"
};

namespace Funcs
{
    namespace Shared
    {
        bool replaceStringInFile(const QString& filePath, const QString& target, const QString& replacement)
        {
            QFile file(filePath);

            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qWarning() << "Failed to open file for reading:" << filePath;
                return false;
            }

            QTextStream in(&file);
            QString content = in.readAll();
            file.close();

            if (!content.contains(target)) {
                qDebug() << "Target string not found in file.";
                return false;
            }

            content.replace(target, replacement, Qt::CaseSensitive);

            if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                qWarning() << "Failed to open file for writing:" << filePath;
                return false;
            }

            QTextStream out(&file);
            out << content;
            file.close();

            return true;
        }

        static bool zoneExistsForDump(const QString& zone, GameType gameType)
        {
            // check each language folder
            QDir zoneDir(getGamePath(gameType) + "/zone/");
            if (!zoneDir.exists()) return false;
            for (const QString& lang : languageFolders) {
                QDir langDir(zoneDir.filePath(lang));
                if (langDir.exists() && QFile::exists(langDir.filePath(zone + ".ff"))) {
                    return true;
                }
            }
            return false;
        }

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
    
    namespace H1
    {
        bool isMapLoad(const QString& name)
        {
            return Funcs::Shared::isMapLoad(name);
        }

        bool isMap(const QString& name)
        {
            return Funcs::Shared::isMap(name, GameType::H1);
        }

        bool isMpMap(const QString& name)
        {
            return Funcs::Shared::isMpMap(name, GameType::H1);
        }

        bool moveToUsermaps(const QString& zone)
        {
            const auto tryMoveFile = [](const QString& folder, const QString& zoneName, const QString& ext = "") -> bool {
                QString sourcePath = Globals.pathH1 + "/zone/" + zoneName + ext;
                QString targetFolder = Globals.pathH1 + "/usermaps/" + folder;

                QFile sourceFile(sourcePath);
                if (!sourceFile.exists())
                    return false;

                QDir dir;
                if (!dir.exists(targetFolder)) {
                    if (!dir.mkpath(targetFolder))
                        return false;
                }

                QString targetPath = targetFolder + "/" + zoneName + ext;
                if (QFile::exists(targetPath))
                    QFile::remove(targetPath); // Overwrite if exists

                return sourceFile.rename(targetPath);
            };

            if (isMapLoad(zone)) {
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
            const QStringList h1ImageFiles = targetDir.entryList(QStringList() << "_reflection_probe*.h1Image", QDir::Files);
            for (const QString& fileName : h1ImageFiles) {
                const QString filePath = targetFolder + fileName;

                if (!QFile::remove(filePath)) {
                    qWarning() << "Failed to delete file:" << filePath;
                    return false;
                }
            }

            return true;
        }

        bool zoneExistsOnDisk(const QString& zone)
        {
            QDir zoneDir(Globals.pathH1 + "/zone/");
            if (!zoneDir.exists()) {
                return false;
            }

            for (const QString& lang : languageFolders) {
                QDir langDir(zoneDir.filePath(lang));
                if (langDir.exists() && QFile::exists(langDir.filePath(zone + ".ff"))) {
                    return true;
                }
            }

            // check if it is a usermap
            QDir usermapsDir(Globals.pathH1 + "/usermaps/" + zone);
            return usermapsDir.exists() && QFile::exists(usermapsDir.filePath(zone + ".ff"));
        }
    }

    namespace IW3
    {
        bool isMapSource(const QString& name)
        {
            const QDir map_source_dir(Globals.pathIW3 + "/map_source/");
            return map_source_dir.exists() && QFile::exists(map_source_dir.filePath(name));
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

void H1ModTools::on_settingsButton_clicked()
{
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        populateLists();
    }
}

QString findGameFolderInAllDrives(const QString& folderName)
{
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

void generateSettings(QWidget* parent)
{
    Globals.pathH1 = findGameFolderInAllDrives("Call of Duty Modern Warfare Remastered");
    Globals.pathIW3 = findGameFolderInAllDrives("Call of Duty 4");
    Globals.pathIW4 = findGameFolderInAllDrives("Call of Duty Modern Warfare 2");
    Globals.pathIW5 = findGameFolderInAllDrives("Call of Duty Modern Warfare 3");
    Globals.h1Executable = "h1-mod_dev.exe";
    
    saveGlobalsToJson(parent);

    qDebug() << "Generated settings.json with auto-discovered paths.";
}

void H1ModTools::loadGlobals()
{
    if (!loadGlobalsFromJson()) {
        // No settings found, let's generate!
        generateSettings(this);
    }
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

    // callback for current item changed on H1 tree
    connect(treeWidgetH1, &QTreeWidget::currentItemChanged, this,
        [this](QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/) {
        if (current && current->parent() != nullptr) {  // Ignore root items
            const auto name = QFileInfo(current->text(0)).completeBaseName();
            qDebug() << "Current H1 item:" << name;

            const bool isMap = Funcs::H1::isMap(name);
            ui.compileReflectionsButton->setEnabled(isMap);
            ui.runMapButton->setEnabled(isMap);
			ui.mapRunCmdsText->setEnabled(isMap);
            ui.cheatsCheckBox->setEnabled(isMap);
            ui.developerCheckBox->setEnabled(isMap);
        }
    });

    // callback for current item changed on IW3 tree
    connect(treeWidgetIW3, &QTreeWidget::currentItemChanged, this,
        [this](QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/) {
        if (current && current->parent() != nullptr) {  // Ignore root items
            const auto raw_name = QFileInfo(current->text(0)); // get this as string
            qDebug() << "Current IW3 item:" << raw_name.completeBaseName();

            auto is_map_source = Funcs::IW3::isMapSource(raw_name.fileName());

            updateExportButtonStates(true, false);
            updateMapButtonStates(true, !is_map_source);
        }
    });

    // callback for current item changed on IW4 tree
    connect(treeWidgetIW4, &QTreeWidget::currentItemChanged, this,
        [this](QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/) {
        if (current && current->parent() != nullptr) {  // Ignore root items
            const auto raw_name = QFileInfo(current->text(0)); // get this as string
            qDebug() << "Current IW4 item:" << raw_name.completeBaseName();
        }
    });

    // callback for current item changed on IW5 tree
    connect(treeWidgetIW5, &QTreeWidget::currentItemChanged, this,
        [this](QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/) {
        if (current && current->parent() != nullptr) {  // Ignore root items
            const auto raw_name = QFileInfo(current->text(0)); // get this as string
            qDebug() << "Current IW5 item:" << raw_name.completeBaseName();
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

    // map_source
    QDir map_source_dir(path + "/map_source");
    if (map_source_dir.exists())
    {
        auto* map_source_root = new QTreeWidgetItem(tree);
        map_source_root->setText(0, "map_source");
        
        auto boldFont = map_source_root->font(0);
        boldFont.setBold(true);
        map_source_root->setFont(0, boldFont);
        
        map_source_root->setFlags(Qt::ItemIsEnabled);

        auto map_sources = map_source_dir.entryInfoList({ "*.map" }, QDir::Files);
        for (const auto& fileInfo : map_sources) {
            auto* item = new QTreeWidgetItem(map_source_root);
            item->setText(0, fileInfo.fileName());
            item->setData(0, Qt::UserRole, fileInfo.filePath());
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
    const auto gameType = getCurrentGameType();
    const auto isH1Selected = gameType == H1;

    const auto canCompile = isH1Selected;

    updateMapButtonStates(gameType == IW3, true);
    updateExportButtonStates(isH1Selected == false, false);

    ui.buildZoneButton->setVisible(canCompile);
    ui.compileReflectionsButton->setVisible(canCompile);
    ui.runMapButton->setVisible(canCompile);
	ui.mapRunCmdsText->setVisible(canCompile);
    ui.cheatsCheckBox->setVisible(canCompile);
    ui.developerCheckBox->setVisible(canCompile);

    // refresh list for tab
    switch (gameType)
    {
    case H1:
        populateListH1(treeWidgetH1, Globals.pathH1);
        break;
    case IW3:
        populateListIW(treeWidgetIW3, Globals.pathIW3);
        break;
    case IW4:
        populateListIW(treeWidgetIW4, Globals.pathIW4);
        break;
    case IW5:
        populateListIW(treeWidgetIW5, Globals.pathIW5);
        break;
    default:
        break;
    }
}

GameType H1ModTools::getCurrentGameType()
{
    if (ui.tabWidget->currentWidget() == ui.tabIW3) return IW3;
    if (ui.tabWidget->currentWidget() == ui.tabIW4) return IW4;
    if (ui.tabWidget->currentWidget() == ui.tabIW5) return IW5;
    return H1;
}

void H1ModTools::on_buildAndExportButton_clicked()
{
    if (!treeWidgetIW3->currentItem() || !treeWidgetIW3->currentItem()->parent())
    {
        qWarning() << "No zone selected.";
        return;
    }

    const auto zonetoolIW3 = Globals.pathIW3 + "/zonetool_iw3.exe";
    if (!QFile::exists(zonetoolIW3))
    {
        qCritical() << "zonetool_iw3.exe not found to dump IW3 zone for H1.";
        restoreUiState();
        return;
    }

    const QString mapName = QFileInfo(treeWidgetIW3->currentItem()->text(0)).completeBaseName();

    // TODO: add all the other options here later
    QStringList lightFlags;
    if (ui.fastCheckBox->isChecked())
        lightFlags << "-fast";
    if (ui.extraCheckBox->isChecked())
        lightFlags << "-extra";
    if (ui.verboseCheckBox->isChecked())
        lightFlags << "-verbose";
    if (ui.modelShadowCheckBox->isChecked())
        lightFlags << "-modelshadow";

    const auto lightOptions = lightFlags.join(' ');

    disableUiAndStoreState();

    // build BSP, reflections, & IW3 fastfile using their mod tools
    // this also handles the rest of the entire process
    compileIW3Map(mapName, Globals.pathIW3, lightOptions);
}

void H1ModTools::on_exportButton_clicked()
{
    exportSelection();
}

void H1ModTools::on_buildZoneButton_clicked()
{
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
    }
}

void H1ModTools::on_compileReflectionsButton_clicked()
{
    const QString executable = Globals.h1Executable;
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

    // check if the zone actually exists
    if (!Funcs::H1::zoneExistsOnDisk(currentSelectedZone))
    {
        QApplication::beep();
        qCritical() << "You must build the zone before you can use Compile Reflections for " << currentSelectedZone;
        restoreUiState();
        return;
    }

    // Compile reflections
    QStringList arguments;
    arguments << "-multiplayer"
        << "+set" << "r_reflectionProbeGenerate" << "1"
        << "+set" << "r_reflectionProbeGenerateExit" << "1";

    if (ui.mapRunCmdsText) {
        const QStringList cmds = ui.mapRunCmdsText->toPlainText().split(' ', Qt::SkipEmptyParts);
        for (const QString& cmd : cmds) {
            if (!cmd.trimmed().isEmpty()) {
                arguments << cmd.trimmed();
            }
        }
    }
    
    arguments << "+map" << currentSelectedZone;

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

void H1ModTools::on_runMapButton_clicked()
{
    const QString executable = Globals.h1Executable;
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

    // check if the zone actually exists
    if (!Funcs::H1::zoneExistsOnDisk(currentSelectedZone))
    {
        QApplication::beep();
        qCritical() << "You must build the zone before you can use Run Map for " << currentSelectedZone;
        restoreUiState();
        return;
    }

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

    if (ui.mapRunCmdsText) {
        const QStringList cmds = ui.mapRunCmdsText->toPlainText().split(' ', Qt::SkipEmptyParts);
        for (const QString& cmd : cmds) {
            if (!cmd.trimmed().isEmpty()) {
                arguments << cmd.trimmed();
            }
        }
    }

    arguments << (ui.cheatsCheckBox->isChecked() ? "+devmap" : "+map") << currentSelectedZone;

    qDebug() << "Launching map" << currentSelectedZone;

    // Launch detached
    QString workingDir = file.absolutePath();
    if (!QProcess::startDetached(pathStr, arguments, workingDir)) {
        qWarning() << "Failed to launch detached process:" << pathStr;
    }
}

void H1ModTools::onOutputBufferContextMenu(const QPoint& pos)
{
    QMenu* menu = ui.outputBuffer->createStandardContextMenu();
    menu->addSeparator();

    QAction* refreshAction = menu->addAction("Clear");
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



void convertMp3ToFlacForFile(const QFileInfo& fileInfo)
{
    QString path = fileInfo.absoluteFilePath();
    QStringList arguments;
    arguments << path;

    int exitCode = QProcess::execute("static/tools/sound/convert_flac.bat", arguments);
    if (exitCode != 0)
    {
        qWarning() << "Failed to convert:" << path << "(exit code:" << exitCode << ")";
    }
    else
    {
        qDebug() << "Converted:" << path;
    }
}

void convertMp3ToFlacForFolder(const QString& folder)
{
    QDir dir(folder);
    if (!dir.exists()) {
        qWarning() << "Folder does not exist:" << folder;
        return;
    }

    // Filter *.mp3 files
    QStringList filters;
    filters << "*.mp3";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    for (const auto& file : files)
    {
        convertMp3ToFlacForFile(file);
    }
}

void dumpMainIwds()
{
    // we need to dump all the .iwd files from main to zonetool_assets/<game>
}

void dumpCommonZones()
{
    // we need to dump 
    // - common_mp
    // - localized_common_mp
    // and move the dumped folders to zonetool_assets/<game>
}

void dumpZoneToolAssets()
{
    // we need to check if zonetool_assets/<game> exists, if not, let's create it.
}

// export
void H1ModTools::exportSelection()
{
    const auto gameType = getCurrentGameType();
    
    static const auto getExecutableName = [gameType]() -> QString {
        switch (gameType)
        {
        case IW3: return "zonetool_iw3.exe";
        case IW4: return "zonetool_iw4.exe";
        case IW5: return "zonetool_iw5.exe";
        default:
            __debugbreak();
            return "";
        }
    };

    const QString executable = getExecutableName();
    const QString pathStr = Funcs::Shared::getGamePath(gameType) + "/" + executable;
    QFileInfo file(pathStr);

    if (!file.exists() || !file.isExecutable()) {
        qWarning() << executable << "not found or not executable at" << pathStr;
        return;
    }

    const auto getCurrentTreeWidget = [this, gameType]() {
        switch (gameType) {
        case IW3: return treeWidgetIW3;
        case IW4: return treeWidgetIW4;
        case IW5: return treeWidgetIW5;
        default:
            return static_cast<QTreeWidget*>(nullptr);
        }
    };

    auto* widget = getCurrentTreeWidget();
    if (!widget || !widget->currentItem() || !widget->currentItem()->parent())
        return;

    const QString currentSelectedText = widget->currentItem()->text(0);
    const QString currentSelectedPath = widget->currentItem()->data(0, Qt::UserRole).toString();
    const bool isUserMap = currentSelectedPath.contains("usermaps");

    // Add a check for map source exporting
    if (gameType == GameType::IW3 && Funcs::IW3::isMapSource(QFileInfo(currentSelectedText).fileName())) {
        if (!Funcs::Shared::zoneExistsForDump(QFileInfo(currentSelectedText).completeBaseName(), gameType)) {
            qWarning() << "Map source not built yet for" << currentSelectedText;
            return;
        }
    }

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
            else
            {
                ui.tabWidget->setCurrentWidget(ui.tabIW3);
            }
        }
	};

    const auto dumpZone = [=](const QString& zone, const bool showH1Widget = false) {
        disableUiAndStoreState();

        QStringList arguments;
        arguments 
            << "-silent" 
            << "-dumpzone" << zone;

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

            const QString dumpFolder = Funcs::Shared::getGamePath(gameType) + "/dump/" + zone;
            const QString destFolder = Globals.pathH1 + "/zonetool/" + zone;
            QtUtils::moveDirectory(dumpFolder, destFolder);

            if (ui.convertGscCheckBox->isChecked()) {
                // Copy template files here, maybe need to change this later
				const auto isMap = Funcs::H1::isMap(zone);
                if (isMap)
                {
                    const auto isMpMap = Funcs::H1::isMpMap(zone);

                    const QString mapsPrefix = isMpMap ? "maps/mp" : "maps";

                    auto mapEntsRead = MapEntsReader(QString("%1/zonetool/%2/%3/%2.d3dbsp.ents").arg(Globals.pathH1, zone, mapsPrefix));

                    CopyGSCFiles(destFolder); // Copy default GSC files to the zone folder
                    ConvertGSCFiles(destFolder, { // Convert GSC files to H1 format
                        .hasDestructibles = !mapEntsRead.getDestructibles().empty(), 
                        .hasAnimatedModels = !mapEntsRead.getAnimatedModels().empty(), 
                        .isMpMap = isMpMap});

                    const auto addTemplateFile = [zone](const QString& templatePath, const QString& destFolder, const QString& destFile)
                    {
                        if (!QFile::exists(destFolder + destFile)) {
                            QtUtils::copyFile(templatePath, destFolder + destFile);
                            QFile::rename(destFolder + QFile(templatePath).fileName(), destFolder + destFile);

							Funcs::Shared::replaceStringInFile(destFolder + destFile, "mapname", zone);

                            return true;
                        }
                        return false;
                    };

                    addTemplateFile("static/templates/vision.vision", destFolder + "/vision/", zone + ".vision");
                    addTemplateFile("static/templates/_art.gsc", destFolder + "/maps/createart/", zone + "_art.gsc");
                    addTemplateFile("static/templates/_fog.gsc", destFolder + "/maps/createart/", zone + "_fog.gsc");
                    addTemplateFile("static/templates/_fog_hdr.gsc", destFolder + "/maps/createart/", zone + "_fog_hdr.gsc");
                    addTemplateFile("static/templates/_lightsets.csv", destFolder + "/maps/createart/", zone + "_lightsets.csv");
                    addTemplateFile("static/templates/_lightsets_hdr.csv", destFolder + "/maps/createart/", zone + "_lightsets_hdr.csv");

                    if (!isMpMap)
                    {
                        Funcs::Shared::replaceStringInFile(destFolder + "/maps/createart/" + zone + "_fog.gsc", "maps\\mp\\_art::create_vision_set_fog", "maps\\_utility::create_vision_set_fog");
                        Funcs::Shared::replaceStringInFile(destFolder + "/maps/createart/" + zone + "_fog_hdr.gsc", "maps\\mp\\_art::create_vision_set_fog", "maps\\_utility::create_vision_set_fog");
                    }

                    // we need to add static/botpathways/ and copy the one for the map if it exists to destFolder/<mapPrefix>
                }
                else
                {
                    ConvertGSCFiles(destFolder, {.isMpMap = Funcs::H1::isMpMap(zone) }); // Convert GSC files to H1 format
                }

                // load <map>.iwd and dump images and sounds folder to destFolder
                // we need to load dumped csv and get all the image references and try to get them from the raw/images folder

                convertMp3ToFlacForFolder(destFolder + "/sound");
                // move converted sounds to loaded_sound
            }

            if (ui.generateCsvCheckBox->isChecked()) {
                generateCSV(zone, destFolder, Funcs::H1::isMpMap(zone), gameType);
            }

            restoreUiState();

            if (showH1Widget)
                showH1WidgetForZone(zone);
        });

        process->start();
        if (!process->waitForStarted()) {
            qWarning() << "Failed to start process:" << executable;
            process->deleteLater();
            restoreUiState();
        }
    };

    dumpZoneToolAssets();

    if (!isUserMap) {
        // move map.ff and map_load.ff to zone/english
        const QString zone = QFileInfo(currentSelectedText).completeBaseName();
        dumpZone(zone, true);
        // dump map load too...
    }
    else {
        const QString zone = QFileInfo(currentSelectedText).completeBaseName();
        dumpZone(zone, true);
    }
}

// used for IW3 map source building inside H1 mod tools
void H1ModTools::buildIW3MapFastfile(const QString& mapName, const QString& cod4Dir)
{
    const auto linkerPath = cod4Dir + "/bin/linker_pc.exe";
    const auto zoneSourceDir = cod4Dir + "/zone_source";
    const auto zoneDir = cod4Dir + "/zone";

    if (!QFile::exists(linkerPath)) {
        qCritical() << "Missing linker_pc.exe at" << linkerPath;
        restoreUiState();
        return;
    }

    const auto hasMapCsv = QFile::exists(zoneSourceDir + "/" + mapName + "_load.csv");
    if (!hasMapCsv)
    {
        qCritical() << "Failed to find map CSV for map" << mapName;
        restoreUiState();
        return;
    }

    const auto hasLoadCsv = QFile::exists(zoneSourceDir + "/" + mapName + "_load.csv");

    QStringList fastfiles;
    fastfiles << mapName;

    // load csv not required, but warn the user
    if (hasLoadCsv)
    {
        fastfiles << (mapName + "_load");
    }
    else
    {
        qWarning() << "Failed to find load CSV for map" << mapName;
    }

    QProcess* linkerProc = new QProcess();
    linkerProc->setProgram(linkerPath);
    linkerProc->setWorkingDirectory(zoneSourceDir);
    linkerProc->setProcessChannelMode(QProcess::MergedChannels);

    QStringList args;
    args << "-language english" << fastfiles;

    linkerProc->setArguments(args);

    QObject::connect(linkerProc, &QProcess::readyRead, [=]() {
        Funcs::Shared::readOutputFromProcess(linkerProc);
    });

    QObject::connect(linkerProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus status)
        {
            Funcs::Shared::readOutputFromProcess(linkerProc);
            linkerProc->deleteLater();

            if (exitCode != 0 || status != QProcess::NormalExit) {
                qCritical() << "Fastfile build failed for" << mapName;
                restoreUiState();
                return;
            }

            qDebug() << "Fastfiles built successfully for" << mapName;
            restoreUiState();

            // TODO: move to localized folder if needed (non-english)
            /*
            const auto engFolder = zoneDir + "/english";
            const auto langFolder = zoneDir + "/english"; // change if other languages supported

            for (const QString& zone : fastfiles) {
                const auto src = engFolder + "/" + zone + ".ff";
                const auto dst = langFolder + "/" + zone + ".ff";
                if (QFile::exists(src)) {
                    QFile::remove(dst);
                    QFile::rename(src, dst);
                }
            }
            */

            // TODO: dump the IW3 zone for H1, and move to zonetool
            // or just simply show the new .ff that can be exported with the export button
            //export_map();
        });

    qDebug() << "Building fastfiles for" << fastfiles.join(", ");
    linkerProc->start();
}

void H1ModTools::compileIW3MapReflections(const QString& mapName, const QString& cod4Dir)
{
    const auto isMP = mapName.startsWith("mp_");
    const auto toolExe = isMP ? "mp_tool.exe" : "sp_tool.exe";
    const auto toolPath = cod4Dir + "/" + toolExe;

    if (!QFile::exists(toolPath)) {
        qCritical() << "Missing executable:" << toolPath;
        restoreUiState();
        return;
    }

    QProcess* proc = new QProcess();
    proc->setProgram(toolPath);
    proc->setWorkingDirectory(cod4Dir);
    proc->setProcessChannelMode(QProcess::MergedChannels);

    QStringList args = {
        "+set", "r_fullscreen", "0",
        "+set", "loc_warnings", "0",
        "+set", "developer", "1",
        "+set", "developer_script", "1",
        "+set", "logfile", "2",
        "+set", "thereisacow", "1337",
        "+set", "sv_pure", "0",
        "+set", "useFastFile", "0",
        "+set", "com_introplayed", "1",
        "+set", "ui_autoContinue", "1",
        "+set", "r_reflectionProbeGenerateExit", "1",
        "+set", "com_hunkMegs", "512",
        "+set", "r_reflectionProbeRegenerateAll", "1",
        "+set", "r_dof_enable", "0",
        "+set", "r_zFeather", "1",
        "+set", "sys_smp_allowed", "0",
        "+set", "r_reflectionProbeGenerate", "1",
        "+devmap", mapName
    };

    proc->setArguments(args);

    connect(proc, &QProcess::readyRead, [=]() {
        Funcs::Shared::readOutputFromProcess(proc);
    });

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus status)
        {
            Funcs::Shared::readOutputFromProcess(proc);
            proc->deleteLater();

            if (exitCode != 0 || status != QProcess::NormalExit) {
                qCritical() << "Reflection generation failed.";
                restoreUiState();
                return;
            }

            qDebug() << "Reflection probe generation complete for" << mapName;

            // finally, build the IW3 fastfile
            buildIW3MapFastfile(mapName, cod4Dir);
        });

    qDebug() << "Launching" << toolExe << "to generate reflections for" << mapName;
    proc->start();
}

void H1ModTools::compileIW3Map(const QString& mapName, const QString& cod4Dir, const QString& lightOptions)
{
    const auto bsppath = cod4Dir + "/raw/maps" + (mapName.startsWith("mp_") ? "/mp" : "");
    const auto mapSourcePath = cod4Dir + "/map_source";
    const auto treePath = cod4Dir + "/";
    const auto binPath = cod4Dir + "/bin/";

    const auto cod4mapPath = binPath + "cod4map.exe";
    const auto cod4radPath = binPath + "cod4rad.exe";
    const auto spToolPath = cod4Dir + "/sp_tool.exe";

    if (!QFile::exists(cod4mapPath) || !QFile::exists(cod4radPath)) {
        qCritical() << "Missing cod4map or cod4rad.";
        restoreUiState();
        return;
    }

    if (!QDir().mkpath(bsppath)) {
        qCritical() << "Couldn't create path:" << bsppath;
        restoreUiState();
        return;
    }

    // ---- copy source map to bsp path temporarily 
    const auto srcMap = mapSourcePath + "/" + mapName + ".map";
    const auto dstMap = bsppath + "/" + mapName + ".map";
    QFile::remove(dstMap); // overwrite
    QFile::copy(srcMap, dstMap);

    // ---- compile BSP
    qDebug() << "Compiling BSP...";
    auto* cod4mapProc = new QProcess();
    cod4mapProc->setProgram(cod4mapPath);
    cod4mapProc->setWorkingDirectory(cod4Dir);
    cod4mapProc->setProcessChannelMode(QProcess::MergedChannels);
    cod4mapProc->setArguments(QStringList{ "-platform", "pc", "-loadFrom", srcMap, dstMap }); // TODO: add bsp options later

    connect(cod4mapProc, &QProcess::readyRead, [cod4mapProc]() {
        Funcs::Shared::readOutputFromProcess(cod4mapProc);
    });

    connect(cod4mapProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus status) {
            Funcs::Shared::readOutputFromProcess(cod4mapProc);
            cod4mapProc->deleteLater();

            if (exitCode != 0 || status != QProcess::NormalExit) {
                qCritical() << "BSP compile failed.";
                restoreUiState();
                return;
            }

            // ---- compile light
            qDebug() << "Compiling lighting...";
            const auto srcGrid = mapSourcePath + "/" + mapName + ".grid";
            const auto dstGrid = bsppath + "/" + mapName + ".grid";
            if (QFile::exists(srcGrid)) {
                QFile::remove(dstGrid);
                QFile::copy(srcGrid, dstGrid);
            }

            auto* cod4radProc = new QProcess();
            cod4radProc->setProgram(cod4radPath);
            cod4radProc->setWorkingDirectory(cod4Dir);
            cod4radProc->setProcessChannelMode(QProcess::MergedChannels);

            QStringList radArgs{ "-platform", "pc" };
            if (!lightOptions.trimmed().isEmpty())
                radArgs.append(lightOptions.trimmed().split(' '));
            radArgs << bsppath + "/" + mapName;

            cod4radProc->setArguments(radArgs);

            connect(cod4radProc, &QProcess::readyRead, [cod4radProc]() {
                Funcs::Shared::readOutputFromProcess(cod4radProc);
            });

            connect(cod4radProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [=](int lightExitCode, QProcess::ExitStatus lightStatus) {
                    Funcs::Shared::readOutputFromProcess(cod4radProc);
                    cod4radProc->deleteLater();

                    if (lightExitCode != 0 || lightStatus != QProcess::NormalExit) {
                        qCritical() << "Lighting compile failed.";
                        restoreUiState();
                        return;
                    }

                    // ---- cleanup
                    for (const QString& ext : { ".map", ".d3dprt", ".d3dpoly", ".vclog", ".grid" })
                        QFile::remove(bsppath + "/" + mapName + ext);

                    // ----move .lin if exists
                    const QString linSrc = bsppath + "/" + mapName + ".lin";
                    const QString linDst = mapSourcePath + "/" + mapName + ".lin";
                    if (QFile::exists(linSrc)) {
                        QFile::remove(linDst);
                        QFile::rename(linSrc, linDst);
                    }

                    // TODO: connect paths later with sp tool?

                    qDebug() << mapName << "has been compiled for IW3";

                    // now, we need to generate the reflections for the map
                    compileIW3MapReflections(mapName, cod4Dir);
                });

            cod4radProc->start();
        });

    cod4mapProc->start();
}

// ui state
void H1ModTools::updateMapButtonStates(const bool is_visible, const bool is_disabled)
{
    static const QList<QWidget*> map_build_widgets = {
        ui.buildAndExportButton,
        ui.fastCheckBox,
        ui.extraCheckBox,
        ui.verboseCheckBox,
        ui.modelShadowCheckBox
    };

    for (auto* widget : map_build_widgets)
    {
        widget->setVisible(is_visible);
        widget->setDisabled(is_disabled);
    }
}

void H1ModTools::updateExportButtonStates(const bool is_visible, const bool is_disabled)
{
    static const QList<QWidget*> map_build_widgets = {
        ui.exportButton,
        ui.generateCsvCheckBox,
        ui.convertGscCheckBox
    };

    for (auto* widget : map_build_widgets)
    {
        widget->setVisible(is_visible);
        widget->setDisabled(is_disabled);
    }
}

void H1ModTools::disableUiAndStoreState() {
    QList<QWidget*> widgets = {
        ui.buildZoneButton,
        ui.exportButton,
        ui.compileReflectionsButton,
        ui.runMapButton,
        ui.mapRunCmdsText,
        ui.settingsButton,
        ui.tabWidget,
        ui.cheatsCheckBox,
        ui.developerCheckBox,
        ui.generateCsvCheckBox,
        ui.convertGscCheckBox,
        ui.buildAndExportButton,
        ui.fastCheckBox,
        ui.extraCheckBox,
        ui.verboseCheckBox,
        ui.modelShadowCheckBox
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