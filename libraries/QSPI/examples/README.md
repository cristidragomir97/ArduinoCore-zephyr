# QSPI Library Examples

This directory contains examples demonstrating various uses of the QSPI library for external flash memory.

## Examples Overview

### 1. BasicQSPI.ino
**Difficulty:** Beginner
**Dependencies:** None

Basic example showing fundamental QSPI operations:
- Initialize QSPI flash
- Get flash information (size, sector size, page size)
- Erase sectors
- Write and read data
- Verify data integrity

**Best for:** Learning QSPI basics and testing your flash hardware.

---

### 2. QSPISimpleFS.ino
**Difficulty:** Intermediate
**Dependencies:** None

A self-contained simple filesystem implementation that works directly on QSPI flash without requiring external filesystem libraries.

**Features:**
- File Allocation Table (FAT) system
- Create, read, and delete files
- File listing and statistics
- Works out-of-the-box without additional configuration
- Supports up to 16 files
- Automatic space management

**Best for:** Projects needing simple file storage without LittleFS configuration.

**Limitations:**
- Maximum 16 files
- No directory support
- Sequential block allocation
- Fixed 4KB block size

---

### 3. QSPIPartitioning.ino
**Difficulty:** Intermediate
**Dependencies:** None

Demonstrates how to partition QSPI flash into logical regions for different purposes.

**Features:**
- Multiple partition support (Config, Logging, User Files, Backup)
- Partition boundary protection
- Per-partition read/write operations
- Partition table visualization
- Safe partition management class

**Best for:** Complex applications needing organized flash storage regions.

**Use cases:**
- Separating configuration from data
- Dedicated logging areas
- Protected backup regions
- Multi-purpose flash organization

---

### 4. QSPIFilesystem.ino
**Difficulty:** Advanced
**Dependencies:** Zephyr LittleFS

Full-featured filesystem example using Zephyr's LittleFS implementation.

**Features:**
- Complete filesystem with LittleFS
- Standard file operations (create, read, write, delete)
- Directory listing
- Filesystem statistics
- Wear leveling (provided by LittleFS)
- Power-loss resilient

**Configuration Required:**

Add to your board's `prj.conf`:
```
CONFIG_FILE_SYSTEM=y
CONFIG_FILE_SYSTEM_LITTLEFS=y
CONFIG_FILE_SYSTEM_MAX_FILE_NAME=128
```

**Best for:** Production applications needing robust filesystem support.

**Why use this over QSPISimpleFS?**
- No file count limit
- Better wear leveling
- Power-loss protection
- Directory support
- Standard POSIX-like API

---

## Quick Start Guide

### Hardware Requirements
- Arduino board with QSPI flash support (e.g., GIGA R1, Portenta H7)
- QSPI flash configured in device tree overlay

### Choosing the Right Example

```
Need basic flash operations?
→ Use BasicQSPI.ino

Need simple file storage without configuration?
→ Use QSPISimpleFS.ino

Need organized flash regions?
→ Use QSPIPartitioning.ino

Need production-grade filesystem?
→ Use QSPIFilesystem.ino (requires LittleFS setup)
```

---

## Configuring LittleFS (for QSPIFilesystem.ino)

### Step 1: Create/Edit prj.conf

Create a `prj.conf` file in your sketch directory with:

```conf
CONFIG_FILE_SYSTEM=y
CONFIG_FILE_SYSTEM_LITTLEFS=y
CONFIG_FILE_SYSTEM_MAX_FILE_NAME=128
```

### Step 2: Verify Device Tree

Your board should have QSPI flash defined in its overlay. Example:

```dts
&qspi {
    status = "okay";

    qspi_flash: qspi-nor-flash@0 {
        compatible = "nordic,qspi-nor";
        reg = <0>;
        /* ... other properties ... */
    };
};
```

### Step 3: Build and Upload

The Arduino-Zephyr build system should automatically pick up your `prj.conf`.

### Troubleshooting LittleFS

**Error: `lfs.h: No such file or directory`**
- LittleFS is not enabled in your build
- Make sure `CONFIG_FILE_SYSTEM_LITTLEFS=y` is in `prj.conf`
- Use `QSPISimpleFS.ino` instead if you don't need LittleFS

**Error: Filesystem mount fails**
- Flash might not be formatted
- Try erasing the flash first using `BasicQSPI.ino`
- Check that QSPI flash is properly configured in device tree

---

## Example Comparison

| Feature | BasicQSPI | QSPISimpleFS | QSPIPartitioning | QSPIFilesystem |
|---------|-----------|--------------|------------------|----------------|
| Difficulty | ⭐ | ⭐⭐ | ⭐⭐ | ⭐⭐⭐ |
| Configuration | None | None | None | LittleFS required |
| File Support | No | Yes (16 max) | No | Unlimited |
| Partitions | No | No | Yes | No |
| Wear Leveling | No | Basic | No | Yes (LittleFS) |
| Power-Loss Safe | No | No | No | Yes |
| Production Ready | No | Good | Good | Excellent |

---

## Common Operations

### Reading Flash Info
```cpp
QSPI.begin();
Serial.println(QSPI.getFlashSize());
Serial.println(QSPI.getSectorSize());
```

### Raw Read/Write
```cpp
// Erase first
QSPI.erase(address, size);

// Write
QSPI.write(address, data, size);

// Read
QSPI.read(address, buffer, size);
```

### Using SimpleFS
```cpp
SimpleFS fs;
fs.begin() || fs.format();
fs.createFile("test.txt", data, size);
fs.readFile("test.txt", buffer, buffer_size);
fs.listFiles();
```

### Using Partitions
```cpp
PartitionManager::initialize();
PartitionManager::writeToPartition(PARTITION_CONFIG, offset, data, size);
PartitionManager::readFromPartition(PARTITION_CONFIG, offset, buffer, size);
```

---

## Tips and Best Practices

1. **Always erase before writing**: Flash memory requires erasure before writing new data
2. **Sector alignment**: Erase operations work on sector boundaries (typically 4KB)
3. **Wear leveling**: Distribute writes across flash to extend lifetime
4. **Check return values**: Always verify that operations succeeded
5. **Checksums**: Use checksums for critical data to detect corruption
6. **Power-loss**: Consider what happens if power is lost during write operations

---

## Further Reading

- [Zephyr Flash API Documentation](https://docs.zephyrproject.org/latest/hardware/peripherals/flash.html)
- [LittleFS Documentation](https://github.com/littlefs-project/littlefs)
- [QSPI Library Reference](../QSPI.h)

---

## Support

For issues or questions:
- Check the example code comments
- Review error messages carefully
- Start with `BasicQSPI.ino` to verify hardware
- Use `QSPISimpleFS.ino` if LittleFS configuration is problematic
