#pragma once

#ifndef ESP32_DEF_H_
#define ESP32_DEF_H_

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

typedef struct
{
	Esp32WirelessType WirelessType;		// What type of wireless connection should be used
	Esp32WiFiMode WiFiMode;					// Access point or connected device behaviour by default
} Esp32Config;

Esp32Config* esp32Config;	// Pointer to the config structure of the application

#endif // ESP32_DEF_H_