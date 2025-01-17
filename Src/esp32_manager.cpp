#include "esp32_manager.h"

#include "midi_handling.h"
#include "wifi_management.h"

#ifdef USE_EXTERNAL_USB_HOST
#include "usb_host.h"
#include "usb_cdc_handling.h"
#endif

// Initialise all included components
void esp32Manager_Init()
{
	midi_Init();

	if(esp32ConfigPtr->wirelessType == Esp32WiFi)
	{
		wifi_Connect(WIFI_HOSTNAME, WIFI_AP_SSID, NULL);
		midi_InitWiFiRTP();
	}

	// Initialise USB host components
#ifdef USE_EXTERNAL_USB_HOST
	usbh_Init();
	cdc_Init();
#endif


}

void esp32Manager_Process()
{
	usbh_Process();
	cdc_Process();

	// Process MIDI events and handling
	midi_ReadAll();
}