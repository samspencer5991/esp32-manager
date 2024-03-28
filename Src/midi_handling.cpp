#include "midi_handling.h"
#include "midi.h"

#ifdef USE_BLE_MIDI
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32_NimBLE.h>
#endif
#ifdef USE_USBH_MIDI
#include "SPI.h"
#endif
#include <Adafruit_TinyUSB.h>



// Global application callbacks
void (*mControlChangeCallback)(MidiInterface interface, uint8_t channel, uint8_t number, uint8_t value) = nullptr;
void (*mSystemExclusiveCallback)(MidiInterface interface, uint8_t * array, unsigned size) = nullptr;
void (*mProgramChangeCallback)(MidiInterface interface, uint8_t channel, uint8_t number) = nullptr;

//-------------- MIDI Input/Output Objects & Handling --------------//
// Bluetooth Low Energy
#ifdef USE_BLE_MIDI
BLEMIDI_CREATE_INSTANCE("uLoopBLE", midiBle);
// State variables
bool bleConnected = false;
bool bleAuthenticated = false;
#endif
// USBD
#ifdef USE_USBD_MIDI
Adafruit_USBD_MIDI usbd_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usbd_midi, usbdMidi);
#endif
// USBH



// Serial0
#ifdef USE_SERIAL0_MIDI
MIDI_CREATE_INSTANCE(HardwareSerial, Serial0, serial0Midi);
#endif
// Serial1
#ifdef USE_SERIAL1_MIDI
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, serial1Midi);
#endif
// Serial2
#ifdef USE_SERIAL2_MIDI
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, serial2Midi);
#endif


//-------------- Private Function Prototypes --------------//
// USBD
#ifdef USE_USBD_MIDI
void usbdMidi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value);
void usbdMidi_ProgramChangeCallback(uint8_t channel, uint8_t number);
void usbdMidi_SysexCallback(uint8_t * array, unsigned size);
#endif

// USBH
#ifdef USE_USBH_MIDI
void usbhMidi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value);
void usbhMidi_ProgramChangeCallback(uint8_t channel, uint8_t number);
void usbhMidi_SysexCallback(uint8_t * array, unsigned size);
#endif

// BLE
#ifdef USE_BLE_MIDI
void bleMidi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value);
void bleMidi_ProgramChangeCallback(uint8_t channel, uint8_t number);
void bleMidi_SysexCallback(uint8_t * array, unsigned size);

void onConnected();
void onDisconnected();
#endif

// WiFi
#ifdef USE_WIFI_MIDI
void wifiMidi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value);
void wifiMidi_ProgramChangeCallback(uint8_t channel, uint8_t number);
void wifiMidi_SysexCallback(uint8_t * array, unsigned size);
#endif

// Serial0
#ifdef USE_SERIAL0_MIDI
void serial0Midi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value);
void serial0Midi_ProgramChangeCallback(uint8_t channel, uint8_t number);
void serial0Midi_SysexCallback(uint8_t * array, unsigned size);
#endif

// Serial1
#ifdef USE_SERIAL1_MIDI
void serial1Midi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value);
void serial1Midi_ProgramChangeCallback(uint8_t channel, uint8_t number);
void serial1Midi_SysexCallback(uint8_t * array, unsigned size);
#endif

// Serial2
#ifdef USE_SERIAL2_MIDI
void serial2Midi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value);
void serial2Midi_ProgramChangeCallback(uint8_t channel, uint8_t number);
void serial2Midi_SysexCallback(uint8_t * array, unsigned size);
#endif

//-------------- Global Function Definitions --------------//
void midi_Init()
{
	// General MIDI callback assignment
	// USBD
#ifdef USE_USBD_MIDI
	usbdMidi.setHandleControlChange(usbdMidi_ControlChangeCallback);
	usbdMidi.setHandleProgramChange(usbdMidi_ProgramChangeCallback);
	usbdMidi.setHandleSystemExclusive(usbdMidi_SysexCallback);
#endif

	// USBH

	// BLE
#ifdef USE_BLE_MIDI
	midiBle.setHandleControlChange(bleMidi_ControlChangeCallback);
	midiBle.setHandleProgramChange(bleMidi_ProgramChangeCallback);
	midiBle.setHandleSystemExclusive(bleMidi_SysexCallback);

	BLEmidiBle.setHandleConnected(onConnected);
	BLEmidiBle.setHandleDisconnected(onDisconnected);
#endif
	// WiFi


	// Serial0
#ifdef USE_SERIAL0_MIDI
	serial0Midi.setHandleControlChange(serial0Midi_ControlChangeCallback);
	serial0Midi.setHandleProgramChange(serial0Midi_ProgramChangeCallback);
	serial0Midi.setHandleSystemExclusive(serial0Midi_SysexCallback);
#endif

	// Serial1
#ifdef USE_SERIAL1_MIDI
	serial1Midi.setHandleControlChange(serial1Midi_ControlChangeCallback);
	serial1Midi.setHandleProgramChange(serial1Midi_ProgramChangeCallback);
	serial1Midi.setHandleSystemExclusive(serial1Midi_SysexCallback);
#endif

	// Serial2
#ifdef USE_SERIAL2_MIDI
	serial2Midi.setHandleControlChange(serial2Midi_ControlChangeCallback);
	serial2Midi.setHandleProgramChange(serial2Midi_ProgramChangeCallback);
	serial2Midi.setHandleSystemExclusive(serial2Midi_SysexCallback);
#endif

	// Begin MIDI interfaces
	// BLE
#ifdef USE_BLE_MIDI
	midiBle.begin();
#endif
	// USBD
#ifdef USE_USBD_MIDI
	usbdMidi.begin();
#endif
	// Serial0
#ifdef USE_SERIAL0_MIDI
	serial0Midi.begin();
#endif
}

void midi_ReadAll()
{
	// BLE
#ifdef USE_BLE_MIDI
	midiBle.read();
#endif
	// USBD
#ifdef USE_USBD_MIDI
	usbdMidi.read();
#endif
	// Serial0
#ifdef USE_SERIAL0_MIDI
	serial0Midi.read();
#endif
}

// Global MIDI callback assignment
void midi_AssignControlChangeCallback(void (*callback)(MidiInterface interface, uint8_t channel, uint8_t number, uint8_t value))
{
	mControlChangeCallback = callback;
}

void midi_AssignProgramChangeCallback(void (*callback)(MidiInterface interface, uint8_t channel, uint8_t number))
{
	mProgramChangeCallback = callback;
}

void midi_AssignSysemExclusiveCallback(void (*callback)(MidiInterface interface, uint8_t* array, unsigned size))
{
	mSystemExclusiveCallback = callback;
}


//-------------- Private Function Definitions --------------//
// USBD
#ifdef USE_USBD_MIDI
void usbdMidi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value)
{
	if (mControlChangeCallback != nullptr)
	{
		mControlChangeCallback(MidiUSBD, channel, number, value);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("USBD MIDI CC: Ch: %d, Num: %d, Val: %d\n", channel, number, value);
#endif
}

void usbdMidi_ProgramChangeCallback(uint8_t channel, uint8_t number)
{
	if (mProgramChangeCallback != nullptr)
	{
		mProgramChangeCallback(MidiUSBD, channel, number);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("USBD MIDI PC: Ch: %d, Num: %d\n", channel, number);
#endif
}

void usbdMidi_SysexCallback(uint8_t * array, unsigned size)
{
	if (mSystemExclusiveCallback != nullptr)
	{
		mSystemExclusiveCallback(MidiUSBD, array, size);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("USBD MIDI SysEx: Size: %d\n", size);
#endif
}
#endif

// USBH
#ifdef USE_USBH_MIDI
void usbhMidi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value)
{
	if (mControlChangeCallback != nullptr)
	{
		mControlChangeCallback(MidiUSBH, channel, number, value);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("USBH MIDI CC: Ch: %d, Num: %d, Val: %d\n", channel, number, value);
#endif
}

void usbhMidi_ProgramChangeCallback(uint8_t channel, uint8_t number)
{
	if (mProgramChangeCallback != nullptr)
	{
		mProgramChangeCallback(MidiUSBH, channel, number);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("USBH MIDI PC: Ch: %d, Num: %d\n", channel, number);
#endif
}

void usbhMidi_SysexCallback(uint8_t * array, unsigned size)
{
	if (mSystemExclusiveCallback != nullptr)
	{
		mSystemExclusiveCallback(MidiUSBH, array, size);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("USBH MIDI SysEx: Size: %d\n", size);
#endif
}
#endif

// BLE
#ifdef USE_BLE_MIDI
void bleMidi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value)
{
	if (mControlChangeCallback != nullptr)
	{
		mControlChangeCallback(MidiBLE, channel, number, value);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("BLE MIDI CC: Ch: %d, Num: %d, Val: %d\n", channel, number, value);
#endif
}

void bleMidi_ProgramChangeCallback(uint8_t channel, uint8_t number)
{
	if (mProgramChangeCallback != nullptr)
	{
		mProgramChangeCallback(MidiBLE, channel, number);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("BLE MIDI PC: Ch: %d, Num: %d\n", channel, number);
#endif
}

void bleMidi_SysexCallback(uint8_t * array, unsigned size)
{
	if (mSystemExclusiveCallback != nullptr)
	{
		mSystemExclusiveCallback(MidiBLE, array, size);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("BLE MIDI SysEx: Size: %d\n", size);
#endif
}

void onConnected()
{
	bleConnected = true;
}

void onDisconnected()
{
	bleConnected = false;
}
#endif


// WiFi


// Serial0
#ifdef USE_SERIAL0_MIDI
void serial0Midi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value)
{
	if (mControlChangeCallback != nullptr)
	{
		mControlChangeCallback(MidiSerial0, channel, number, value);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("Serial0 MIDI CC: Ch: %d, Num: %d, Val: %d\n", channel, number, value);
#endif
}

void serial0Midi_ProgramChangeCallback(uint8_t channel, uint8_t number)
{
	if (mProgramChangeCallback != nullptr)
	{
		mProgramChangeCallback(MidiSerial0, channel, number);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("Serial0 MIDI PC: Ch: %d, Num: %d\n", channel, number);
#endif
}

void serial0Midi_SysexCallback(uint8_t * array, unsigned size)
{
	if (mSystemExclusiveCallback != nullptr)
	{
		mSystemExclusiveCallback(MidiSerial0, array, size);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("Serial0 MIDI SysEx: Size: %d\n", size);
#endif
}
#endif

// Serial1
#ifdef USE_SERIAL1_MIDI
void serial1Midi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value)
{
	if (mControlChangeCallback != nullptr)
	{
		mControlChangeCallback(MidiSerial1, channel, number, value);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("Serial1 MIDI CC: Ch: %d, Num: %d, Val: %d\n", channel, number, value);
#endif
}

void serial1Midi_ProgramChangeCallback(uint8_t channel, uint8_t number)
{
	if (mProgramChangeCallback != nullptr)
	{
		mProgramChangeCallback(MidiSerial1, channel, number);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("Serial1 MIDI PC: Ch: %d, Num: %d\n", channel, number);
#endif
}

void serial1Midi_SysexCallback(uint8_t * array, unsigned size)
{
	if (mSystemExclusiveCallback != nullptr)
	{
		mSystemExclusiveCallback(MidiSerial1, array, size);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("Serial1 MIDI SysEx: Size: %d\n", size);
#endif
}
#endif


// Serial2
#ifdef USE_SERIAL2_MIDI
void serial2Midi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value)
{
	if (mControlChangeCallback != nullptr)
	{
		mControlChangeCallback(MidiSerial2, channel, number, value);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("Serial2 MIDI CC: Ch: %d, Num: %d, Val: %d\n", channel, number, value);
#endif
}

void serial2Midi_ProgramChangeCallback(uint8_t channel, uint8_t number)
{
	if (mProgramChangeCallback != nullptr)
	{
		mProgramChangeCallback(MidiSerial2, channel, number);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("Serial2 MIDI PC: Ch: %d, Num: %d\n", channel, number);
#endif
}

void serial2Midi_SysexCallback(uint8_t * array, unsigned size)
{
	if (mSystemExclusiveCallback != nullptr)
	{
		mSystemExclusiveCallback(MidiSerial2, array, size);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("Serial2 MIDI SysEx: Size: %d\n", size);
#endif
}
#endif


