#ifndef _USB_CDC_HANDLING_H_
#define _USB_CDC_HANDLING_H_

#include "Adafruit_TinyUSB.h"

typedef enum
{
	CDCDeviceNone = 0,
	CDCDeviceTonexOne = 1,
	CDCDeviceUnknown = 2,
} CDCDeviceType;

void cdc_Init();
void cdch_ProcessTask(void* parameter);
uint16_t cdc_Transmit(uint8_t* buffer, size_t len);
void cdc_DeviceConfiguredHandler();

extern Adafruit_USBH_CDC SerialHost;

extern uint8_t cdcDeviceInitRequired;
extern uint8_t cdcDeviceMounted;
extern CDCDeviceType cdcDeviceType;

#endif // _USB_CDC_HANDLING_H_