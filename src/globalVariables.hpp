#pragma once

int recordingInterval; // interval between lis3dh reads
int reportingInterval; // interval between reporting data to server in seconds
String payload = "";
bool valuesChanged = false;
String unixTime;
String ssid, password = "";
float x, y, z;
uint8_t storedValues [256];
long storedTimes [256];
float prevX, prevY, prevZ;
int storedValuesIndex = 0;
String rolloverPayload = ""; 
enum firmwareStateEnum {
  BLEWAIT,
  RECORDING,
  SENDING
};
uint8_t firmwareState = BLEWAIT;
bool bleWaitForConfig = false; //when true, firmware is waiting for user input over BLE b/c BLE was connected
String bleInputBuffer; // buffer for reading from BLE and writing to EEPROM
int bleQuestionCount, dsid, size;
bool waitingForOTA = false;
os_thread_t reportingThreadHandle;
os_mutex_t payloadAccessLock;


const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid rxUuid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
