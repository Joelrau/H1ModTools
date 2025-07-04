#pragma once

#include <QString>

namespace QtUtils {

    // === Directory Operations ===

    // Recursively copies a directory and its contents.
    bool copyDirectory(const QString& sourcePath, const QString& destinationPath);

    // Recursively deletes a directory and its contents.
    bool deleteDirectory(const QString& dirPath);

    // Moves a directory by copying then deleting the source.
    bool moveDirectory(const QString& sourcePath, const QString& destinationPath);

    // === File Operations ===

    // Copies a single file, replacing the destination if it exists.
    bool copyFile(const QString& sourceFile, const QString& destinationFile);

    // Deletes a single file.
    bool deleteFile(const QString& filePath);

    // Moves a single file by copying then deleting the source.
    bool moveFile(const QString& sourceFile, const QString& destinationFile);

    // Reads a file
    QString readFile(const QString& path);
}