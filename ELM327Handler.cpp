#include "ELM327Handler.h"
#include <Arduino.h>

ELM327Handler::ELM327Handler(BLEHandler& handler) : bleHandler(handler), ecu_rpm(0.0f), ecu_speed(0) {
    // Register the data callback
    bleHandler.setDataCallback(
        std::bind(&ELM327Handler::handleResponse, this, std::placeholders::_1));
}

bool ELM327Handler::connect(const std::string& address) {
    return bleHandler.connect(address, "e7810a71-73ae-499d-8c15-faa9aef0c3f2", "bef8d6c9-9c21-4c9e-b632-bd58c1009f9f");
}

void ELM327Handler::initialize() {
    // Send initialization commands and wait for acknowledgments
    const char* initCommands[] = {"ATZ", "ATE0", "ATL0", "ATS0", "ATH1", "ATAL", "ATSP0"};
    for (const char* cmd : initCommands) {
        bleHandler.sendCommand(cmd);
        // Implement a mechanism to wait for and validate the response
        // For now, we use a delay (but a response-based mechanism is better)
        delay(500);
    }

    Serial.println("ELM327 initialized.");
}

void ELM327Handler::queryRPM() {
    bleHandler.sendCommand("010C");
}

void ELM327Handler::querySpeed() {
    bleHandler.sendCommand("010D");
}

float ELM327Handler::getRPM() const {
    return ecu_rpm;
}

int ELM327Handler::getSpeed() const {
    return ecu_speed;
}

void ELM327Handler::handleResponse(const std::string& response) {
    Serial.printf("Received Response: %s\n", response.c_str());

    // Parse the response
    size_t idx = response.find("41");
    if (idx == std::string::npos) {
        Serial.println("Invalid response format.");
        return;
    }

    std::string pid = response.substr(idx + 2, 2);
    std::string data = response.substr(idx + 4);

    if (pid == "0C") {
        // RPM PID: 0C
        if (data.length() >= 4) {
            int a = strtol(data.substr(0, 2).c_str(), nullptr, 16);
            int b = strtol(data.substr(2, 2).c_str(), nullptr, 16);
            ecu_rpm = ((a * 256) + b) / 4.0f;  // RPM calculation
            Serial.printf("Parsed RPM: %.2f\n", ecu_rpm);
        } else {
            Serial.println("Invalid data length for RPM.");
        }
    } else if (pid == "0D") {
        // Speed PID: 0D
        if (data.length() >= 2) {
            ecu_speed = strtol(data.substr(0, 2).c_str(), nullptr, 16);  // Speed is a single byte
            Serial.printf("Parsed Speed: %d km/h\n", ecu_speed);
        } else {
            Serial.println("Invalid data length for speed.");
        }
    } else {
        Serial.printf("Unhandled PID: %s\n", pid.c_str());
    }
}
