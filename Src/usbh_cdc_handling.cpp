#ifdef USE_EXTERNAL_USB_HOST

#include "Arduino.h"
#include "usb_helpers.h"
#include "usbh_cdc_handling.h"
#include "usb_host.h"

#include "tonexOne.h"
#include "tonexOne_Interface.h"

#define BULK_XFER_DELAY 3 // ms

Adafruit_USBH_CDC SerialHost;

static const char *TAG = "USB_CDC_Handling";

uint8_t cdcDeviceIdx = 0;
uint8_t cdcDeviceMounted = 0;
dev_info_t *cdcDevice;
CDCDeviceType cdcDeviceType = CDCDeviceNone;
uint8_t cdcDeviceInitRequired = 0;

uint8_t cdcTransferInProgress = 0;

//---------------------- Private Function Prototypes ----------------------//

//---------------------- Public Functions ----------------------//
void cdc_Init()
{
	SerialHost.begin(115200);
}

void cdch_ProcessTask(void* parameter)
{
	while(1)
	{
		//UBaseType_t uxHighWaterMark;
		//uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
		//ESP_LOGD(TAG, "CDC Process Task High Water Mark: %d", uxHighWaterMark);
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
				if(cdcDeviceInitRequired == 0)
					tonexOne_Process();
			}
			if(cdcTransferInProgress)
			{
				cdc_Transmit(NULL, 0);
			}
		}
		if(cdcDeviceInitRequired)
		{
			if(cdcDeviceType == CDCDeviceTonexOne)
			{
				tonexOne_Init();
				tonexOne_InterfaceInit();
				tonexOne_SendHello();
				//tonexOne_SendHello();
			}
			cdcDeviceInitRequired = 0;
		}
		// Feed watchdog and ensure task splitting
		vTaskDelay(5 / portTICK_PERIOD_MS);
	}
}

uint16_t cdc_Transmit(uint8_t* buffer, size_t len)
{
	int availableSize = 0;						// size of the buffer available for writing (to check for errors)
	uint32_t transmitStartTime = millis();	// start time of the transmission
	static uint16_t remainingBytes = 0;
	static uint16_t sentBytes = 0;
	static uint8_t *bufferPtr = NULL;
	static size_t bufferSize = 0;
	static uint16_t xferCounter = 0;

	if(!cdcDeviceMounted)
	{
		ESP_LOGD(TAG, "No CDC device mounted, cannot transmit data");
		return 0;
	}
	// Check that an existing transfer is not already in progress
	if(cdcTransferInProgress && len != 0)
	{
		ESP_LOGD(TAG, "CDC transmit already in progress, aborting new transfer");
		return 0;
	}
	
	// Start of a new transfer
	if(!cdcTransferInProgress && len > 0 && buffer != NULL)
	{
		bufferPtr = buffer;
		bufferSize = len;
		xferCounter = 1;
		cdcTransferInProgress = 1;
		sentBytes = 0;
		remainingBytes = bufferSize;
		uint8_t packetSize = 0;
		// Transfer is longer than 64 bytes and needs to be split into multiple packets
		if(bufferSize > 64)
		{
			packetSize = 64;
		}
		else
		{
			packetSize = bufferSize;
		}
		ESP_LOGD(TAG, "First transfer of %d bytes to CDC device", packetSize);
		// Check that the driver is ready for writing more data
		availableSize = SerialHost.availableForWrite();
		while(availableSize < packetSize)
		{
			// check for timeout event of 50ms
			if(millis() - transmitStartTime > 50)
			{
				ESP_LOGE(TAG, "CDC transmit timeout after %d ms", millis() - transmitStartTime);
				return sentBytes;
			}
			availableSize = SerialHost.availableForWrite();
			vTaskDelay(1/portTICK_PERIOD_MS);
		}
		// print the data packet for error checking
		esp_log_buffer_hex_internal(TAG, bufferPtr, packetSize, ESP_LOG_DEBUG);
		if(SerialHost.write(bufferPtr, packetSize) != packetSize)
		{
			return sentBytes;
		}
		SerialHost.flush();
		
		remainingBytes -= packetSize;
		sentBytes = packetSize;
		// If all the data has been sent, reset the transfer variables
		if(remainingBytes == 0)
		{
			cdcTransferInProgress = 0;
			bufferPtr = NULL;
			bufferSize = 0;
			ESP_LOGD(TAG, "CDC transfer complete, %d bytes sent", sentBytes);
		}
		//vTaskDelay(BULK_XFER_DELAY/portTICK_PERIOD_MS);
		return sentBytes;

	}
	// If a transfer is in progress, send the next packet
	else if(cdcTransferInProgress && buffer == NULL && len == 0)
	{
		uint8_t packetSize = 0;
		// Transfer is longer than 64 bytes and needs to be split into multiple packets
		if(remainingBytes > 64)
		{
			packetSize = 64;
		}
		else
		{
			packetSize = remainingBytes;
		}
		ESP_LOGD(TAG, "Transfer (%d) of %d bytes to CDC device", xferCounter+1, packetSize);
		// Check that the driver is ready for writing more data
		availableSize = SerialHost.availableForWrite();
		while(availableSize < packetSize)
		{
			// check for timeout event of 50ms
			if(millis() - transmitStartTime > 50)
			{
				// Flush and try again
				ESP_LOGE(TAG, "CDC transmit timeout after %d ms", millis() - transmitStartTime);
				SerialHost.flush();
				ESP_LOGD(TAG, "Flushing USBH buffers to retry.");
				return sentBytes;
			}
			availableSize = SerialHost.availableForWrite();
			vTaskDelay(1/portTICK_PERIOD_MS);
		}
		xferCounter++;
		// print the data packet for error checking
		//esp_log_buffer_hex_internal(TAG, bufferPtr, packetSize, ESP_LOG_DEBUG);
		if(SerialHost.write(&bufferPtr[sentBytes], packetSize) != packetSize)
		{
			return sentBytes;
		}
		SerialHost.flush();
		
		remainingBytes -= packetSize;
		sentBytes += packetSize;
		// If all the data has been sent, reset the transfer variables
		if(remainingBytes == 0)
		{
			cdcTransferInProgress = 0;
			bufferPtr = NULL;
			bufferSize = 0;
			xferCounter = 0;
			ESP_LOGD(TAG, "CDC transfer complete, %d bytes sent", sentBytes);
		}
		else
		{
			
		}
		//vTaskDelay(BULK_XFER_DELAY/portTICK_PERIOD_MS);
		return sentBytes;
	}
}

uint16_t cdc_HandleTransfer(uint8_t *buffer, uint16_t length)
{

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
	}
}


//---------------------- Private Functions ----------------------//


//---------------------- Tiny USB Callbacks ----------------------//
// Invoked when a device with CDC interface is mounted
// idx is index of cdc interface in the internal pool.

extern "C" void tuh_cdc_mount_cb(uint8_t idx)
{
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