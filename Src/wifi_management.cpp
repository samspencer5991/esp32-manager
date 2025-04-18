#include <WiFiManager.h>
#include "WiFi.h"
#include "ota_updating.h"
#include "wifi_management.h"
#include <WiFiClientSecure.h>
#include "ESP32Ping.h"
#include "esp32_manager.h"

WiFiManager wifiManager;

const char* WIFI_TAG = "WIFI_MANAGER";

void wifi_UpdateInfoTask();

// RTOS Tasks
void wifi_ProcessTask(void* parameter)
{
	static uint16_t wifiProcessCount = 0;
	//UBaseType_t uxHighWaterMark;
	while(1)
	{
		//uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
		//ESP_LOGI(WIFI_TAG, "WiFi Process Task High Water Mark: %d", uxHighWaterMark);
		wifiManager.process();
		ota_Process();
		vTaskDelay(10 / portTICK_PERIOD_MS);
		wifiProcessCount++;
		if(wifiProcessCount >= 250)
		{
			wifi_UpdateInfoTask();
			wifiProcessCount = 0;
		}
	}
}

void wifi_UpdateInfoTask()
{
	// Check the WiFi connection status
	wl_status_t status = WiFi.status();
	if(status != WL_CONNECTED)
	{
		esp32Info.wifiConnected = 0;
		ESP_LOGI(WIFI_TAG, "WiFi not connected, status: %d", status);
	}
	else
	{
		esp32Info.currentRssi = WiFi.RSSI();
		ESP_LOGI(WIFI_TAG, "WiFi connected, RSSI: %d", esp32Info.currentRssi);
	}
}

// General Functions
uint8_t wifi_Connect(const char* hostName, const char* apName, const char* apPassword)
{
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
			esp32Info.wifiConnected = 0;
		}
		else
		{
			ESP_LOGI("WiFi", "WiFi connected! @ ");
			Serial.println(WiFi.macAddress());
			ota_Begin();
			esp32Info.wifiConnected = 1;
		}
	}
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

