#pragma once

#ifndef MIDI_HANDLING_H_
#define MIDI_HANDLING_H_

#include "stdint.h"
#include "MIDI.h"

#define SYSEX_ADDRESS_BYTE1 			0x00
#define SYSEX_ADDRESS_BYTE2			0x22
#define SYSEX_ADDRESS_BYTE3 			0x33

#define SYSEX_DEVICE_API_COMMAND 	0x01
#define SYSEX_PETAL_COMMAND 			0x02

// This order is used to reference MIDI handles
typedef enum
{
#ifdef USE_USBD_MIDI
	MidiUSBD,
#endif
#ifdef USE_USBH_MIDI
	MidiUSBH,
#endif
#ifdef USE_BLE_MIDI
	MidiBLE,
#endif
#ifdef USE_WIFI_RTP_MIDI
	MidiWiFiRTP,
#endif
#ifdef USE_SERIAL0_MIDI
	MidiSerial0,
#endif
#ifdef USE_SERIAL1_MIDI
	MidiSerial1,
#endif
#ifdef USE_SERIAL2_MIDI
	MidiSerial2,
#endif
	MidiNone
} MidiInterfaceType;

typedef enum
{
	SysExGeneral,
	SysExDeviceApi,
	SysExPetal
} SysExCommandType;

//-------------- FreeRTOS Tasks --------------//
void midi_ProcessTask(void* parameter);

void midi_Init();
void midi_ApplyThruSettings();
void midi_InitWiFiRTP();
void midi_ReadAll();
//uint8_t midi_BleConnected();


#ifdef USE_BLE_MIDI
extern uint8_t bleEnabled;
void turnOffBLE();
void turnOnBLE();
#endif

extern uint8_t bleEnabled;

extern uint8_t numMidiHandles;
extern uint8_t* usbdMidiThruHandlesPtr;
extern uint8_t* usbhMidiThruHandlesPtr;
extern uint8_t* bleMidiThruHandlesPtr;
extern uint8_t* wifiMidiThruHandlesPtr;
extern uint8_t* serial0MidiThruHandlesPtr;
extern uint8_t* serial1MidiThruHandlesPtr;
extern uint8_t* serial2MidiThruHandlesPtr;

void midi_AssignControlChangeCallback(void (*callback)(MidiInterfaceType interface, uint8_t channel, uint8_t number, uint8_t value));
void midi_AssignProgramChangeCallback(void (*callback)(MidiInterfaceType interface, uint8_t channel, uint8_t number));
void midi_AssignSysemExclusiveCallback(void (*callback)(MidiInterfaceType interface, uint8_t* array, unsigned size));

void midi_AssignPetalControlChangeCallback(void (*callback)(MidiInterfaceType interface, uint8_t channel, uint8_t number, uint8_t value));
void midi_AssignPetalProgramChangeCallback(void (*callback)(MidiInterfaceType interface, uint8_t channel, uint8_t number));
void midi_AssignPetalSysemExclusiveCallback(void (*callback)(MidiInterfaceType interface, uint8_t* array, unsigned size));

void midi_SendDeviceApiSysExString(const char* array, unsigned size, uint8_t containsFraming);
void midi_SendPetalSysex(const uint8_t* data, unsigned size);

void midih_Loop();
void midih_Setup();
void testRtpSendNote();
#endif /* MIDI_HANDLING_H_ */