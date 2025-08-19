#ifdef USE_TONEX_ONE
#include "tonexOne_Interface.h"
#include "tonexOne.h"
#include "esp_log.h"

static QueueHandle_t usb_input_queue;

const char *TONEX_INTERFACE_TAG = "TonexOne Interface";

void tonexOne_InterfaceInit()
{
	// create queue for commands from other threads
	usb_input_queue = xQueueCreate(10, sizeof(TonexOneMessage));
	if (usb_input_queue == NULL)
	{
		ESP_LOGE(TONEX_INTERFACE_TAG, "Failed to create usb input queue!");
	}
	tonexOne_InitQueue(usb_input_queue);
}

void tonexOne_SendGoToPreset(uint8_t presetNum)
{
}

void tonexOne_SendNextPreset()
{
	TonexOneMessage message;
	/*
	if (usb_input_queue == NULL)
	{
		ESP_LOGE(TONEX_INTERFACE_TAG, "usb_next_preset queue null");
	}
	else
	{
		message.command = tonexOneNextPresetCommand;

		// send to queue
		if (xQueueSend(usb_input_queue, (void *)&message, 0) != pdPASS)
		{
			ESP_LOGE(TONEX_INTERFACE_TAG, "usb_next_preset queue send failed!");
		}
		else
		{
			ESP_LOGI(TONEX_INTERFACE_TAG, "usb_next_preset queue send success!");
		}
	}

	*/
	tonexOne_NextPreset();
}

void tonexOne_SendPreviousPreset()
{
	TonexOneMessage message;
	/*
	if (usb_input_queue == NULL)
	{
		ESP_LOGE(TONEX_INTERFACE_TAG, "usb_previous_preset queue null");
	}
	else
	{
		message.command = tonexOnePreviousPresetCommand;

		// send to queue
		if (xQueueSend(usb_input_queue, (void *)&message, 0) != pdPASS)
		{
			ESP_LOGE(TONEX_INTERFACE_TAG, "usb_previous_preset queue send failed!");
		}
		else
		{
			ESP_LOGI(TONEX_INTERFACE_TAG, "usb_previous_preset queue send success!");
		}
	}
		*/
	tonexOne_PreviousPreset();
}

void tonexOne_SendParameter(uint16_t parameter, float value)
{
	TonexOneMessage message;

	if (usb_input_queue == NULL)
	{
		ESP_LOGE(TONEX_INTERFACE_TAG, "usb queue null");
	}
	else
	{
		message.command = tonexOneModifyParameterCommand;
		message.payload = parameter;
		message.payloadFloat = value;

		// send to queue
		if (xQueueSend(usb_input_queue, (void *)&message, 0) != pdPASS)
		{
			ESP_LOGE(TONEX_INTERFACE_TAG, "Send parameter command to queue failed!");
		}
		else
		{
			ESP_LOGI(TONEX_INTERFACE_TAG, "Send parameter command to queue success!");
		}
	}
}
#endif
