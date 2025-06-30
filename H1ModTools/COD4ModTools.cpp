#define _CRT_SECURE_NO_WARNINGS

#include <functional>
#include <iomanip>
#include <sstream>
#include <utility>

#include <Windows.h>

#include "Globals.h"
#include "COD4ModTools.h"

const char* gLanguages[] = { "english" };
enum mlItemType
{
	ML_ITEM_UNKNOWN,
	ML_ITEM_MAP
};

mlBuildThread::mlBuildThread(const QList<QPair<QString, QStringList>>& Commands, bool IgnoreErrors)
	: mCommands(Commands), mSuccess(false), mCancel(false), mIgnoreErrors(IgnoreErrors)
{
}

void mlBuildThread::run()
{
	bool Success = true;

	for (const QPair<QString, QStringList>& Command : mCommands)
	{
		QProcess* Process = new QProcess();
		connect(Process, SIGNAL(finished(int)), Process, SLOT(deleteLater()));
		Process->setWorkingDirectory(QFileInfo(Command.first).absolutePath());
		Process->setProcessChannelMode(QProcess::MergedChannels);

		emit OutputReady(Command.first + ' ' + Command.second.join(' ') + "\n");

		Process->start(Command.first, Command.second);
		for (;;)
		{
			Sleep(100);

			if (Process->waitForReadyRead(0))
				emit OutputReady(Process->readAll());

			QProcess::ProcessState State = Process->state();
			if (State == QProcess::NotRunning)
				break;

			if (mCancel)
				Process->kill();
		}

		if (Process->exitStatus() != QProcess::NormalExit)
			return;

		if (Process->exitCode() != 0)
		{
			Success = false;
			if (!mIgnoreErrors)
				return;
		}
	}

	mSuccess = Success;
}

mlConvertThread::mlConvertThread(QStringList& Files, QString& OutputDir, bool IgnoreErrors, bool OverwriteFiles)
	: mFiles(Files), mOutputDir(OutputDir), mSuccess(false), mCancel(false), mIgnoreErrors(IgnoreErrors), mOverwrite(OverwriteFiles)
{
}

void mlConvertThread::run()
{

}

COD4ModTools::COD4ModTools()
{
	QSettings Settings;

	loadGlobalsFromJson();

	mBuildThread = NULL;
	mBuildLanguage = Settings.value("BuildLanguage", "english").toString();
	mTreyarchTheme = Settings.value("useAuroraTheme", false).toBool();

	// Qt prefers '/' over '\\'
	mGamePath = Globals.pathIW3.replace('\\', '/');
	mToolsPath = mGamePath;

	UpdateTheme();

	setWindowIcon(QIcon(":/resources/ModLauncher.png"));
	setWindowTitle("Call of Duty 4 Mod Tools Launcher");

	resize(1024, 768);

	CreateActions();
	CreateMenu();
	CreateToolBar();

	QSplitter* CentralWidget = new QSplitter();
	CentralWidget->setOrientation(Qt::Vertical);

	QWidget* TopWidget = new QWidget();
	CentralWidget->addWidget(TopWidget);

	QHBoxLayout* TopLayout = new QHBoxLayout(TopWidget);
	TopWidget->setLayout(TopLayout);

	mFileListWidget = new QTreeWidget();
	mFileListWidget->setHeaderHidden(true);
	mFileListWidget->setUniformRowHeights(true);
	mFileListWidget->setRootIsDecorated(false);
	mFileListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	TopLayout->addWidget(mFileListWidget);

	connect(mFileListWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ContextMenuRequested()));

	QVBoxLayout* ActionsLayout = new QVBoxLayout();
	TopLayout->addLayout(ActionsLayout);

	QHBoxLayout* CompileLayout = new QHBoxLayout();
	ActionsLayout->addLayout(CompileLayout);

	mCompileEnabledWidget = new QCheckBox("Compile");
	CompileLayout->addWidget(mCompileEnabledWidget);

	mCompileModeWidget = new QComboBox();
	mCompileModeWidget->addItems(QStringList() << "Ents" << "Full");
	mCompileModeWidget->setCurrentIndex(1);
	CompileLayout->addWidget(mCompileModeWidget);

	QHBoxLayout* LightLayout = new QHBoxLayout();
	ActionsLayout->addLayout(LightLayout);

	mLightEnabledWidget = new QCheckBox("Light");
	LightLayout->addWidget(mLightEnabledWidget);

	mLightQualityWidget = new QComboBox();
	mLightQualityWidget->addItems(QStringList() << "Low" << "Medium" << "High");
	mLightQualityWidget->setCurrentIndex(1);
	mLightQualityWidget->setMinimumWidth(64); // Fix for "Medium" being cut off in the dark theme
	LightLayout->addWidget(mLightQualityWidget);

	mLinkEnabledWidget = new QCheckBox("Link");
	ActionsLayout->addWidget(mLinkEnabledWidget);

	mRunEnabledWidget = new QCheckBox("Run");
	ActionsLayout->addWidget(mRunEnabledWidget);

	mRunOptionsWidget = new QLineEdit();
	mRunOptionsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	ActionsLayout->addWidget(mRunOptionsWidget);

	mBuildButton = new QPushButton("Build");
	connect(mBuildButton, SIGNAL(clicked()), mActionEditBuild, SLOT(trigger()));
	ActionsLayout->addWidget(mBuildButton);

	mLogButton = new QPushButton("Save Log");
	connect(mLogButton, SIGNAL(clicked()), this, SLOT(OnSaveLog()));
	ActionsLayout->addWidget(mLogButton);

	mIgnoreErrorsWidget = new QCheckBox("Ignore Errors");
	ActionsLayout->addWidget(mIgnoreErrorsWidget);

	ActionsLayout->addStretch(1);

	mOutputWidget = new QPlainTextEdit(this);
	mOutputWidget->setReadOnly(true);
	CentralWidget->addWidget(mOutputWidget);

	setCentralWidget(CentralWidget);

	mShippedMapList << "mp_vacant" << "mp_showdown" << "mp_strike" << "mp_pipeline" << "mp_shipment" << "mp_killhouse" << "mp_overgrown" << "mp_crossfire" << "mp_farm" << "mp_countdown" << "mp_crash" << "mp_crash_snow" << "mp_creek" << "mp_citystreets" << "mp_convoy" << "mp_broadcast" << "mp_carentan" << "mp_cargoship" << "mp_backlot" << "mp_bloc" << "mp_bog";

	Settings.beginGroup("MainWindow");
	resize(QSize(800, 600));
	move(QPoint(200, 200));
	restoreGeometry(Settings.value("Geometry").toByteArray());
	restoreState(Settings.value("State").toByteArray());
	Settings.endGroup();

	connect(&mTimer, SIGNAL(timeout()), this, SLOT(SteamUpdate()));
	mTimer.start(1000);

	PopulateFileList();
}

COD4ModTools::~COD4ModTools()
{
}

void COD4ModTools::CreateActions()
{
	mActionFileNew = new QAction(QIcon(":/resources/FileNew.png"), "&New...", this);
	mActionFileNew->setShortcut(QKeySequence("Ctrl+N"));
	connect(mActionFileNew, SIGNAL(triggered()), this, SLOT(OnFileNew()));

	mActionFileAssetEditor = new QAction(QIcon(":/resources/AssetEditor.png"), "&Asset Editor", this);
	mActionFileAssetEditor->setShortcut(QKeySequence("Ctrl+A"));
	connect(mActionFileAssetEditor, SIGNAL(triggered()), this, SLOT(OnFileAssetEditor()));

	mActionFileLevelEditor = new QAction(QIcon(":/resources/Radiant.png"), "Open in &Radiant", this);
	mActionFileLevelEditor->setShortcut(QKeySequence("Ctrl+R"));
	mActionFileLevelEditor->setToolTip("Level Editor");
	connect(mActionFileLevelEditor, SIGNAL(triggered()), this, SLOT(OnFileLevelEditor()));

	mActionFileExit = new QAction("E&xit", this);
	connect(mActionFileExit, SIGNAL(triggered()), this, SLOT(close()));

	mActionEditBuild = new QAction(QIcon(":/resources/Go.png"), "Build", this);
	mActionEditBuild->setShortcut(QKeySequence("Ctrl+B"));
	connect(mActionEditBuild, SIGNAL(triggered()), this, SLOT(OnEditBuild()));

	mActionEditOptions = new QAction("&Options...", this);
	connect(mActionEditOptions, SIGNAL(triggered()), this, SLOT(OnEditOptions()));

	mActionHelpAbout = new QAction("&About...", this);
	connect(mActionHelpAbout, SIGNAL(triggered()), this, SLOT(OnHelpAbout()));
}

void COD4ModTools::CreateMenu()
{
	QMenuBar* MenuBar = new QMenuBar(this);

	QMenu* FileMenu = new QMenu("&File", MenuBar);
	FileMenu->addAction(mActionFileNew);
	FileMenu->addSeparator();
	FileMenu->addAction(mActionFileAssetEditor);
	FileMenu->addAction(mActionFileLevelEditor);
	FileMenu->addSeparator();
	FileMenu->addAction(mActionFileExit);
	MenuBar->addAction(FileMenu->menuAction());

	QMenu* EditMenu = new QMenu("&Edit", MenuBar);
	EditMenu->addAction(mActionEditBuild);
	EditMenu->addSeparator();
	EditMenu->addAction(mActionEditOptions);
	MenuBar->addAction(EditMenu->menuAction());

	QMenu* HelpMenu = new QMenu("&Help", MenuBar);
	HelpMenu->addAction(mActionHelpAbout);
	MenuBar->addAction(HelpMenu->menuAction());

	setMenuBar(MenuBar);
}

void COD4ModTools::CreateToolBar()
{
	QToolBar* ToolBar = new QToolBar("Standard", this);
	ToolBar->setObjectName(QStringLiteral("StandardToolBar"));

	ToolBar->addAction(mActionFileNew);
	ToolBar->addAction(mActionEditBuild);
	ToolBar->addSeparator();
	ToolBar->addAction(mActionFileAssetEditor);
	ToolBar->addAction(mActionFileLevelEditor);

	addToolBar(Qt::TopToolBarArea, ToolBar);
}

void COD4ModTools::closeEvent(QCloseEvent* Event)
{
	QSettings Settings;
	Settings.beginGroup("MainWindow");
	Settings.setValue("Geometry", saveGeometry());
	Settings.setValue("State", saveState());
	Settings.endGroup();

	Event->accept();
}

void COD4ModTools::StartBuildThread(const QList<QPair<QString, QStringList>>& Commands)
{
	mBuildButton->setText("Cancel");
	mOutputWidget->clear();

	mBuildThread = new mlBuildThread(Commands, mIgnoreErrorsWidget->isChecked());
	connect(mBuildThread, SIGNAL(OutputReady(QString)), this, SLOT(BuildOutputReady(QString)));
	connect(mBuildThread, SIGNAL(finished()), this, SLOT(BuildFinished()));
	mBuildThread->start();
}

void COD4ModTools::StartConvertThread(QStringList& pathList, QString& outputDir, bool allowOverwrite)
{
	mConvertThread = new mlConvertThread(pathList, outputDir, true, allowOverwrite);
	connect(mConvertThread, SIGNAL(OutputReady(QString)), this, SLOT(BuildOutputReady(QString)));
	connect(mConvertThread, SIGNAL(finished()), this, SLOT(BuildFinished()));
	mConvertThread->start();
}

void COD4ModTools::PopulateFileList()
{
	mFileListWidget->clear();

	QString MapSourceFolder = QDir::cleanPath(QString("%1/map_source/").arg(mGamePath));
	QStringList MapSources = QDir(MapSourceFolder).entryList(QDir::Files | QDir::NoDotAndDotDot);
	QTreeWidgetItem* MapsRootItem = new QTreeWidgetItem(mFileListWidget, QStringList() << "Maps");

	QFont Font = MapsRootItem->font(0);
	Font.setBold(true);
	MapsRootItem->setFont(0, Font);

	for (QString MapName : MapSources)
	{
		QFileInfo MapNameInfo = QFileInfo(MapName);
		QString ZoneFileName = QString("%1/%2").arg(MapSourceFolder, MapNameInfo.fileName());

		if (QFileInfo(ZoneFileName).isFile() && MapNameInfo.fileName().toStdString().ends_with(".map"))
		{
			QTreeWidgetItem* MapItem = new QTreeWidgetItem(MapsRootItem, QStringList() << MapNameInfo.completeBaseName());
			MapItem->setCheckState(0, Qt::Unchecked);
			MapItem->setData(0, Qt::UserRole, ML_ITEM_MAP);
		}
	}

	mFileListWidget->expandAll();
}

void COD4ModTools::ContextMenuRequested()
{
	QList<QTreeWidgetItem*> ItemList = mFileListWidget->selectedItems();
	if (ItemList.isEmpty())
		return;

	QTreeWidgetItem* Item = ItemList[0];
	QString ItemType = (Item->data(0, Qt::UserRole).toInt() == ML_ITEM_MAP) ? "Map" : "Mod";

	if (Item->data(0, Qt::UserRole).toInt() == ML_ITEM_UNKNOWN)
		return;

	QIcon GameIcon(":/resources/BlackOps3.png");

	QMenu* Menu = new QMenu;
	Menu->addAction(GameIcon, QString("Run %1").arg(ItemType), this, SLOT(OnRunMapOrMod()));

	if (Item->data(0, Qt::UserRole).toInt() == ML_ITEM_MAP)
		Menu->addAction(mActionFileLevelEditor);

	Menu->addAction("Edit Zone File", this, SLOT(OnOpenZoneFile()));
	Menu->addAction(QString("Open %1 Folder").arg(ItemType), this, SLOT(OnOpenModRootFolder()));

	Menu->addSeparator();
	Menu->addAction("Delete", this, SLOT(OnDelete()));

	Menu->exec(QCursor::pos());
}

void COD4ModTools::OnFileAssetEditor()
{
	QProcess* Process = new QProcess();
	connect(Process, SIGNAL(finished(int)), Process, SLOT(deleteLater()));
	Process->start(QString("%1/bin/asset_manager.exe").arg(mToolsPath), QStringList());
}

void COD4ModTools::OnFileLevelEditor()
{
	QProcess* Process = new QProcess();
	connect(Process, SIGNAL(finished(int)), Process, SLOT(deleteLater()));

	QList<QTreeWidgetItem*> ItemList = mFileListWidget->selectedItems();
	if (ItemList.count() && ItemList[0]->data(0, Qt::UserRole).toInt() == ML_ITEM_MAP)
	{
		QString MapName = ItemList[0]->text(0);
		Process->start(QString("%1/bin/CoD4Radiant.exe").arg(mToolsPath), QStringList() << QString("%1/map_source/%2/%3.map").arg(mGamePath, MapName.left(2), MapName));
	}
}

void COD4ModTools::OnFileNew()
{
	QDir TemplatesFolder(QString("%1/rex/templates").arg(mToolsPath));
	QStringList Templates = TemplatesFolder.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

	if (Templates.isEmpty())
	{
		QMessageBox::information(this, "Error", "Could not find any map templates.");
		return;
	}

	QDialog Dialog(this, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
	Dialog.setWindowTitle("New Map or Mod");

	QVBoxLayout* Layout = new QVBoxLayout(&Dialog);

	QFormLayout* FormLayout = new QFormLayout();
	Layout->addLayout(FormLayout);

	QLineEdit* NameWidget = new QLineEdit();
	NameWidget->setValidator(new QRegularExpressionValidator(QRegularExpression("[a-zA-Z0-9_]*"), this));
	FormLayout->addRow("Name:", NameWidget);

	QComboBox* TemplateWidget = new QComboBox();
	TemplateWidget->addItems(Templates);
	FormLayout->addRow("Template:", TemplateWidget);

	QFrame* Frame = new QFrame();
	Frame->setFrameShape(QFrame::HLine);
	Frame->setFrameShadow(QFrame::Raised);
	Layout->addWidget(Frame);

	QDialogButtonBox* ButtonBox = new QDialogButtonBox(&Dialog);
	ButtonBox->setOrientation(Qt::Horizontal);
	ButtonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	ButtonBox->setCenterButtons(true);

	Layout->addWidget(ButtonBox);

	connect(ButtonBox, SIGNAL(accepted()), &Dialog, SLOT(accept()));
	connect(ButtonBox, SIGNAL(rejected()), &Dialog, SLOT(reject()));

	if (Dialog.exec() != QDialog::Accepted)
		return;

	QString Name = NameWidget->text();

	if (Name.isEmpty())
	{
		QMessageBox::information(this, "Error", "Map name cannot be empty.");
		return;
	}

	if (mShippedMapList.contains(Name, Qt::CaseInsensitive))
	{
		QMessageBox::information(this, "Error", "Map name cannot be the same as a built-in map.");
		return;
	}

	QByteArray MapName = NameWidget->text().toLatin1().toLower();
	QString Output;

	QString Template = Templates[TemplateWidget->currentIndex()];

	if ((Template == "MP Mod Level" && !MapName.startsWith("mp_")) || (Template == "ZM Mod Level" && !MapName.startsWith("zm_")))
	{
		QMessageBox::information(this, "Error", "Map name must start with 'mp_' or 'zm_'.");
		return;
	}

	std::function<bool(const QString&, const QString&)> RecursiveCopy = [&](const QString& SourcePath, const QString& DestPath) -> bool
	{
		QDir Dir(SourcePath);
		if (!Dir.exists())
			return false;

		foreach(QString DirEntry, Dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
		{
			QString NewPath = QString(DestPath + QDir::separator() + DirEntry).replace(QString("template"), MapName);
			Dir.mkpath(NewPath);
			if (!RecursiveCopy(SourcePath + QDir::separator() + DirEntry, NewPath))
				return false;
		}

		foreach(QString DirEntry, Dir.entryList(QDir::Files))
		{
			QFile SourceFile(SourcePath + QDir::separator() + DirEntry);
			QString DestFileName = QString(DestPath + QDir::separator() + DirEntry).replace(QString("template"), MapName);
			QFile DestFile(DestFileName);

			if (!SourceFile.open(QFile::ReadOnly) || !DestFile.open(QFile::WriteOnly))
				return false;

			while (!SourceFile.atEnd())
			{
				QByteArray Line = SourceFile.readLine();

				if (Line.contains("guid"))
				{
					QString LineString(Line);
					LineString.replace(QRegularExpression("guid \"\\{(.*)\\}\""), QString("guid \"%1\"").arg(QUuid::createUuid().toString()));
					Line = LineString.toLatin1();
				}
				else
					Line.replace("template", MapName);

				DestFile.write(Line);
			}

			Output += DestFileName + "\n";
		}

		return true;
	};

	if (RecursiveCopy(TemplatesFolder.absolutePath() + QDir::separator() + Templates[TemplateWidget->currentIndex()], QDir::cleanPath(mGamePath)))
	{
		PopulateFileList();

		QMessageBox::information(this, "New Map Created", QString("Files created:\n") + Output);
	}
	else
		QMessageBox::information(this, "Error", "Error creating map files.");
}

void COD4ModTools::OnEditBuild()
{
	if (mBuildThread)
	{
		mBuildThread->Cancel();
		return;
	}

	QList<QPair<QString, QStringList>> Commands;

	QList<QTreeWidgetItem*> CheckedItems;

	std::function<void(QTreeWidgetItem*)> SearchCheckedItems = [&](QTreeWidgetItem* ParentItem) -> void
	{
		for (int ChildIdx = 0; ChildIdx < ParentItem->childCount(); ChildIdx++)
		{
			QTreeWidgetItem* Child = ParentItem->child(ChildIdx);
			if (Child->checkState(0) == Qt::Checked)
				CheckedItems.append(Child);
			else
				SearchCheckedItems(Child);
		}
	};

	SearchCheckedItems(mFileListWidget->invisibleRootItem());
	QString LastMap, LastMod;

	QStringList LanguageArgs;
	LanguageArgs;

	if (mBuildLanguage != "All")
		LanguageArgs << "-language" << mBuildLanguage;
	else for (const QString& Language : gLanguages)
		LanguageArgs << "-language" << Language;

	for (QTreeWidgetItem* Item : CheckedItems)
	{
		if (Item->data(0, Qt::UserRole).toInt() == ML_ITEM_MAP)
		{
			QString MapName = Item->text(0);

			if (mCompileEnabledWidget->isChecked())
			{
				QStringList Args;
				Args << "-platform" << "pc";

				if (mCompileModeWidget->currentIndex() == 0)
					Args << "-onlyents";
				else
					Args << "-navmesh" << "-navvolume";

				Args << "-loadFrom" << QString("%1\\map_source\\%2\\%3.map").arg(mGamePath, MapName.left(2), MapName);
				Args << QString("%1\\share\\raw\\maps\\%2\\%3.d3dbsp").arg(mGamePath, MapName.left(2), MapName);

				Commands.append(QPair<QString, QStringList>(QString("%1\\bin\\cod2map64.exe").arg(mToolsPath), Args));
			}

			if (mLightEnabledWidget->isChecked())
			{
				QStringList Args;
				Args << "-ledSilent";

				switch (mLightQualityWidget->currentIndex())
				{
				case 0:
					Args << "+low";
					break;

				default:
				case 1:
					Args << "+medium";
					break;

				case 2:
					Args << "+high";
					break;
				}

				Args << "+localprobes" << "+forceclean" << "+recompute" << QString("%1/map_source/%2/%3.map").arg(mGamePath, MapName.left(2), MapName);
				Commands.append(QPair<QString, QStringList>(QString("%1/bin/CoD4Radiant.exe").arg(mToolsPath), Args));
			}

			if (mLinkEnabledWidget->isChecked())
			{
				Commands.append(QPair<QString, QStringList>(QString("%1/bin/linker_modtools.exe").arg(mToolsPath), QStringList() << LanguageArgs << "-modsource" << MapName));
			}

			LastMap = MapName;
		}
	}

	if (mRunEnabledWidget->isChecked() && (!LastMod.isEmpty() || !LastMap.isEmpty()))
	{
		QStringList Args;

		Args << "+set" << "fs_game" << (LastMod.isEmpty() ? LastMap : LastMod);

		if (!LastMap.isEmpty())
			Args << "+devmap" << LastMap;

		QString ExtraOptions = mRunOptionsWidget->text();
		if (!ExtraOptions.isEmpty())
			Args << ExtraOptions.split(' ');

		Commands.append(QPair<QString, QStringList>(QString("%1/iw3mp.exe").arg(mGamePath), Args));
	}

	if (Commands.size() == 0)
	{
		QMessageBox::information(this, "No Tasks", "Please selected at least one file from the list and one action to be performed.");
		return;
	}

	StartBuildThread(Commands);
}

void COD4ModTools::OnEditOptions()
{
	QDialog Dialog(this, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
	Dialog.setWindowTitle("Options");

	QVBoxLayout* Layout = new QVBoxLayout(&Dialog);

	QSettings Settings;
	QCheckBox* Checkbox = new QCheckBox("Use Treyarch Theme");
	Checkbox->setToolTip("Toggle between the dark grey Treyarch colors and the default Windows colors");
	Checkbox->setChecked(Settings.value("UseAuroraTheme", false).toBool());
	Layout->addWidget(Checkbox);

	QHBoxLayout* LanguageLayout = new QHBoxLayout();
	LanguageLayout->addWidget(new QLabel("Build Language:"));

	QStringList Languages;
	Languages << "All";
	for (int LanguageIdx = 0; LanguageIdx < ARRAYSIZE(gLanguages); LanguageIdx++)
		Languages << gLanguages[LanguageIdx];

	QComboBox* LanguageCombo = new QComboBox();
	LanguageCombo->addItems(Languages);
	LanguageCombo->setCurrentText(mBuildLanguage);
	LanguageLayout->addWidget(LanguageCombo);

	Layout->addLayout(LanguageLayout);

	QDialogButtonBox* ButtonBox = new QDialogButtonBox(&Dialog);
	ButtonBox->setOrientation(Qt::Horizontal);
	ButtonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	ButtonBox->setCenterButtons(true);

	Layout->addWidget(ButtonBox);

	connect(ButtonBox, SIGNAL(accepted()), &Dialog, SLOT(accept()));
	connect(ButtonBox, SIGNAL(rejected()), &Dialog, SLOT(reject()));

	if (Dialog.exec() != QDialog::Accepted)
		return;

	mBuildLanguage = LanguageCombo->currentText();
	mTreyarchTheme = Checkbox->isChecked();

	Settings.setValue("BuildLanguage", mBuildLanguage);
	Settings.setValue("UseAuroraTheme", mTreyarchTheme);

	UpdateTheme();
}

void COD4ModTools::UpdateTheme()
{
	if (mTreyarchTheme)
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

void COD4ModTools::OnHelpAbout()
{
	QMessageBox::about(this, "About Modtools Launcher", "Treyarch Modtools Launcher\nCopyright 2016 Treyarch");
}

void COD4ModTools::OnOpenZoneFile()
{
	QList<QTreeWidgetItem*> ItemList = mFileListWidget->selectedItems();
	if (ItemList.isEmpty())
		return;

	QTreeWidgetItem* Item = ItemList[0];

	if (Item->data(0, Qt::UserRole).toInt() == ML_ITEM_MAP)
	{
		QString MapName = Item->text(0);
		ShellExecuteA(NULL, "open", QString("\"%1/zone_source/%3.csv\"").arg(mGamePath, MapName).toLatin1().constData(), "", NULL, SW_SHOWDEFAULT);
	}
}

void COD4ModTools::OnOpenModRootFolder()
{
	QList<QTreeWidgetItem*> ItemList = mFileListWidget->selectedItems();
	if (ItemList.isEmpty())
		return;

	QTreeWidgetItem* Item = ItemList[0];

	if (Item->data(0, Qt::UserRole).toInt() == ML_ITEM_MAP)
	{
		QString MapName = Item->text(0);
		ShellExecuteA(NULL, "open", (QString("\"%1/usermaps/%2\"").arg(mGamePath, MapName)).toLatin1().constData(), "", NULL, SW_SHOWDEFAULT);
	}
}

void COD4ModTools::OnRunMapOrMod()
{
	QList<QTreeWidgetItem*> ItemList = mFileListWidget->selectedItems();
	if (ItemList.isEmpty())
		return;

	QTreeWidgetItem* Item = ItemList[0];

	QDir(mGamePath + "/mods/emptymod").mkpath(".");

	QStringList Args;
	Args << "+set" << "fs_game" << "mods/emptymod";
	Args << "+set" << "developer" << "2";
	Args << "+set" << "developer_script" << "1";

	if (Item->data(0, Qt::UserRole).toInt() == ML_ITEM_MAP)
	{
		QString MapName = Item->text(0);
		Args << "+devmap" << MapName;
	}

	QString ExtraOptions = mRunOptionsWidget->text();
	if (!ExtraOptions.isEmpty())
		Args << ExtraOptions.split(' ');

	QList<QPair<QString, QStringList>> Commands;
	Commands.append(QPair<QString, QStringList>(QString("%1/iw3mp.exe").arg(mGamePath), Args));
	StartBuildThread(Commands);
}

void COD4ModTools::OnSaveLog() const
{
	// want to make a logs directory for easy management of launcher logs (exe_dir/logs)
	const auto dir = QDir{};
	if (!dir.exists("logs"))
	{
		const auto result = dir.mkdir("logs");
		if (!result)
		{
			QMessageBox::warning(nullptr, "Error", QString("Could not create the \"logs\" directory"));
			return;
		}
	}

	const auto time = std::time(nullptr);
	auto ss = std::stringstream{};
	const auto timeStr = std::put_time(std::localtime(&time), "%F_%T");

	ss << timeStr;

	auto dateStr = ss.str();
	std::replace(dateStr.begin(), dateStr.end(), ':', '_');

	QFile log(QString{ "logs/modlog_%1.txt" }.arg(dateStr.c_str()));

	if (!log.open(QIODevice::WriteOnly))
		return;

	QTextStream stream(&log);
	stream << mOutputWidget->toPlainText();

	QMessageBox::information(nullptr, QString("Save Log"), QString("The console log has been saved to %1").arg(log.fileName()));
}

void COD4ModTools::OnDelete()
{
	QList<QTreeWidgetItem*> ItemList = mFileListWidget->selectedItems();
	if (ItemList.isEmpty())
		return;

	QTreeWidgetItem* Item = ItemList[0];
	QString Folder;

	if (Item->data(0, Qt::UserRole).toInt() == ML_ITEM_MAP)
	{
		QString MapName = Item->text(0);
		Folder = QString("%1/map_source/%2.map").arg(mGamePath, MapName);
	}

	if (QMessageBox::question(this, "Delete File", QString("Are you sure you want to delete the file '%1' and all of its contents?").arg(Folder), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	QDir(QString("%1/map_source").arg(mGamePath)).remove(QString("%1.map").arg(Item->text(0)));
	PopulateFileList();
}

void COD4ModTools::BuildOutputReady(QString Output)
{
	mOutputWidget->appendPlainText(Output);
}

void COD4ModTools::BuildFinished()
{
	mBuildButton->setText("Build");
	mBuildThread->deleteLater();
	mBuildThread = NULL;
}
