#ifndef DEVICE_API_UTILITY_H_
#define DEVICE_API_UTILITY_H_

#include "stdint.h"
#include "tusb.h"
#include "device_api.h"

#define SYSEX_DEVICE_API_COMMAND 	0x01

#define STRING_NOT_FOUND -1
#define ARRAY_LEN(arr) ((int) (sizeof (arr) / sizeof (arr)[0]))

struct CustomWriter {
	uint8_t repeatSerialisation = 0;
	uint8_t firstTransfer = 1;
	uint8_t braceRemoved = 0;
	uint8_t txBuf[128];
	uint8_t numBytes = 0;
	uint8_t transport = USB_CDC_TRANSPORT;
	size_t writeBuffer(const uint8_t *buffer, size_t length);
	size_t write(uint8_t c);
	size_t write(const uint8_t *buffer, size_t length);
	size_t flush();
};

void sendSysexOpening(uint8_t* address, uint8_t addressSize);
void sendPacketTermination(uint8_t transport);
int getStringPos(const char **array, const char *string, uint16_t len);
long devApi_map(long x, long in_min, long in_max, long out_min, long out_max);
long devApi_roundMap(long x, long in_min, long in_max, long out_min, long out_max);

#endif /*DEVICE_API_UTILITY_H_*/