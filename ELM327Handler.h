#ifndef ELM327HANDLER_H
#define ELM327HANDLER_H

#include <string>
#include "BLEHandler.h"

class ELM327Handler {
private:
    BLEHandler& bleHandler;
    float ecu_rpm;
    int ecu_speed;

    void handleResponse(const std::string& response);

public:
    ELM327Handler(BLEHandler& handler);
    bool connect(const std::string& address);
    void initialize();
    void queryRPM();
    void querySpeed();
    float getRPM() const;
    int getSpeed() const;
};

#endif // ELM327HANDLER_H
