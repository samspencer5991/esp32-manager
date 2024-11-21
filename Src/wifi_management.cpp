#include <WiFiManager.h>
#include "WiFi.h"
#include "ota_updating.h"
#include "wifi_management.h"

WiFiManager wifiManager;

bool wifiConnected = false;


bool wifi_Connect(const char* hostName, const char* apName, const char* apPassword)
{
	if(hostName != NULL)
		WiFi.setHostname(hostName);
	
	if (!wifiManager.autoConnect(apName, apPassword))
	{
		Serial.println("failed to connect and hit timeout");
		wifiConnected = false;
  	}
	else
	{
		Serial.print("WiFi connected! @ ");
		Serial.println(WiFi.macAddress());
		ota_Begin();
		wifiConnected = true;
	}
	return wifiConnected;
}

void wifi_Reset()
{
	wifiManager.resetSettings();
}