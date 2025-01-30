#pragma once

#ifndef ESP32_HANDLER_H_
#define ESP32_HANDLER_H_

//#include "midi_handling.h"
//#include "ota_updating.h"
//#include "usb_host.h"
//#include "wifi_management.h"

typedef enum
{
	Esp32WiFi,
	Esp32BLE,
	Esp32None
} Esp32WirelessType;

typedef enum
{
	Esp32WiFiAP,
	Esp32WiFiDevice
} Esp32WiFiMode;

typedef enum
{
	Esp32BLEServer,
	Esp32BLEClient
} Esp32BLEMode;

typedef enum
{
	Esp32BLEClientAll,
	Esp32BLEClientInclusion,
	Esp32BLEClientExclusion
} ESP32BLEClientFilter;

typedef struct
{
	Esp32WirelessType wirelessType;		// What type of wireless connection should be used
	Esp32WiFiMode wifiMode;					// Access point or connected device behaviour by default
	Esp32BLEMode bleMode;					// Client or server behaviour for BLE
	ESP32BLEClientFilter bleFilterMode;	// Filter for BLE client mode
} Esp32ManagerConfig;

extern Esp32ManagerConfig* esp32ConfigPtr;	// Pointer to the config structure of the application

void esp32Manager_Init();
void esp32Manager_Process();

#endif // ESP32_HANDLER_H_