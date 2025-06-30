#pragma once

#include <QThread>
#include <QtWidgets/QtWidgets>

class mlBuildThread : public QThread
{
	Q_OBJECT

public:
	mlBuildThread(const QList<QPair<QString, QStringList>>& Commands, bool IgnoreErrors);
	void run();
	bool Succeeded() const
	{
		return mSuccess;
	}

	void Cancel()
	{
		mCancel = true;
	}

signals:
	void OutputReady(const QString& Output);

protected:
	QList<QPair<QString, QStringList>> mCommands;
	bool mSuccess;
	bool mCancel;
	bool mIgnoreErrors;
};

class mlConvertThread : public QThread
{
	Q_OBJECT

public:
	mlConvertThread(QStringList& Files, QString& OutputDir, bool IgnoreErrors, bool mOverwrite);
	void run();
	bool Succeeded() const
	{
		return mSuccess;
	}

	void Cancel()
	{
		mCancel = true;
	}

signals:
	void OutputReady(const QString& Output);

protected:
	QStringList mFiles;
	QString mOutputDir;
	bool mOverwrite;

	bool mSuccess;
	bool mCancel;
	bool mIgnoreErrors;
};

class COD4ModTools : public QMainWindow
{
	Q_OBJECT

public:
	COD4ModTools();
	~COD4ModTools();

protected slots:
	void OnFileNew();
	void OnFileAssetEditor();
	void OnFileLevelEditor();
	void OnEditBuild();
	void OnEditOptions();
	void OnHelpAbout();
	void OnOpenZoneFile();
	void OnOpenModRootFolder();
	void OnRunMapOrMod();
	void OnSaveLog() const;
	void OnDelete();
	void BuildOutputReady(QString Output);
	void BuildFinished();
	void ContextMenuRequested();

protected:
	void closeEvent(QCloseEvent* Event);

	void StartBuildThread(const QList<QPair<QString, QStringList>>& Commands);
	void StartConvertThread(QStringList& pathList, QString& outputDir, bool allowOverwrite);

	void PopulateFileList();
	void UpdateTheme();

	void CreateActions();
	void CreateMenu();
	void CreateToolBar();

	QAction* mActionFileNew;
	QAction* mActionFileAssetEditor;
	QAction* mActionFileLevelEditor;
	QAction* mActionFileExit;
	QAction* mActionEditBuild;
	QAction* mActionEditPublish;
	QAction* mActionEditOptions;
	QAction* mActionHelpAbout;

	QTreeWidget* mFileListWidget;
	QPlainTextEdit* mOutputWidget;

	QPushButton* mBuildButton;
	QPushButton* mLogButton;
	QCheckBox* mCompileEnabledWidget;
	QComboBox* mCompileModeWidget;
	QCheckBox* mLightEnabledWidget;
	QComboBox* mLightQualityWidget;
	QCheckBox* mLinkEnabledWidget;
	QCheckBox* mRunEnabledWidget;
	QLineEdit* mRunOptionsWidget;
	QCheckBox* mIgnoreErrorsWidget;

	mlBuildThread* mBuildThread;
	mlConvertThread* mConvertThread;

	bool mTreyarchTheme;
	QString mBuildLanguage;

	QStringList mShippedMapList;
	QTimer mTimer;

	quint64 mFileId;
	QString mTitle;
	QString mDescription;
	QString mThumbnail;
	QString mWorkshopFolder;
	QString mFolderName;
	QString mType;
	QStringList mTags;

	QString mGamePath;
	QString mToolsPath;
};
