#include "QtUtils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

namespace QtUtils {

    bool copyDirectory(const QString& sourcePath, const QString& destinationPath)
    {
        QDir sourceDir(sourcePath);
        if (!sourceDir.exists()) {
            qWarning() << "[QtUtils] Source directory does not exist:" << sourcePath;
            return false;
        }

        QDir destDir(destinationPath);
        if (!destDir.exists()) {
            if (!destDir.mkpath(".")) {
                qWarning() << "[QtUtils] Failed to create destination path:" << destinationPath;
                return false;
            }
        }

        QFileInfoList entries = sourceDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
        for (const QFileInfo& entry : entries) {
            QString srcFilePath = entry.filePath();
            QString destFilePath = destinationPath + QDir::separator() + entry.fileName();

            if (entry.isDir()) {
                // Recurse into existing or new directory
                if (!copyDirectory(srcFilePath, destFilePath)) {
                    return false;
                }
            }
            else {
                if (QFile::exists(destFilePath)) {
                    if (!QFile::remove(destFilePath)) {
                        qWarning() << "[QtUtils] Failed to remove existing file before overwrite:" << destFilePath;
                        return false;
                    }
                }
                if (!QFile::copy(srcFilePath, destFilePath)) {
                    qWarning() << "[QtUtils] Failed to copy file:" << srcFilePath << "to" << destFilePath;
                    return false;
                }
            }
        }

        return true;
    }

    bool deleteDirectory(const QString& dirPath)
    {
        QDir dir(dirPath);
        if (!dir.exists())
            return true; // Nothing to delete

        QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
        for (const QFileInfo& entry : entries) {
            if (entry.isDir()) {
                if (!deleteDirectory(entry.filePath())) {
                    qWarning() << "[QtUtils] Failed to remove directory:" << entry.filePath();
                    return false;
                }  
            }
            else {
                if (!QFile::remove(entry.filePath())) {
                    qWarning() << "[QtUtils] Failed to remove file:" << entry.filePath();
                    return false;
                }
            }
        }

        return dir.rmdir(dirPath);
    }

    bool moveDirectory(const QString& sourcePath, const QString& destinationPath)
    {
        if (!copyDirectory(sourcePath, destinationPath)) {
            return false;
        }
        return deleteDirectory(sourcePath);
    }

} // namespace QtUtils