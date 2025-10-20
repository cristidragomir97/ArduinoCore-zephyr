#ifndef QSPI_H
#define QSPI_H

#include <Arduino.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/devicetree.h>

class QSPIClass {

public:
    QSPIClass();

    // Initialize QSPI flash
    bool begin();

    // Read data from QSPI flash
    bool read(uint32_t address, void* data, size_t size);

    // Write data to QSPI flash
    bool write(uint32_t address, const void* data, size_t size);

    // Erase sector/block
    bool erase(uint32_t address, size_t size);

    // Get flash information
    size_t getFlashSize();
    size_t getSectorSize();
    size_t getPageSize();

    // Check if flash is ready
    bool isReady();

    // Get flash ID
    uint32_t getFlashID();

    // Utility functions
    bool isValidAddress(uint32_t address, size_t size = 1);

    // End/deinitialize
    void end();

private:
    const struct device *flash_dev;
    bool initialized;
};

extern QSPIClass QSPI;

#endif