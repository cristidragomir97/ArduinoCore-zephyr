/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QSPI_STORAGE_H
#define QSPI_STORAGE_H

#include <Arduino.h>
#include <ArduinoStorage.h>
#include "QSPIFolder.h"

/**
 * @brief QSPI Storage manager class
 *
 * Provides high-level access to QSPI flash storage with LittleFS filesystem.
 *
 * IMPORTANT: This library requires LittleFS to be auto-mounted via devicetree FSTAB.
 * Your board's devicetree must include an FSTAB entry that mounts LittleFS at "/storage".
 */
class QSPIStorage {
public:
    QSPIStorage() : mounted_(false) {}
    ~QSPIStorage() {}

    /**
     * Initialize and verify the QSPI file system is mounted
     * @param error Optional error output parameter
     * @return true if successful, false otherwise
     */
    bool begin(StorageError* error = nullptr);

    /**
     * Mark storage as not in use
     * @param error Optional error output parameter
     */
    void end(StorageError* error = nullptr) { mounted_ = false; }

    /**
     * Check if storage is mounted and ready
     * @return true if mounted
     */
    bool isMounted() const { return mounted_; }

    /**
     * Get the mount point path
     * @return Mount point string "/storage"
     */
    const char* getMountPoint() const { return "/storage"; }

    /**
     * Get storage statistics
     * @param total Output total bytes
     * @param used Output used bytes
     * @param available Output available bytes
     * @param error Optional error output parameter
     * @return true if successful
     */
    bool getStorageInfo(size_t& total, size_t& used, size_t& available, StorageError* error = nullptr);

    /**
     * Get the root folder of the storage
     * @param error Optional error output parameter
     * @return QSPIFolder representing the root directory
     */
    QSPIFolder getRootFolder(StorageError* error = nullptr);

    /**
     * Format is not supported with FSTAB mounting
     */
    bool format(FilesystemType fsType = FilesystemType::LITTLEFS, StorageError* error = nullptr) {
        if (error) {
            error->setError(StorageErrorCode::INVALID_OPERATION, "Format not supported with FSTAB mounting.");
        }
        return false;
    }

private:
    bool mounted_;
};

#endif // QSPI_STORAGE_H
