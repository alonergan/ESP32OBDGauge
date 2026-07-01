#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <BLEDevice.h>
#include "config.h"

#define MAX_SCANNED_BLE_DEVICES 8
#define MAX_SAVED_BLE_DEVICES 4

struct BLEDeviceRecord {
    String name;
    String address;
    int rssi;
};

extern BLERemoteCharacteristic* pWriteChar;
extern BLERemoteCharacteristic* pNotifyChar;
extern BLEClient* pClient;
extern boolean connected;
extern String responseBuffer;

bool connectToOBD();
bool reconnectToOBD();
void disconnectOBD();
int scanForBLEDevices(BLEDeviceRecord* devices, int maxDevices, uint32_t durationSeconds = 4);
int getSavedBLEDevices(BLEDeviceRecord* devices, int maxDevices);
bool pairAndStoreBLEDevice(const BLEDeviceRecord& device);
bool removeSavedBLEDevice(int index);
String getActiveBLEAddress();

#endif
