#ifndef MIDI_BRIDGE_H_
#define MIDI_BRIDGE_H_

#ifdef USE_ESP_LINK

#define ESP_LINK_PACKET_ADDRESS 0x002234

#define ESP_LINK_DEVICE_INFO_HEADER	0x10
#define ESP_LINK_WIFI_INFO_HEADER 	0x11
#define ESP_LINK_MIDI_DATA_HEADER 	0x12

#define LINK_USBD_MIDI_ID 				0x00
#define LINK_USBH_MIDI_ID 				0x01
#define LINK_BLE_MIDI_ID 				0x02
#define LINK_WIFI_RTP_MIDI_ID 		0x03
#define LINK_SERIAL0_MIDI_ID 			0x04
#define LINK_SERIAL1_MIDI_ID 			0x05
#define LINK_SERIAL2_MIDI_ID 			0x06

typedef enum
{
	LinkWaiting,	// Waiting for status line toggle from main controller
	LinkInit,		// Status line toggle received, sending initial config data
	LinkRunning,	// Link running normally
	LinkError		// Error condition has occured
} EspLinkState;

// Free RTOS tasks
void espLink_ProcessTask(void* parameter);


EspLinkState espLink_GetState();

#endif
#endif // MIDI_BRIDGE_H_