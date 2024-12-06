#include <NimBLEDevice.h>

#define TARGET_DEVICE_ADDRESS "D2:E0:2F:8D:65:A3"
#define SERVICE_UUID "e7810a71-73ae-499d-8c15-faa9aef0c3f2"
#define CHARACTERISTIC_UUID "bef8d6c9-9c21-4c9e-b632-bd58c1009f9f"

// Initialization commands
const char* initCommands[] = {
    "ATZ",   // Reset
    "ATE0",  // Disable echo
    "ATL0",  // Disable linefeeds
    "ATS0",  // Disable spaces
    "ATH0",  // Disable headers
    "ATSP6"  // Set protocol to ISO 15765-4 CAN
};
const size_t numInitCommands = sizeof(initCommands) / sizeof(initCommands[0]);

// OBD-II Command for engine RPM
const char* ENGINE_RPM_COMMAND = "010C\r";

// Global variable for latest RPM
String latestRPM = "";

// Notification handler
void notifyCallback(NimBLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    String rawResponse = String((char*)pData).substring(0, length);
    rawResponse.trim();

    //Serial.printf("Raw Response: %s\n", rawResponse.c_str());

    // Ignore prompt ">"
    if (rawResponse == ">") {
        return;
    }

    // Skip command echo "010C"
    if (rawResponse.startsWith("010C")) {
        return;
    }

    // Parse valid RPM response
    if (rawResponse.startsWith("410C")) {
        if (rawResponse.length() >= 8) {  // Ensure response length is sufficient
            int A = strtol(rawResponse.substring(4, 6).c_str(), nullptr, 16);
            int B = strtol(rawResponse.substring(6, 8).c_str(), nullptr, 16);
            int rpm = ((A * 256) + B) / 4;

            latestRPM = String(rpm) + " RPM";
            Serial.printf("Engine RPM: %s\n", latestRPM.c_str());
        } else {
            Serial.println("Incomplete RPM response.");
        }
    } else {
        Serial.println("Invalid RPM response.");
    }
}

void sendCommand(NimBLERemoteCharacteristic* pWriteCharacteristic, const char* command) {
    if (pWriteCharacteristic) {
        // Send the command
        Serial.printf("Sending command: %s\n", command);
        pWriteCharacteristic->writeValue((uint8_t*)command, strlen(command), false);
    }
}

// Initialize the ELM327 with required commands
void initializeELM327(NimBLERemoteCharacteristic* pWriteCharacteristic) {
    for (size_t i = 0; i < numInitCommands; ++i) {
        sendCommand(pWriteCharacteristic, initCommands[i]);
        delay(500); // Allow time for the device to process the command
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting NimBLE Client...");
    NimBLEDevice::init("ESP32-ELM327");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Max power

    // Connect to the device
    NimBLEClient* client = NimBLEDevice::createClient();
    if (!client->connect(NimBLEAddress(TARGET_DEVICE_ADDRESS, BLE_ADDR_RANDOM))) {
        Serial.println("Failed to connect to ELM327.");
        return;
    }
    Serial.println("Connected to ELM327.");

    // Get the characteristic for write and notifications
    NimBLERemoteCharacteristic* pCharacteristic = client->getService(SERVICE_UUID)
                                                    ->getCharacteristic(CHARACTERISTIC_UUID);

    if (!pCharacteristic) {
        Serial.println("Characteristic not found.");
        client->disconnect();
        return;
    }

    // Check characteristic properties
    if (!pCharacteristic->canWrite() || !pCharacteristic->canNotify()) {
        Serial.println("Characteristic is not writable or does not support notifications.");
        client->disconnect();
        return;
    }
    Serial.println("Found writable and notifiable characteristic.");

    // Register for notifications
    if (!pCharacteristic->registerForNotify(notifyCallback)) {
        Serial.println("Failed to register for notifications.");
        client->disconnect();
        return;
    }
    Serial.println("Registered for notifications.");

    // Initialize ELM327
    initializeELM327(pCharacteristic);

    Serial.println("Starting continuous monitoring...");

    // Main loop to send RPM commands
    while (client->isConnected()) {
        sendCommand(pCharacteristic, ENGINE_RPM_COMMAND); // Request RPM
        delay(1000); // Wait 1 second before sending the next command
    }

    client->disconnect();
    Serial.println("Disconnected from ELM327.");
}

void loop() {
    // Empty loop since the main logic is inside setup
}
