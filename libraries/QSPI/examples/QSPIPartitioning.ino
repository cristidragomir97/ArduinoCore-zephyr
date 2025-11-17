/*
  QSPI Partitioning Example

  This example demonstrates how to partition QSPI flash memory into
  logical regions for different purposes:
  - Configuration storage
  - Data logging
  - User files
  - Reserved/backup area

  Features:
  - Define multiple partitions with different sizes
  - Demonstrate isolated read/write operations per partition
  - Partition boundary checking
  - Efficient partition management

  Note: QSPI flash must be configured in the board's device tree overlay.
*/

#include <QSPI.h>

// Partition definitions
enum PartitionID {
  PARTITION_CONFIG = 0,    // Small partition for configuration (64KB)
  PARTITION_LOGGING,       // Medium partition for data logging (256KB)
  PARTITION_USER_FILES,    // Large partition for user files (remaining space - 128KB)
  PARTITION_BACKUP,        // Reserved backup partition (128KB)
  PARTITION_COUNT
};

struct Partition {
  const char* name;
  uint32_t start_address;
  uint32_t size;
};

// Partition table (will be initialized based on flash size)
Partition partitions[PARTITION_COUNT];

// Helper class for partition management
class PartitionManager {
public:
  static bool initialize() {
    uint32_t flash_size = QSPI.getFlashSize();
    uint32_t sector_size = QSPI.getSectorSize();

    if (flash_size == 0) {
      return false;
    }

    Serial.print("Initializing partition table for ");
    Serial.print(flash_size / 1024);
    Serial.println(" KB flash");

    // Define partition layout
    partitions[PARTITION_CONFIG] = {"CONFIG", 0, 64 * 1024};
    partitions[PARTITION_LOGGING] = {"LOGGING", partitions[PARTITION_CONFIG].start_address + partitions[PARTITION_CONFIG].size, 256 * 1024};
    partitions[PARTITION_BACKUP] = {"BACKUP", flash_size - 128 * 1024, 128 * 1024};
    partitions[PARTITION_USER_FILES] = {
      "USER_FILES",
      partitions[PARTITION_LOGGING].start_address + partitions[PARTITION_LOGGING].size,
      partitions[PARTITION_BACKUP].start_address - (partitions[PARTITION_LOGGING].start_address + partitions[PARTITION_LOGGING].size)
    };

    // Validate partitions
    for (int i = 0; i < PARTITION_COUNT; i++) {
      // Align to sector boundaries
      if (partitions[i].start_address % sector_size != 0) {
        Serial.print("Warning: Partition ");
        Serial.print(partitions[i].name);
        Serial.println(" is not sector-aligned!");
      }

      if (partitions[i].start_address + partitions[i].size > flash_size) {
        Serial.print("Error: Partition ");
        Serial.print(partitions[i].name);
        Serial.println(" exceeds flash size!");
        return false;
      }
    }

    return true;
  }

  static void printPartitionTable() {
    Serial.println("\nPartition Table:");
    Serial.println("================");
    Serial.println("ID  Name          Start       Size        End");
    Serial.println("--  ------------  ----------  ----------  ----------");

    for (int i = 0; i < PARTITION_COUNT; i++) {
      char line[80];
      snprintf(line, sizeof(line), "%-2d  %-12s  0x%08X  0x%08X  0x%08X",
               i,
               partitions[i].name,
               partitions[i].start_address,
               partitions[i].size,
               partitions[i].start_address + partitions[i].size);
      Serial.println(line);
    }
    Serial.println();
  }

  static bool writeToPartition(PartitionID id, uint32_t offset, const void* data, size_t size) {
    if (id >= PARTITION_COUNT) {
      return false;
    }

    uint32_t address = partitions[id].start_address + offset;

    // Boundary check
    if (offset + size > partitions[id].size) {
      Serial.print("Error: Write exceeds partition ");
      Serial.print(partitions[id].name);
      Serial.println(" boundary!");
      return false;
    }

    return QSPI.write(address, data, size);
  }

  static bool readFromPartition(PartitionID id, uint32_t offset, void* data, size_t size) {
    if (id >= PARTITION_COUNT) {
      return false;
    }

    uint32_t address = partitions[id].start_address + offset;

    // Boundary check
    if (offset + size > partitions[id].size) {
      Serial.print("Error: Read exceeds partition ");
      Serial.print(partitions[id].name);
      Serial.println(" boundary!");
      return false;
    }

    return QSPI.read(address, data, size);
  }

  static bool erasePartition(PartitionID id) {
    if (id >= PARTITION_COUNT) {
      return false;
    }

    Serial.print("Erasing partition ");
    Serial.print(partitions[id].name);
    Serial.print("... ");

    bool result = QSPI.erase(partitions[id].start_address, partitions[id].size);

    if (result) {
      Serial.println("OK");
    } else {
      Serial.println("FAILED");
    }

    return result;
  }

  static uint32_t getPartitionSize(PartitionID id) {
    if (id >= PARTITION_COUNT) {
      return 0;
    }
    return partitions[id].size;
  }

  static const char* getPartitionName(PartitionID id) {
    if (id >= PARTITION_COUNT) {
      return "UNKNOWN";
    }
    return partitions[id].name;
  }
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

  // Initialize partition table
  if (!PartitionManager::initialize()) {
    Serial.println("Failed to initialize partition table!");
    while (1) {
      delay(1000);
    }
  }

  // Display partition table
  PartitionManager::printPartitionTable();

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
  PartitionManager::erasePartition(PARTITION_CONFIG);

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
  if (PartitionManager::writeToPartition(PARTITION_CONFIG, 0, &config, sizeof(config))) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    return;
  }

  // Read back and verify
  Config read_config;
  Serial.print("Reading config... ");
  if (PartitionManager::readFromPartition(PARTITION_CONFIG, 0, &read_config, sizeof(read_config))) {
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
  PartitionManager::erasePartition(PARTITION_LOGGING);

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
  if (PartitionManager::writeToPartition(PARTITION_LOGGING, 0, logs, sizeof(logs))) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    return;
  }

  // Read back
  LogEntry read_logs[num_entries];
  Serial.print("Reading log entries... ");
  if (PartitionManager::readFromPartition(PARTITION_LOGGING, 0, read_logs, sizeof(read_logs))) {
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

  // Don't erase - just show we can write to different offsets
  const char* file1 = "This is user file 1 data";
  const char* file2 = "User file 2 contains different content";

  Serial.print("Writing file 1 at offset 0... ");
  if (PartitionManager::writeToPartition(PARTITION_USER_FILES, 0, file1, strlen(file1) + 1)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
  }

  Serial.print("Writing file 2 at offset 4KB... ");
  if (PartitionManager::writeToPartition(PARTITION_USER_FILES, 4096, file2, strlen(file2) + 1)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
  }

  // Read back
  char buffer[64];
  Serial.print("Reading file 1... ");
  if (PartitionManager::readFromPartition(PARTITION_USER_FILES, 0, buffer, sizeof(buffer))) {
    Serial.println("OK");
    Serial.print("  Content: ");
    Serial.println(buffer);
  } else {
    Serial.println("FAILED");
  }

  Serial.print("Reading file 2... ");
  if (PartitionManager::readFromPartition(PARTITION_USER_FILES, 4096, buffer, sizeof(buffer))) {
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
  PartitionManager::erasePartition(PARTITION_BACKUP);

  const char* backup_data = "Critical backup data that should be preserved!";

  Serial.print("Writing backup data... ");
  if (PartitionManager::writeToPartition(PARTITION_BACKUP, 0, backup_data, strlen(backup_data) + 1)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    return;
  }

  char buffer[64];
  Serial.print("Reading backup data... ");
  if (PartitionManager::readFromPartition(PARTITION_BACKUP, 0, buffer, sizeof(buffer))) {
    Serial.println("OK");
    Serial.print("  Content: ");
    Serial.println(buffer);
  } else {
    Serial.println("FAILED");
  }

  Serial.println();
}
