#include "midi_handling.h"
#include "midi.h"
#include "midi_Defs.h"

#ifdef USE_BLE_MIDI
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32_NimBLE.h>
#endif
#ifdef USE_USBH_MIDI
#include "SPI.h"
#endif
#include <Adafruit_TinyUSB.h>
#include <device_api/device_api.h>

#define SYSEX_START	0xF0
#define SYSEX_END		0xF7

#ifndef BLE_DEVICE_NAME
#define BLE_DEVICE_NAME "ESP32-MIDI"
#endif

// To avoid all MIDI ports have overly large SysEx buffers, Device API is supported only on BLE and USBD
struct DeviceApiPortSettings : public MIDI_NAMESPACE::DefaultSettings
{
	static const bool Use1ByteParsing = true;
	static const unsigned SysExMaxSize = 64*1024; // Accept SysEx messages up to 64K bytes long.
};

struct StandardPortSettings : public midi::DefaultSettings
{
    static const unsigned SysExMaxSize = 8*1024; // Accept SysEx messages up to 8K bytes long.
};

// Reception of sysex messages is locked to a single source
// This prevents contamination from multiple sources to the main buffer
uint8_t sysExBuffer[64*1024];
MidiInterfaceType sysExRxLock = MidiNone;
MidiInterfaceType sysExLastReceptionType = MidiNone;
SysExCommandType sysExCommand = SysExGeneral;
uint8_t receivedSysExAddress = 0;
uint8_t sysExRxComplete = 0;
uint32_t sysExBufferIndex = 0;


// Global application callbacks
void (*mControlChangeCallback)(MidiInterfaceType interface, uint8_t channel, uint8_t number, uint8_t value) = nullptr;
void (*mSystemExclusiveCallback)(MidiInterfaceType interface, uint8_t * array, unsigned size) = nullptr;
void (*mProgramChangeCallback)(MidiInterfaceType interface, uint8_t channel, uint8_t number) = nullptr;

void (*mPetalControlChangeCallback)(MidiInterfaceType interface, uint8_t channel, uint8_t number, uint8_t value) = nullptr;
void (*mPetalSystemExclusiveCallback)(MidiInterfaceType interface, uint8_t * array, unsigned size) = nullptr;
void (*mPetalProgramChangeCallback)(MidiInterfaceType interface, uint8_t channel, uint8_t number) = nullptr;

//-------------- MIDI Input/Output Objects & Handling --------------//
// Bluetooth Low Energy
#ifdef USE_BLE_MIDI
BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32_NimBLE> BLUEMIDI(BLE_DEVICE_NAME); \
MIDI_NAMESPACE::MidiInterface<BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32_NimBLE>,DeviceApiPortSettings> blueMidi((BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32_NimBLE> &)BLUEMIDI);

// State variables
bool bleConnected = false;
bool bleAuthenticated = false;
#endif

// USBD
#ifdef USE_USBD_MIDI
Adafruit_USBD_MIDI usbd_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usbd_midi, usbdMidi);

//MIDI_NAMESPACE::SerialMIDI<Adafruit_USBD_MIDI> USBDMIDI(usbd_midi);\
//MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<Adafruit_USBD_MIDI>> usbdMidi((MIDI_NAMESPACE::SerialMIDI<Adafruit_USBD_MIDI>&)USBDMIDI);

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
void processSysEx(MidiInterfaceType interface, uint8_t* array, unsigned size);

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
void blueMidi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value);
void blueMidi_ProgramChangeCallback(uint8_t channel, uint8_t number);
void blueMidi_SysexCallback(uint8_t * array, unsigned size);

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
	blueMidi.setHandleControlChange(blueMidi_ControlChangeCallback);
	blueMidi.setHandleProgramChange(blueMidi_ProgramChangeCallback);
	blueMidi.setHandleSystemExclusive(blueMidi_SysexCallback);

	BLUEMIDI.setHandleConnected(onConnected);
	BLUEMIDI.setHandleDisconnected(onDisconnected);
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
	blueMidi.begin();
#endif
	// USBD
#ifdef USE_USBD_MIDI
	usbdMidi.begin();
	usbdMidi.turnThruOff();
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
	blueMidi.read();
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
void midi_AssignControlChangeCallback(void (*callback)(MidiInterfaceType interface, uint8_t channel, uint8_t number, uint8_t value))
{
	mControlChangeCallback = callback;
}

void midi_AssignProgramChangeCallback(void (*callback)(MidiInterfaceType interface, uint8_t channel, uint8_t number))
{
	mProgramChangeCallback = callback;
}

void midi_AssignSysemExclusiveCallback(void (*callback)(MidiInterfaceType interface, uint8_t* array, unsigned size))
{
	mSystemExclusiveCallback = callback;
}

void midi_AssignPetalControlChangeCallback(void (*callback)(MidiInterfaceType interface, uint8_t channel, uint8_t number, uint8_t value))
{
	mPetalControlChangeCallback = callback;
}

void midi_AssignPetalProgramChangeCallback(void (*callback)(MidiInterfaceType interface, uint8_t channel, uint8_t number))
{
	mPetalProgramChangeCallback = callback;
}

void midi_AssignPetalSysemExclusiveCallback(void (*callback)(MidiInterfaceType interface, uint8_t* array, unsigned size))
{
	mPetalSystemExclusiveCallback = callback;
}

void midi_SendDeviceApiSysExString(const char* array, unsigned size, uint8_t containsFraming)
{
	usbdMidi.sendSysEx(size, (uint8_t*)array, containsFraming);
}

// Petal integration specific functions
void midi_SendPetalSysEx(const uint8_t* data, size_t size)
{

}

void midi_SendPetalControlChange(uint8_t channel, uint8_t number, uint8_t value)
{

}

void midi_SendPetalProgramChange(uint8_t channel, uint8_t number)
{

}

//-------------- Local Function Definitions --------------//
void processSysEx(MidiInterfaceType interface, uint8_t* array, unsigned size)
{
	// If this is the first SysEx packet received in the reception
	if(array[0] == SYSEX_START)
	{
		// Check for a valid received address
		if(array[1] == SYSEX_ADDRESS_BYTE1 &&
			array[2] == SYSEX_ADDRESS_BYTE2 &&
			array[3] == SYSEX_ADDRESS_BYTE3)
		{
			receivedSysExAddress = 1;
			if(array[4] == SYSEX_DEVICE_API_COMMAND)
				sysExCommand = SysExDeviceApi;
			else if(array[4] == SYSEX_PETAL_COMMAND)
				sysExCommand = SysExPetal;
			else
				sysExCommand = SysExGeneral;

			// For non-general SysEx receptions
			if(sysExCommand != SysExGeneral)
			{
				sysExRxLock = interface;
				// Copy the received SysEx address to the buffer
				// The start/end, address, and command bytes are not copied
				memcpy(&sysExBuffer[0], &array[5], size-6);
				sysExBufferIndex = size-6;
			}
		}
		else
		{
			sysExRxLock = MidiNone;
			sysExCommand = SysExGeneral;
		}
		if(array[size-1] == SYSEX_END)
		{
			sysExRxComplete = 1;
			sysExLastReceptionType = sysExRxLock;
			sysExRxLock = MidiNone;
		}
	}
	// Continuation of existing reception
	else if(array[0] == SYSEX_END)
	{
		// For non-general SysEx receptions
		if(sysExCommand != SysExGeneral)
		{
			// Copy the received SysEx address to the buffer
			// The start/end, address, and command bytes are not copied
			memcpy(&sysExBuffer[sysExBufferIndex], &array[5], size-2);
			sysExBufferIndex += size-2;
		}
		if(array[size-1] == SYSEX_END)
		{
			sysExRxComplete = 1;
			sysExLastReceptionType = sysExRxLock;
			sysExRxLock = MidiNone;
		}
	}
	if(sysExRxComplete)
	{
		Serial.printf("SysEx Complete with %d bytes with command type %d on port %d \n", sysExBufferIndex, sysExCommand, interface);
		if(sysExCommand == SysExDeviceApi)
		{
			deviceApi_Handler((char*)sysExBuffer, MIDI_TRANSPORT);
		}
		else if(sysExCommand == SysExPetal)
		{

		}
		else if(sysExCommand == SysExGeneral && mSystemExclusiveCallback != nullptr)
		{
			mSystemExclusiveCallback(MidiUSBD, sysExBuffer, sysExBufferIndex);
		}
		sysExRxComplete = 0;
		sysExBufferIndex = 0;
		sysExRxLock = MidiNone;
		sysExCommand = SysExGeneral;
	}
}

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
	processSysEx(MidiUSBD, array, size);
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("USBD MIDI SysEx: Size: %d\n", size);
#endif
	Serial.println(array[0]);
	Serial.println(array[size-1]);
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
void blueMidi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value)
{
	if (mControlChangeCallback != nullptr)
	{
		mControlChangeCallback(MidiBLE, channel, number, value);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("BLE MIDI CC: Ch: %d, Num: %d, Val: %d\n", channel, number, value);
#endif
}

void blueMidi_ProgramChangeCallback(uint8_t channel, uint8_t number)
{
	if (mProgramChangeCallback != nullptr)
	{
		mProgramChangeCallback(MidiBLE, channel, number);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("BLE MIDI PC: Ch: %d, Num: %d\n", channel, number);
#endif
}

void blueMidi_SysexCallback(uint8_t * array, unsigned size)
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


