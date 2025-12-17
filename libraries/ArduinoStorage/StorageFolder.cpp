/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "StorageFolder.h"
#include <cstring>

Folder::Folder() {
    path_[0] = '\0';
}

Folder::Folder(const char* path) {
    if (path != nullptr) {
        strncpy(path_, path, sizeof(path_) - 1);
        path_[sizeof(path_) - 1] = '\0';
    } else {
        path_[0] = '\0';
    }
}

Folder::Folder(const String& path) {
    strncpy(path_, path.c_str(), sizeof(path_) - 1);
    path_[sizeof(path_) - 1] = '\0';
}

Folder::~Folder() {
}

const char* Folder::getPath() const {
    return path_;
}

String Folder::getPathAsString() const {
    return String(path_);
}

String Folder::getFolderName() const {
    const char* lastSep = strrchr(path_, '/');
    if (lastSep != nullptr && *(lastSep + 1) != '\0') {
        return String(lastSep + 1);
    }
    // Handle root path or trailing slash
    if (path_[0] == '/' && path_[1] == '\0') {
        return String("/");
    }
    return String(path_);
}

void Folder::setErrorIfNotNull(StorageError* error, StorageErrorCode code, const char* message) {
    if (error != nullptr) {
        error->setError(code, message);
    }
}
