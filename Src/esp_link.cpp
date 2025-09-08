#include "esp_link.h"
#ifdef USE_ESP_LINK
#include "Arduino.h"
#include "midi_handling.h"

#define STATUS_GPIO_PIN		0
#define INFO_PACKET_SIZE	64

static const char* ESP_LINK_TAG = "ESP_LINK";

EspLinkState linkState;

void espLink_ProcessTask(void* parameter)
{
	// Set the GPIO states and perform initial link setup
	// Boot pin is used as a status toggle to preserve main chip pincount
	pinMode(STATUS_GPIO_PIN, INPUT);
	linkState = LinkInit;
	ESP_LOGD(ESP_LINK_TAG, "ESP Link initialising. Status: 'init'.");
	while(1)
	{
		switch(linkState)
		{
			case LinkInit:
				// Status toggle received
				if(digitalRead(STATUS_GPIO_PIN) == LOW)
				{
					ESP_LOGD(ESP_LINK_TAG, "Status line LOW. Sending Device Info Packet");
					// Build the device info data packet
					uint8_t infoPacket[INFO_PACKET_SIZE];
					char fwVersion[] = {FW_VERSION};
					uint8_t fwVersionSize = sizeof(fwVersion);
					infoPacket[0] = ESP_LINK_DEVICE_INFO_HEADER;
					memcpy(&infoPacket[1], fwVersion, fwVersionSize);
					midi_SendSysEx(MidiSerial1, infoPacket, fwVersionSize+1, false);
					linkState = LinkWaiting;
					ESP_LOGD(ESP_LINK_TAG, "ESP Link status: 'waiting'.");
				}
				else
				{
					vTaskDelay(50 / portTICK_PERIOD_MS);
				}
			break;

			case LinkWaiting:
				// Wait for the status pin to be set high as confirmation from the main controller
				while(digitalRead(STATUS_GPIO_PIN) == LOW)
				{
					vTaskDelay(50 / portTICK_PERIOD_MS);
				}
				ESP_LOGD(ESP_LINK_TAG, "Status line HIGH. Commencing run routine. ESP Link status: 'running'.");
				linkState = LinkRunning;

			break;

			case LinkRunning:
				vTaskDelay(5 / portTICK_PERIOD_MS);
			break;

			case LinkError:
			break;

		}
		vTaskDelay(50 / portTICK_PERIOD_MS);
	}
}

EspLinkState espLink_GetState()
{
	return linkState;
}


#endif // MIDI_BRIDGE


