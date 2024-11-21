#include "device_api.h"
#include "tusb.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <Arduino.h>
#include "midi_handling.h"


#define USB_DATA_LEN					8192
#define COMMAND_TYPE_BUF_SIZE		20
#define FALSE	0
#define TRUE	1

DeviceApiState apiState = DeviceApiReady;
DeviceApiCommand receivedCommand = NoCommand;
DeviceApiDataType apiDataType = DeviceApiNoData;

const char termChar[2] = "~";
const char commaChar[2] = ",";
const char newLineChar[2] = "\n";
char data[20];									// Incoming command type packet
char usbTxBuf[64];

// C++ Wrapper Declarations
// These must be implemented externally
// Transmit functions
void sendCheckResponse(uint8_t transport);
void sendGlobalSettings(uint8_t transport);
void sendBankSettings(int bankNum, uint8_t transport);
void sendBankId(int bankNum, uint8_t transport);
void sendCurrentBank(uint8_t transport);

// Parsing functions
void parseGlobalSettings(char* appData, uint8_t transport);
void parseBankSettings(char* appData, uint16_t bankNum, uint8_t transport);
void ctrlCommandHandler(char* appData, uint8_t transport);


//--------- Local Function Prototypes ---------//
void sendInvalidCommandPacket(uint8_t transport);
void sendOkPacket(uint8_t transport);
void sendTxOverflowMessage(uint8_t transport);
void sendInvalidTerminationPacket(uint8_t transport);
void sendEventPacket(char* message, uint8_t transport);
void resetDeviceApi();


//------------- Local Functions --------------//
void sendInvalidCommandPacket(uint8_t transport)
{
	const char buffer[] = USB_INVALID_COMMAND_STRING USB_PACKET_DELIMITER "\n";
	if(transport == MIDI_TRANSPORT)
	{
		midi_SendDeviceApiSysExString(buffer, strlen(buffer), 0);
	}
	else if(transport == USB_CDC_TRANSPORT)
	{
		Serial.write(buffer, strlen(buffer));
	}
}

void sendOkPacket(uint8_t transport)
{
	const char buffer[] = USB_VALID_COMMAND_STRING USB_PACKET_DELIMITER "\n";
	if(transport == MIDI_TRANSPORT)
	{
		midi_SendDeviceApiSysExString(buffer, strlen(buffer), 0);
	}
	else if(transport == USB_CDC_TRANSPORT)
	{
		Serial.write(buffer, strlen(buffer));
	}
}

void sendTxOverflowMessage(uint8_t transport)
{
	const char buffer[] = USB_TX_BUF_OVERFLOW_STRING USB_PACKET_DELIMITER "\n";
	if(transport == MIDI_TRANSPORT)
	{
		midi_SendDeviceApiSysExString(buffer, strlen(buffer), 0);
	}
	else if(transport == USB_CDC_TRANSPORT)
	{
		Serial.write(buffer, strlen(buffer));
	}
}

void sendInvalidTerminationPacket(uint8_t transport)
{
	const char buffer[] = USB_INVALID_TERMINATOR_STRING USB_PACKET_DELIMITER "\n";
	if(transport == MIDI_TRANSPORT)
	{
		midi_SendDeviceApiSysExString(buffer, strlen(buffer), 0);
	}
	else if(transport == USB_CDC_TRANSPORT)
	{
		Serial.write(buffer, strlen(buffer));
	}
}

void sendEventPacket(char* message, uint8_t transport)
{
	if(transport == MIDI_TRANSPORT)
	{
		midi_SendDeviceApiSysExString(message, strlen(message), 0);
	}
	else if(transport == USB_CDC_TRANSPORT)
	{
		Serial.write(message, strlen(message));
	}
}

void resetDeviceApi()
{
	receivedCommand = NoCommand;
	apiState = DeviceApiReady;
	apiDataType = DeviceApiNoData;
}


//------------- Global Functions -------------//
DeviceApiState deviceApi_Handler(char* appData, uint8_t transport)
{
	static uint16_t bankIndex = 0;
	// Device is ready to receive a new command
	if(apiState == DeviceApiReady)
	{
		// Fetch the four character command using a '~' for delimiting
		char command[6];
		size_t len = 0;
		if(transport == USB_CDC_TRANSPORT)
			len = Serial.readBytesUntil('~', command, 5);
		else if(transport == MIDI_TRANSPORT)
		{
			for(uint8_t i=0; i<5; i++)
			{
				command[i] = appData[i];
				if(command[i] == '~')
				{
					len = i;
				}
			}
		}

		// Reset the API and send an error message if no valid command terminator was found
		if(len != 4)
		{
			sendInvalidTerminationPacket(transport);
			resetDeviceApi();
			return DeviceApiError;
		}
		// Null terminate the string
		command[4] = '\0';	
		Serial.println(command);
		// CTRL (control) - Control the device
		if(strcmp(command, USB_CONTROL_COMMAND_TYPE_STRING) == 0)
		{
			receivedCommand = ControlCommand;
			apiState = DeviceApiNewCommand;
			sendOkPacket(transport);
		}
		// DREQ (data request) - Request data from the device
		else if(strcmp(command, USB_DATA_REQUEST_TYPE_STRING) == 0)
		{
			receivedCommand = DataRequestCommand;
			apiState = DeviceApiNewCommand;
			sendOkPacket(transport);
		}
		// DTXR (data transmit) - Transmit data to the device
		else if(strcmp(command, USB_DATA_TX_REQUEST_TYPE_STRING) == 0)
		{
			receivedCommand = DataTxRequestCommand;
			apiState = DeviceApiNewCommand;
			sendOkPacket(transport);
		}
		// CHCK (check) - Check if the device is connected and return device properties
		else if(strcmp(command, USB_CHECK_TYPE_STRING) == 0)
		{
			sendCheckResponse(transport);
		}
		// RSET (reset) - Reset the device api
		else if(strcmp(command, USB_RESET_TYPE_STRING) == 0)
		{
			resetDeviceApi();
			sendOkPacket(transport);
		}
		// No valid instruction type received
		else
		{
			sendInvalidCommandPacket(transport);
			receivedCommand = NoCommand;
			return DeviceApiError;
		}
		return DeviceApiNewCommand;
	}
	// An existing command has been received. Copy the data and check for a valid delimiter
	else
	{
		uint32_t startTime = millis();
		uint32_t byteCount = 0;
		while(1)
		{
			// Fetch the four character command using a '~' for delimiting
			size_t len = Serial.readBytesUntil('~', appData, USB_DATA_LEN);
			// Check for a timeout
			if(len == 0)
			{
				sendInvalidTerminationPacket(transport);
				resetDeviceApi();
				return DeviceApiError;
			}
			// Check for buffer overflow
			else if(len == USB_DATA_LEN)
			{
				sendTxOverflowMessage(transport);
				resetDeviceApi();
				return DeviceApiError;
			}

			// Valid terminating character found
			else
			{	
				// Null terminate the string
				appData[len] = '\0';
				// CTRL
				if(receivedCommand == ControlCommand)
				{
					ctrlCommandHandler(appData, transport);
					apiState = DeviceApiReady;
					sendOkPacket(transport);
					return DeviceApiReady;
				}
				// DREQ
				else if(receivedCommand == DataRequestCommand)
				{
					// Transmit global settings
					if(strcmp(appData, USB_GLOBAL_SETTINGS_STRING) == 0)
					{
						sendGlobalSettings(transport);
						apiState = DeviceApiReady;
						return DeviceApiReady;
					}
					// Commands with an argument separated by a comma
					else
					{
						char *dataTokenPtr = strtok(appData, commaChar);
						// Transmit bank settings
						if(strcmp(dataTokenPtr, USB_BANK_SETTINGS_STRING) == 0)
						{
							dataTokenPtr = strtok(NULL, termChar);
							uint8_t index = atoi(dataTokenPtr);
							sendBankSettings(index, transport);
							apiState = DeviceApiReady;
							return DeviceApiReady;
						}
						// Transmit current bank
						else if(strcmp(appData, USB_CURRENT_BANK_STRING) == 0)
						{
							sendCurrentBank(transport);
							apiState = DeviceApiReady;
							return DeviceApiReady;
						}
						// Transmit bank ID
						else if(strcmp(appData, USB_BANK_ID_STRING) == 0)
						{
							dataTokenPtr = strtok(NULL, termChar);
							uint8_t index = atoi(dataTokenPtr);
							sendBankId(index, transport);
							apiState = DeviceApiReady;
							return DeviceApiReady;
						}
						else
						{
							sendInvalidCommandPacket(transport);
							resetDeviceApi();
							return DeviceApiError;
						}
					}
				}
				// DTXR
				else if(receivedCommand == DataTxRequestCommand)
				{
					// No data, just globalSettings or bankSettings,x to parse
					if(apiDataType == DeviceApiNoData)
					{
						char *dataTokenPtr = strtok(appData, commaChar);
						// Receive bank settings
						if(strcmp(dataTokenPtr, USB_BANK_SETTINGS_STRING) == 0)
						{
							dataTokenPtr = strtok(NULL, termChar);
							bankIndex = atoi(dataTokenPtr);
							apiDataType = DeviceApiBankSettings;
							sendOkPacket(transport);
							return DeviceApiRxData;
						}
						// Receive global settings
						else if(strcmp(dataTokenPtr, USB_GLOBAL_SETTINGS_STRING) == 0)
						{
							apiDataType = DeviceApiGlobalSettings;
							sendOkPacket(transport);
							return DeviceApiRxData;
						}
						else
						{
							sendInvalidCommandPacket(transport);
							resetDeviceApi();
							return DeviceApiError;
						}
					}
					// Data packet for either bank or global settings
					else if(apiDataType == DeviceApiGlobalSettings)
					{
						parseGlobalSettings(appData, transport);
						resetDeviceApi();
						sendOkPacket(transport);
						return DeviceApiReady;
					}
					else if(apiDataType == DeviceApiBankSettings)
					{
						parseBankSettings(appData, bankIndex, transport);
						resetDeviceApi();
						sendOkPacket(transport);
						return DeviceApiReady;
					}
				}
				// RSET
				else
				{
					resetDeviceApi();
					sendOkPacket(transport);
					return DeviceApiReady;
				}
			}
		}
	}
	return DeviceApiError;
}

