#include "esp_link.h"
#ifdef USE_ESP_LINK
#include "Arduino.h"
#include "midi_handling.h"

#define STATUS_GPIO_PIN		0
#define INFO_PACKET_SIZE	64

typedef enum
{
	LinkWaiting,	// Waiting for status line toggle from main controller
	LinkInit,		// Status line toggle received, sending initial config data
	LinkRunning,	// Link running normally
	LinkError		// Error condition has occured
} EspLinkState;

EspLinkState linkState;

void espLink_ProcessTask(void* parameter)
{
	// Set the GPIO states and perform initial link setup
	// Boot pin is used as a status toggle to preserve main chip pincount
	pinMode(STATUS_GPIO_PIN, INPUT);
	linkState = LinkWaiting;
	while(1)
	{
		switch(linkState)
		{
			case LinkWaiting:
				// Status toggle received
				//if(digitalRead(STATUS_GPIO_PIN) == LOW)
				if(1)
				{
					// Build the device info data packet
					uint8_t infoPacket[INFO_PACKET_SIZE];
					char fwVersion[] = {FW_VERSION};
					uint8_t fwVersionSize = sizeof(fwVersion);
					infoPacket[0] = ESP_LINK_DEVICE_INFO_HEADER;
					memcpy(fwVersion, &infoPacket[1], fwVersionSize);
					midi_SendSysEx(MidiSerial1, infoPacket, fwVersionSize+1, false);
				}
				else
				{
					vTaskDelay(10 / portTICK_PERIOD_MS);
				}
			break;

			case LinkInit:

			break;

			case LinkRunning:
			break;

			case LinkError:
			break;

		}
	}
}




#endif // MIDI_BRIDGE


