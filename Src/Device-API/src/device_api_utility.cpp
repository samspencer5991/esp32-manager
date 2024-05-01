#include "device_api_utility.h"
#include <Arduino.h>
#include "midi_handling.h"

size_t transmitBuffer(const uint8_t *buffer, size_t size);
size_t transmitBufferMidi(const uint8_t *buffer, size_t size);

// Custom JSON writer
size_t CustomWriter::write(uint8_t c)
{
	return writeBuffer(&c, 1);
}

size_t CustomWriter::write(const uint8_t *buffer, size_t length)
{
	return writeBuffer(buffer, length);
}

size_t CustomWriter::writeBuffer(const uint8_t *buffer, size_t length)
{
	uint8_t modBuffer[64];
	memcpy(modBuffer, buffer, length);
	// Check for a repeat serialisation
	if(repeatSerialisation && !braceRemoved && !firstTransfer)
	{
		// Replace the opening brace with a ',' for repeat serialisation
		if(modBuffer[0] == '{')
		{
			modBuffer[0] = ',';
		}
		braceRemoved = 1;
	}
	if(firstTransfer)
	{
		if(transport == MIDI_TRANSPORT)
		{
			uint8_t addressBuffer[] = {0x00, 0x22, 0x33};
			sendSysexOpening(addressBuffer, 3);
		}
		firstTransfer = 0;
	}
	// If the new data will overflow the buffer, send a full buffer and store the rest
	uint16_t totalBytes = length;
	uint16_t writeCount = 0;
	if(numBytes + length > 64)
	{
		// Copy as many bytes as will fit into the buffer and transmit
		uint16_t remainingBytes = 64 - numBytes;
		memcpy(&txBuf[numBytes], modBuffer, remainingBytes);
		if(transport == USB_CDC_TRANSPORT)
			transmitBuffer(txBuf, 64);
		else if(transport == MIDI_TRANSPORT)
			transmitBufferMidi(txBuf, 64);
		writeCount = 64;
		totalBytes -= remainingBytes;
		// Perform as many full buffer writes as required to reduce the remaining bytes to less than 64
		while(totalBytes > 64)
		{
			memcpy(txBuf, &modBuffer[writeCount], 64);
			if(transport == USB_CDC_TRANSPORT)
				transmitBuffer(txBuf, 64);
			else if(transport == MIDI_TRANSPORT)
				transmitBufferMidi(txBuf, 64);
			writeCount += 64;
			totalBytes -= 64;
		}
		// Store the remaining available bytes
		memcpy(txBuf, &modBuffer[remainingBytes], totalBytes);
		numBytes = totalBytes;
		return length;
	}
	// If the new data does not fill the buffer, store it and return
	else
	{
		memcpy(&txBuf[numBytes], modBuffer, length);
		numBytes += length;
		return length;
	}
}

size_t CustomWriter::flush()
{
	Serial.println(numBytes);
	// Check for a repeat serialisation
	if(repeatSerialisation)
	{
		// Remove the closing brace for repeat serialisation
		// These are added manually to enable consecutive serialisations
		if(txBuf[numBytes-1] == '}')
		{
			txBuf[numBytes-1] = 0;
			numBytes--;
		}
		braceRemoved = 0;
	}
	size_t num = 0;
	if(transport == USB_CDC_TRANSPORT)
		num = transmitBuffer(txBuf, numBytes);
	else if(transport == MIDI_TRANSPORT)
		num = transmitBufferMidi(txBuf, numBytes);
	
	numBytes = 0;
	return num;
}

size_t transmitBuffer(const uint8_t *buffer, size_t size)
{
	size_t remain = size;
	while (remain && tud_cdc_n_connected(0))
	{
		size_t wrcount = tud_cdc_n_write(0, buffer, remain);
		remain -= wrcount;
		buffer += wrcount;
		tud_cdc_n_write_flush(0);
		// Write FIFO is full, run usb background to flush
		if (remain)
			tud_task();
	}
	return size - remain;
}

size_t transmitBufferMidi(const uint8_t *buffer, size_t size)
{
	size_t remain = size;
	while (remain && tud_midi_n_mounted(0))
	{
		midi_SendDeviceApiSysExString((const char*)buffer, remain, 1);
		//size_t wrcount = tud_cdc_n_write(0, buffer, remain);
		remain = 0;
	}
	return size - remain;
}

void sendSysexOpening(uint8_t* address, uint8_t addressSize)
{
	// Sends the address and device api command byte
	uint8_t openingBuffer[2+addressSize];
	openingBuffer[0] = 0xF0;
	for(uint8_t i = 0; i < addressSize; i++)
	{
		openingBuffer[i+1] = address[i];
	}
	openingBuffer[addressSize+1] = SYSEX_DEVICE_API_COMMAND;
	midi_SendDeviceApiSysExString((const char*)openingBuffer, addressSize+2, 1);
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
		uint8_t termBuffer[] = {'~', 0xF7};
		midi_SendDeviceApiSysExString((const char*)termBuffer, 2, 1);
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