#include "QSPI.h"

// Define the QSPI flash device - will be available when overlay is active
#if DT_NODE_EXISTS(DT_NODELABEL(qspi_flash))
#define QSPI_FLASH_NODE DT_NODELABEL(qspi_flash)
#define QSPI_FLASH_DEVICE DEVICE_DT_GET(QSPI_FLASH_NODE)
#else
#define QSPI_FLASH_DEVICE NULL
#warning "No QSPI flash available on this board"
#endif

QSPIClass::QSPIClass() : flash_dev(nullptr), initialized(false) {
}

bool QSPIClass::begin() {
    if (QSPI_FLASH_DEVICE == NULL) {
        return false;
    }

    flash_dev = QSPI_FLASH_DEVICE;

    if (!device_is_ready(flash_dev)) {
        flash_dev = nullptr;
        return false;
    }

    initialized = true;
    return true;
}

bool QSPIClass::read(uint32_t address, void* data, size_t size) {
    if (!initialized || !flash_dev) {
        return false;
    }

    int ret = flash_read(flash_dev, address, data, size);
    return (ret == 0);
}

bool QSPIClass::write(uint32_t address, const void* data, size_t size) {
    if (!initialized || !flash_dev) {
        return false;
    }

    int ret = flash_write(flash_dev, address, data, size);
    return (ret == 0);
}

bool QSPIClass::erase(uint32_t address, size_t size) {
    if (!initialized || !flash_dev) {
        return false;
    }

    int ret = flash_erase(flash_dev, address, size);
    return (ret == 0);
}

size_t QSPIClass::getFlashSize() {
    if (!initialized || !flash_dev) {
        return 0;
    }

    uint64_t size = 0;
    int ret = flash_get_size(flash_dev, &size);
    if (ret != 0) {
        return 0;
    }

    return (size_t)size;
}

size_t QSPIClass::getSectorSize() {
    if (!initialized || !flash_dev) {
        return 0;
    }

    struct flash_pages_info page_info;
    int ret = flash_get_page_info_by_offs(flash_dev, 0, &page_info);
    if (ret != 0) {
        return 0;
    }

    return page_info.size;
}

size_t QSPIClass::getPageSize() {
    if (!initialized || !flash_dev) {
        return 0;
    }

    const struct flash_parameters *flash_params = flash_get_parameters(flash_dev);
    if (!flash_params) {
        return 0;
    }

    return flash_params->write_block_size;
}

bool QSPIClass::isReady() {
    if (!flash_dev) {
        return false;
    }

    return device_is_ready(flash_dev);
}

uint32_t QSPIClass::getFlashID() {
    // This would require implementing JEDEC ID reading
    // For now, return 0 as placeholder
    return 0;
}

bool QSPIClass::isValidAddress(uint32_t address, size_t size) {
    if (!initialized || !flash_dev) {
        return false;
    }

    size_t flash_size = getFlashSize();
    return (address + size <= flash_size);
}

const struct device* QSPIClass::getDevice() {
    return flash_dev;
}

void QSPIClass::end() {
    flash_dev = nullptr;
    initialized = false;
}

// Create global instance
QSPIClass QSPI;