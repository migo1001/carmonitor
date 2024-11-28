#include "BLEHandler.h"
#include <Arduino.h>

BLEHandler::BLEHandler() : client(nullptr), characteristic(nullptr), dataCallback(nullptr) {}

void BLEHandler::setDataCallback(std::function<void(const std::string&)> callback) {
    dataCallback = callback;
}

bool BLEHandler::connect(const std::string& address, const std::string& serviceUUID, const std::string& characteristicUUID) {
    BLEDevice::init("ESP32_Client");
    client = BLEDevice::createClient();
    client->setClientCallbacks(this);

    Serial.printf("Attempting to connect to BLE device at %s...\n", address.c_str());
    if (!client->connect(BLEAddress(address), BLE_ADDR_TYPE_RANDOM)) {
        Serial.println("Failed to connect to BLE device.");
        return false;
    }

    // Set MTU after connection
    client->setMTU(500);

    Serial.println("Connected to BLE device!");

    BLERemoteService* service = client->getService(BLEUUID(serviceUUID));
    if (!service) {
        Serial.println("Failed to find service.");
        return false;
    }

    Serial.println("Service found!");

    characteristic = service->getCharacteristic(BLEUUID(characteristicUUID));
    if (!characteristic) {
        Serial.println("Failed to find characteristic.");
        return false;
    }

    Serial.println("Characteristic found!");

    // Register for notifications
    if (characteristic->canNotify()) {
        characteristic->registerForNotify(
            std::bind(&BLEHandler::notifyCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    return true;
}

void BLEHandler::sendCommand(const std::string& command) {
    if (characteristic && characteristic->canWrite()) {
        characteristic->writeValue(command + "\r");
    }
}

void BLEHandler::onConnect(BLEClient* pClient) {
    Serial.println("BLE Client Connected.");
}

void BLEHandler::onDisconnect(BLEClient* pClient) {
    Serial.println("BLE Client Disconnected.");
}

// Notification callback
void BLEHandler::notifyCallback(
    BLERemoteCharacteristic* pCharacteristic,
    uint8_t* pData,
    size_t length,
    bool isNotify) {
    std::string response(reinterpret_cast<char*>(pData), length);
    Serial.printf("Notification received: %s\n", response.c_str());

    // Pass the response to the registered callback
    if (dataCallback) {
        dataCallback(response);
    }
}
