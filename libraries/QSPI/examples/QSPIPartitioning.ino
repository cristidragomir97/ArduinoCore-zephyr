/*
  QSPI Partitioning Example

  This example demonstrates how to partition QSPI flash memory into
  logical regions for different purposes.

  Features:
  - Define multiple partitions with different sizes
  - Demonstrate isolated read/write operations per partition
  - Partition boundary checking
  - Efficient partition management

  Partitions created:
  - CONFIG: Small partition for configuration (64KB)
  - LOGGING: Medium partition for data logging (256KB)
  - USER_FILES: Large partition for user files (remaining space - 128KB)
  - BACKUP: Reserved backup partition (128KB)

  Note: QSPI flash must be configured in the board's device tree overlay.
*/

#include <QSPI.h>

// Maximum number of partitions supported
#define MAX_PARTITIONS 8

// Partition definition
struct PartitionInfo {
  const char* name;
  uint32_t start_address;
  uint32_t size;
};

// Global partition state
PartitionInfo partitions[MAX_PARTITIONS];
uint8_t partition_count = 0;
bool partition_initialized = false;
uint32_t flash_size = 0;
uint32_t sector_size = 0;

// Forward declarations
bool validatePartitions();
bool isValidPartition(uint8_t partition_id);
bool checkPartitionBoundaries(uint8_t partition_id, uint32_t offset, size_t size);

// Initialize partition system
bool initPartitions() {
  if (!QSPI.isReady()) {
    return false;
  }

  flash_size = QSPI.getFlashSize();
  sector_size = QSPI.getSectorSize();

  if (flash_size == 0 || sector_size == 0) {
    return false;
  }

  partition_initialized = true;
  return true;
}

// Define a new partition
bool definePartition(uint8_t id, const char* name, uint32_t start_address, uint32_t size) {
  if (!partition_initialized || id >= MAX_PARTITIONS) {
    return false;
  }

  // Check if partition fits in flash
  if (start_address + size > flash_size) {
    return false;
  }

  // Warn if not sector-aligned
  if (start_address % sector_size != 0 || size % sector_size != 0) {
    Serial.print("Warning: Partition '");
    Serial.print(name);
    Serial.println("' is not sector-aligned!");
  }

  partitions[id].name = name;
  partitions[id].start_address = start_address;
  partitions[id].size = size;

  if (id >= partition_count) {
    partition_count = id + 1;
  }

  return validatePartitions();
}

// Write data to partition
bool writePartition(uint8_t partition_id, uint32_t offset, const void* data, size_t size) {
  if (!checkPartitionBoundaries(partition_id, offset, size)) {
    return false;
  }

  uint32_t address = partitions[partition_id].start_address + offset;
  return QSPI.write(address, data, size);
}

// Read data from partition
bool readPartition(uint8_t partition_id, uint32_t offset, void* data, size_t size) {
  if (!checkPartitionBoundaries(partition_id, offset, size)) {
    return false;
  }

  uint32_t address = partitions[partition_id].start_address + offset;
  return QSPI.read(address, data, size);
}

// Erase entire partition
bool erasePartition(uint8_t partition_id) {
  if (!isValidPartition(partition_id)) {
    return false;
  }

  return QSPI.erase(partitions[partition_id].start_address, partitions[partition_id].size);
}

// Erase region within partition
bool erasePartitionRegion(uint8_t partition_id, uint32_t offset, size_t size) {
  if (!checkPartitionBoundaries(partition_id, offset, size)) {
    return false;
  }

  uint32_t address = partitions[partition_id].start_address + offset;

  // Align to sector boundary
  uint32_t aligned_address = (address / sector_size) * sector_size;
  uint32_t aligned_size = ((size + sector_size - 1) / sector_size) * sector_size;

  return QSPI.erase(aligned_address, aligned_size);
}

// Get partition size
uint32_t getPartitionSize(uint8_t partition_id) {
  if (!isValidPartition(partition_id)) {
    return 0;
  }
  return partitions[partition_id].size;
}

// Get partition start address
uint32_t getPartitionStart(uint8_t partition_id) {
  if (!isValidPartition(partition_id)) {
    return 0;
  }
  return partitions[partition_id].start_address;
}

// Get partition name
const char* getPartitionName(uint8_t partition_id) {
  if (!isValidPartition(partition_id)) {
    return "INVALID";
  }
  return partitions[partition_id].name;
}

// Check if partition is valid
bool isValidPartition(uint8_t partition_id) {
  return partition_initialized && partition_id < partition_count && partitions[partition_id].size > 0;
}

// Check partition boundaries
bool checkPartitionBoundaries(uint8_t partition_id, uint32_t offset, size_t size) {
  if (!isValidPartition(partition_id)) {
    return false;
  }

  if (offset + size > partitions[partition_id].size) {
    Serial.print("Error: Access exceeds partition '");
    Serial.print(partitions[partition_id].name);
    Serial.println("' boundary!");
    return false;
  }

  return true;
}

// Display partition table
void printPartitionTable() {
  if (!partition_initialized) {
    Serial.println("Partition table not initialized");
    return;
  }

  Serial.println("\nPartition Table:");
  Serial.println("================");
  Serial.print("Flash Size: ");
  Serial.print(flash_size / 1024);
  Serial.println(" KB");
  Serial.print("Sector Size: ");
  Serial.print(sector_size);
  Serial.println(" bytes\n");

  Serial.println("ID  Name          Start       Size        End         Size(KB)");
  Serial.println("--  ------------  ----------  ----------  ----------  --------");

  for (uint8_t i = 0; i < partition_count; i++) {
    if (partitions[i].size > 0) {
      char line[100];
      snprintf(line, sizeof(line), "%-2d  %-12s  0x%08X  0x%08X  0x%08X  %8u",
               i,
               partitions[i].name,
               partitions[i].start_address,
               partitions[i].size,
               partitions[i].start_address + partitions[i].size,
               partitions[i].size / 1024);
      Serial.println(line);
    }
  }
  Serial.println();
}

// Validate partitions don't overlap
bool validatePartitions() {
  for (uint8_t i = 0; i < partition_count; i++) {
    if (partitions[i].size == 0) continue;

    uint32_t i_end = partitions[i].start_address + partitions[i].size;

    for (uint8_t j = i + 1; j < partition_count; j++) {
      if (partitions[j].size == 0) continue;

      uint32_t j_end = partitions[j].start_address + partitions[j].size;

      // Check for overlap
      if (!(i_end <= partitions[j].start_address || j_end <= partitions[i].start_address)) {
        Serial.print("Error: Partitions '");
        Serial.print(partitions[i].name);
        Serial.print("' and '");
        Serial.print(partitions[j].name);
        Serial.println("' overlap!");
        return false;
      }
    }
  }

  return true;
}

// Partition IDs
enum PartitionID {
  PARTITION_CONFIG = 0,
  PARTITION_LOGGING,
  PARTITION_USER_FILES,
  PARTITION_BACKUP,
  PARTITION_COUNT
};

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println("QSPI Partitioning Example");
  Serial.println("=========================\n");

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

  // Initialize partition manager
  if (!initPartitions()) {
    Serial.println("Failed to initialize partition manager!");
    while (1) {
      delay(1000);
    }
  }

  // Define partition layout
  uint32_t total_flash_size = QSPI.getFlashSize();

  // CONFIG partition at the start (64KB)
  definePartition(PARTITION_CONFIG, "CONFIG", 0, 64 * 1024);

  // LOGGING partition after CONFIG (256KB)
  definePartition(PARTITION_LOGGING, "LOGGING", 64 * 1024, 256 * 1024);

  // BACKUP partition at the end (128KB)
  definePartition(PARTITION_BACKUP, "BACKUP", total_flash_size - 128 * 1024, 128 * 1024);

  // USER_FILES partition in the middle (remaining space)
  uint32_t user_start = 64 * 1024 + 256 * 1024;
  uint32_t user_size = (total_flash_size - 128 * 1024) - user_start;
  definePartition(PARTITION_USER_FILES, "USER_FILES", user_start, user_size);

  // Display partition table
  printPartitionTable();

  // Test each partition
  testConfigPartition();
  testLoggingPartition();
  testUserFilesPartition();
  testBackupPartition();

  Serial.println("\n=== All partition tests completed ===");
}

void loop() {
  // Nothing to do in loop
  delay(1000);
}

void testConfigPartition() {
  Serial.println("Testing CONFIG Partition:");
  Serial.println("-------------------------");

  // Erase partition first
  Serial.print("Erasing CONFIG partition... ");
  if (erasePartition(PARTITION_CONFIG)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    return;
  }

  // Simulate storing configuration data
  struct Config {
    uint32_t magic;
    uint8_t version;
    char device_name[32];
    uint32_t flags;
    uint32_t checksum;
  } config;

  config.magic = 0xC0FF1234;
  config.version = 1;
  strncpy(config.device_name, "QSPI-Device-001", sizeof(config.device_name));
  config.flags = 0x0000ABCD;
  config.checksum = 0xDEADBEEF;

  Serial.print("Writing config... ");
  if (writePartition(PARTITION_CONFIG, 0, &config, sizeof(config))) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    return;
  }

  // Read back and verify
  Config read_config;
  Serial.print("Reading config... ");
  if (readPartition(PARTITION_CONFIG, 0, &read_config, sizeof(read_config))) {
    Serial.println("OK");

    Serial.print("  Magic: 0x");
    Serial.println(read_config.magic, HEX);
    Serial.print("  Version: ");
    Serial.println(read_config.version);
    Serial.print("  Device Name: ");
    Serial.println(read_config.device_name);
    Serial.print("  Flags: 0x");
    Serial.println(read_config.flags, HEX);

    if (memcmp(&config, &read_config, sizeof(config)) == 0) {
      Serial.println("  Verification: PASSED");
    } else {
      Serial.println("  Verification: FAILED");
    }
  } else {
    Serial.println("FAILED");
  }

  Serial.println();
}

void testLoggingPartition() {
  Serial.println("Testing LOGGING Partition:");
  Serial.println("--------------------------");

  // Erase partition first
  Serial.print("Erasing LOGGING partition... ");
  if (erasePartition(PARTITION_LOGGING)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    return;
  }

  // Simulate logging sensor data
  struct LogEntry {
    uint32_t timestamp;
    float temperature;
    float humidity;
    uint16_t pressure;
  };

  const int num_entries = 5;
  LogEntry logs[num_entries];

  // Generate sample log entries
  for (int i = 0; i < num_entries; i++) {
    logs[i].timestamp = millis() + i * 1000;
    logs[i].temperature = 20.0 + i * 0.5;
    logs[i].humidity = 45.0 + i * 2.0;
    logs[i].pressure = 1013 + i;
  }

  Serial.print("Writing ");
  Serial.print(num_entries);
  Serial.print(" log entries... ");
  if (writePartition(PARTITION_LOGGING, 0, logs, sizeof(logs))) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    return;
  }

  // Read back
  LogEntry read_logs[num_entries];
  Serial.print("Reading log entries... ");
  if (readPartition(PARTITION_LOGGING, 0, read_logs, sizeof(read_logs))) {
    Serial.println("OK");

    Serial.println("  Log entries:");
    for (int i = 0; i < num_entries; i++) {
      Serial.print("    Entry ");
      Serial.print(i);
      Serial.print(": T=");
      Serial.print(read_logs[i].temperature);
      Serial.print("C, H=");
      Serial.print(read_logs[i].humidity);
      Serial.print("%, P=");
      Serial.println(read_logs[i].pressure);
    }
  } else {
    Serial.println("FAILED");
  }

  Serial.println();
}

void testUserFilesPartition() {
  Serial.println("Testing USER_FILES Partition:");
  Serial.println("-----------------------------");

  const char* file1 = "This is user file 1 data";
  const char* file2 = "User file 2 contains different content";

  Serial.print("Writing file 1 at offset 0... ");
  if (writePartition(PARTITION_USER_FILES, 0, file1, strlen(file1) + 1)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
  }

  Serial.print("Writing file 2 at offset 4KB... ");
  if (writePartition(PARTITION_USER_FILES, 4096, file2, strlen(file2) + 1)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
  }

  // Read back
  char buffer[64];
  Serial.print("Reading file 1... ");
  if (readPartition(PARTITION_USER_FILES, 0, buffer, sizeof(buffer))) {
    Serial.println("OK");
    Serial.print("  Content: ");
    Serial.println(buffer);
  } else {
    Serial.println("FAILED");
  }

  Serial.print("Reading file 2... ");
  if (readPartition(PARTITION_USER_FILES, 4096, buffer, sizeof(buffer))) {
    Serial.println("OK");
    Serial.print("  Content: ");
    Serial.println(buffer);
  } else {
    Serial.println("FAILED");
  }

  Serial.println();
}

void testBackupPartition() {
  Serial.println("Testing BACKUP Partition:");
  Serial.println("-------------------------");

  // Erase partition first
  Serial.print("Erasing BACKUP partition... ");
  if (erasePartition(PARTITION_BACKUP)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    return;
  }

  const char* backup_data = "Critical backup data that should be preserved!";

  Serial.print("Writing backup data... ");
  if (writePartition(PARTITION_BACKUP, 0, backup_data, strlen(backup_data) + 1)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    return;
  }

  char buffer[64];
  Serial.print("Reading backup data... ");
  if (readPartition(PARTITION_BACKUP, 0, buffer, sizeof(buffer))) {
    Serial.println("OK");
    Serial.print("  Content: ");
    Serial.println(buffer);
  } else {
    Serial.println("FAILED");
  }

  Serial.println();
}
