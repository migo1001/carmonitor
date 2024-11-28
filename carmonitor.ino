#include <Arduino.h>
#include "BLEHandler.h"
#include "ELM327Handler.h"

// Global instances
BLEHandler bleHandler;
ELM327Handler elm327(bleHandler);

void setup() {
    Serial.begin(115200);

    if (!elm327.connect("D2:E0:2F:8D:65:A3")) {
        Serial.println("Failed to connect to ELM327.");
        return;
    }

    elm327.initialize();
}

void loop() {
    elm327.queryRPM();
    delay(500);  // Adjust as per response time

    elm327.querySpeed();
    delay(500);  // Adjust as per response time

    Serial.printf("Current RPM: %.2f | Current Speed: %d km/h\n",
                  elm327.getRPM(), elm327.getSpeed());
    delay(1000);
}
