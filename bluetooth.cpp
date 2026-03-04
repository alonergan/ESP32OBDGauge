#include "bluetooth.h"
#include "commands.h"

BLEAddress obdAddress(BLUETOOTH_DEVICE_ADDRESS);
BLERemoteCharacteristic* pWriteChar = nullptr;
BLERemoteCharacteristic* pNotifyChar = nullptr;
BLEClient* pClient = nullptr;
boolean connected = false;
String responseBuffer = "";

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) { connected = true; }
  void onDisconnect(BLEClient* pclient) { 
      connected = false;
      pWriteChar = nullptr;
      pNotifyChar = nullptr;
  }
};

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  String chunk = "";
  for (int i = 0; i < length; i++) {
    chunk += (char)pData[i];
  }
  responseBuffer += chunk;
}

void intializeELM327() {
    Commands initCommands;
    // Send initialization commands to ELM327
    initCommands.sendCommand("ATZ");     // Reset adapter
    initCommands.sendCommand("ATE0");    // Echo off
    initCommands.sendCommand("ATL0");    // Linefeeds off
    initCommands.sendCommand("ATS0");    // Spaces off
    initCommands.sendCommand("ATH0");    // Headers off
    initCommands.sendCommand("ATSP 6");  // Set protocol
    initCommands.sendCommand("ATSH 7DF"); // Set ECU address
    initCommands.sendCommand("AT ST 10"); // Set optimal timeout to 160ms
    initCommands.sendCommand("AT AT 2");  // Set aggressive adaptive timing
    String pidSupport = initCommands.sendCommand("0100"); // Check supported PIDs
    Serial.println("PID 0100 response: " + pidSupport);
}

bool connectToOBD() {
    Serial.println("Initializing BLEDevice");
    BLEDevice::init("ESP32_OBD");
    BLEDevice::setMTU(517);
    BLEDevice::setPower(ESP_PWR_LVL_P9);
    Serial.println("Done initializing BLEDevice");

    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());

    Serial.println("Attempting device connection...");
    if (pClient->connect(obdAddress)) {
        Serial.println("Device connected");
        pClient->setMTU(517);
        BLERemoteService* pRemoteService = pClient->getService(BLUETOOTH_SERVICE_UUID);
        if (pRemoteService != nullptr) {
            pWriteChar = pRemoteService->getCharacteristic(BLUETOOTH_WRITE_CHAR_UUID);
            pNotifyChar = pRemoteService->getCharacteristic(BLUETOOTH_NOTIFY_CHAR_UUID);
            if (pWriteChar != nullptr && pNotifyChar != nullptr && pNotifyChar->canNotify()) {
                pNotifyChar->registerForNotify(notifyCallback);
                pNotifyChar->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)"\x01\x00", 2);
                Serial.println("Initializing ELM327");
                intializeELM327();
                Serial.println("Done initializing ELM327");
                return true;
            }
        }
    }
    connected = false;
    pWriteChar = nullptr;
    pNotifyChar = nullptr;
    Serial.println("Failed to connect to OBD");
    return false;
}

bool reconnectToOBD() {
    if (connected) {
        return true;
    }
    Serial.println("Attempting to reconnect to OBD...");
    if (pClient != nullptr) {
        pClient->disconnect();
        delete pClient;
        pClient = nullptr;
    }
    return connectToOBD();
}