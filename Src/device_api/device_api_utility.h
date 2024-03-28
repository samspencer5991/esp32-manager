#ifndef DEVICE_API_UTILITY_H_
#define DEVICE_API_UTILITY_H_

#include "stdint.h"
#include "tusb.h"
#include "device_api.h"

#define STRING_NOT_FOUND -1
#define ARRAY_LEN(arr) ((int) (sizeof (arr) / sizeof (arr)[0]))

void sendSysexOpening(uint8_t* address, uint8_t addressSize);
void sendPacketTermination(uint8_t transport);
int getStringPos(const char **array, const char *string, uint16_t len);
long devApi_map(long x, long in_min, long in_max, long out_min, long out_max);
long devApi_roundMap(long x, long in_min, long in_max, long out_min, long out_max);

#endif /*DEVICE_API_UTILITY_H_*/