/*
  QSPIStorage - Advanced Storage Usage Example

  This example demonstrates advanced storage operations including:
  - Binary data storage with structs
  - Data logging with rotation
  - Recursive folder operations
  - Configuration file management
  - Error recovery patterns
  - Storage statistics and health monitoring

  Note: QSPI flash must be configured in the board's device tree overlay.
*/

#include <QSPIStorage.h>

QSPIStorage storage;

// Sensor data structure for binary storage
struct SensorReading {
    uint32_t timestamp;
    float temperature;
    float humidity;
    float pressure;
    uint16_t lightLevel;
    uint8_t batteryPercent;
    uint8_t flags;
};

// Configuration structure
struct DeviceConfig {
    char deviceName[32];
    uint32_t sampleInterval;
    uint32_t logRotationSize;
    bool debugEnabled;
    uint8_t logLevel;
    uint8_t reserved[26];  // Padding to 64 bytes
};

// Constants
const char* CONFIG_FILE = "/qspi/config.bin";
const char* DATA_DIR = "/qspi/data";
const char* LOG_DIR = "/qspi/logs";
const uint32_t MAX_LOG_SIZE = 4096;  // Rotate logs at 4KB

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    Serial.println("QSPIStorage - Advanced Usage Example");
    Serial.println("=====================================\n");

    // Initialize storage with error recovery
    if (!initializeWithRecovery()) {
        Serial.println("Fatal: Could not initialize storage");
        while (1) delay(1000);
    }

    // Setup directory structure
    setupDirectoryStructure();

    // Demo advanced features
    demoConfigManagement();
    demoBinaryDataStorage();
    demoDataLogging();
    demoFolderOperations();
    demoStorageStatistics();

    Serial.println("\n=== All advanced demos completed ===");
}

void loop() {
    // Simulate periodic data logging
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 5000) {
        lastLog = millis();
        logSensorReading();
    }
}

// Initialize storage with automatic recovery
bool initializeWithRecovery() {
    StorageError error;

    Serial.println("Initializing storage with recovery...");

    // First attempt: normal mount
    if (storage.begin(&error)) {
        Serial.println("Storage mounted successfully");
        return true;
    }

    Serial.print("Initial mount failed: ");
    Serial.println(error.getMessage());

    // Recovery attempt based on error type
    switch (error.getCode()) {
        case StorageErrorCode::STORAGE_NOT_FORMATTED:
        case StorageErrorCode::STORAGE_CORRUPTED:
            Serial.println("Attempting format recovery...");
            error.clear();

            if (storage.format(FilesystemType::LITTLEFS, &error)) {
                Serial.println("Format successful, retrying mount...");
                error.clear();
                return storage.begin(&error);
            }
            Serial.print("Format failed: ");
            Serial.println(error.getMessage());
            break;

        case StorageErrorCode::HARDWARE_ERROR:
            Serial.println("Hardware error - check QSPI connections");
            break;

        default:
            Serial.println("Unknown error - cannot recover");
            break;
    }

    return false;
}

// Create the directory structure for the application
void setupDirectoryStructure() {
    Serial.println("\nSetting up directory structure...");

    StorageError error;
    QSPIFolder root = storage.getRootFolder(&error);

    // Create data directory
    QSPIFolder dataDir = root.createSubfolder("data", false, &error);
    if (!error || error.getCode() == StorageErrorCode::ALREADY_EXISTS) {
        Serial.println("  /data - OK");
    }
    error.clear();

    // Create logs directory
    QSPIFolder logsDir = root.createSubfolder("logs", false, &error);
    if (!error || error.getCode() == StorageErrorCode::ALREADY_EXISTS) {
        Serial.println("  /logs - OK");
    }
    error.clear();

    // Create backup directory
    QSPIFolder backupDir = root.createSubfolder("backup", false, &error);
    if (!error || error.getCode() == StorageErrorCode::ALREADY_EXISTS) {
        Serial.println("  /backup - OK");
    }

    Serial.println();
}

// Demo: Configuration file management
void demoConfigManagement() {
    Serial.println("--- Demo: Configuration Management ---");

    StorageError error;
    DeviceConfig config;

    // Try to load existing config
    if (loadConfig(config, &error)) {
        Serial.println("Loaded existing configuration:");
    } else {
        Serial.println("No config found, creating default...");

        // Initialize default config
        strncpy(config.deviceName, "QSPI_Device_001", sizeof(config.deviceName));
        config.sampleInterval = 1000;
        config.logRotationSize = MAX_LOG_SIZE;
        config.debugEnabled = true;
        config.logLevel = 2;

        if (!saveConfig(config, &error)) {
            Serial.print("Failed to save config: ");
            Serial.println(error.getMessage());
            return;
        }
        Serial.println("Default configuration saved.");
    }

    // Display config
    Serial.print("  Device Name: ");
    Serial.println(config.deviceName);
    Serial.print("  Sample Interval: ");
    Serial.print(config.sampleInterval);
    Serial.println(" ms");
    Serial.print("  Log Rotation Size: ");
    Serial.print(config.logRotationSize);
    Serial.println(" bytes");
    Serial.print("  Debug Enabled: ");
    Serial.println(config.debugEnabled ? "Yes" : "No");
    Serial.print("  Log Level: ");
    Serial.println(config.logLevel);

    // Modify and save config
    config.sampleInterval = 2000;
    if (saveConfig(config, &error)) {
        Serial.println("Configuration updated successfully");
    }

    Serial.println();
}

// Load configuration from binary file
bool loadConfig(DeviceConfig& config, StorageError* error) {
    QSPIFile file(CONFIG_FILE);

    if (!file.exists(error)) {
        return false;
    }

    if (!file.open(FileMode::READ, error)) {
        return false;
    }

    size_t bytesRead = file.read(reinterpret_cast<uint8_t*>(&config), sizeof(config), error);
    file.close(error);

    return (bytesRead == sizeof(config));
}

// Save configuration to binary file
bool saveConfig(const DeviceConfig& config, StorageError* error) {
    QSPIFile file(CONFIG_FILE);

    if (!file.open(FileMode::WRITE, error)) {
        return false;
    }

    size_t bytesWritten = file.write(reinterpret_cast<const uint8_t*>(&config), sizeof(config), error);
    file.flush(error);
    file.close(error);

    return (bytesWritten == sizeof(config));
}

// Demo: Binary data storage
void demoBinaryDataStorage() {
    Serial.println("--- Demo: Binary Data Storage ---");

    StorageError error;

    // Create sample sensor readings
    const int NUM_READINGS = 10;
    SensorReading readings[NUM_READINGS];

    Serial.println("Generating sensor readings...");
    for (int i = 0; i < NUM_READINGS; i++) {
        readings[i].timestamp = millis() + (i * 100);
        readings[i].temperature = 20.0 + (random(100) / 10.0);
        readings[i].humidity = 40.0 + (random(200) / 10.0);
        readings[i].pressure = 1000.0 + (random(50) / 10.0);
        readings[i].lightLevel = random(1024);
        readings[i].batteryPercent = 80 + random(20);
        readings[i].flags = 0x01;
    }

    // Write binary data
    QSPIFile dataFile("/qspi/data/readings.bin");

    if (dataFile.open(FileMode::WRITE, &error)) {
        // Write header (number of readings)
        uint32_t count = NUM_READINGS;
        dataFile.write(reinterpret_cast<uint8_t*>(&count), sizeof(count), &error);

        // Write all readings
        size_t dataSize = sizeof(SensorReading) * NUM_READINGS;
        size_t written = dataFile.write(reinterpret_cast<uint8_t*>(readings), dataSize, &error);
        dataFile.close(&error);

        Serial.print("Wrote ");
        Serial.print(written + sizeof(count));
        Serial.println(" bytes of binary data");
    }

    // Read binary data back
    if (dataFile.open(FileMode::READ, &error)) {
        uint32_t readCount;
        dataFile.read(reinterpret_cast<uint8_t*>(&readCount), sizeof(readCount), &error);

        Serial.print("Reading ");
        Serial.print(readCount);
        Serial.println(" sensor readings:");

        SensorReading reading;
        for (uint32_t i = 0; i < min(readCount, (uint32_t)3); i++) {  // Show first 3
            dataFile.read(reinterpret_cast<uint8_t*>(&reading), sizeof(reading), &error);
            Serial.print("  [");
            Serial.print(i);
            Serial.print("] T=");
            Serial.print(reading.temperature, 1);
            Serial.print("C, H=");
            Serial.print(reading.humidity, 1);
            Serial.print("%, P=");
            Serial.print(reading.pressure, 1);
            Serial.println("hPa");
        }
        if (readCount > 3) {
            Serial.println("  ...");
        }

        dataFile.close(&error);
    }

    Serial.println();
}

// Demo: Data logging with rotation
void demoDataLogging() {
    Serial.println("--- Demo: Data Logging with Rotation ---");

    StorageError error;

    // Create some log entries
    for (int i = 0; i < 5; i++) {
        String logEntry = "[" + String(millis()) + "] INFO: Sample log entry #" + String(i + 1);
        appendToLog("app.log", logEntry, &error);
        delay(50);
    }

    // Show log content
    QSPIFile logFile("/qspi/logs/app.log");
    if (logFile.open(FileMode::READ, &error)) {
        Serial.println("Current log content:");
        Serial.println("---");
        Serial.println(logFile.readAsString(&error));
        Serial.println("---");
        Serial.print("Log size: ");
        Serial.print(logFile.size(&error));
        Serial.println(" bytes");
        logFile.close(&error);
    }

    Serial.println();
}

// Append to log file with automatic rotation
void appendToLog(const char* logName, const String& message, StorageError* error) {
    char logPath[64];
    snprintf(logPath, sizeof(logPath), "%s/%s", LOG_DIR, logName);

    QSPIFile logFile(logPath);

    // Check if rotation is needed
    if (logFile.exists(error) && logFile.size(error) > MAX_LOG_SIZE) {
        rotateLog(logName, error);
    }

    // Append new entry
    if (logFile.open(FileMode::APPEND, error)) {
        logFile.write(message + "\n", error);
        logFile.close(error);
    }
}

// Rotate log file
void rotateLog(const char* logName, StorageError* error) {
    char currentPath[64];
    char backupPath[64];

    snprintf(currentPath, sizeof(currentPath), "%s/%s", LOG_DIR, logName);
    snprintf(backupPath, sizeof(backupPath), "%s/%s.old", LOG_DIR, logName);

    // Delete old backup if exists
    QSPIFile oldBackup(backupPath);
    if (oldBackup.exists(error)) {
        oldBackup.remove(error);
    }

    // Rename current to backup
    QSPIFile current(currentPath);
    current.rename(backupPath, error);

    Serial.println("Log rotated");
}

// Log a single sensor reading (called from loop)
void logSensorReading() {
    StorageError error;

    // Generate fake reading
    SensorReading reading;
    reading.timestamp = millis();
    reading.temperature = 22.0 + (random(50) / 10.0);
    reading.humidity = 45.0;
    reading.pressure = 1013.25;
    reading.lightLevel = random(1024);
    reading.batteryPercent = 95;
    reading.flags = 0;

    String logEntry = "[" + String(reading.timestamp) + "] SENSOR: T=" +
                      String(reading.temperature, 1) + "C, L=" +
                      String(reading.lightLevel);
    appendToLog("sensor.log", logEntry, &error);
}

// Demo: Folder operations
void demoFolderOperations() {
    Serial.println("--- Demo: Folder Operations ---");

    StorageError error;
    QSPIFolder root = storage.getRootFolder(&error);

    // Create nested folder structure
    Serial.println("Creating nested folders...");
    QSPIFolder dataFolder = root.getSubfolder("data", &error);

    QSPIFolder year2024 = dataFolder.createSubfolder("2024", false, &error);
    QSPIFolder jan = year2024.createSubfolder("01", false, &error);
    QSPIFolder feb = year2024.createSubfolder("02", false, &error);

    // Create files in nested folders
    QSPIFile janData = jan.createFile("data.csv", FileMode::WRITE, &error);
    if (janData.isOpen()) {
        janData.write("timestamp,value\n1704067200,42\n", &error);
        janData.close(&error);
    }

    // Count items
    Serial.print("Files in /data/2024: ");
    Serial.println(year2024.getFileCount(&error));
    Serial.print("Folders in /data/2024: ");
    Serial.println(year2024.getFolderCount(&error));

    // List all folders
    Serial.println("\nFolders in /data/2024:");
    std::vector<QSPIFolder> subfolders = year2024.getFolders(&error);
    for (auto& folder : subfolders) {
        Serial.print("  [DIR] ");
        Serial.println(folder.getFolderName());

        // List files in each subfolder
        std::vector<QSPIFile> files = folder.getFiles(&error);
        for (auto& file : files) {
            Serial.print("        - ");
            Serial.println(file.getFilename());
        }
    }

    // Get parent folder
    QSPIFolder parent = jan.getParentFolder(&error);
    Serial.print("\nParent of /data/2024/01: ");
    Serial.println(parent.getPath());

    Serial.println();
}

// Demo: Storage statistics
void demoStorageStatistics() {
    Serial.println("--- Demo: Storage Statistics ---");

    StorageError error;
    size_t total, used, available;

    if (storage.getStorageInfo(total, used, available, &error)) {
        Serial.println("Storage Statistics:");
        Serial.println("-------------------");
        Serial.print("Total Space:     ");
        printSize(total);
        Serial.print("Used Space:      ");
        printSize(used);
        Serial.print("Available Space: ");
        printSize(available);
        Serial.print("Usage:           ");
        Serial.print((used * 100) / total);
        Serial.println("%");
    }

    // Count all files recursively
    QSPIFolder root = storage.getRootFolder(&error);
    uint32_t fileCount = 0;
    uint32_t folderCount = 0;
    countRecursive(root, fileCount, folderCount, &error);

    Serial.print("\nTotal Files:   ");
    Serial.println(fileCount);
    Serial.print("Total Folders: ");
    Serial.println(folderCount);

    Serial.println();
}

// Helper: Print size in human-readable format
void printSize(size_t bytes) {
    if (bytes >= 1024 * 1024) {
        Serial.print(bytes / (1024 * 1024));
        Serial.println(" MB");
    } else if (bytes >= 1024) {
        Serial.print(bytes / 1024);
        Serial.println(" KB");
    } else {
        Serial.print(bytes);
        Serial.println(" bytes");
    }
}

// Helper: Count files and folders recursively
void countRecursive(QSPIFolder& folder, uint32_t& fileCount, uint32_t& folderCount, StorageError* error) {
    std::vector<QSPIFile> files = folder.getFiles(error);
    fileCount += files.size();

    std::vector<QSPIFolder> subfolders = folder.getFolders(error);
    folderCount += subfolders.size();

    for (auto& subfolder : subfolders) {
        countRecursive(subfolder, fileCount, folderCount, error);
    }
}
