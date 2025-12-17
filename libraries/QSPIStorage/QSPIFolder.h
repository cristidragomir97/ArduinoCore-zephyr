/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QSPI_FOLDER_H
#define QSPI_FOLDER_H

#include <Arduino.h>
#include <ArduinoStorage.h>
#include <vector>

// Note: zephyr/fs/fs.h is only included in the .cpp file
// to avoid static initialization issues

#include "QSPIFile.h"

/**
 * @brief QSPI Folder implementation
 *
 * Implements the Folder interface for QSPI storage using Zephyr's
 * file system API.
 */
class QSPIFolder : public Folder {
public:
    QSPIFolder();
    QSPIFolder(const char* path);
    QSPIFolder(const String& path);
    ~QSPIFolder() override;

    // File Operations
    QSPIFile createFile(const char* filename, FileMode mode = FileMode::WRITE, StorageError* error = nullptr);
    QSPIFile createFile(const String& filename, FileMode mode = FileMode::WRITE, StorageError* error = nullptr);
    QSPIFile getFile(const char* filename, StorageError* error = nullptr);
    QSPIFile getFile(const String& filename, StorageError* error = nullptr);

    // Directory Management
    bool exists(StorageError* error = nullptr) const override;
    bool create(StorageError* error = nullptr) override;
    bool remove(bool recursive = false, StorageError* error = nullptr) override;
    bool rename(const char* newName, StorageError* error = nullptr) override;
    bool rename(const String& newName, StorageError* error = nullptr) override;

    // Subfolder Operations
    QSPIFolder createSubfolder(const char* name, bool overwrite = false, StorageError* error = nullptr);
    QSPIFolder createSubfolder(const String& name, bool overwrite = false, StorageError* error = nullptr);
    QSPIFolder getSubfolder(const char* name, StorageError* error = nullptr);
    QSPIFolder getSubfolder(const String& name, StorageError* error = nullptr);

    // Content Enumeration
    std::vector<QSPIFile> getFiles(StorageError* error = nullptr);
    std::vector<QSPIFolder> getFolders(StorageError* error = nullptr);
    size_t getFileCount(StorageError* error = nullptr) override;
    size_t getFolderCount(StorageError* error = nullptr) override;

    // Path Information
    QSPIFolder getParentFolder(StorageError* error = nullptr) const;

private:
    // Internal helpers
    bool resolvePath(const char* path, char* resolved, StorageError* error);
    bool removeRecursive(const char* path, StorageError* error);

    // Map Zephyr error codes to StorageErrorCode
    static StorageErrorCode mapZephyrError(int err);
};

#endif // QSPI_FOLDER_H
