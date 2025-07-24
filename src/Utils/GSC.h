#pragma once

#include <QtWidgets/QtWidgets>

struct GSC_Convert_Settings
{
	bool hasDestructibles = false;
	bool hasAnimatedModels = false;
	bool hasPipes = false;
	bool hasMinefields = false;
	bool hasRadiation = false;

	bool isMpMap = false;
};

QStringList findAllGSCFiles(const QString& root);

void ConvertGSCFiles(const QString& destinationPath, GSC_Convert_Settings settings);
void CopyGSCFiles(const QString& destinationPath);