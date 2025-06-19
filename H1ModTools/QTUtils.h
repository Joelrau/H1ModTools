#pragma once

#include <QString>

namespace QtUtils {

	/**
	 * @brief Moves a directory from sourcePath to destinationPath.
	 *        Recursively copies contents and deletes the original folder.
	 * @param sourcePath The full path of the directory to move.
	 * @param destinationPath The destination directory path.
	 * @return True if successful, false otherwise.
	 */
	bool moveDirectory(const QString& sourcePath, const QString& destinationPath);

	/**
	 * @brief Recursively copies a directory and its contents.
	 * @param sourcePath The source directory to copy.
	 * @param destinationPath The destination directory path.
	 * @return True if successful, false otherwise.
	 */
	bool copyDirectory(const QString& sourcePath, const QString& destinationPath);

	/**
	 * @brief Recursively deletes a directory and all its contents.
	 * @param dirPath The path of the directory to delete.
	 * @return True if successful, false otherwise.
	 */
	bool deleteDirectory(const QString& dirPath);

}