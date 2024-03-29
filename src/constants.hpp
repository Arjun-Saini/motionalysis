#pragma once

#define kBLEConnectedLED D7
#define kLIS3DHInterruptPin D2
#define kHTTPRequestPort 80
#define kHTTPHostname "digiglue.io"
// #define kHTTPHostname "192.168.0.104" // debug at lab IP
#define kLis3dhAddress 0x18
#define kRecordingIntervalEEPROMAddress 100
#define kReportingIntervalEEPROMAddress 200
#define kReportingModeEEPROMAddress 364
#define kSleepPauseDurationEEPROMAddress 500
#define kDefaultReportingMode 0
#define kDsidEEPROMAddress 0
#define kEEPROMEmptyValue -1
#define kDefaultRecordingInterval 500 // in milliseconds
#define kDefaultReportingInterval 15 // in seconds
#define kDefaultSleepPauseDuration 5 // in seconds
#define kWiFiConnectionTimeout 20000 // in milliseconds
#define kWiFiCheckInterval 100 // in milliseconds
#define kDeltaAccelThreshold 0.05 // in g
#define kSecondsToMilliseconds 1000
#define kWatchDogTimeout 3600000 // 1 hr in milliseconds

LEDStatus bleListening(RGB_COLOR_BLUE, LED_PATTERN_FADE, LED_SPEED_FAST, LED_PRIORITY_IMPORTANT);
LEDStatus wifiSyncing(RGB_COLOR_RED, LED_PATTERN_FADE, LED_SPEED_SLOW, LED_PRIORITY_IMPORTANT);