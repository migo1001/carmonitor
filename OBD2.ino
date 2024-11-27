#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>

// BLE Configuration
#define ELM327_ADDRESS "D2:E0:2F:8D:65:A3"
#define CUSTOM_SERVICE_UUID "e7810a71-73ae-499d-8c15-faa9aef0c3f2"
#define CHARACTERISTIC_UUID "bef8d6c9-9c21-4c9e-b632-bd58c1009f9f"
#define BUFFER_MAX_SIZE 256  // Limit responseBuffer to 256 bytes

// Global Variables for ECU Data
float ecu_rpm = 0.0;
int ecu_speed = 0;

// Function to handle RPM command (010C)
void handleRPMCommand(const std::string& data) {
    if (data.length() >= 4) {
        int a = strtol(data.substr(0, 2).c_str(), nullptr, 16);
        int b = strtol(data.substr(2, 2).c_str(), nullptr, 16);
        ecu_rpm = ((a * 256) + b) / 4.0;
    }
}

// Function to handle Speed command (010D)
void handleSpeedCommand(const std::string& data) {
    if (data.length() >= 2) {
        ecu_speed = strtol(data.substr(0, 2).c_str(), nullptr, 16);
    }
}

// BLEHandler class
class BLEHandler {
private:
    static BLEHandler* instance;  // Static instance pointer
    BLEClient* pClient;
    BLERemoteCharacteristic* pCharacteristic;
    std::string responseBuffer;

    void processCompleteResponse() {
        Serial.printf("Processing Response: %s\n", responseBuffer.c_str());

        size_t pos = 0;
        while ((pos = responseBuffer.find("41")) != std::string::npos) {
            if (responseBuffer.length() < pos + 6) {
                Serial.println("Incomplete frame, clearing buffer.");
                responseBuffer.clear();
                return;
            }

            std::string frame = responseBuffer.substr(pos, 12);
            responseBuffer.erase(0, pos + 12);

            // Extract PID and Data
            std::string pid = frame.substr(2, 2);
            std::string data = frame.substr(4);

            // Handle PID
            if (pid == "0C") {
                handleRPMCommand(data);  // Process RPM
            } else if (pid == "0D") {
                handleSpeedCommand(data);  // Process Speed
            }
        }

        responseBuffer.clear();  // Clear buffer after processing
    }

    void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                        uint8_t* pData, size_t length, bool isNotify) {
        if (length > BUFFER_MAX_SIZE) {  // Discard oversized fragments
            Serial.println("Received oversized fragment, ignoring.");
            return;
        }

        std::string fragment((char*)pData, length);
        Serial.printf("Fragment Received: %s\n", fragment.c_str());

        // Append fragment to responseBuffer if within limits
        if (responseBuffer.length() + fragment.length() > BUFFER_MAX_SIZE) {
            Serial.println("Buffer overflow risk, clearing buffer.");
            responseBuffer.clear();
            return;
        }

        responseBuffer += fragment;

        // Process response if '>' indicates the end
        if (responseBuffer.find('>') != std::string::npos) {
            // Clean up response
            responseBuffer.erase(std::remove(responseBuffer.begin(), responseBuffer.end(), '\r'), responseBuffer.end());
            responseBuffer.erase(std::remove(responseBuffer.begin(), responseBuffer.end(), '\n'), responseBuffer.end());
            responseBuffer.erase(std::remove(responseBuffer.begin(), responseBuffer.end(), '>'), responseBuffer.end());

            processCompleteResponse();  // Process the response
        }
    }

    static void staticNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                                     uint8_t* pData, size_t length, bool isNotify) {
        if (instance) {
            instance->notifyCallback(pBLERemoteCharacteristic, pData, length, isNotify);
        }
    }

public:
    BLEHandler() : pClient(nullptr), pCharacteristic(nullptr) {
        instance = this;  // Set the static instance pointer
    }

    bool connect(const std::string& address, const std::string& serviceUUID, const std::string& characteristicUUID) {
        Serial.println("Initializing BLE...");
        BLEDevice::init("ESP32 Client");

        pClient = BLEDevice::createClient();
        BLEAddress deviceAddress(address);

        Serial.println("Connecting to device...");
        if (!pClient->connect(deviceAddress, BLE_ADDR_TYPE_RANDOM)) {
            Serial.println("Failed to connect.");
            return false;
        }
        Serial.println("Connected!");

        BLERemoteService* pService = pClient->getService(BLEUUID(serviceUUID));
        if (pService == nullptr) {
            Serial.println("Custom service not found.");
            return false;
        }
        Serial.println("Custom service found!");

        pCharacteristic = pService->getCharacteristic(BLEUUID(characteristicUUID));
        if (pCharacteristic == nullptr) {
            Serial.println("Characteristic not found.");
            return false;
        }

        if (pCharacteristic->canNotify()) {
            pCharacteristic->registerForNotify(staticNotifyCallback);
            Serial.println("Notifications enabled.");
        } else {
            Serial.println("Characteristic does not support notifications.");
            return false;
        }

        return true;
    }

    void sendCommand(const std::string& command) {
        if (pCharacteristic && pCharacteristic->canWrite()) {
            pCharacteristic->writeValue(command + "\r");
        }
    }

    void initializeELM327() {
        sendCommand("ATZ");  // Reset
        delay(500);
        sendCommand("ATE0"); // Disable echo
        delay(500);
        sendCommand("ATSP0"); // Set protocol
        delay(500);
        sendCommand("ATL0"); // Disable linefeeds
        delay(500);
        sendCommand("ATS0"); // No useless spaces
        delay(500);
        sendCommand("ATH1"); // Add Headers
        delay(500);
        sendCommand("ATAL"); // Allow long messages
        delay(500);
        Serial.println("ELM327 initialized.");
    }
};

// Initialize static member
BLEHandler* BLEHandler::instance = nullptr;

// Global BLEHandler object
BLEHandler bleHandler;

void setup() {
    Serial.begin(115200);

    if (!bleHandler.connect(ELM327_ADDRESS, CUSTOM_SERVICE_UUID, CHARACTERISTIC_UUID)) {
        Serial.println("Failed to initialize BLE communication.");
        return;
    }

    Serial.println("BLE communication initialized.");
    bleHandler.initializeELM327();
}

void loop() {
    // Send commands for RPM and Speed
    bleHandler.sendCommand("01 0C");  // Query RPM
    delay(500);

    bleHandler.sendCommand("01 0D");  // Query Speed
    delay(500);

    // Display ECU Data
    Serial.printf("Current RPM: %.2f | Current Speed: %d km/h\n", ecu_rpm, ecu_speed);
    delay(1000);  // Display data every second
}
