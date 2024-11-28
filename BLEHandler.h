#ifndef BLEHANDLER_H
#define BLEHANDLER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <functional>

class BLEHandler : public BLEClientCallbacks {
private:
    BLEClient* client;
    BLERemoteCharacteristic* characteristic;
    std::function<void(const std::string&)> dataCallback;

public:
    BLEHandler();
    bool connect(const std::string& address, const std::string& serviceUUID, const std::string& characteristicUUID);
    void sendCommand(const std::string& command);
    void setDataCallback(std::function<void(const std::string&)> callback);

    // Inherited from BLEClientCallbacks
    void onConnect(BLEClient* pClient) override;
    void onDisconnect(BLEClient* pClient) override;

    // Notification callback
    void notifyCallback(
        BLERemoteCharacteristic* pCharacteristic,
        uint8_t* pData,
        size_t length,
        bool isNotify);
};

#endif // BLEHANDLER_H
