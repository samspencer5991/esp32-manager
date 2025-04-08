#include "esp32_manager.h"

#include "midi_handling.h"
#include "wifi_management.h"
#include "WiFi.h"

#ifdef USE_EXTERNAL_USB_HOST
#include "usb_host.h"
#include "usbh_cdc_handling.h"
#endif

#include "esp32_manager_task_priorities.h"

Esp32ManagerConfig* esp32ConfigPtr;
Esp32ManagerInfo esp32Info;

const char* ESP32_TAG = "ESP32_MANAGER";

// Initialise all included components
void esp32Manager_Init()
{
	if(esp32ConfigPtr->wirelessType == Esp32WiFi)
	{
		wifi_Connect(WIFI_HOSTNAME, WIFI_AP_SSID, NULL);
		strcpy(esp32Info.macAddress, WiFi.macAddress().c_str());
		strcpy(esp32Info.currentSsid, WiFi.SSID().c_str());
		esp32Info.currentRssi = WiFi.RSSI();
		strcpy(esp32Info.currentIP, WiFi.localIP().toString().c_str());
		// Check an internet ping
		wifi_CheckConnectionPing();
		midi_InitWiFiRTP();
	}
		

	// Initialise USB host components
#ifdef USE_EXTERNAL_USB_HOST
	usbh_Init();
	cdc_Init();
#endif

	midi_Init();
}

void esp32Manager_CreateTasks()
{
	BaseType_t taskResult;
	// WiFi processing
	taskResult = xTaskCreatePinnedToCore(
		wifi_ProcessTask, 	// Task function. 
		"WiFiProcess", 		// name of task. 
		5000, 					// Stack size of task 
		NULL, 					// parameter of the task 
		WIFI_TASK_PRIORITY, 						// priority of the task 
		NULL, 					// Task handle to keep track of created task 
		0); 						// pin task to core 1 
	if(taskResult != pdPASS)
		ESP_LOGE(ESP32_TAG, "Failed to create WiFi task: %d", taskResult);
	else
		ESP_LOGI(ESP32_TAG, "WiFi task created: %d", taskResult);

	
		taskResult = xTaskCreatePinnedToCore(
		usbh_ProcessTask,
		"USB Host Process",
		5000,
		NULL,
		USBH_TASK_PRIORITY,
		NULL,
		1);
	if(taskResult != pdPASS)
		ESP_LOGE(ESP32_TAG, "Failed to create USB Host task: %d", taskResult);
	else
		ESP_LOGI(ESP32_TAG, "USB Host task created: %d", taskResult);

	taskResult = xTaskCreatePinnedToCore(
		cdch_ProcessTask,
		"CDC Host Process",
		50000,
		NULL, 
		CDC_TASK_PRIORITY, 
		NULL,
		1);
if(taskResult != pdPASS)
		ESP_LOGE(ESP32_TAG, "Failed to create CDC task: %d", taskResult);
	else
		ESP_LOGI(ESP32_TAG, "CDC task created: %d", taskResult);

	taskResult = xTaskCreatePinnedToCore(
		midi_ProcessTask,
		"MIDIProcess",
		5000,
		NULL, // parameter of the task 
		MIDI_TASK_PRIORITY, // priority of the task 
		NULL, // Task handle to keep track of created task 
		1); // pin task to core 1 
	if(taskResult != pdPASS)
		ESP_LOGE(ESP32_TAG, "Failed to create MIDI task: %d", taskResult);
	else
		ESP_LOGI(ESP32_TAG, "MIDI task created: %d", taskResult);
}

void esp32Manager_Process()
{
	//midi_ProcessTask(NULL);
	//cdch_ProcessTask(NULL);
	//usbh_ProcessTask(NULL);
	//wifi_ProcessTask(NULL);
}