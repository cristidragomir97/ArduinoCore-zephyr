/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "StorageFile.h"
#include <cstring>

File::File() {
    path_[0] = '\0';
}

File::File(const char* path) {
    if (path != nullptr) {
        strncpy(path_, path, sizeof(path_) - 1);
        path_[sizeof(path_) - 1] = '\0';
    } else {
        path_[0] = '\0';
    }
}

File::File(const String& path) {
    strncpy(path_, path.c_str(), sizeof(path_) - 1);
    path_[sizeof(path_) - 1] = '\0';
}

File::~File() {
}

const char* File::getPath() const {
    return path_;
}

String File::getPathAsString() const {
    return String(path_);
}

String File::getFilename() const {
    const char* lastSep = strrchr(path_, '/');
    if (lastSep != nullptr) {
        return String(lastSep + 1);
    }
    return String(path_);
}

void File::setErrorIfNotNull(StorageError* error, StorageErrorCode code, const char* message) {
    if (error != nullptr) {
        error->setError(code, message);
    }
}
