#include <NimBLEDevice.h>
#include <string>

#define TARGET_DEVICE_ADDRESS "D2:E0:2F:8D:65:A3"
#define SERVICE_UUID "e7810a71-73ae-499d-8c15-faa9aef0c3f2"
#define CHARACTERISTIC_UUID "bef8d6c9-9c21-4c9e-b632-bd58c1009f9f"

class Elm327;

class BLEHandler {
public:
    BLEHandler(Elm327* elm) : elmInstance(elm), pWriteCharacteristic(nullptr) {}

    bool connect();
    void notifyCallback(NimBLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify);
    void sendCommand(const std::string& command);

private:
    NimBLEClient* client;
    NimBLERemoteCharacteristic* pWriteCharacteristic;
    Elm327* elmInstance; // Pointer to Elm327 instance
};

class Elm327 {
public:
    Elm327() : bleHandler(this) {} 

    void initialize();
    void sendCommand(const std::string& command);
    void processResponse(const std::string& response); // expects std::string

    BLEHandler bleHandler; // Handles BLE communications

private:
    std::string latestRPM;
};

// BLEHandler methods

bool BLEHandler::connect() {
    NimBLEDevice::init("ESP32-ELM327");
    client = NimBLEDevice::createClient();

    if (!client->connect(NimBLEAddress(TARGET_DEVICE_ADDRESS, BLE_ADDR_RANDOM))) {
        Serial.println("Failed to connect to ELM327.");
        return false;
    }

    Serial.println("Connected to ELM327.");

    NimBLERemoteService* service = client->getService(SERVICE_UUID);
    if (!service) {
        Serial.println("Service not found.");
        client->disconnect();
        return false;
    }

    pWriteCharacteristic = service->getCharacteristic(CHARACTERISTIC_UUID);
    if (!pWriteCharacteristic || !pWriteCharacteristic->canWrite() || !pWriteCharacteristic->canNotify()) {
        Serial.println("Characteristic not writable or notifiable.");
        client->disconnect();
        return false;
    }

    Serial.println("Writable and notifiable characteristic found.");
    if (!pWriteCharacteristic->registerForNotify([this](NimBLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
        this->notifyCallback(pCharacteristic, pData, length, isNotify);
    })) {
        Serial.println("Failed to register for notifications.");
        client->disconnect();
        return false;
    }

    Serial.println("Registered for notifications.");
    return true;
}

void BLEHandler::notifyCallback(NimBLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify) {

    std::string response((char*)pData, length);
    
    response.erase(std::remove(response.begin(), response.end(), '\r'), response.end());
    response.erase(std::remove(response.begin(), response.end(), '>'), response.end());

    // Return if the cleaned response has no length
    if (response.empty()) {
        return;
    }

    // Debug output for the cleaned response
    Serial.print("Cleaned response (ASCII): ");
    for (char c : response) {
        if (c == '\n') {
            Serial.print("\\n");
        } else {
            Serial.printf("%c", c); // Print character if printable
        }
        Serial.printf(" (%d)", int(c)); // Print ASCII value for each byte
    }
    Serial.println();
    
    Serial.printf("Cleaned response (string): %s\n", response.c_str());

    if (elmInstance) { 
        elmInstance->processResponse(response);
    }
}

void BLEHandler::sendCommand(const std::string& command) {
    if (pWriteCharacteristic) {
        std::string cmdWithTerminator = command + "\r";
        Serial.printf("Sending command: %s\n", cmdWithTerminator.c_str());
        pWriteCharacteristic->writeValue((uint8_t*)cmdWithTerminator.c_str(), cmdWithTerminator.length(), false);
    }
}

// Elm327 methods

void Elm327::initialize() {
    const char* initCommands[] = {"ATZ", "ATI", "ATE0", "ATL0", "ATS0", "ATH0", "ATSP6"};
    for (const auto& command : initCommands) {
        sendCommand(command);
        delay(500); // Assuming delay is included or available
    }
}

void Elm327::sendCommand(const std::string& command) {
    // Delegate BLE operations to BLEHandler
    bleHandler.sendCommand(command);
}

void Elm327::processResponse(const std::string& response) {
    if (response.substr(0, 4) == "410C" && response.length() >= 8) {
        int A = std::stoi(response.substr(4, 2), nullptr, 16);
        int B = std::stoi(response.substr(6, 2), nullptr, 16);
        int rpm = ((A * 256) + B) / 4;
        latestRPM = std::to_string(rpm) + " RPM";
        Serial.printf("Engine RPM: %s\n", latestRPM.c_str());
    } else {
        Serial.printf("Unhandled response: %s\n", response.c_str());
    }
}

// Main setup and loop
Elm327 elm327;

void setup() {
    Serial.begin(115200);

    if (!elm327.bleHandler.connect()) {
        Serial.println("Failed to connect to BLE device.");
        return;
    }

    elm327.initialize();
    Serial.println("Initialization complete. Starting monitoring...");

    while (true) {
        elm327.sendCommand("010C"); // Request RPM
        delay(1000);
    }
}

void loop() {
    // Empty because all logic is in the setup for simplicity
}
