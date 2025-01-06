#ifdef USE_EXTERNAL_USB_HOST

#include "Arduino.h"
#include "usb_helpers.h"
#include "usb_cdc_handling.h"
#include "usb_host.h"

#include "tonexOne.h"



Adafruit_USBH_CDC SerialHost;

static const char *TAG = "USB_CDC_Handling";

uint8_t cdcDeviceIdx = 0;
uint8_t cdcDeviceMounted = 0;
dev_info_t *cdcDevice;
CDCDeviceType cdcDeviceType = CDCDeviceNone;
uint8_t cdcDeviceInitRequired = 0;
//---------------------- Private Function Prototypes ----------------------//

//---------------------- Public Functions ----------------------//
void cdc_Init()
{
	SerialHost.begin(115200);
}

void cdc_Process()
{
	// If a CDC device has been mounted correctly, read any available serial data and process events
	if (cdcDeviceMounted)
	{
		uint8_t buf[64];
		if (SerialHost.connected() && SerialHost.available())
		{
			size_t count = SerialHost.read(buf, sizeof(buf));

			// For specific connected devices
			if (cdcDeviceType == CDCDeviceTonexOne)
			{
				tonexOne_HandleReceivedData((char *)buf, count);
			}
		}
		if(cdcDeviceType == CDCDeviceTonexOne)
		{
			tonexOne_Process();
		}
	}
	if(cdcDeviceInitRequired)
	{
		if(cdcDeviceType == CDCDeviceTonexOne)
		{
			//tonexOne_SendHello();
		}
		cdcDeviceInitRequired = 0;
	}
}

uint16_t cdc_Transmit(uint8_t* buffer, size_t len)
{
	if(!cdcDeviceMounted)
		return 0;
	uint16_t remainingBytes = len;
	uint16_t sentBytes = 0;
	if(len > 64)
	{
		if(SerialHost.write(buffer, 64) != 64)
		{
			return sentBytes;
		}
		SerialHost.flush();
		delay(5);
		remainingBytes -= 64;
		sentBytes = 64;
	}
	while(1)
	{	
		if(remainingBytes > 64)
		{
			if(SerialHost.write(&buffer[sentBytes], 64) != 64)
			{
				return sentBytes;
			}
			SerialHost.flush();
			delay(5);
			sentBytes += 64;
			remainingBytes -= 64;
		}
		else
		{
			if(SerialHost.write(&buffer[sentBytes], remainingBytes) != remainingBytes)
			{
				return sentBytes;
			}
			SerialHost.flush();
			delay(5);
			sentBytes += remainingBytes;
			remainingBytes = 0;
			break;
		}
		tuh_task();
	}
	return sentBytes;
}

void cdc_DeviceConfiguredHandler()
{
	cdcDevice = &dev_info[cdcDeviceIdx];
	tusb_desc_device_t *desc = &cdcDevice->desc_device;
	ESP_LOGI(TAG, "  idVendor            0x%04x\r\n", desc->idVendor);
	ESP_LOGI(TAG, "  idProduct           0x%04x\r\n", desc->idProduct);

	// Currently, the only supported CDC device is the Tonex One
	if (cdcDevice->desc_device.idVendor == TONEX_ONE_VID &&
		 cdcDevice->desc_device.idProduct == TONEX_ONE_PID)
	{
		ESP_LOGI(TAG, "Tonex One found at idx %d. Mounting CDC Serial device.", cdcDeviceIdx);
		SerialHost.mount(cdcDeviceIdx);
		cdcDeviceType = CDCDeviceTonexOne;
		cdcDeviceInitRequired = 1;
		tonexOne_SendHello();
	}
}


//---------------------- Private Functions ----------------------//


//---------------------- Tiny USB Callbacks ----------------------//
// Invoked when a device with CDC interface is mounted
// idx is index of cdc interface in the internal pool.

extern "C" void tuh_cdc_mount_cb(uint8_t idx)
{
	/*
	// Set connection flags
	// Check for a supported CDC device
	// Getting the device descriptors is also done in the generic tuh_mount_cb, however this cb is called first
	// So to avoid an empty device descriptor, it is re-fetched to check for supported device connection
	// TODO check that idx is a valid method for sorting through attached devices
	cdcDevice = &dev_info[idx];
	tuh_descriptor_get_device_sync(idx + 1, &cdcDevice->desc_device, 18);

	tusb_desc_device_t *desc = &cdcDevice->desc_device;
	Serial0.printf("  idVendor            0x%04x\r\n", desc->idVendor);
	Serial0.printf("  idProduct           0x%04x\r\n", desc->idProduct);

	// Currently, the only supported CDC device is the Tonex One
	if (cdcDevice->desc_device.idVendor == TONEX_ONE_VID &&
		 cdcDevice->desc_device.idProduct == TONEX_ONE_PID)
	{
		ESP_LOGE(TAG, "Tonex One found at idx %d. Mounting CDC Serial device.", idx);
		SerialHost.mount(idx);
		cdcDeviceType = CDCDeviceTonexOne;
		cdcDeviceInitRequired = 1;
		cdcDeviceIdx = idx;
		tonexOne_SendHello();
	}
	*/
	// Bind SerialHost object to this interface index
	cdcDeviceIdx = idx;
	cdcDeviceMounted = 1;
	SerialHost.mount(idx);
}

// Invoked when a device with CDC interface is unmounted
extern "C" void tuh_cdc_umount_cb(uint8_t idx)
{
	cdcDeviceMounted = 0;
	cdcDevice = NULL;
	SerialHost.umount(idx);
	ESP_LOGV(TAG, "SerialHost is disconnected");
}

#endif