#include "Arduino.h"
#include "esp32_manager.h"
#include "MIDI.h"
#include "midi_handling.h"

#ifdef USE_BLE_MIDI
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32_NimBLE.h>
#ifdef USE_BLE_MIDI_CLIENT
#include <hardware/BLEMIDI_Client_ESP32.h>
#endif
#endif
#ifdef USE_WIFI_RTP_MIDI
#include <AppleMIDI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include "WiFi_management.h"
#endif

#include "SPI.h"
#ifdef USE_USBH_MIDI

#include "usbhost.h"
#endif
#include <Adafruit_TinyUSB.h>
#include <device_api.h>

#define SYSEX_START	0xF0
#define SYSEX_END		0xF7

#ifndef BLE_DEVICE_NAME
#define BLE_DEVICE_NAME "MIDI BLE"
#endif

const char* TAG = "MIDI";

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

// MIDI thru array pointers
uint8_t numMidiHandles = 0;
uint8_t* usbdidiThruHandlesPtr;
uint8_t* usbhMidiThruHandlesPtr;
uint8_t* bleMidiThruHandlesPtr;
uint8_t* wifiMidiThruHandlesPtr;
uint8_t* serial0MidiThruHandlesPtr;
uint8_t* serial1MidiThruHandlesPtr;
uint8_t* serial2MidiThruHandlesPtr;


//-------------- MIDI Input/Output Objects & Handling --------------//
// Bluetooth Low Energy
#ifdef USE_BLE_MIDI

BLEMIDI_CREATE_CUSTOM_INSTANCE("FootCtrl", blueMidi, BLEMIDI_NAMESPACE::DefaultSettings)

#ifdef USE_BLE_MIDI_CLIENT
BLEMIDI_CREATE_CUSTOM_INSTANCE("FootCtrl", blueMidiClient, BLEMIDI_NAMESPACE::DefaultSettingsClient)
#endif

// State variables
uint8_t bleEnabled = 0;
bool bleConnected = false;
#endif

// WiFi Apple/RTP
#ifdef USE_WIFI_RTP_MIDI
APPLEMIDI_NAMESPACE::AppleMIDISession<WiFiUDP> RTP(RTP_SESSION_NAME, DEFAULT_CONTROL_PORT); \
MIDI_NAMESPACE::MidiInterface<APPLEMIDI_NAMESPACE::AppleMIDISession<WiFiUDP>, APPLEMIDI_NAMESPACE::AppleMIDISettings> rtpMidi((APPLEMIDI_NAMESPACE::AppleMIDISession<WiFiUDP> &)RTP);

// State variables
int rtpIsConnected = 0;
#endif

// USBD
#ifdef USE_USBD_MIDI
Adafruit_USBD_MIDI usbd_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usbd_midi, usbdMidi);
#endif

// USBH
#ifdef USE_USBH_MIDI
extern Adafruit_USBH_Host usbh_midi;

#endif

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

void blueMidi_OnConnected();
void blueMidi_OnDisconnected();
#endif

// WiFi
#ifdef USE_WIFI_RTP_MIDI
void rtpMidi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value);
void rtpMidi_ProgramChangeCallback(uint8_t channel, uint8_t number);
void rtpMidi_SysexCallback(uint8_t * array, unsigned size);
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


//-------------- FreeRTOS Tasks --------------//
void midi_ProcessTask(void* parameter)
{
	//UBaseType_t uxHighWaterMark;
	while(1)
	{
		midi_ReadAll();
		//uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
		//ESP_LOGD(TAG, "MIDI Process Task High Water Mark: %d", uxHighWaterMark);
		vTaskDelay(5);
	}
}


//-------------- Global Function Definitions --------------//
void midi_Init()
{
	// General MIDI callback assignment
	// USBD
#ifdef USE_USBD_MIDI
	numMidiHandles++;
	usbdMidi.setHandleControlChange(usbdMidi_ControlChangeCallback);
	usbdMidi.setHandleProgramChange(usbdMidi_ProgramChangeCallback);
	usbdMidi.setHandleSystemExclusive(usbdMidi_SysexCallback);
#endif

	// USBH
#ifdef USE_USBH_MIDI
	numMidiHandles++;
	//midih_Init();
#endif

	// BLE
#ifdef USE_BLE_MIDI
	numMidiHandles++;
	// Server
	blueMidi.setHandleControlChange(blueMidi_ControlChangeCallback);
	blueMidi.setHandleProgramChange(blueMidi_ProgramChangeCallback);
	blueMidi.setHandleSystemExclusive(blueMidi_SysexCallback);

	BLEblueMidi.setHandleConnected(blueMidi_OnConnected);
	BLEblueMidi.setHandleDisconnected(blueMidi_OnDisconnected);

	// Client
#ifdef USE_BLE_MIDI_CLIENT
	BLEblueMidiClient.setHandleConnected([]()
		{
			Serial.println("---------CONNECTED---------");
		});

	BLEblueMidiClient.setHandleDisconnected([]()
		{
			Serial.println("---------NOT CONNECTED---------");
		});
#endif
#endif

	// WiFi RTP (Apple MIDI)
#ifdef USE_WIFI_RTP_MIDI
	numMidiHandles++;		
	RTP.setHandleConnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc, const char* name) {
    rtpIsConnected++;
	 Serial.println("Connected to session");
    //Serial.printf("Connected to session %s %s", ssrc, name);
  });
  RTP.setHandleDisconnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc) {
    rtpIsConnected--;
    Serial.printf("Disconnected %s", ssrc);
  });
  
  rtpMidi.setHandleNoteOn([](byte channel, byte note, byte velocity) {
    Serial.printf("NoteOn %d", note);
  });
  rtpMidi.setHandleNoteOff([](byte channel, byte note, byte velocity) {
    Serial.printf("NoteOff %d", note);
  });
#endif

	// Serial0
#ifdef USE_SERIAL0_MIDI
  	numMidiHandles++;
	serial0Midi.setHandleControlChange(serial0Midi_ControlChangeCallback);
	serial0Midi.setHandleProgramChange(serial0Midi_ProgramChangeCallback);
	serial0Midi.setHandleSystemExclusive(serial0Midi_SysexCallback);
#endif

	// Serial1
#ifdef USE_SERIAL1_MIDI
	numMidiHandles++;		
	serial1Midi.setHandleControlChange(serial1Midi_ControlChangeCallback);
	serial1Midi.setHandleProgramChange(serial1Midi_ProgramChangeCallback);
	serial1Midi.setHandleSystemExclusive(serial1Midi_SysexCallback);
#endif

	// Serial2
#ifdef USE_SERIAL2_MIDI
	numMidiHandles++;
	serial2Midi.setHandleControlChange(serial2Midi_ControlChangeCallback);
	serial2Midi.setHandleProgramChange(serial2Midi_ProgramChangeCallback);
	serial2Midi.setHandleSystemExclusive(serial2Midi_SysexCallback);
#endif

	// Begin MIDI interfaces
		// USBD
#ifdef USE_USBD_MIDI
	ESP_LOGV(TAG, "Starting USBD MIDI");
	usbdMidi.begin(MIDI_CHANNEL_OMNI);
	usbdMidi.turnThruOff();
#endif
	// BLE
#ifdef USE_BLE_MIDI
	if(esp32ConfigPtr->wirelessType == Esp32BLE)
	{
		if(esp32ConfigPtr->bleMode == Esp32BLEServer)
		{
			ESP_LOGV(TAG, "Starting BLE MIDI Server");
			blueMidi.begin(MIDI_CHANNEL_OMNI);
		}
#ifdef USE_BLE_MIDI_CLIENT
		else if(esp32ConfigPtr->bleMode == Esp32BLEClient)
		{
			ESP_LOGV(TAG, "Starting BLE MIDI Central");
			blueMidiClient.begin(MIDI_CHANNEL_OMNI);
		}
#endif
		
	}
#endif
	// Serial0
#ifdef USE_SERIAL0_MIDI
	ESP_LOGV(TAG, "Starting Serial0 MIDI");
	serial0Midi.begin(MIDI_CHANNEL_OMNI);
#endif
	// Serial1
#ifdef USE_SERIAL1_MIDI
	ESP_LOGV(TAG, "Starting Serial1 MIDI");
	serial1Midi.begin(MIDI_CHANNEL_OMNI);
#endif
	// Serial2
#ifdef USE_SERIAL2_MIDI
	ESP_LOGV(TAG, "Starting Serial2 MIDI");
	serial2Midi.begin(MIDI_CHANNEL_OMNI);
#endif
}

void midi_ApplyThruSettings()
{
	// Apply MIDI thru settings
	// USBD
#ifdef USE_USBD_MIDI
	if(usbdMidiThruHandlesPtr[MidiUSBD])
		usbdMidi.turnThruOn();
	else
		usbdMidi.turnThruOff();
#endif
	// USBH
#ifdef USE_USBH_MIDI	
	if(usbhMidiThruHandlesPtr[MidiUSBH])
		usbhMidi.turnThruOn();
	else
		usbhMidi.turnThruOff();
#endif
	// BLE
#ifdef USE_BLE_MIDI
	if(bleMidiThruHandlesPtr[MidiBLE])
		blueMidi.turnThruOn();
	else
		blueMidi.turnThruOff();
#endif
	// WiFi
#ifdef USE_WIFI_RTP_MIDI
	if(wifiMidiThruHandlesPtr[MidiWiFiRTP])
		rtpMidi.turnThruOn();
	else
		rtpMidi.turnThruOff();
#endif
	// Serial0
#ifdef USE_SERIAL0_MIDI
	if(serial0MidiThruHandlesPtr[MidiSerial0])
		serial0Midi.turnThruOn();
	else 
		serial0Midi.turnThruOff();
#endif
	// Serial1
#ifdef USE_SERIAL1_MIDI
	if(serial1MidiThruHandlesPtr[MidiSerial1])
		serial1Midi.turnThruOn();
	else
		serial1Midi.turnThruOff();
#endif
	// Serial2
#ifdef USE_SERIAL2_MIDI
	if(serial2MidiThruHandlesPtr[MidiSerial2])
		serial2Midi.turnThruOn();
	else
		serial2Midi.turnThruOff();
#endif
}

// WiFi RTP must be initialised after WiFi is connected
#ifdef USE_WIFI_RTP_MIDI
void midi_InitWiFiRTP()
{
	if(esp32ConfigPtr->wirelessType != Esp32WiFi)
	{
		ESP_LOGI(TAG, "WiFi not enabled, cannot start RTP MIDI");
		return;
	}
	else
	{
		if(esp32Info.wifiConnected)
		{
			Serial.print("{\"debug\":{\"address\":\"");
			Serial.print(WiFi.localIP());
			Serial.print("\",\"port\":");
			Serial.print(RTP.getPort());
			Serial.print(",\"name\":\"");
			Serial.print(RTP.getName());
			Serial.print("\"}}~\n");
			rtpMidi.begin(MIDI_CHANNEL_OMNI);
		}
		else
		{
			//Serial.println("WiFi not connected, cannot start RTP MIDI");
		}
	}
}
#endif



void midi_ReadAll()
{
	// BLE
#ifdef USE_BLE_MIDI
	if(esp32ConfigPtr->wirelessType == Esp32BLE)
	{
		if(esp32ConfigPtr->bleMode == Esp32BLEServer)
		{
			blueMidi.read();
		}
#ifdef USE_BLE_MIDI_CLIENT
		else
		{
			if(blueMidiClient.read())
			{
				
			}
		}
#endif
		
	}
#endif
	// WiFi RTP
#ifdef USE_WIFI_RTP_MIDI
	if(esp32ConfigPtr->wirelessType == Esp32WiFi && esp32Info.wifiConnected)
	{
		rtpMidi.read();
	}
#endif
	// USBD
#ifdef USE_USBD_MIDI
	usbdMidi.read();
#endif
	// USBH
#ifdef USE_USBH_MIDI
	//midih_Loop();
#endif
	// Serial0
#ifdef USE_SERIAL0_MIDI
	serial0Midi.read();
#endif
	// Serial1
#ifdef USE_SERIAL1_MIDI
	serial1Midi.read();
#endif
	// Serial2
#ifdef USE_SERIAL2_MIDI
	serial2Midi.read();
#endif
}

// Global MIDI callback assignment functions
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
#ifdef USE_USBD_MIDI
	if(sysExLastReceptionType == MidiUSBD)
	{
		usbdMidi.sendSysEx(size, (uint8_t*)array, containsFraming);
		return;
	}
#endif

#ifdef USE_BLE_MIDI
	if(sysExLastReceptionType == MidiBLE)
	{
		blueMidi.sendSysEx(size, (uint8_t*)array, containsFraming);
		return;
	}
#endif

#ifdef USE_WIFI_RTP_MIDI
	else if(sysExLastReceptionType == MidiWiFiRTP)
	{
		const char testArray[] = {0xF0, 0x00, 0x22, 0x33, 0x01, 0x02, 0x03, 0x04, 0x05, 0xF7};
		rtpMidi.sendSysEx(10, (uint8_t*)testArray, 1);
		rtpMidi.sendSysEx(size, (uint8_t*)array, containsFraming);
		for(int i=0; i<size; i++)
		{
			Serial.printf("%02X ", array[i]);
		}
		Serial.println();
		//usbdMidi.sendSysEx(size, (uint8_t*)array, containsFraming);
	}
#endif
}

// Petal integration specific functions. These are typically called by the Petal execution
void midi_SendPetalSysEx(const uint8_t* data, size_t size)
{

}

void midi_SendPetalControlChange(uint8_t channel, uint8_t number, uint8_t value)
{

}

void midi_SendPetalProgramChange(uint8_t channel, uint8_t number)
{

}


// Process SysEx data received on any interface.
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

void testMidi()
{
	//rtpMidi.read();
	//midi_ReadAll();
  // send a note every second
  // (dont cáll delay(1000) as it will stall the pipeline)
  //if ((rtpIsConnected > 0) && (millis() - t0) > 1000)

  
}


//-------------- Handle Specific Functions --------------//
// USBD
#ifdef USE_USBD_MIDI
void usbdMidi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value)
{
	if (mControlChangeCallback != nullptr)
	{
		mControlChangeCallback(MidiUSBD, channel, number, value);
	}
	//ESP_LOGI(TAG, "USBD MIDI CC: Ch: %d, Num: %d, Val: %d\n", channel, number, value);

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

	ESP_LOGI(TAG, "USBD MIDI SysEx: Size: %d\n", size);

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
	ESP_LOGI(TAG, "USBH MIDI CC: Ch: %d, Num: %d, Val: %d\n", channel, number, value);
}

void usbhMidi_ProgramChangeCallback(uint8_t channel, uint8_t number)
{
	if (mProgramChangeCallback != nullptr)
	{
		mProgramChangeCallback(MidiUSBH, channel, number);
	}
	ESP_LOGI(TAG, "USBH MIDI PC: Ch: %d, Num: %d\n", channel, number);
}

void usbhMidi_SysexCallback(uint8_t * array, unsigned size)
{
	if (mSystemExclusiveCallback != nullptr)
	{
		mSystemExclusiveCallback(MidiUSBH, array, size);
	}
	ESP_LOGI(TAG, "USBH MIDI SysEx: Size: %d\n", size);

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
	ESP_LOGI(TAG, "BLE MIDI CC: Ch: %d, Num: %d, Val: %d\n", channel, number, value);
}

void blueMidi_ProgramChangeCallback(uint8_t channel, uint8_t number)
{
	if (mProgramChangeCallback != nullptr)
	{
		mProgramChangeCallback(MidiBLE, channel, number);
	}
	ESP_LOGI(TAG, "BLE MIDI PC: Ch: %d, Num: %d\n", channel, number);
}

void blueMidi_SysexCallback(uint8_t * array, unsigned size)
{
	processSysEx(MidiBLE, array, size);
	if (mSystemExclusiveCallback != nullptr)
	{
		mSystemExclusiveCallback(MidiBLE, array, size);
	}
	ESP_LOGI(TAG, "BLE MIDI SysEx: Size: %d\n", size);
}

void blueMidi_OnConnected()
{
	bleConnected = true;
	Serial.println("BLE connected");
}

void blueMidi_OnDisconnected()
{
	bleConnected = false;
	Serial.println("BLE disconnected");
}
#endif

// WiFi RTP
#ifdef USE_WIFI_RTP_MIDI
void rtpMidi_ControlChangeCallback(uint8_t channel, uint8_t number, uint8_t value)
{
	if (mControlChangeCallback != nullptr)
	{
		mControlChangeCallback(MidiWiFiRTP, channel, number, value);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("WiFi RTP MIDI CC: Ch: %d, Num: %d, Val: %d\n", channel, number, value);
#endif
}

void rtpMidi_ProgramChangeCallback(uint8_t channel, uint8_t number)
{
	if (mProgramChangeCallback != nullptr)
	{
		mProgramChangeCallback(MidiWiFiRTP, channel, number);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("WiFi RTP MIDI PC: Ch: %d, Num: %d\n", channel, number);
#endif
}

void rtpMidi_SysexCallback(uint8_t * array, unsigned size)
{
	processSysEx(MidiWiFiRTP, array, size);
	if (mSystemExclusiveCallback != nullptr)
	{
		mSystemExclusiveCallback(MidiWiFiRTP, array, size);
	}
#if(CORE_DEBUG_LEVEL >= 4)
	Serial.printf("WiFi RTP MIDI SysEx: Size: %d\n", size);
#endif
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


//-------------------- BLE Management --------------------//
#ifdef USE_BLE_MIDI
void turnOnBLE()
{
	bleEnabled = 1;
	blueMidi.begin();
}

void turnOffBLE()
{
	bleEnabled = 0;
	//BLUEMIDI.end();
	NimBLEDevice::deinit(true);
}
#endif
