/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDUINO_STORAGE_ERROR_H
#define ARDUINO_STORAGE_ERROR_H

#include <Arduino.h>

enum class StorageErrorCode {
    NONE = 0,

    // File/Folder errors
    FILE_NOT_FOUND,
    FOLDER_NOT_FOUND,
    ALREADY_EXISTS,
    INVALID_PATH,
    PERMISSION_DENIED,

    // I/O errors
    READ_ERROR,
    WRITE_ERROR,
    SEEK_ERROR,
    OPEN_ERROR,
    CLOSE_ERROR,

    // Storage errors
    STORAGE_FULL,
    STORAGE_NOT_MOUNTED,
    STORAGE_CORRUPTED,
    STORAGE_NOT_FORMATTED,

    // Operation errors
    INVALID_OPERATION,
    INVALID_MODE,
    BUFFER_OVERFLOW,
    OUT_OF_MEMORY,
    TIMEOUT,

    // Hardware errors
    HARDWARE_ERROR,
    NOT_INITIALIZED,

    // Generic
    UNKNOWN_ERROR
};

class StorageError {
public:
    StorageError();

    // Error state
    StorageErrorCode getCode() const;
    const char* getMessage() const;
    bool hasError() const;

    // Error setting (for implementations)
    void setError(StorageErrorCode code, const char* message = nullptr);
    void clear();

    // Convenience operators
    operator bool() const;  // Returns true if error exists

private:
    StorageErrorCode code_;
    char message_[128];
};

#endif // ARDUINO_STORAGE_ERROR_H
