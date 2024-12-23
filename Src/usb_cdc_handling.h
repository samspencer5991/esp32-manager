#ifndef _USB_CDC_HANDLING_H_
#define _USB_CDC_HANDLING_H_

#include "Adafruit_TinyUSB.h"

extern Adafruit_USBH_CDC SerialHost;

void cdc_Init();
void cdc_Process();

extern uint8_t cdcDeviceInitRequired;

#endif // _USB_CDC_HANDLING_H_