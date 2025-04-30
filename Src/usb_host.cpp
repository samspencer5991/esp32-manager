#include "usb_host.h"

#ifdef USE_EXTERNAL_USB_HOST

#include "SPI.h"
#include "usb_helpers.h"
#include "usbh_cdc_handling.h"
#include "tonexone.h"

// Language ID: English
#define LANGUAGE_ID 0x0409


dev_info_t dev_info[CFG_TUH_DEVICE_MAX] = {0};

Adafruit_USBH_Host USBHost(new SPIClass(USBH_SPI), USBH_SPI_SCK_PIN, USBH_SPI_MOSI_PIN, USBH_SPI_MISO_PIN, USBH_CS_PIN, USBH_INT_PIN);


//---------------------- Private Function Prototypes ----------------------//
void parse_config_descriptor(uint8_t dev_addr, tusb_desc_configuration_t const *desc_cfg);
uint16_t count_interface_total_len(tusb_desc_interface_t const *desc_itf, uint8_t itf_count, uint16_t max_len);
void print_lsusb(void);


// FreeRTOS Tasks
void usbh_ProcessTask(void* parameter)
{
	UBaseType_t uxHighWaterMark;
	static uint8_t usbProcessCount = 0;
	while(1)
	{
		//uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
		//ESP_LOGD("USBH", "USBH Process Task High Water Mark: %d", uxHighWaterMark);
		USBHost.task(0);
		// Feed watchdog and ensure task splitting
		vTaskDelay(1 / portTICK_PERIOD_MS);
	}

}

//---------------------- Public Functions ----------------------//
void usbh_Init()
{
	USBHost.begin(1);
}

void usbh_Process()
{
	USBHost.task();
}

void usbh_PrintDeviceDescriptor(dev_info_t *dev, uint8_t daddr)
{
	tusb_desc_device_t *desc = &dev->desc_device;

	Serial0.printf("Device %u: ID %04x:%04x\r\n", daddr, desc->idVendor, desc->idProduct);
	Serial0.printf("Device Descriptor:\r\n");
	Serial0.printf("  bLength             %u\r\n", desc->bLength);
	Serial0.printf("  bDescriptorType     %u\r\n", desc->bDescriptorType);
	Serial0.printf("  bcdUSB              %04x\r\n", desc->bcdUSB);
	Serial0.printf("  bDeviceClass        %u\r\n", desc->bDeviceClass);
	Serial0.printf("  bDeviceSubClass     %u\r\n", desc->bDeviceSubClass);
	Serial0.printf("  bDeviceProtocol     %u\r\n", desc->bDeviceProtocol);
	Serial0.printf("  bMaxPacketSize0     %u\r\n", desc->bMaxPacketSize0);
	Serial0.printf("  idVendor            0x%04x\r\n", desc->idVendor);
	Serial0.printf("  idProduct           0x%04x\r\n", desc->idProduct);
	Serial0.printf("  bcdDevice           %04x\r\n", desc->bcdDevice);

	// Get String descriptor using Sync API
	Serial0.printf("  iManufacturer       %u     ", desc->iManufacturer);
	if (XFER_RESULT_SUCCESS ==
		 tuh_descriptor_get_manufacturer_string_sync(daddr, LANGUAGE_ID, dev->manufacturer, sizeof(dev->manufacturer)))
	{
		utf16_to_utf8(dev->manufacturer, sizeof(dev->manufacturer));
		Serial0.printf((char *)dev->manufacturer);
	}
	Serial0.printf("\r\n");

	Serial0.printf("  iProduct            %u     ", desc->iProduct);
	if (XFER_RESULT_SUCCESS ==
		 tuh_descriptor_get_product_string_sync(daddr, LANGUAGE_ID, dev->product, sizeof(dev->product)))
	{
		utf16_to_utf8(dev->product, sizeof(dev->product));
		Serial0.printf((char *)dev->product);
	}
	Serial0.printf("\r\n");

	Serial0.printf("  iSerialNumber       %u     ", desc->iSerialNumber);
	if (XFER_RESULT_SUCCESS ==
		 tuh_descriptor_get_serial_string_sync(daddr, LANGUAGE_ID, dev->serial, sizeof(dev->serial)))
	{
		utf16_to_utf8(dev->serial, sizeof(dev->serial));
		Serial0.printf((char *)dev->serial);
	}
	Serial0.printf("\r\n");

	Serial0.printf("  bNumConfigurations  %u\r\n", desc->bNumConfigurations);

	// print device summary
	print_lsusb();
}


//---------------------- Private Functions ----------------------//

//---------------------- Tiny USB Callbacks ----------------------//
// Invoked when device is mounted (configured)
// This is the generic device mount callback. Class specific callbacks are used separately
void tuh_mount_cb(uint8_t daddr)
{
	Serial0.printf("Device attached, address = %d\r\n", daddr);

	dev_info_t *dev = &dev_info[0];
	tusb_desc_device_t *desc = &dev->desc_device;
	dev->mounted = true;

	// Get Device Descriptor
	tuh_descriptor_get_device_sync(daddr, &dev->desc_device, 18);

	uint16_t temp_buf[1024];
	if (XFER_RESULT_SUCCESS == tuh_descriptor_get_configuration_sync(daddr, 0, temp_buf, sizeof(temp_buf)))
	{
		usbh_PrintDeviceDescriptor(dev, daddr);
		parse_config_descriptor(daddr, (tusb_desc_configuration_t *)temp_buf);
	}

}

/// Invoked when device is unmounted (bus reset/unplugged)
void tuh_umount_cb(uint8_t daddr)
{
	Serial0.printf("Device removed, address = %d\r\n", daddr);
	dev_info_t *dev = &dev_info[daddr - 1];
	dev->mounted = false;

	// print device summary
	print_lsusb();
}

void print_lsusb(void)
{
	bool no_device = true;
	for (uint8_t daddr = 1; daddr < CFG_TUH_DEVICE_MAX + 1; daddr++)
	{
		// TODO can use tuh_mounted(daddr), but tinyusb has an bug
		// use local connected flag instead
		dev_info_t *dev = &dev_info[daddr - 1];
		if (dev->mounted)
		{
			Serial0.printf("Device %u: ID %04x:%04x %s %s\r\n", daddr,
								dev->desc_device.idVendor, dev->desc_device.idProduct,
								(char *)dev->manufacturer, (char *)dev->product);
			no_device = false;
		}
	}

	if (no_device)
	{
		Serial0.println("No device connected (except hub)");
	}
}

uint16_t count_interface_total_len(tusb_desc_interface_t const *desc_itf, uint8_t itf_count, uint16_t max_len)
{
	uint8_t const *p_desc = (uint8_t const *)desc_itf;
	uint16_t len = 0;

	while (itf_count--)
	{
		// Next on interface desc
		len += tu_desc_len(desc_itf);
		p_desc = tu_desc_next(p_desc);

		while (len < max_len)
		{
			// return on IAD regardless of itf count
			if (tu_desc_type(p_desc) == TUSB_DESC_INTERFACE_ASSOCIATION)
				return len;

			if ((tu_desc_type(p_desc) == TUSB_DESC_INTERFACE) &&
				 ((tusb_desc_interface_t const *)p_desc)->bAlternateSetting == 0)
			{
				break;
			}

			len += tu_desc_len(p_desc);
			p_desc = tu_desc_next(p_desc);
		}
	}

	return len;
}

// simple configuration parser to open
void parse_config_descriptor(uint8_t dev_addr, tusb_desc_configuration_t const *desc_cfg)
{
	uint8_t const *desc_end = ((uint8_t const *)desc_cfg) + tu_le16toh(desc_cfg->wTotalLength);
	uint8_t const *p_desc = tu_desc_next(desc_cfg);

	// parse each interfaces
	while (p_desc < desc_end)
	{
		uint8_t assoc_itf_count = 1;

		// Class will always starts with Interface Association (if any) and then Interface descriptor
		if (TUSB_DESC_INTERFACE_ASSOCIATION == tu_desc_type(p_desc))
		{
			tusb_desc_interface_assoc_t const *desc_iad = (tusb_desc_interface_assoc_t const *)p_desc;
			assoc_itf_count = desc_iad->bInterfaceCount;

			p_desc = tu_desc_next(p_desc); // next to Interface
		}

		// must be interface from now
		if (TUSB_DESC_INTERFACE != tu_desc_type(p_desc))
			return;
		tusb_desc_interface_t const *desc_itf = (tusb_desc_interface_t const *)p_desc;

		uint16_t const drv_len = count_interface_total_len(desc_itf, assoc_itf_count, (uint16_t)(desc_end - p_desc));

		// probably corrupted descriptor
		if (drv_len < sizeof(tusb_desc_interface_t))
			return;

		// Handle CDC devices
		if (desc_itf->bInterfaceClass == TUSB_CLASS_CDC)
		{
			cdc_DeviceConfiguredHandler();
			//cdcDeviceInitRequired = 1;
			//tonexOne_SendHello();
		}

		// next Interface or IAD descriptor
		p_desc += drv_len;
	}
}

#endif