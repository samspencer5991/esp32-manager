#include "tonexone.h"
#include "esp_log.h"
#include "Arduino.h"

static const char *TAG = "tonexOne";

#define FRAMING_BYTE 0x7E
#define MAX_RAW_DATA 4096
#define RX_BUFFER_SIZE 4096
#define PACKET_TIMEOUT 1000
#define MAX_PRESETS 20
#define TONEX_ONE_RESP_OFFSET_PRESET_NAME_LEN 32

typedef enum
{
	CommsStateIdle,
	CommsStateHello,
	CommsStateReady,
	CommsStateGetState
} CommsState;

typedef enum
{
	ParsingOk,
	ParsingInvalidFrame,
	ParsingInvalidEscapeSequence,
	ParsingCRCError
} ParsingStatus;

typedef enum
{
	SlotA = 0,
	SlotB = 1,
	SlotC = 2
} Slot;

typedef enum
{
	PacketUnknown,
	PacketStateUpdate,
	PacketHello
} PacketType;

typedef struct
{
	PacketType type;
	uint16_t size;
	uint16_t unknown;
} PacketHeader;

typedef struct
{
	uint8_t rawData[MAX_RAW_DATA];
	uint16_t length;
} PedalData;

typedef struct
{
	PacketHeader header;
	uint8_t slotAPreset;
	uint8_t slotBPreset;
	uint8_t slotCPreset;
	Slot currentSlot;
	PedalData pedalData;
} TonexMessage;

typedef struct
{
	TonexMessage message;
	uint8_t tonexState;
} TonexData;

typedef struct
{
	uint8_t command;
	uint32_t payload;
} USBMessage;

// Private Function Prototypes
esp_err_t tonexOne_RequestState(void);
esp_err_t tonexOne_SetActiveSlot(Slot newSlot);
uint16_t tonexOne_GetCurrentActivePreset(void);
static esp_err_t tonexOne_SetPresetInSlot(uint16_t preset, Slot newSlot, uint8_t selectSlot);

ParsingStatus tonexOne_ParsePacket(uint8_t *message, uint16_t inlength);
uint16_t tonexOne_ParseValue(uint8_t *message, uint8_t *index);
ParsingStatus tonexOne_ParseState(uint8_t *unframed, uint16_t length, uint16_t index);

uint16_t tonexOne_CalculateCRC(uint8_t *data, uint16_t length);
uint16_t tonexOne_addByteWithStuffing(uint8_t *output, uint8_t byte);
uint16_t tonexOne_AddFraming(uint8_t *input, uint16_t inlength, uint8_t *output);
ParsingStatus tonexOne_RemoveFraming(uint8_t *input, uint16_t inlength, uint8_t *output, uint16_t *outlength);

uint8_t framedBuffer[MAX_RAW_DATA];
uint8_t txBuffer[MAX_RAW_DATA];
uint8_t tonexOneRawRxData[RX_BUFFER_SIZE];
TonexData tonexData;
uint8_t bootInitNeeded = 0;
char presetName[TONEX_ONE_RESP_OFFSET_PRESET_NAME_LEN + 1];
volatile uint8_t newDataReception = 0;
uint16_t rxDataSize = 0;

// Response from Tonex One to a preset change is about 1202, 1352, 1361 bytes with details of the preset.
// preset name is proceeded by this byte sequence:
static const uint8_t presetByteMarker[] = {0xB9, 0x04, 0xB9, 0x02, 0xBC, 0x21};

//---------------------- Public Functions ----------------------//
void tonexOne_SendHello()
{
	static uint8_t framedBuffer[3072];
	uint16_t outlength;
	ESP_LOGI(TAG, "Sent: Hello Packet");

	// build message
	uint8_t request[] = {						// Header
								0xb9, 0x03,			// object with 3 elements
								0x00,					// message type
								0x82, 0x04, 0x00, // message size (not including the header)
								0x80, 0x0b, 0x01, // (not sure)
														// Body
								0xb9, 0x02,			// object with 2 elements
								0x02,
								0x0b};
	// add framing
	outlength = tonexOne_AddFraming(request, sizeof(request), framedBuffer);
	SerialHost.write(framedBuffer, sizeof(framedBuffer));
	tonexData.tonexState = CommsStateHello;
}

void tonexOne_HandleReceivedData(char *rxData, uint16_t len)
{
	static uint16_t receivedByteIndex = 0;
	// Check that the new reception will not cause a buffer overflow
	if (receivedByteIndex + len > RX_BUFFER_SIZE)
	{
		// Reset the buffer index and ignore the incoming data
		receivedByteIndex = 0;
		ESP_LOGE(TAG, "Buffer overflow on incoming CDC data: %d bytes overflow.",
					RX_BUFFER_SIZE - (receivedByteIndex + len));
	}
	// Copy the received data into the raw rx buffer
	memcpy(&tonexOneRawRxData[receivedByteIndex], rxData, len);
	receivedByteIndex += len;

	// Check if a complete message has been received yet
	if ((receivedByteIndex >= 2) && (tonexOneRawRxData[0] == FRAMING_BYTE) && (tonexOneRawRxData[receivedByteIndex - 1] == FRAMING_BYTE))
	{
		ESP_LOGE(TAG, "Got new packet of length: %d", receivedByteIndex);
		newDataReception = 1;
		rxDataSize = receivedByteIndex;
		receivedByteIndex = 0;
	}

	// If not, keep waiting for more data
	else
	{
		ESP_LOGW(TAG, "Missing start or end bytes. Len:%d Start:%X End:%X", len, rxData[0], rxData[len - 1]);
	}
}

void tonexOne_Process()
{
	void *tempPtr;
	USBMessage message;

	// Check if a complete message has been received yet
	if (newDataReception)
	{
		ParsingStatus status = tonexOne_ParsePacket((uint8_t *)tonexOneRawRxData, rxDataSize);
		if (status != ParsingOk)
		{
			ESP_LOGE(TAG, "Error parsing message: %d", (int)status);
		}
		else
		{
			// check what we got
			switch (tonexData.message.header.type)
			{
			case PacketStateUpdate:
			{
				uint16_t current_preset = tonexOne_GetCurrentActivePreset();

				ESP_LOGI(TAG, "Received State Update. Current slot: %d. Preset: %d", (int)tonexData.message.currentSlot, (int)current_preset);

				// check for presetByteMarker[] to get preset name
				tempPtr = memmem((void *)tonexOneRawRxData, rxDataSize, (void *)presetByteMarker, sizeof(presetByteMarker));
				if (tempPtr != NULL)
				{
					ESP_LOGI(TAG, "Got preset name");

					// grab name
					memcpy((void *)presetName, (void *)(tempPtr + sizeof(presetByteMarker)), TONEX_ONE_RESP_OFFSET_PRESET_NAME_LEN);
				}

				// make sure we are showing the correct preset as active
				// control_sync_preset_details(current_preset, presetName);

				tonexData.tonexState = CommsStateReady;

				// note here: after boot, the state doesn't contain the preset name
				// work around here is to request a change of preset A, but not to the currently active sloy.
				// this results in pedal sending the full status details including the preset name
				if (bootInitNeeded)
				{
					uint8_t tempPreset = tonexData.message.slotAPreset;

					if (tempPreset < (MAX_PRESETS - 1))
					{
						tempPreset++;
					}
					else
					{
						tempPreset--;
					}

					tonexOne_SetPresetInSlot(tempPreset, SlotA, 0);

					bootInitNeeded = 0;
				}
			}
			break;

			case PacketHello:
			{
				ESP_LOGI(TAG, "Received Hello");

				// get current state
				tonexOne_RequestState();
				tonexData.tonexState = CommsStateGetState;

				// flag that we need to do the boot init procedure
				bootInitNeeded = 1;
			}
			break;

			default:
			{
				ESP_LOGI(TAG, "Message unknown %d", (int)tonexData.message.header.type);
			}
			break;
			}
		}
		// Clear the flags and wait for a new packet reception
		rxDataSize = 0;
		newDataReception = 0;
	}
}

void tonexOne_SendGoToPreset(uint8_t presetNum)
{
	if (presetNum >= MAX_PRESETS)
		return;

	tonexOne_SetPresetInSlot(presetNum, SlotC, 1);
}

void tonexOne_SendNextPreset()
{
	tonexOne_SetPresetInSlot(tonexData.message.slotCPreset + 1, SlotC, 1);
}

void tonexOne_SendPreviousPreset()
{
	tonexOne_SetPresetInSlot(tonexData.message.slotCPreset - 1, SlotC, 1);
}

//---------------------- Private Functions ----------------------//
esp_err_t tonexOne_RequestState(void)
{
	uint16_t outlength;

	// build message
	uint8_t request[] = {0xb9, 0x03, 0x00, 0x82, 0x06, 0x00, 0x80, 0x0b, 0x03, 0xb9, 0x02, 0x81, 0x06, 0x03, 0x0b};

	// add framing
	outlength = tonexOne_AddFraming(request, sizeof(request), framedBuffer);

	// send it
	SerialHost.write(framedBuffer, sizeof(framedBuffer));
	ESP_LOGE(TAG, "Sent: Request State\n");
	return 0;
	// return usb_tonex_one_transmit(FramedBuffer, outlength);
}

esp_err_t tonexOne_SetActiveSlot(Slot newSlot)
{
	uint16_t framedLength;

	ESP_LOGI(TAG, "Setting slot %d", (int)newSlot);

	// Build message, length to 0 for now                    len LSB  len MSB
	uint8_t message[] = {0xb9, 0x03, 0x81, 0x06, 0x03, 0x82, 0, 0, 0x80, 0x0b, 0x03};

	// set length
	message[6] = tonexData.message.pedalData.length & 0xFF;
	message[7] = (tonexData.message.pedalData.length >> 8) & 0xFF;

	// save the slot
	tonexData.message.currentSlot = newSlot;

	// firmware v1.1.4: offset needed is 12
	// firmware v1.2.6: offset needed is 18
	// todo could do version check and support multiple versions
	uint8_t offset_from_end = 18;

	// modify the buffer with the new slot
	tonexData.message.pedalData.rawData[tonexData.message.pedalData.length - offset_from_end + 7] = (uint8_t)newSlot;

	// build total message
	memcpy((void *)txBuffer, (void *)message, sizeof(message));
	memcpy((void *)&txBuffer[sizeof(message)], (void *)tonexData.message.pedalData.rawData, tonexData.message.pedalData.length);

	// add framing
	framedLength = tonexOne_AddFraming(txBuffer, sizeof(message) + tonexData.message.pedalData.length, framedBuffer);

	// send it
	//ESP_LOGE(TAG, "Sent: Set Active Slot: %d\n", newSlot);
	SerialHost.write(framedBuffer, framedLength);
	return 0;
	// return usb_tonex_one_transmit(framedBuffer, framedLength);
}

uint16_t tonexOne_GetCurrentActivePreset(void)
{
	uint16_t result = 0;

	switch (tonexData.message.currentSlot)
	{
	case SlotA:
	{
		result = tonexData.message.slotAPreset;
	}
	break;

	case SlotB:
	{
		result = tonexData.message.slotBPreset;
	}
	break;

	case SlotC:
	default:
	{
		result = tonexData.message.slotCPreset;
	}
	break;
	}

	return result;
}

static esp_err_t tonexOne_SetPresetInSlot(uint16_t preset, Slot newSlot, uint8_t selectSlot)
{
	uint16_t framedLength;

	// firmware v1.1.4: offset needed is 12
	// firmware v1.2.6: offset needed is 18
	// todo could do version check and support multiple versions
	uint8_t offset_from_end = 18;

	ESP_LOGI(TAG, "Setting preset %d in slot %d", (int)preset, (int)newSlot);

	// Build message, length to 0 for now                    len LSB  len MSB
	uint8_t message[] = {0xb9, 0x03, 0x81, 0x06, 0x03, 0x82, 0, 0, 0x80, 0x0b, 0x03};

	// set length
	message[6] = tonexData.message.pedalData.length & 0xFF;
	message[7] = (tonexData.message.pedalData.length >> 8) & 0xFF;

	// force pedal to Stomp mode. 0 here = A/B mode, 1 = stomp mode
	tonexData.message.pedalData.rawData[14] = 1;

	// check if setting same preset twice will set bypass
	/*
	if (control_get_config_double_toggle())
	{
		 if (selectSlot && (TonexData.Message.CurrentSlot == newSlot) && (preset == usb_tonex_one_get_current_active_preset()))
		 {
			  // are we in bypass mode?
			  if (TonexData.Message.PedalData.RawData[TonexData.Message.PedalData.Length - offset_from_end + 6] == 1)
			  {
					ESP_LOGI(TAG, "Disabling bypass mode");

					// disable bypass mode
					TonexData.Message.PedalData.RawData[TonexData.Message.PedalData.Length - offset_from_end + 6] = 0;
			  }
			  else
			  {
					ESP_LOGI(TAG, "Enabling bypass mode");

					// enable bypass mode
					TonexData.Message.PedalData.RawData[TonexData.Message.PedalData.Length - offset_from_end + 6] = 1;
			  }
		 }
		 else
		 {
			  // new preset, disable bypass mode to be sure
			  TonexData.Message.PedalData.RawData[TonexData.Message.PedalData.Length - offset_from_end + 6] = 0;
		 }
	}
	*/

	tonexData.message.currentSlot = newSlot;

	// set the preset index into the slot position
	switch (newSlot)
	{
	case SlotA:
	{
		tonexData.message.pedalData.rawData[tonexData.message.pedalData.length - offset_from_end] = preset;
	}
	break;

	case SlotB:
	{
		tonexData.message.pedalData.rawData[tonexData.message.pedalData.length - offset_from_end + 2] = preset;
	}
	break;

	case SlotC:
	{
		tonexData.message.pedalData.rawData[tonexData.message.pedalData.length - offset_from_end + 4] = preset;
	}
	break;
	}

	if (selectSlot)
	{
		// modify the buffer with the new slot
		tonexData.message.pedalData.rawData[tonexData.message.pedalData.length - offset_from_end + 7] = (uint8_t)newSlot;
	}

	// ESP_LOGI(TAG, "State Data after changes");
	// ESP_LOG_BUFFER_HEXDUMP(TAG, TonexData.Message.PedalData.RawData, TonexData.Message.PedalData.Length, ESP_LOG_INFO);

	// build total message
	memcpy((void *)txBuffer, (void *)message, sizeof(message));
	memcpy((void *)&txBuffer[sizeof(message)], (void *)tonexData.message.pedalData.rawData, tonexData.message.pedalData.length);

	// do framing
	framedLength = tonexOne_AddFraming(txBuffer, sizeof(message) + tonexData.message.pedalData.length, framedBuffer);

	// send it
	SerialHost.write(framedBuffer, sizeof(framedBuffer));
	return 1;
	// return usb_tonex_one_transmit(framedBuffer, framedLength);
}

ParsingStatus tonexOne_ParsePacket(uint8_t *message, uint16_t inlength)
{
	uint16_t out_len = 0;

	ParsingStatus status = tonexOne_RemoveFraming(message, inlength, framedBuffer, &out_len);

	Serial0.print("De-framed message: ");
	for(uint16_t i=0; i<out_len; i++)
	{
		Serial0.print(framedBuffer[i], HEX);
		Serial0.print(" ");
	}
	Serial0.println();

	if (status != ParsingOk)
	{
		ESP_LOGE(TAG, "Remove framing failed");
		return ParsingInvalidFrame;
	}

	if (out_len < 5)
	{
		ESP_LOGE(TAG, "Message too short");
		return ParsingInvalidFrame;
	}

	if ((framedBuffer[0] != 0xB9) || (framedBuffer[1] != 0x03))
	{
		ESP_LOGE(TAG, "Invalid header");
		return ParsingInvalidFrame;
	}

	PacketHeader header;
	uint8_t index = 2;
	uint16_t type = tonexOne_ParseValue(framedBuffer, &index);

	switch (type)
	{
	case 0x0306:
	{
		header.type = PacketStateUpdate;
	}
	break;

	case 0x02:
	{
		header.type = PacketHello;
	}
	break;

	default:
	{
		ESP_LOGI(TAG, "Unknown type %d", (int)type);
		header.type = PacketUnknown;
	}
	break;
	};

	header.size = tonexOne_ParseValue(framedBuffer, &index);
	header.unknown = tonexOne_ParseValue(framedBuffer, &index);

	// ESP_LOGI(TAG, "Structure ID: %d", header.type);
	// ESP_LOGI(TAG, "Size: %d", header.size);

	if ((out_len - index) != header.size)
	{
		ESP_LOGE(TAG, "Invalid message size");
		return ParsingInvalidFrame;
	}

	// check message type
	switch (header.type)
	{
	case PacketHello:
	{
		ESP_LOGI(TAG, "Hello response");
		memcpy((void *)&tonexData.message.header, (void *)&header, sizeof(header));
		return ParsingOk;
	}

	case PacketStateUpdate:
	{
		return tonexOne_ParseState(framedBuffer, out_len, index);
	}

	default:
	{
		ESP_LOGI(TAG, "Unknown structure. Skipping.");
		memcpy((void *)&tonexData.message.header, (void *)&header, sizeof(header));
		return ParsingOk;
	}
	};
}

uint16_t tonexOne_ParseValue(uint8_t *message, uint8_t *index)
{
	uint16_t value = 0;

	if (message[*index] == 0x81 || message[*index] == 0x82)
	{
		value = (message[(*index) + 2] << 8) | message[(*index) + 1];
		(*index) += 3;
	}
	else if (message[*index] == 0x80)
	{
		value = message[(*index) + 1];
		(*index) += 2;
	}
	else
	{
		value = message[*index];
		(*index)++;
	}

	return value;
}

ParsingStatus tonexOne_ParseState(uint8_t *unframed, uint16_t length, uint16_t index)
{
	tonexData.message.header.type = PacketStateUpdate;

	tonexData.message.pedalData.length = length - index;
	memcpy((void *)tonexData.message.pedalData.rawData, (void *)&unframed[index], tonexData.message.pedalData.length);

	// firmware v1.1.4: offset needed is 12
	// firmware v1.2.6: offset needed is 18
	// todo could do version check and support multiple versions
	uint8_t offset_from_end = 18;

	index += (tonexData.message.pedalData.length - offset_from_end);
	tonexData.message.slotAPreset = (Slot)unframed[index];
	index += 2;
	tonexData.message.slotBPreset = (Slot)unframed[index];
	index += 2;
	tonexData.message.slotCPreset = (Slot)unframed[index];
	index += 3;
	tonexData.message.currentSlot = (Slot)unframed[index];

	ESP_LOGI(TAG, "Slot A: %d. Slot B:%d. Slot C:%d. Current slot: %d", (int)tonexData.message.slotAPreset, (int)tonexData.message.slotBPreset, (int)tonexData.message.slotCPreset, (int)tonexData.message.currentSlot);

	// ESP_LOGI(TAG, "State Data Rx: %d %d", (int)length, (int)index);
	// ESP_LOG_BUFFER_HEXDUMP(TAG, tonexData.message.PedalData.RawData, tonexData.message.PedalData.Length, ESP_LOG_INFO);

	return ParsingOk;
}

uint16_t tonexOne_CalculateCRC(uint8_t *data, uint16_t length)
{
	uint16_t crc = 0xFFFF;

	for (uint16_t loop = 0; loop < length; loop++)
	{
		crc ^= data[loop];

		for (uint8_t i = 0; i < 8; ++i)
		{
			if (crc & 1)
			{
				crc = (crc >> 1) ^ 0x8408; // 0x8408 is the reversed polynomial x^16 + x^12 + x^5 + 1
			}
			else
			{
				crc = crc >> 1;
			}
		}
	}

	return ~crc;
}

uint16_t tonexOne_addByteWithStuffing(uint8_t *output, uint8_t byte)
{
	uint16_t length = 0;

	if (byte == FRAMING_BYTE || byte == 0x7D)
	{
		output[length] = 0x7D;
		length++;
		output[length] = byte ^ 0x20;
		length++;
	}
	else
	{
		output[length] = byte;
		length++;
	}

	return length;
}

// Adds the opening '7E' element, along with terminating and CRC framing elements
uint16_t tonexOne_AddFraming(uint8_t *input, uint16_t inlength, uint8_t *output)
{
	uint16_t outlength = 0;

	// Start flag
	output[outlength] = FRAMING_BYTE;
	outlength++;

	// add input bytes
	for (uint16_t byte = 0; byte < inlength; byte++)
	{
		outlength += tonexOne_addByteWithStuffing(&output[outlength], input[byte]);
	}

	// add CRC
	uint16_t crc = tonexOne_CalculateCRC(input, inlength);
	outlength += tonexOne_addByteWithStuffing(&output[outlength], crc & 0xFF);
	outlength += tonexOne_addByteWithStuffing(&output[outlength], (crc >> 8) & 0xFF);

	// End flag
	output[outlength] = FRAMING_BYTE;
	outlength++;

	return outlength;
}

ParsingStatus tonexOne_RemoveFraming(uint8_t *input, uint16_t inlength, uint8_t *output, uint16_t *outlength)
{
	*outlength = 0;
	uint8_t *output_ptr = output;

	if ((inlength < 4) || (input[0] != 0x7E) || (input[inlength - 1] != 0x7E))
	{
		ESP_LOGE(TAG, "Invalid Frame (1)");
		return ParsingInvalidFrame;
	}

	for (uint16_t i = 1; i < inlength - 1; ++i)
	{
		if (input[i] == 0x7D)
		{
			if ((i + 1) >= (inlength - 1))
			{
				ESP_LOGE(TAG, "Invalid Escape sequence");
				return ParsingInvalidEscapeSequence;
			}

			*output_ptr = input[i + 1] ^ 0x20;
			output_ptr++;
			(*outlength)++;
			++i;
		}
		else if (input[i] == 0x7E)
		{
			break;
		}
		else
		{
			*output_ptr = input[i];
			output_ptr++;
			(*outlength)++;
		}
	}

	if (*outlength < 2)
	{
		ESP_LOGE(TAG, "Invalid Frame (2)");
		return ParsingInvalidFrame;
	}

	// ESP_LOGI(TAG, "In:");
	// ESP_LOG_BUFFER_HEXDUMP(TAG, input, inlength, ESP_LOG_INFO);
	// ESP_LOGI(TAG, "Out:");
	// ESP_LOG_BUFFER_HEXDUMP(TAG, output, *outlength, ESP_LOG_INFO);

	uint16_t received_crc = (output[(*outlength) - 1] << 8) | output[(*outlength) - 2];
	(*outlength) -= 2;

	uint16_t calculated_crc = tonexOne_CalculateCRC(output, *outlength);

	if (received_crc != calculated_crc)
	{
		ESP_LOGE(TAG, "Crc mismatch: %X, %X", (int)received_crc, (int)calculated_crc);
		return ParsingCRCError;
	}

	return ParsingOk;
}