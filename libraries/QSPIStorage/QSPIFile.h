/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QSPI_FILE_H
#define QSPI_FILE_H

#include <Arduino.h>
#include <ArduinoStorage.h>

// Forward declarations - avoid including zephyr headers in public header
// to prevent static initialization issues
struct fs_file_t;
class QSPIFolder;

/**
 * @brief QSPI File implementation
 *
 * Implements the File interface for QSPI storage using Zephyr's
 * file system API.
 */
class QSPIFile : public File {
public:
    QSPIFile();
    QSPIFile(const char* path);
    QSPIFile(const String& path);
    ~QSPIFile() override;

    // Opening and Closing
    bool open(const char* filename, FileMode mode, StorageError* error = nullptr) override;
    bool open(const String& filename, FileMode mode, StorageError* error = nullptr) override;
    bool open(FileMode mode = FileMode::READ, StorageError* error = nullptr) override;
    bool close(StorageError* error = nullptr) override;
    bool changeMode(FileMode mode, StorageError* error = nullptr) override;
    bool isOpen() const override;

    // Reading Operations
    size_t read(uint8_t* buffer, size_t size, StorageError* error = nullptr) override;
    int read(StorageError* error = nullptr) override;
    String readAsString(StorageError* error = nullptr) override;
    uint32_t available(StorageError* error = nullptr) override;
    bool seek(size_t offset, StorageError* error = nullptr) override;
    size_t position(StorageError* error = nullptr) override;
    size_t size(StorageError* error = nullptr) override;

    // Writing Operations
    size_t write(const uint8_t* buffer, size_t size, StorageError* error = nullptr) override;
    size_t write(const String& data, StorageError* error = nullptr) override;
    size_t write(uint8_t value, StorageError* error = nullptr) override;
    bool flush(StorageError* error = nullptr) override;

    // File Management
    bool exists(StorageError* error = nullptr) const override;
    bool remove(StorageError* error = nullptr) override;
    bool rename(const char* newFilename, StorageError* error = nullptr) override;
    bool rename(const String& newFilename, StorageError* error = nullptr) override;

    // Path Information
    QSPIFolder getParentFolder(StorageError* error = nullptr) const;

private:
    struct fs_file_t* file_;  // Pointer to avoid including zephyr headers
    bool is_open_;
    FileMode mode_;

    // Internal helpers
    bool resolvePath(const char* path, char* resolved, StorageError* error);
    int fileModeToFlags(FileMode mode);
    bool ensureFileHandle();  // Allocate file handle if needed
    void freeFileHandle();    // Free file handle

    // Map Zephyr error codes to StorageErrorCode
    static StorageErrorCode mapZephyrError(int err);
};

#endif // QSPI_FILE_H
