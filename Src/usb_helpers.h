#ifndef USB_HELPERS_H_
#define USB_HELPERS_H_

#include "Arduino.h"

static void _convert_utf16le_to_utf8(const uint16_t *utf16, size_t utf16_len, uint8_t *utf8, size_t utf8_len);
static int _count_utf8_bytes(const uint16_t *buf, size_t len);
void utf16_to_utf8(uint16_t *temp_buf, size_t buf_len) ;

#endif // USB_HELPERS_H_
