#include "device_api_utility.h"
#include <Arduino.h>

void sendSysexOpening(uint8_t* address, uint8_t addressSize)
{
	// Sends the opening SysEx byte and the address
	uint8_t openingBuffer[1+addressSize];
	for(uint8_t i = 0; i < addressSize; i++)
	{
		openingBuffer[i+1] = address[i];
	}
	tud_midi_n_stream_write(0, 0, openingBuffer, addressSize+1);
}

void sendPacketTermination(uint8_t transport)
{
	if(transport == USB_CDC_TRANSPORT)
	{
		// Sends terminating character and new-line for readability
		const uint8_t txStr[] = {"~\n"};
		Serial.write(txStr, 2);
	}
	else if(transport == MIDI_TRANSPORT)
	{
		// Sends terminating character, new-line for readability, and end of SysEx byte
		uint8_t termBuffer[] = {'~', '\n', 0xF7};
		tud_midi_n_stream_write(0, 0, termBuffer, 3);
	}
}

// Gets the position of string in an array of strings. Returns -1 if not found
int getStringPos(const char **array, const char *string, uint16_t len)
{
	if(string != NULL)
	{
		for(uint16_t i=0; i<len; i++)
		{
			if(strcmp(array[i], string) == 0)
			{
				return i;
			}
		}
	}
	return STRING_NOT_FOUND;
}

long devApi_map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

long devApi_roundMap(long x, long in_min, long in_max, long out_min, long out_max)
{
	x *= 10;
	in_min *= 10;
	in_max *= 10;
	out_min *= 10;
	out_max *= 10;

	long temp = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
	int remainder = temp % 10;
	if(remainder >= 5)
		temp += 10-remainder;
	else
		temp -= remainder;
	return temp/10;
}