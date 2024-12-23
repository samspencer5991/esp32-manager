#ifndef USBHOST_H_
#define USBHOST_H_

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

typedef struct
{
	tusb_desc_device_t desc_device;
	uint16_t manufacturer[32];
	uint16_t product[48];
	uint16_t serial[16];
	bool mounted;
} dev_info_t;

extern dev_info_t dev_info[];

void usbh_Init();
void usbh_Process();

void usbh_PrintDeviceDescriptor(dev_info_t *dev, uint8_t daddr);


#endif // USBHOST_H_