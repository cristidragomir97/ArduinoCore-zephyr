/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "StorageError.h"
#include <cstring>

StorageError::StorageError() : code_(StorageErrorCode::NONE) {
    message_[0] = '\0';
}

StorageErrorCode StorageError::getCode() const {
    return code_;
}

const char* StorageError::getMessage() const {
    if (message_[0] != '\0') {
        return message_;
    }

    // Return default message based on error code
    switch (code_) {
        case StorageErrorCode::NONE:
            return "No error";
        case StorageErrorCode::FILE_NOT_FOUND:
            return "File not found";
        case StorageErrorCode::FOLDER_NOT_FOUND:
            return "Folder not found";
        case StorageErrorCode::ALREADY_EXISTS:
            return "Already exists";
        case StorageErrorCode::INVALID_PATH:
            return "Invalid path";
        case StorageErrorCode::PERMISSION_DENIED:
            return "Permission denied";
        case StorageErrorCode::READ_ERROR:
            return "Read error";
        case StorageErrorCode::WRITE_ERROR:
            return "Write error";
        case StorageErrorCode::SEEK_ERROR:
            return "Seek error";
        case StorageErrorCode::OPEN_ERROR:
            return "Open error";
        case StorageErrorCode::CLOSE_ERROR:
            return "Close error";
        case StorageErrorCode::STORAGE_FULL:
            return "Storage full";
        case StorageErrorCode::STORAGE_NOT_MOUNTED:
            return "Storage not mounted";
        case StorageErrorCode::STORAGE_CORRUPTED:
            return "Storage corrupted";
        case StorageErrorCode::STORAGE_NOT_FORMATTED:
            return "Storage not formatted";
        case StorageErrorCode::INVALID_OPERATION:
            return "Invalid operation";
        case StorageErrorCode::INVALID_MODE:
            return "Invalid mode";
        case StorageErrorCode::BUFFER_OVERFLOW:
            return "Buffer overflow";
        case StorageErrorCode::OUT_OF_MEMORY:
            return "Out of memory";
        case StorageErrorCode::TIMEOUT:
            return "Timeout";
        case StorageErrorCode::HARDWARE_ERROR:
            return "Hardware error";
        case StorageErrorCode::NOT_INITIALIZED:
            return "Not initialized";
        case StorageErrorCode::UNKNOWN_ERROR:
        default:
            return "Unknown error";
    }
}

bool StorageError::hasError() const {
    return code_ != StorageErrorCode::NONE;
}

void StorageError::setError(StorageErrorCode code, const char* message) {
    code_ = code;
    if (message != nullptr) {
        strncpy(message_, message, sizeof(message_) - 1);
        message_[sizeof(message_) - 1] = '\0';
    } else {
        message_[0] = '\0';
    }
}

void StorageError::clear() {
    code_ = StorageErrorCode::NONE;
    message_[0] = '\0';
}

StorageError::operator bool() const {
    return hasError();
}
