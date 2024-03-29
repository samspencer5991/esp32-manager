#ifndef MIDI_HANDLING_H_
#define MIDI_HANDLING_H_

#include "stdint.h"

#define SYSEX_ADDRESS_BYTE1 			0x00
#define SYSEX_ADDRESS_BYTE2			0x22
#define SYSEX_ADDRESS_BYTE3 			0x33

#define SYSEX_DEVICE_API_COMMAND 	0x01
#define SYSEX_PETAL_COMMAND 			0x02

typedef enum
{
	MidiUSBD,
	MidiUSBH,
	MidiBLE,
	MidiWiFi,
	MidiSerial0,
	MidiSerial1,
	MidiSerial2,
	MidiNone
} MidiInterfaceType;

typedef enum
{
	SysExGeneral,
	SysExDeviceApi,
	SysExPetal
} SysExCommandType;

void midi_Init();
void midi_ReadAll();
uint8_t midi_BleConnected();

void midi_AssignControlChangeCallback(void (*callback)(MidiInterfaceType interface, uint8_t channel, uint8_t number, uint8_t value));
void midi_AssignProgramChangeCallback(void (*callback)(MidiInterfaceType interface, uint8_t channel, uint8_t number));
void midi_AssignSysemExclusiveCallback(void (*callback)(MidiInterfaceType interface, uint8_t* array, unsigned size));


void midi_sendDeviceApiSysexString(const char* array, unsigned size);

void midih_loop();
void midih_setup();

#endif /* MIDI_HANDLING_H_ */