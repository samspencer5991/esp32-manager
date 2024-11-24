#include <WiFiManager.h>
#include "WiFi.h"
#include "ota_updating.h"
#include "wifi_management.h"

WiFiManager wifiManager;

uint8_t wifiConnected = 0;


uint8_t wifi_Connect(const char* hostName, const char* apName, const char* apPassword)
{
	if(hostName != NULL)
		WiFi.setHostname(hostName);
	
	if (!wifiManager.autoConnect(apName, apPassword))
	{
		Serial.println("failed to connect and hit timeout");
		wifiConnected = 0;
  	}
	else
	{
		Serial.print("WiFi connected! @ ");
		Serial.println(WiFi.macAddress());
		ota_Begin();
		wifiConnected = 1;
	}
	return wifiConnected;
}

void wifi_Reset()
{
	wifiManager.resetSettings();
}

uint8_t wifi_ConnectionStatus()
{
	return wifiConnected;
}