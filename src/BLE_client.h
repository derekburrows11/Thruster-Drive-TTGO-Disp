#pragma once

#include <Arduino.h>
//#include <inttypes.h>

void PrintHexData(uint8_t* pData, size_t len);

void remoteCharacteristicWrite(uint8_t* msg, size_t len);
bool remoteCharacteristicRead(std::string& value);


class BLE_client {

// was static 
    boolean connected = false;
    boolean doConnect = false;
    boolean doScan = false;
    int scanNumWithServiceUUID;


public:
    void init();
    void scan();       // Added by Derek
    void connect();    // Added by Derek
    void loop();

    void foundNewServiceUUID() { scanNumWithServiceUUID++; }
};


