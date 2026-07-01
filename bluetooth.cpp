#include "bluetooth.h"
#include "commands.h"
#include <Preferences.h>

BLERemoteCharacteristic* pWriteChar = nullptr;
BLERemoteCharacteristic* pNotifyChar = nullptr;
BLEClient* pClient = nullptr;
boolean connected = false;
String responseBuffer = "";
static bool bleInitialized = false;

static void ensureBLEInitialized() {
    if (bleInitialized) return;
    BLEDevice::init("ESP32_OBD");
    BLEDevice::setMTU(517);
    BLEDevice::setPower(ESP_PWR_LVL_P9);
    bleInitialized = true;
}

String getActiveBLEAddress() {
    Preferences prefs;
    prefs.begin("OBDGAUGE", true);
    bool removed = prefs.getBool("bleRemoved", false);
    String address = prefs.getString("bleActive", removed ? "" : BLUETOOTH_DEVICE_ADDRESS);
    prefs.end();
    return address;
}

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
    ensureBLEInitialized();
    String activeAddress = getActiveBLEAddress();
    if (activeAddress.length() == 0) return false;

    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());

    Serial.println("Attempting device connection...");
    BLEAddress obdAddress(activeAddress.c_str());
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

void disconnectOBD() {
    if (pClient != nullptr && pClient->isConnected()) pClient->disconnect();
    connected = false;
    pWriteChar = nullptr;
    pNotifyChar = nullptr;
}

int scanForBLEDevices(BLEDeviceRecord* devices, int maxDevices, uint32_t durationSeconds) {
    if (devices == nullptr || maxDevices <= 0) return 0;
    ensureBLEInitialized();
    BLEScan* scan = BLEDevice::getScan();
    scan->setActiveScan(true);
    BLEScanResults* results = scan->start(durationSeconds, false);
    if (results == nullptr) return 0;
    int count = min(results->getCount(), maxDevices);
    for (int i = 0; i < count; i++) {
        BLEAdvertisedDevice device = results->getDevice(i);
        devices[i].name = device.haveName() ? device.getName() : "Unnamed BLE device";
        devices[i].address = device.getAddress().toString();
        devices[i].rssi = device.getRSSI();
    }
    scan->clearResults();
    return count;
}

int getSavedBLEDevices(BLEDeviceRecord* devices, int maxDevices) {
    if (devices == nullptr || maxDevices <= 0) return 0;
    Preferences prefs;
    prefs.begin("OBDGAUGE", true);
    int storedCount = static_cast<int>(prefs.getInt("bleCount", 0));
    int count = min(storedCount, min(maxDevices, MAX_SAVED_BLE_DEVICES));
    for (int i = 0; i < count; i++) {
        String addressKey = "bleAddr" + String(i);
        String nameKey = "bleName" + String(i);
        devices[i].address = prefs.getString(addressKey.c_str(), "");
        devices[i].name = prefs.getString(nameKey.c_str(), "Saved BLE device");
        devices[i].rssi = 0;
    }
    prefs.end();
    return count;
}

bool pairAndStoreBLEDevice(const BLEDeviceRecord& device) {
    BLEDeviceRecord saved[MAX_SAVED_BLE_DEVICES];
    int count = getSavedBLEDevices(saved, MAX_SAVED_BLE_DEVICES);
    int slot = -1;
    for (int i = 0; i < count; i++) if (saved[i].address == device.address) slot = i;
    if (slot < 0) {
        if (count >= MAX_SAVED_BLE_DEVICES) return false;
        slot = count++;
    }
    saved[slot] = device;

    Preferences prefs;
    prefs.begin("OBDGAUGE", false);
    prefs.putInt("bleCount", count);
    for (int i = 0; i < count; i++) {
        String addressKey = "bleAddr" + String(i);
        String nameKey = "bleName" + String(i);
        prefs.putString(addressKey.c_str(), saved[i].address);
        prefs.putString(nameKey.c_str(), saved[i].name);
    }
    prefs.putString("bleActive", device.address);
    prefs.putBool("bleRemoved", false);
    prefs.end();
    disconnectOBD();
    return reconnectToOBD();
}

bool removeSavedBLEDevice(int index) {
    BLEDeviceRecord saved[MAX_SAVED_BLE_DEVICES];
    int count = getSavedBLEDevices(saved, MAX_SAVED_BLE_DEVICES);
    if (index < 0 || index >= count) return false;
    String removedAddress = saved[index].address;
    for (int i = index; i < count - 1; i++) saved[i] = saved[i + 1];
    count--;

    Preferences prefs;
    prefs.begin("OBDGAUGE", false);
    prefs.putInt("bleCount", count);
    for (int i = 0; i < count; i++) {
        String addressKey = "bleAddr" + String(i);
        String nameKey = "bleName" + String(i);
        prefs.putString(addressKey.c_str(), saved[i].address);
        prefs.putString(nameKey.c_str(), saved[i].name);
    }
    String active = prefs.getString("bleActive", "");
    if (active == removedAddress) {
        prefs.remove("bleActive");
        prefs.putBool("bleRemoved", true);
        disconnectOBD();
    }
    prefs.end();
    return true;
}
