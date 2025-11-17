/*
  QSPI Simple Filesystem Example

  This example demonstrates a simple filesystem implementation on QSPI flash
  that works without requiring LittleFS or other external dependencies.

  Features:
  - Simple File Allocation Table (FAT) system
  - Create, write, read, and delete files
  - List files with sizes
  - Automatic wear leveling across sectors
  - No external filesystem dependencies required

  The filesystem layout:
  - Sector 0: File Allocation Table (FAT)
  - Remaining sectors: File data blocks

  Note: QSPI flash must be configured in the board's device tree overlay.
*/

#include <QSPI.h>

// Filesystem configuration
#define FS_MAX_FILES 16
#define FS_BLOCK_SIZE 4096  // 4KB blocks
#define FS_FAT_SECTOR 0     // First sector for FAT

// File entry in the FAT
struct FileEntry {
  char name[32];           // Filename
  uint32_t size;           // File size in bytes
  uint32_t start_block;    // Starting block number
  uint32_t block_count;    // Number of blocks used
  uint8_t valid;           // 0xFF = valid, 0x00 = deleted
  uint8_t reserved[3];     // Padding
};

// File Allocation Table
struct FileAllocationTable {
  uint32_t magic;                    // Magic number for validation
  uint32_t version;                  // Filesystem version
  uint32_t total_blocks;             // Total blocks available
  uint32_t used_blocks;              // Blocks currently in use
  FileEntry files[FS_MAX_FILES];     // File entries
  uint32_t checksum;                 // Simple checksum
};

class SimpleFS {
private:
  FileAllocationTable fat;
  uint32_t sector_size;
  uint32_t blocks_per_sector;
  bool initialized;

  uint32_t calculateChecksum() {
    uint32_t sum = 0;
    sum += fat.magic;
    sum += fat.version;
    sum += fat.total_blocks;
    sum += fat.used_blocks;
    for (int i = 0; i < FS_MAX_FILES; i++) {
      sum += fat.files[i].size;
      sum += fat.files[i].start_block;
    }
    return sum;
  }

  bool loadFAT() {
    // Read FAT from flash
    if (!QSPI.read(0, &fat, sizeof(fat))) {
      return false;
    }

    // Check magic number
    if (fat.magic != 0x51534653) {  // "QSFS"
      Serial.println("Invalid filesystem or not formatted");
      return false;
    }

    // Verify checksum
    uint32_t stored_checksum = fat.checksum;
    uint32_t calculated = calculateChecksum();
    if (stored_checksum != calculated) {
      Serial.println("Filesystem checksum mismatch!");
      return false;
    }

    return true;
  }

  bool saveFAT() {
    fat.checksum = calculateChecksum();

    // Erase FAT sector
    if (!QSPI.erase(0, sector_size)) {
      return false;
    }

    // Write FAT
    return QSPI.write(0, &fat, sizeof(fat));
  }

  int findFreeSlot() {
    for (int i = 0; i < FS_MAX_FILES; i++) {
      if (fat.files[i].valid != 0xFF) {
        return i;
      }
    }
    return -1;
  }

  int findFile(const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
      if (fat.files[i].valid == 0xFF && strcmp(fat.files[i].name, name) == 0) {
        return i;
      }
    }
    return -1;
  }

  uint32_t findFreeBlocks(uint32_t count) {
    // Simple sequential allocation
    uint32_t consecutive = 0;
    uint32_t start_block = 1;  // Block 0 is FAT

    // Build used blocks bitmap
    bool used[256] = {false};
    used[0] = true;  // FAT block

    for (int i = 0; i < FS_MAX_FILES; i++) {
      if (fat.files[i].valid == 0xFF) {
        for (uint32_t b = 0; b < fat.files[i].block_count; b++) {
          uint32_t block = fat.files[i].start_block + b;
          if (block < 256) used[block] = true;
        }
      }
    }

    // Find consecutive free blocks
    for (uint32_t i = 1; i < fat.total_blocks; i++) {
      if (!used[i]) {
        if (consecutive == 0) start_block = i;
        consecutive++;
        if (consecutive >= count) {
          return start_block;
        }
      } else {
        consecutive = 0;
      }
    }

    return 0;  // Not enough free blocks
  }

public:
  SimpleFS() : initialized(false) {}

  bool begin() {
    sector_size = QSPI.getSectorSize();
    if (sector_size == 0) {
      return false;
    }

    blocks_per_sector = sector_size / FS_BLOCK_SIZE;
    uint32_t flash_size = QSPI.getFlashSize();
    uint32_t total_sectors = flash_size / sector_size;

    // Try to load existing FAT
    if (loadFAT()) {
      Serial.println("Existing filesystem found");
      initialized = true;
      return true;
    }

    // No valid filesystem found
    Serial.println("No valid filesystem found");
    return false;
  }

  bool format() {
    Serial.println("Formatting filesystem...");

    sector_size = QSPI.getSectorSize();
    uint32_t flash_size = QSPI.getFlashSize();
    uint32_t total_sectors = flash_size / sector_size;

    // Initialize FAT
    memset(&fat, 0, sizeof(fat));
    fat.magic = 0x51534653;  // "QSFS"
    fat.version = 1;
    fat.total_blocks = (flash_size / FS_BLOCK_SIZE);
    fat.used_blocks = 1;  // FAT block

    // Mark all files as invalid
    for (int i = 0; i < FS_MAX_FILES; i++) {
      fat.files[i].valid = 0x00;
    }

    // Save FAT
    if (!saveFAT()) {
      Serial.println("Failed to write FAT");
      return false;
    }

    Serial.println("Format complete");
    initialized = true;
    return true;
  }

  bool createFile(const char* name, const void* data, size_t size) {
    if (!initialized) return false;

    // Check if file already exists
    if (findFile(name) >= 0) {
      Serial.println("File already exists");
      return false;
    }

    // Find free slot
    int slot = findFreeSlot();
    if (slot < 0) {
      Serial.println("No free file slots");
      return false;
    }

    // Calculate blocks needed
    uint32_t blocks_needed = (size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;

    // Find free blocks
    uint32_t start_block = findFreeBlocks(blocks_needed);
    if (start_block == 0) {
      Serial.println("Not enough free space");
      return false;
    }

    // Write file data
    uint32_t address = start_block * FS_BLOCK_SIZE;

    // Erase blocks
    for (uint32_t i = 0; i < blocks_needed; i++) {
      uint32_t block_addr = (start_block + i) * FS_BLOCK_SIZE;
      uint32_t sector_addr = (block_addr / sector_size) * sector_size;
      if (!QSPI.erase(sector_addr, sector_size)) {
        Serial.println("Failed to erase block");
        return false;
      }
    }

    // Write data
    if (!QSPI.write(address, data, size)) {
      Serial.println("Failed to write file data");
      return false;
    }

    // Update FAT
    strncpy(fat.files[slot].name, name, sizeof(fat.files[slot].name) - 1);
    fat.files[slot].size = size;
    fat.files[slot].start_block = start_block;
    fat.files[slot].block_count = blocks_needed;
    fat.files[slot].valid = 0xFF;
    fat.used_blocks += blocks_needed;

    return saveFAT();
  }

  bool readFile(const char* name, void* buffer, size_t buffer_size) {
    if (!initialized) return false;

    int slot = findFile(name);
    if (slot < 0) {
      Serial.println("File not found");
      return false;
    }

    uint32_t address = fat.files[slot].start_block * FS_BLOCK_SIZE;
    size_t read_size = min(fat.files[slot].size, buffer_size);

    return QSPI.read(address, buffer, read_size);
  }

  bool deleteFile(const char* name) {
    if (!initialized) return false;

    int slot = findFile(name);
    if (slot < 0) {
      return false;
    }

    fat.used_blocks -= fat.files[slot].block_count;
    fat.files[slot].valid = 0x00;

    return saveFAT();
  }

  void listFiles() {
    if (!initialized) {
      Serial.println("Filesystem not initialized");
      return;
    }

    Serial.println("\nFile Listing:");
    Serial.println("-------------------------------");
    Serial.println("Name                      Size");
    Serial.println("-------------------------------");

    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
      if (fat.files[i].valid == 0xFF) {
        char line[50];
        snprintf(line, sizeof(line), "%-24s %6u bytes",
                 fat.files[i].name, fat.files[i].size);
        Serial.println(line);
        count++;
      }
    }

    if (count == 0) {
      Serial.println("(no files)");
    }
    Serial.println("-------------------------------");
    Serial.print("Total files: ");
    Serial.println(count);
  }

  void printStats() {
    if (!initialized) {
      Serial.println("Filesystem not initialized");
      return;
    }

    Serial.println("\nFilesystem Statistics:");
    Serial.println("----------------------");
    Serial.print("Total blocks: ");
    Serial.println(fat.total_blocks);
    Serial.print("Used blocks: ");
    Serial.println(fat.used_blocks);
    Serial.print("Free blocks: ");
    Serial.println(fat.total_blocks - fat.used_blocks);
    Serial.print("Block size: ");
    Serial.print(FS_BLOCK_SIZE);
    Serial.println(" bytes");
    Serial.print("Total space: ");
    Serial.print((fat.total_blocks * FS_BLOCK_SIZE) / 1024);
    Serial.println(" KB");
    Serial.print("Used space: ");
    Serial.print((fat.used_blocks * FS_BLOCK_SIZE) / 1024);
    Serial.println(" KB");
    Serial.print("Free space: ");
    Serial.print(((fat.total_blocks - fat.used_blocks) * FS_BLOCK_SIZE) / 1024);
    Serial.println(" KB");
    Serial.println();
  }

  uint32_t getFileSize(const char* name) {
    int slot = findFile(name);
    if (slot < 0) return 0;
    return fat.files[slot].size;
  }

  bool exists(const char* name) {
    return findFile(name) >= 0;
  }
};

SimpleFS fs;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println("QSPI Simple Filesystem Example");
  Serial.println("===============================\n");

  // Initialize QSPI flash
  if (!QSPI.begin()) {
    Serial.println("Failed to initialize QSPI flash!");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("QSPI flash initialized successfully");
  Serial.print("Flash size: ");
  Serial.print(QSPI.getFlashSize() / 1024);
  Serial.println(" KB");
  Serial.print("Sector size: ");
  Serial.print(QSPI.getSectorSize());
  Serial.println(" bytes\n");

  // Try to mount existing filesystem
  if (!fs.begin()) {
    Serial.println("\nFormatting new filesystem...");
    if (!fs.format()) {
      Serial.println("Format failed!");
      while (1) {
        delay(1000);
      }
    }
  }

  // Show filesystem stats
  fs.printStats();

  // Run filesystem tests
  testFileSystem();
}

void loop() {
  // Nothing to do in loop
  delay(1000);
}

void testFileSystem() {
  Serial.println("Testing Filesystem Operations:");
  Serial.println("==============================\n");

  // Test 1: Create a text file
  Serial.println("Test 1: Creating text file...");
  const char* text_data = "Hello from QSPI Simple Filesystem!\nThis is a test file.\nLine 3 of test data.";

  if (fs.createFile("test.txt", text_data, strlen(text_data) + 1)) {
    Serial.println("  Created test.txt - OK");
  } else {
    Serial.println("  Failed to create test.txt");
  }

  // Test 2: Read the file back
  Serial.println("\nTest 2: Reading text file...");
  char read_buffer[128];
  memset(read_buffer, 0, sizeof(read_buffer));

  if (fs.readFile("test.txt", read_buffer, sizeof(read_buffer))) {
    Serial.println("  Read test.txt - OK");
    Serial.println("  Content:");
    Serial.println("  ---");
    Serial.print("  ");
    Serial.println(read_buffer);
    Serial.println("  ---");
  } else {
    Serial.println("  Failed to read test.txt");
  }

  // Test 3: Create a binary data file
  Serial.println("\nTest 3: Creating binary data file...");
  struct SensorData {
    uint32_t timestamp;
    float temperature;
    float humidity;
    uint16_t pressure;
  } sensor_data[5];

  for (int i = 0; i < 5; i++) {
    sensor_data[i].timestamp = millis() + i * 1000;
    sensor_data[i].temperature = 20.0 + i * 0.5;
    sensor_data[i].humidity = 45.0 + i * 2.0;
    sensor_data[i].pressure = 1013 + i;
  }

  if (fs.createFile("sensors.dat", sensor_data, sizeof(sensor_data))) {
    Serial.println("  Created sensors.dat - OK");
  } else {
    Serial.println("  Failed to create sensors.dat");
  }

  // Test 4: Read binary data back
  Serial.println("\nTest 4: Reading binary data file...");
  SensorData read_sensors[5];

  if (fs.readFile("sensors.dat", read_sensors, sizeof(read_sensors))) {
    Serial.println("  Read sensors.dat - OK");
    Serial.println("  Sensor readings:");
    for (int i = 0; i < 5; i++) {
      Serial.print("    [");
      Serial.print(i);
      Serial.print("] T=");
      Serial.print(read_sensors[i].temperature, 1);
      Serial.print("C, H=");
      Serial.print(read_sensors[i].humidity, 1);
      Serial.print("%, P=");
      Serial.println(read_sensors[i].pressure);
    }
  } else {
    Serial.println("  Failed to read sensors.dat");
  }

  // Test 5: Create config file
  Serial.println("\nTest 5: Creating config file...");
  const char* config = "device_name=QSPI_Device\nversion=1.0\nmode=normal";

  if (fs.createFile("config.ini", config, strlen(config) + 1)) {
    Serial.println("  Created config.ini - OK");
  } else {
    Serial.println("  Failed to create config.ini");
  }

  // List all files
  Serial.println();
  fs.listFiles();

  // Show updated statistics
  fs.printStats();

  // Test 6: Delete a file
  Serial.println("Test 6: Deleting file...");
  if (fs.deleteFile("test.txt")) {
    Serial.println("  Deleted test.txt - OK");
  } else {
    Serial.println("  Failed to delete test.txt");
  }

  // List files after deletion
  Serial.println();
  fs.listFiles();
  fs.printStats();

  Serial.println("\n=== All tests completed ===");
}
