/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QSPIStorage.h"

#include <zephyr/fs/fs.h>
#include <errno.h>

bool QSPIStorage::begin(StorageError* error) {
    if (mounted_) {
        return true;
    }

    // Check if the filesystem is already mounted (via devicetree FSTAB auto-mount)
    struct fs_statvfs stat;
    int ret = fs_statvfs("/qspi", &stat);

    if (ret == 0) {
        mounted_ = true;
        return true;
    }

    // Filesystem not mounted - provide helpful error message
    if (error) {
        if (ret == -ENOENT) {
            error->setError(StorageErrorCode::STORAGE_NOT_MOUNTED,
                "Filesystem not mounted. Ensure LittleFS FSTAB is configured in devicetree.");
        } else {
            error->setError(StorageErrorCode::UNKNOWN_ERROR, "Failed to access filesystem");
        }
    }
    return false;
}

bool QSPIStorage::getStorageInfo(size_t& total, size_t& used, size_t& available, StorageError* error) {
    if (!mounted_) {
        if (error) {
            error->setError(StorageErrorCode::STORAGE_NOT_MOUNTED, "Storage not mounted");
        }
        return false;
    }

    struct fs_statvfs stat;
    int ret = fs_statvfs("/qspi", &stat);
    if (ret != 0) {
        if (error) {
            error->setError(StorageErrorCode::READ_ERROR, "Failed to get storage info");
        }
        return false;
    }

    total = stat.f_frsize * stat.f_blocks;
    available = stat.f_frsize * stat.f_bfree;
    used = total - available;

    return true;
}
