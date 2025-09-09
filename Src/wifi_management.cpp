#include <WiFiManager.h>
#include "WiFi.h"
#include "ota_updating.h"
#include "wifi_management.h"
#include <WiFiClientSecure.h>
#include "ESP32Ping.h"
#include "esp32_manager.h"
#include "esp_link.h"
#include "midi_handling.h"

WiFiManager wifiManager;

const char* WIFI_TAG = "WIFI_MANAGER";

const char* wifiHostName = NULL;
const char* wifiApName = NULL;
const char* wifiApPassword = NULL;

uint8_t newWifiEvent = 0;

void wifi_UpdateInfoTask();

// RTOS Tasks
void wifi_ProcessTask(void* parameter)
{
	static uint16_t wifiProcessCount = 0;
	//UBaseType_t uxHighWaterMark;
	while(1)
	{
		if(esp32ConfigPtr->wirelessType != Esp32WiFi)
		{
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}
		else
		{
			//uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
			//ESP_LOGI(WIFI_TAG, "WiFi Process Task High Water Mark: %d", uxHighWaterMark);
			wifiManager.process();
			ota_Process();
			vTaskDelay(2 / portTICK_PERIOD_MS);
			wifiProcessCount++;
			if(wifiProcessCount >= 1000)
			{
				wifi_UpdateInfoTask();
				wifiProcessCount = 0;
			}
		}
	}
}

void wifi_UpdateInfoTask()
{
	// Check the WiFi connection status
	wl_status_t status = WiFi.status();
	static wl_status_t lastStatus = WL_DISCONNECTED;
	if(status != lastStatus)
	{
		if(status != WL_CONNECTED)
		{
			// Check if the device is in AP mode (config portal) or STA mode (normal connection)
			wifi_mode_t wifiMode = WiFi.getMode();
			if(wifiMode == WIFI_MODE_AP)
			{
				// If in AP mode, set the connection status to 3 (config portal)
				esp32Info.wifiConnected = 3;
				ESP_LOGI(WIFI_TAG, "WiFi in Config Portal (AP mode), status: %d", status);
			}
			else if(wifiMode == WIFI_MODE_STA)
			{
				// If in STA mode, set the connection status to 0 (not connected)
				esp32Info.wifiConnected = 0;

				ESP_LOGI(WIFI_TAG, "WiFi not connected, status: %d", status);
			}
		}
		lastStatus = status;
		newWifiEvent = 1;
	}
	if(status == WL_CONNECTED)
	{
		// Set the connection state, assume internet is available as ping is checked on device boot
		esp32Info.wifiConnected = 2;
		esp32Info.currentRssi = WiFi.RSSI();
		ESP_LOGI(WIFI_TAG, "WiFi connected, RSSI: %d", esp32Info.currentRssi);
#ifdef USE_ESP_LINK
		uint8_t txString[64];
		txString[0] = (ESP_LINK_PACKET_ADDRESS >> 16) & 0xFF;
		txString[1] = (ESP_LINK_PACKET_ADDRESS >> 8) & 0xFF;
		txString[2] = ESP_LINK_PACKET_ADDRESS & 0xFF;
		txString[3] = ESP_LINK_WIFI_INFO_HEADER;
		txString[4] = esp32Info.currentRssi;
		midi_SendSysEx(MidiSerial1, (uint8_t*)txString, 5, 0);
#endif
	}
}

// General Functions
uint8_t wifi_Connect(const char* hostName, const char* apName, const char* apPassword)
{
	// Store the WiFi credentials in the global variables
	wifiHostName = hostName;
	wifiApName = apName;
	wifiApPassword = apPassword;

	// Ensure the ESP32 manager is in WiFi mode
	if(esp32ConfigPtr->wirelessType == Esp32WiFi)
	{
		if(hostName != NULL)
			WiFi.setHostname(hostName);

		WiFi.mode(WIFI_STA);	
		wifiManager.setConfigPortalBlocking(false);
		wifiManager.setConfigPortalTimeout(300);
		if (!wifiManager.autoConnect(apName, apPassword))
		{
			ESP_LOGI("WiFi", "Configuration portal beginning");
			esp32Info.wifiConnected = 3;
		}
		else
		{
			ESP_LOGI("WiFi", "WiFi connected! @ ");
			Serial.println(WiFi.macAddress());
			if(esp32ConfigPtr->useStaticIp)
			{
				IPAddress localIP(esp32ConfigPtr->staticIp[0], esp32ConfigPtr->staticIp[1], esp32ConfigPtr->staticIp[2], esp32ConfigPtr->staticIp[3]);
				IPAddress gateway(esp32ConfigPtr->staticGatewayIp[0], esp32ConfigPtr->staticGatewayIp[1], esp32ConfigPtr->staticGatewayIp[2], esp32ConfigPtr->staticGatewayIp[3]);
				IPAddress subnet(255, 255, 255, 0);
				if(!WiFi.config(localIP, gateway, subnet))
				{
					ESP_LOGI("WiFi", "Static IP configuration failed.");
				}
				else
				{
					ESP_LOGI("WiFi", "Static IP configured to: %s", localIP.toString().c_str());
				}
			}
			ota_Begin();
			esp32Info.wifiConnected = 1;
		}
	}
	return esp32Info.wifiConnected;
}

uint8_t wifi_Disconnect()
{
	if(esp32Info.wifiConnected == 3)
	{
		wifiManager.stopConfigPortal();
	}
	WiFi.disconnect(true);
	WiFi.mode(WIFI_OFF);
	esp32Info.wifiConnected = 0;
	return esp32Info.wifiConnected;
}

void wifi_TurnOff()
{
	WiFi.mode(WIFI_OFF);
	esp32Info.wifiConnected = 0;
}

void wifi_ResetSettings()
{
	wifiManager.resetSettings();
}

uint8_t wifi_ConnectionStatus()
{
	return 1;
}

uint8_t wifi_CheckConnectionPing()
{
	uint8_t success = Ping.ping("www.google.com", 3);
	if(!success)
	{
		ESP_LOGI("WiFi", "Ping failed");
		esp32Info.wifiConnected = 1;
		return 0;
	}

	ESP_LOGI("WiFi", "Ping succesful.");
	esp32Info.wifiConnected = 2;
	return 1;
}

