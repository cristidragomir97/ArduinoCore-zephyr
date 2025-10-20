/*
  Basic QSPI Flash Example

  This example demonstrates how to use the QSPI library to read and write
  data to external QSPI flash memory on Arduino boards with QSPI support.

  Note: QSPI flash must be configured in the board's device tree overlay.
*/

#include <QSPI.h>

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println("QSPI Flash Test");

  // Initialize QSPI flash
  if (!QSPI.begin()) {
    Serial.println("Failed to initialize QSPI flash!");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("QSPI flash initialized successfully");

  // Get flash information
  Serial.print("Flash size: ");
  Serial.print(QSPI.getFlashSize());
  Serial.println(" bytes");

  Serial.print("Sector size: ");
  Serial.print(QSPI.getSectorSize());
  Serial.println(" bytes");

  Serial.print("Page size: ");
  Serial.print(QSPI.getPageSize());
  Serial.println(" bytes");

  // Test write and read
  testWriteRead();
}

void loop() {
  // Nothing to do in loop
  delay(5000);

  Serial.println("Running periodic test...");
  testWriteRead();
}

void testWriteRead() {
  const uint32_t test_address = 0x1000;  // Test address (4KB offset)
  const char test_data[] = "Hello QSPI Flash!";
  char read_buffer[32];

  Serial.println("\n--- Testing Write/Read ---");

  // Erase sector first
  Serial.print("Erasing sector at 0x");
  Serial.print(test_address, HEX);
  Serial.print("... ");

  if (QSPI.erase(test_address, QSPI.getSectorSize())) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    return;
  }

  // Write test data
  Serial.print("Writing data... ");
  if (QSPI.write(test_address, test_data, strlen(test_data) + 1)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    return;
  }

  // Read back data
  Serial.print("Reading data... ");
  memset(read_buffer, 0, sizeof(read_buffer));
  if (QSPI.read(test_address, read_buffer, sizeof(read_buffer))) {
    Serial.println("OK");

    Serial.print("Read data: ");
    Serial.println(read_buffer);

    // Verify data
    if (strcmp(test_data, read_buffer) == 0) {
      Serial.println("Data verification: PASSED");
    } else {
      Serial.println("Data verification: FAILED");
    }
  } else {
    Serial.println("FAILED");
  }
}