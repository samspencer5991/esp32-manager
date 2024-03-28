#ifndef MIDI_HANDLING_H_
#define MIDI_HANDLING_H_

#include "stdint.h"

typedef enum
{
	MidiUSBD,
	MidiUSBH,
	MidiBLE,
	MidiWiFi,
	MidiSerial0,
	MidiSerial1,
	MidiSerial2
} MidiInterface;

void midi_Init();
void midi_ReadAll();
uint8_t midi_BleConnected();

void midi_AssignControlChangeCallback(void (*callback)(MidiInterface interface, uint8_t channel, uint8_t number, uint8_t value));
void midi_AssignProgramChangeCallback(void (*callback)(MidiInterface interface, uint8_t channel, uint8_t number));
void midi_AssignSysemExclusiveCallback(void (*callback)(MidiInterface interface, uint8_t* array, unsigned size));

void midih_loop();
void midih_setup();

#endif /* MIDI_HANDLING_H_ */