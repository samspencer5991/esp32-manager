#ifndef MIDI_BRIDGE_H_
#define MIDI_BRIDGE_H_

#ifdef USE_ESP_LINK

#define ESP_LINK_PACKET_ADDRESS 0x002234

#define ESP_LINK_DEVICE_INFO_HEADER	0x10
#define ESP_LINK_WIFI_INFO_HEADER 	0x11
#define ESP_LINK_MIDI_DATA_HEADER 	0x12

#define USBD_MIDI_ID 		0x01
#define USBH_MIDI_ID 		0x02
#define BLE_MIDI_ID 			0x03
#define WIFI_RTP_MIDI_ID 	0x04
#define SERIAL0_MIDI_ID 	0x05
#define SERIAL1_MIDI_ID 	0x06
#define SERIAL2_MIDI_ID 	0x07


// Free RTOS tasks
void espLink_ProcessTask(void* parameter);

#endif
#endif // MIDI_BRIDGE_H_