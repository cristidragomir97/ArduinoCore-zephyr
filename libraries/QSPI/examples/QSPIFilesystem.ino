/*
  QSPI Filesystem Example with LittleFS

  This example demonstrates how to use Zephyr's LittleFS filesystem
  to store and retrieve files on external QSPI flash memory.

  Features:
  - Create, write, and read files on the auto-mounted /storage partition
  - List directory contents
  - Check filesystem statistics

  The filesystem is automatically mounted at boot via device tree FSTAB.
  No manual mounting is required - just use files at /storage/filename.

  Note:
  - QSPI flash partitions are defined in the board's device tree overlay
  - The /storage partition uses LittleFS (7MB user data)
  - The /wlan: and /ota: partitions use FAT
*/

#include <Arduino.h>
#include <zephyr/fs/fs.h>

// Mount point for the LittleFS user data partition (auto-mounted via FSTAB)
#define MOUNT_POINT "/storage"

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println("QSPI Filesystem Example");
  Serial.println("========================\n");

  // Check if filesystem is mounted (should be auto-mounted via FSTAB)
  struct fs_statvfs stats;
  int ret = fs_statvfs(MOUNT_POINT, &stats);

  if (ret == 0) {
    Serial.println("Filesystem is mounted at " MOUNT_POINT);
  } else {
    Serial.print("Filesystem not available (error: ");
    Serial.print(ret);
    Serial.println(")");
    Serial.println("Note: The filesystem should be auto-mounted via FSTAB.");
    Serial.println("You may need to format the partition first using the");
    Serial.println("FlashFormat example from the Storage library.");
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

  // List all mounted filesystems
  listMounts();
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

  if (written == (ssize_t)strlen(data)) {
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

void listMounts() {
  Serial.println("\n=== Mounted Filesystems ===");

  const char *mnt_point;
  int idx = 0;
  int res;
  bool found = false;

  while (true) {
    res = fs_readmount(&idx, &mnt_point);
    if (res < 0) {
      break;
    }

    Serial.print("Mount point ");
    Serial.print(idx - 1);
    Serial.print(": ");
    Serial.print(mnt_point);

    // Detect filesystem type by mount point naming convention
    // FAT mount points typically end with ':', LittleFS mount points don't
    size_t len = strlen(mnt_point);
    if (len > 0 && mnt_point[len - 1] == ':') {
      Serial.print(" (FAT)");
    } else {
      Serial.print(" (LittleFS)");
    }
    Serial.println();

    found = true;
  }

  if (!found) {
    Serial.println("No mounted filesystems found!");
  }
}
