#include <wifiManager.h>
#include "WiFi.h"
#include "ota_updating.h"
#include "wifi_management.h"
#include <WiFiClientSecure.h>
#include "ESP32Ping.h"

WiFiManager wifiManager;

uint8_t wifiConnected = 0;
uint8_t wifiEnabled = 0;

uint8_t wifi_Connect(const char* hostName, const char* apName, const char* apPassword)
{
	if(wifiEnabled)
	{
		if(hostName != NULL)
			WiFi.setHostname(hostName);

		WiFi.mode(WIFI_STA);	
		wifiManager.setConfigPortalTimeout(60);
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
	}
	return wifiConnected;
}

void wifi_TurnOff()
{
	WiFi.mode(WIFI_OFF);
	wifiConnected = 0;
}

void wifi_ResetSettings()
{
	wifiManager.resetSettings();
}

uint8_t wifi_ConnectionStatus()
{
	return wifiConnected;
}

uint8_t wifi_CheckConnectionPing()
{
	uint8_t success = Ping.ping("www.google.com", 3);
	if(!success)
	{
		Serial.println("Ping failed");
		return 0;
	}

	Serial.println("Ping succesful.");
	return 1;
}

void wifi_Loop()
{
	wifiManager.process();
}