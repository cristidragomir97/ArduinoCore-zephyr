/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDUINO_STORAGE_FOLDER_H
#define ARDUINO_STORAGE_FOLDER_H

#include <Arduino.h>
#include <vector>
#include "StorageCommon.h"
#include "StorageError.h"
#include "StorageFile.h"

/**
 * @brief Abstract base class for folder/directory operations in Arduino storage implementations.
 *
 * This class defines the interface for all folder operations. Concrete implementations
 * (QSPIFolder, SDFolder, etc.) should inherit from this class and implement all
 * pure virtual methods.
 */
class Folder {
public:
    Folder();
    Folder(const char* path);
    Folder(const String& path);
    virtual ~Folder();

    // Directory Management
    virtual bool exists(StorageError* error = nullptr) const = 0;
    virtual bool create(StorageError* error = nullptr) = 0;
    virtual bool remove(bool recursive = false, StorageError* error = nullptr) = 0;
    virtual bool rename(const char* newName, StorageError* error = nullptr) = 0;
    virtual bool rename(const String& newName, StorageError* error = nullptr) = 0;

    // Content Enumeration
    virtual size_t getFileCount(StorageError* error = nullptr) = 0;
    virtual size_t getFolderCount(StorageError* error = nullptr) = 0;

    // Path Information
    virtual const char* getPath() const;
    virtual String getPathAsString() const;
    virtual String getFolderName() const;

protected:
    char path_[STORAGE_MAX_PATH_LENGTH];

    // Helper to set error if pointer is not null
    static void setErrorIfNotNull(StorageError* error, StorageErrorCode code, const char* message = nullptr);
};

#endif // ARDUINO_STORAGE_FOLDER_H
