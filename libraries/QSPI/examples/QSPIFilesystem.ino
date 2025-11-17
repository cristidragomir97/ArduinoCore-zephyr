/*
  QSPI Filesystem Example

  This example demonstrates how to use the QSPI library with LittleFS
  filesystem to store and retrieve files on external QSPI flash memory.

  Features:
  - Mount LittleFS on QSPI flash
  - Create, write, and read files
  - List directory contents
  - Check filesystem statistics

  Note:
  - QSPI flash must be configured in the board's device tree overlay
  - Zephyr CONFIG_FILE_SYSTEM=y and CONFIG_FILE_SYSTEM_LITTLEFS=y must be enabled
  - Add to prj.conf:
      CONFIG_FILE_SYSTEM=y
      CONFIG_FILE_SYSTEM_LITTLEFS=y
      CONFIG_FILE_SYSTEM_MAX_FILE_NAME=128
*/

#include <QSPI.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>

// Mount point for the filesystem
#define MOUNT_POINT "/qspi"

// LittleFS configuration
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t mp = {
    .type = FS_LITTLEFS,
    .fs_data = &storage,
    .mnt_point = MOUNT_POINT,
};

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println("QSPI Filesystem Example");
  Serial.println("========================\n");

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
  Serial.println(" KB\n");

  // Mount the filesystem
  Serial.print("Mounting LittleFS at " MOUNT_POINT "... ");
  int ret = fs_mount(&mp);

  if (ret == 0) {
    Serial.println("OK");
  } else if (ret == -EBUSY) {
    Serial.println("Already mounted");
  } else {
    Serial.print("FAILED (");
    Serial.print(ret);
    Serial.println(")");
    Serial.println("Note: First mount may fail - filesystem might need formatting");
    Serial.println("Try erasing the flash first or format the filesystem");
    while (1) {
      delay(1000);
    }
  }

  // Show filesystem statistics
  printFilesystemStats();

  // Test filesystem operations
  testFileOperations();

  // List files
  listFiles();
}

void loop() {
  // Nothing to do in loop
  delay(1000);
}

void printFilesystemStats() {
  struct fs_statvfs stats;
  int ret = fs_statvfs(MOUNT_POINT, &stats);

  if (ret == 0) {
    Serial.println("\nFilesystem Statistics:");
    Serial.print("  Block size: ");
    Serial.print(stats.f_bsize);
    Serial.println(" bytes");

    Serial.print("  Total blocks: ");
    Serial.println(stats.f_blocks);

    Serial.print("  Free blocks: ");
    Serial.println(stats.f_bfree);

    uint32_t total_kb = (stats.f_blocks * stats.f_bsize) / 1024;
    uint32_t free_kb = (stats.f_bfree * stats.f_bsize) / 1024;
    uint32_t used_kb = total_kb - free_kb;

    Serial.print("  Total space: ");
    Serial.print(total_kb);
    Serial.println(" KB");

    Serial.print("  Used space: ");
    Serial.print(used_kb);
    Serial.println(" KB");

    Serial.print("  Free space: ");
    Serial.print(free_kb);
    Serial.println(" KB\n");
  } else {
    Serial.print("Failed to get filesystem stats: ");
    Serial.println(ret);
  }
}

void testFileOperations() {
  Serial.println("Testing File Operations:");
  Serial.println("------------------------");

  // Create and write to a file
  const char *filepath = MOUNT_POINT "/test.txt";
  const char *data = "Hello from QSPI filesystem!\nThis is a test file.\n";

  Serial.print("Writing file: ");
  Serial.print(filepath);
  Serial.print("... ");

  struct fs_file_t file;
  fs_file_t_init(&file);

  int ret = fs_open(&file, filepath, FS_O_CREATE | FS_O_WRITE);
  if (ret < 0) {
    Serial.print("FAILED to open (");
    Serial.print(ret);
    Serial.println(")");
    return;
  }

  ssize_t written = fs_write(&file, data, strlen(data));
  fs_close(&file);

  if (written == strlen(data)) {
    Serial.print("OK (");
    Serial.print(written);
    Serial.println(" bytes)");
  } else {
    Serial.println("FAILED");
    return;
  }

  // Read the file back
  Serial.print("Reading file... ");
  char read_buffer[128];
  memset(read_buffer, 0, sizeof(read_buffer));

  fs_file_t_init(&file);
  ret = fs_open(&file, filepath, FS_O_READ);
  if (ret < 0) {
    Serial.print("FAILED to open (");
    Serial.print(ret);
    Serial.println(")");
    return;
  }

  ssize_t bytes_read = fs_read(&file, read_buffer, sizeof(read_buffer) - 1);
  fs_close(&file);

  if (bytes_read > 0) {
    Serial.print("OK (");
    Serial.print(bytes_read);
    Serial.println(" bytes)");
    Serial.println("\nFile contents:");
    Serial.println("---");
    Serial.print(read_buffer);
    Serial.println("---\n");
  } else {
    Serial.println("FAILED");
  }

  // Write a second file with sensor data simulation
  const char *datafile = MOUNT_POINT "/sensor_data.txt";
  Serial.print("Creating sensor data file... ");

  fs_file_t_init(&file);
  ret = fs_open(&file, datafile, FS_O_CREATE | FS_O_WRITE);
  if (ret == 0) {
    // Simulate writing sensor readings
    for (int i = 0; i < 10; i++) {
      char line[64];
      snprintf(line, sizeof(line), "Reading %d: Temperature=%.1f C, Humidity=%.1f%%\n",
               i, 20.0 + i * 0.5, 45.0 + i * 1.2);
      fs_write(&file, line, strlen(line));
    }
    fs_close(&file);
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
  }
}

void listFiles() {
  Serial.println("\nDirectory Listing:");
  Serial.println("------------------");

  struct fs_dir_t dir;
  fs_dir_t_init(&dir);

  int ret = fs_opendir(&dir, MOUNT_POINT);
  if (ret < 0) {
    Serial.print("Failed to open directory (");
    Serial.print(ret);
    Serial.println(")");
    return;
  }

  struct fs_dirent entry;
  int count = 0;

  while (true) {
    ret = fs_readdir(&dir, &entry);
    if (ret < 0) {
      Serial.println("Error reading directory");
      break;
    }

    if (entry.name[0] == 0) {
      // End of directory
      break;
    }

    Serial.print("  ");
    if (entry.type == FS_DIR_ENTRY_DIR) {
      Serial.print("[DIR]  ");
    } else {
      Serial.print("[FILE] ");
    }
    Serial.print(entry.name);
    Serial.print(" (");
    Serial.print(entry.size);
    Serial.println(" bytes)");

    count++;
  }

  fs_closedir(&dir);

  if (count == 0) {
    Serial.println("  (empty)");
  }

  Serial.print("\nTotal items: ");
  Serial.println(count);
}
