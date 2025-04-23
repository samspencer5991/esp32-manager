#ifdef USE_EXTERNAL_USB_HOST

#include "tonexOne.h"
#include "esp_log.h"
#include "tonexOne_Parameters.h"

static const char *TAG = "tonexOne";

#define FRAMING_BYTE 0x7E

// Data buffer definitions
#define RX_TEMP_BUFFER_SIZE                         8192
#define MAX_INPUT_BUFFERS                           2
#define USB_TX_BUFFER_SIZE                          8192 

#define MAX_SHORT_PRESET_DATA                       3072
#define MAX_FULL_PRESET_DATA                        RX_TEMP_BUFFER_SIZE
#define MAX_STATE_DATA                              512

#define TONEX_ONE_RESP_OFFSET_PRESET_NAME_LEN 32

#define TONEX_STATE_OFFSET_START_INPUT_TRIM     15          // 0x000070c1 (-15.0) -> 0x000058c1 (0) -> 0x00007041 (15.0) 
#define TONEX_STATE_OFFSET_START_STOMP_MODE     19          // 0x00 - off, 0x01 - on
#define TONEX_STATE_OFFSET_START_CAB_BYPASS     20          // 0x00 - off, 0x01 - on
#define TONEX_STATE_OFFSET_START_TUNING_MODE    21          // 0x00 - mute, 0x01 - through

#define TONEX_STATE_OFFSET_END_BPM              4          
#define TONEX_STATE_OFFSET_END_TEMPO_SOURCE     6           // 00 - GLOBAL, 01 - PRESET 
#define TONEX_STATE_OFFSET_END_DIRECT_MONITOR   7           // 0x00 - off, 0x01 - on 
#define TONEX_STATE_OFFSET_END_TUNING_REF       9           
#define TONEX_STATE_OFFSET_END_CURRENT_SLOT     11           
#define TONEX_STATE_OFFSET_END_BYPASS_MODE      12
#define TONEX_STATE_OFFSET_END_SLOT_C_PRESET    14
#define TONEX_STATE_OFFSET_END_SLOT_B_PRESET    16
#define TONEX_STATE_OFFSET_END_SLOT_A_PRESET    18

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
	PacketHello,
	PacketStatePresetDetails,
	PacketStatePresetDetailsFull,
	PacketStateParamChanged
} PacketType;

typedef struct
{
	PacketType type;
	uint16_t size;
	uint16_t unknown;
} PacketHeader;

typedef struct
{
	// storage for current pedal state data
	uint8_t stateData[MAX_STATE_DATA]; 
	uint16_t stateDataLength;

	// storage for current preset details data (short version)
	uint8_t presetData[MAX_SHORT_PRESET_DATA];
	uint16_t presetDataLength;
	uint16_t presetParameterStartOffset;
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

typedef struct
{
    uint8_t data[RX_TEMP_BUFFER_SIZE];
    uint16_t length;
    uint8_t readyToRead : 1;
    uint8_t readyToWrite : 1;
} InputBufferEntry;


// Private Function Prototypes
esp_err_t tonexOne_RequestState(void);
esp_err_t tonexOne_SetActiveSlot(Slot newSlot);
uint16_t tonexOne_GetCurrentActivePreset(void);
esp_err_t tonexOne_SetPresetInSlot(uint16_t preset, Slot newSlot, uint8_t selectSlot);

ParsingStatus tonexOne_ParsePacket(uint8_t *message, uint16_t inlength);
uint16_t tonexOne_ParseValue(uint8_t *message, uint8_t *index);
ParsingStatus tonexOne_ParseState(uint8_t *unframed, uint16_t length, uint16_t index);

uint16_t tonexOne_CalculateCRC(uint8_t *data, uint16_t length);
uint16_t tonexOne_AddByteWithStuffing(uint8_t *output, uint8_t byte);
uint16_t tonexOne_AddFraming(uint8_t *input, uint16_t inlength, uint8_t *output);
ParsingStatus tonexOne_RemoveFraming(uint8_t *input, uint16_t inlength, uint8_t *output, uint16_t *outlength);

esp_err_t tonexOne_ProcessSingleMessage(uint8_t* data, uint16_t length);
esp_err_t tonexOne_RequestPresetDetails(uint8_t preset_index, uint8_t full_details);
uint16_t tonexOne_LocateMessageEnd(uint8_t* data, uint16_t length);
void tonexOne_ParsePresetParameters(uint8_t* raw_data, uint16_t length);
ParsingStatus tonexOne_ParsePresetDetails(uint8_t* unframed, uint16_t length, uint16_t index);

esp_err_t tonexOne_ModifyParameter(uint16_t index, float value);
esp_err_t tonexOne_ModifyGlobal(uint16_t global_val, float value);
esp_err_t tonexOne_SendSingleParameter(uint16_t index, float value);

//uint8_t framedBuffer[MAX_RAW_DATA];
//uint8_t txBuffer[MAX_RAW_DATA];
//uint8_t tonexOneRawRxData[RX_BUFFER_SIZE];
static volatile InputBufferEntry* inputBuffers;
static TonexData* tonexData;
static char preset_name[TONEX_ONE_RESP_OFFSET_PRESET_NAME_LEN + 1];
static uint8_t* txBuffer;
static uint8_t* framedBuffer;
static QueueHandle_t input_queue;

uint8_t bootInitNeeded = 1;
char presetName[TONEX_ONE_RESP_OFFSET_PRESET_NAME_LEN + 1];
volatile uint8_t newDataReception = 0;
uint16_t rxDataSize = 0;

// Response from Tonex One to a preset change is about 1202, 1352, 1361 bytes with details of the preset.
// preset name is proceeded by this byte sequence:
static const uint8_t presetByteMarker[] = {0xB9, 0x04, 0xB9, 0x02, 0xBC, 0x21};



//---------------------- Public Functions ----------------------//
void tonexOne_Init()
{
	// Initialise the paramter protection mutex
	tonexOne_Parameters_Init();
	// allocate RX buffers in PSRAM
	inputBuffers = (InputBufferEntry*)heap_caps_malloc(sizeof(InputBufferEntry) * MAX_INPUT_BUFFERS, MALLOC_CAP_SPIRAM);
	if (inputBuffers == NULL)
	{
		 ESP_LOGE(TAG, "Failed to allocate input buffers!");
		 return;
	}

	// set all buffers as ready for writing
	for (uint8_t loop = 0; loop < MAX_INPUT_BUFFERS; loop++)
	{
		 memset((void*)inputBuffers[loop].data, 0, RX_TEMP_BUFFER_SIZE);
		 inputBuffers[loop].readyToWrite = 1;
		 inputBuffers[loop].readyToRead = 0;
		 inputBuffers[loop].length = 0;
	}

	// more big buffers in PSRAM
	txBuffer = (uint8_t*)heap_caps_malloc(RX_TEMP_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
	if (txBuffer == NULL)
	{
		 ESP_LOGE(TAG, "Failed to allocate txBuffer buffer!");
		 return;
	}
	
	framedBuffer = (uint8_t*)heap_caps_malloc(RX_TEMP_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
	if (framedBuffer == NULL)
	{
		 ESP_LOGE(TAG, "Failed to allocate framedBuffer buffer!");
		 return;
	}

	tonexData = (TonexData*)heap_caps_malloc(sizeof(TonexData), MALLOC_CAP_SPIRAM);
	if (tonexData == NULL)
	{
		 ESP_LOGE(TAG, "Failed to allocate TonexData buffer!");
		 return;
	}

	memset((void*)tonexData, 0, sizeof(tonexData));
	tonexData->tonexState = CommsStateIdle;
	ESP_LOGE(TAG, "Tonex One buffers allocated in PSRAM: %d", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void tonexOne_InitQueue(QueueHandle_t comms_queue)
{
	input_queue = comms_queue;
}

void tonexOne_SendHello()
{
	if(!cdcDeviceMounted || cdcDeviceType != CDCDeviceTonexOne)
	{
		ESP_LOGE(TAG, "No Tonex One device mounted.");
		return;
	}
	static uint8_t framedBuffer[3072];
	uint16_t outLength;
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
	outLength = tonexOne_AddFraming(request, sizeof(request), framedBuffer);

	SerialHost.write(framedBuffer, outLength);
	SerialHost.flush();
	tonexData->tonexState = CommsStateHello;
}

uint8_t tonexOne_HandleReceivedData(char *rxData, uint16_t len)
{
	// Check that the new reception will not cause a buffer overflow
	if (len > RX_TEMP_BUFFER_SIZE)
	{
		// Ignore the incoming data
		ESP_LOGD(TAG, "Buffer overflow on incoming CDC data: %d bytes overflow.", (len) - RX_TEMP_BUFFER_SIZE);
		return 0;
	}
	else
	{
		// locate a buffer to put it
		for (uint8_t loop = 0; loop < MAX_INPUT_BUFFERS; loop++)
		{
			if (inputBuffers[loop].readyToWrite == 1)
			{
				// Copy the received data into the raw rx buffer
				memcpy((void*)&inputBuffers[loop].data[inputBuffers[loop].length], (void*)rxData, len);

				// set buffer as used
				inputBuffers[loop].length += len;
				
				// Check if a complete message has been received yet
				if ((inputBuffers[loop].length >= 2) && (inputBuffers[loop].data[0] == FRAMING_BYTE) && (inputBuffers[loop].data[inputBuffers[loop].length - 1] == FRAMING_BYTE))
				{
					//ESP_LOGD(TAG, "Got new packet of length: %d", inputBuffers[loop].length);
					inputBuffers[loop].readyToWrite = 0;
					inputBuffers[loop].readyToRead = 1;   
				}

				// If not, keep waiting for more data
				else
				{
					//ESP_LOGD(TAG, "Missing start or end bytes. Len:%d Start:%X End:%X", len, rxData[0], rxData[len - 1]);
				}
				// debug
				//ESP_LOGI(TAG, "CDC Data buffered into %d", (int)loop);
				break;
			}
		}
	}
	//ESP_LOGE(TAG, "usb_tonex_one_handle_rx no available buffers!");

	for (uint8_t loop = 0; loop < MAX_INPUT_BUFFERS; loop++)
    {
        if (inputBuffers[loop].readyToRead)
        {
				ESP_LOGI(TAG, "Got data via CDC %d", inputBuffers[loop].length);
            // debug
            //ESP_LOG_BUFFER_HEXDUMP(TAG, inputBuffers[loop].Data, inputBuffers[loop].Length, ESP_LOG_INFO);

            uint16_t end_marker_pos;
            uint16_t bytes_consumed = 0;
            uint8_t* rx_entry_ptr = (uint8_t*)inputBuffers[loop].data;
            uint16_t rx_entry_length = inputBuffers[loop].length;

            // process all messages received (may be multiple messages appended)
            do
            {    
                // locate the end of the message
                end_marker_pos = tonexOne_LocateMessageEnd(rx_entry_ptr, rx_entry_length);

                if (end_marker_pos == 0)
                {
                    //ESP_LOGW(TAG, "Missing end marker!");
                    //ESP_LOG_BUFFER_HEXDUMP(TAG, rx_entry_ptr, rx_entry_length, ESP_LOG_INFO);
                    break;
                }
                else
                {
                    //ESP_LOGI(TAG, "Found end marker: %d", end_marker_pos);
                }

                // debug
                //ESP_LOG_BUFFER_HEXDUMP(TAG, rx_entry_ptr, end_marker_pos + 1, ESP_LOG_INFO);

                // process it
                if (tonexOne_ProcessSingleMessage(rx_entry_ptr, end_marker_pos + 1) != ESP_OK)
                {
                    break;    
                }
            
                // skip this message
                rx_entry_ptr += (end_marker_pos + 1);
                bytes_consumed += (end_marker_pos + 1);

                //ESP_LOGI(TAG, "After message, pos %d cons %d len %d", (int)end_marker_pos, (int)bytes_consumed, (int)rx_entry_length);
            } while (bytes_consumed < rx_entry_length);

            // set buffer as available       
            inputBuffers[loop].readyToRead = 0;
            inputBuffers[loop].readyToWrite = 1;   
				inputBuffers[loop].length = 0; // reset length

            vTaskDelay(2 / portTICK_PERIOD_MS); // give time to process the data
        } 
    }
	 return 0;
}

void tonexOne_Process()
{
	TonexOneMessage message;

	switch (tonexData->tonexState)
	{
		case CommsStateReady:
		{
				// check for any input messages
				if (xQueueReceive(input_queue, (void*)&message, 0) == pdPASS)
				{
					ESP_LOGI(TAG, "Got Input message: %d", message.command);

					// process it
					switch (message.command)
					{
						case tonexOneSetPresetCommand:
						{
								if (message.payload < MAX_TONEX_ONE_PRESETS)
								{
									// always using Stomp mode C for preset setting
									if (tonexOne_SetPresetInSlot(message.payload, SlotC, 1) != ESP_OK)
									{
										// failed return to queue?
									}
								}
						} break;   

						case tonexOneNextPresetCommand:
						{
							uint8_t nextPreset = 0;
							if (tonexData->message.slotCPreset < (MAX_TONEX_ONE_PRESETS - 1))
								nextPreset = tonexData->message.slotCPreset + 1;

							else
								nextPreset = 0;

							// always using Stomp mode C for preset setting
							if (tonexOne_SetPresetInSlot(nextPreset, SlotC, 1) != ESP_OK)
							{
								ESP_LOGE(TAG, "Failed to set next preset!");
							}
							else
							{
								ESP_LOGI(TAG, "Set next preset successfully!");
							}
						} break;

						case tonexOnePreviousPresetCommand:
						{
							uint8_t previousPreset = 0;
							if (tonexData->message.slotCPreset > 0)
								tonexData->message.slotCPreset - 1;
							else
								previousPreset = MAX_TONEX_ONE_PRESETS - 1;

							// always using Stomp mode C for preset setting
							if (tonexOne_SetPresetInSlot(tonexData->message.slotCPreset - 1, SlotC, 1) != ESP_OK)
							{
								ESP_LOGE(TAG, "Failed to set previous preset!");
							}
							else
							{
								ESP_LOGI(TAG, "Set previous preset successfully!");
							}
						} break;

						case tonexOneModifyParameterCommand:
						{
								if (message.payload < TONEX_PARAM_LAST)
								{
									// modify the param
									tonexOne_ModifyParameter(message.payload, message.payloadFloat);

									// send it
									tonexOne_SendSingleParameter(message.payload, message.payloadFloat);
								}
								else if (message.payload < TONEX_GLOBAL_LAST)
								{
									// modify the global
									tonexOne_ModifyGlobal(message.payload, message.payloadFloat);

									// debug
									//usb_tonex_one_dump_state(&TonexData->Message.PedalData.TonexStateData);

									// send it by setting the same preset active again, which sends the state data
									tonexOne_SetPresetInSlot(tonexData->message.slotCPreset, SlotC, 1);
								}
								else
								{
									ESP_LOGW(TAG, "Attempt to modify unknown param %d", (int)message.payload);
								}
						} break;
					}
				}
		} break;
		case CommsStateIdle:
		{
			tonexOne_SendHello();
		}

		default:
		{
			ESP_LOGI(TAG, "Unknown State: %d", (tonexData->tonexState));
		} break;
	}
}


//---------------------- Private Functions ----------------------//
esp_err_t tonexOne_ProcessSingleMessage(uint8_t* data, uint16_t length)
{
	ESP_LOGD(TAG, "Processing message of length %d", (int)length);
    void* temp_ptr;  
    uint16_t current_preset;

    // check if we got a complete message(s)
    if ((length >= 2) && (data[0] == 0x7E) && (data[length - 1] == 0x7E))
    {
        ESP_LOGI(TAG, "Processing messages len: %d", (int)length);
        ParsingStatus status = tonexOne_ParsePacket(data, length);

        if (status != ParsingOk)
        {
            ESP_LOGE(TAG, "Error parsing message: %d", (int)status);
        }
        else
        {
            ESP_LOGI(TAG, "Message Header type: %d", (int)tonexData->message.header.type);

            // check what we got
            switch (tonexData->message.header.type)
            {
                case PacketStateUpdate:
                {
                    current_preset = tonexOne_GetCurrentActivePreset();
                    ESP_LOGI(TAG, "Received State Update. Current slot: %d. Preset: %d", (int)tonexData->message.currentSlot, (int)current_preset);
                    
                    // debug
                    //ESP_LOG_BUFFER_HEXDUMP(TAG, data, length, ESP_LOG_INFO);

                    tonexData->tonexState = CommsStateReady;   

                    if (bootInitNeeded)
                    {
                        // request details of the current preset, so we can update UI
                        tonexOne_RequestPresetDetails(current_preset, 0);
                        bootInitNeeded = 0;
                    }
                    else
                    {
                        // signal to refresh param UI with Globals
                        //UI_RefreshParameterValues();

                        // update web UI
                        //wifi_request_sync(WIFI_SYNC_TYPE_PARAMS, NULL, NULL);
                    }
                } break;

                case PacketStatePresetDetails:
                {
                    // locate the ToneOnePresetByteMarker[] to get preset name
                    temp_ptr = memmem((void*)data, length, (void*)presetByteMarker, sizeof(presetByteMarker));
                    if (temp_ptr != NULL)
                    {
                        ESP_LOGI(TAG, "Got preset name");

                        // grab name
                        memcpy((void*)preset_name, (void*)(temp_ptr + sizeof(presetByteMarker)), TONEX_ONE_RESP_OFFSET_PRESET_NAME_LEN);                
                    }

                    current_preset = tonexOne_GetCurrentActivePreset();
                    ESP_LOGI(TAG, "Received State Update. Current slot: %d. Preset: %d", (int)tonexData->message.currentSlot, (int)current_preset);
                    
                    // make sure we are showing the correct preset as active                
                    //control_sync_preset_details(current_preset, preset_name);

                    // read the preset params
                    tonexOne_ParsePresetParameters(data, length);

                    // signal to refresh param UI
                    //UI_RefreshParameterValues();

                    // update web UI
                    //wifi_request_sync(WIFI_SYNC_TYPE_PARAMS, NULL, NULL);

                    // debug dump parameters
                    //tonex_dump_parameters();
                } break;

                case PacketHello:
                {
                    ESP_LOGI(TAG, "Received Hello");

                    // get current state
                    tonexOne_RequestState();
                    tonexData->tonexState = CommsStateGetState;

                    // flag that we need to do the boot init procedure
                    bootInitNeeded = 1;
                } break;

                case PacketStatePresetDetailsFull:
                {
                    // ignore
                    ESP_LOGI(TAG, "Received Preset details full");
                } break;

                default:
                {
                    ESP_LOGI(TAG, "Message unknown %d", (int)tonexData->message.header.type);
                } break;
            }
        }

        return ESP_OK;
    }
    else
    {
        ESP_LOGW(TAG, "Missing start or end bytes. %d, %d, %d", (int)length, (int)data[0], (int)data[length - 1]);
        //ESP_LOG_BUFFER_HEXDUMP(TAG, data, length, ESP_LOG_INFO);
        return ESP_FAIL;
    }
}

uint16_t tonexOne_LocateMessageEnd(uint8_t* data, uint16_t length)
{
    // locate the 0x7E end of packet marker
    // starting at index 1 to skip the start marker
    for (uint16_t loop = 1; loop < length; loop++)
    {
        if (data[loop] == 0x7E)
        {
            return loop;
        }
    }

    // not found
    return 0;
}

void tonexOne_DumpState()
{
	float InputTrim;
    float BPM;
    uint16_t TuningRef;

    memcpy((void*)&BPM, (void*)&tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_BPM], sizeof(float));
    memcpy((void*)&InputTrim, (void*)&tonexData->message.pedalData.stateData[TONEX_STATE_OFFSET_START_INPUT_TRIM], sizeof(float));    
    memcpy((void*)&TuningRef, (void*)&tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_TUNING_REF], sizeof(uint16_t));

    ESP_LOGI(TAG, "**** Tonex State Data ****");
    ESP_LOGI(TAG, "Input Trim: %3.2f.\t\tStomp Mode: %d", InputTrim, 
                                                          (int)tonexData->message.pedalData.stateData[TONEX_STATE_OFFSET_START_STOMP_MODE]);

    ESP_LOGI(TAG, "Cab Sim Bypass: %d.\t\tTuning Mode: %d", (int)tonexData->message.pedalData.stateData[TONEX_STATE_OFFSET_START_CAB_BYPASS], 
                                                            (int)tonexData->message.pedalData.stateData[TONEX_STATE_OFFSET_START_TUNING_MODE]);

    ESP_LOGI(TAG, "Slot A Preset: %d,\t\tSlot B Preset: %d", (int)tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_SLOT_A_PRESET], 
                                                             (int)tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_SLOT_B_PRESET]);

    ESP_LOGI(TAG, "Slot C Preset: %d.\t\tCurrent Slot: %d", (int)tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_SLOT_C_PRESET], 
                                                            (int)tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_CURRENT_SLOT]);

    ESP_LOGI(TAG, "Tuning Reference: %d.\t\tDirect Monitoring: %d", (int)TuningRef, 
                                                                    (int)tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_DIRECT_MONITOR]);

    ESP_LOGI(TAG, "BPM: %3.2f\t\t\tTempo Source: %d", BPM, (int)tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_TEMPO_SOURCE]);   
}

void tonexOne_ParsePresetParameters(uint8_t* raw_data, uint16_t length)
{
    uint8_t param_start_marker[] = {0xBA, 0x03, 0xBA, 0x6D}; 
    tTonexParameter* param_ptr = NULL;

    ESP_LOGI(TAG, "Parsing Preset parameters");

    // try to locate the start of the first parameter block 
	 const void* tempDataPtr = (void*)raw_data;
    uint8_t* temp_ptr = (uint8_t*)memmem(tempDataPtr, length, (void*)param_start_marker, sizeof(param_start_marker));
    if (temp_ptr != NULL)
    {
        // skip the start marker
        temp_ptr += sizeof(param_start_marker);

        // save the offset where the parameters start
        tonexData->message.pedalData.presetParameterStartOffset = temp_ptr - raw_data;
        ESP_LOGI(TAG, "Preset parameters offset: %d", (int)tonexData->message.pedalData.presetParameterStartOffset);

        if (tonex_params_get_locked_access(&param_ptr) == ESP_OK)
        {
            // params here are start marker of 0x88, followed by a 4-byte float
            for (uint32_t loop = 0; loop < TONEX_PARAM_LAST; loop++)
            {
                if (*temp_ptr == 0x88)
                {
                    // skip the marker
                    temp_ptr++;

                    // get the value
                    memcpy((void*)&param_ptr[loop].Value, (void*)temp_ptr, sizeof(float));

                    // skip the float
                    temp_ptr += sizeof(float);
                }
                else
                {
                    ESP_LOGW(TAG, "Unexpected value during Param parse: %d, %d", (int)loop, (int)*temp_ptr);  
                    break;
                }
            }

            tonex_params_release_locked_access();
        }

        ESP_LOGI(TAG, "Parsing Preset parameters complete");
    }
    else
    {
        ESP_LOGW(TAG, "Parsing Preset parameters failed to find start marker");
    }
}

ParsingStatus tonexOne_ParsePresetDetails(uint8_t* unframed, uint16_t length, uint16_t index)
{
    tonexData->message.header.type = PacketStatePresetDetails;

    tonexData->message.pedalData.presetDataLength = length - index;
    memcpy((void*)tonexData->message.pedalData.presetData, (void*)&unframed[index], tonexData->message.pedalData.presetDataLength);
    ESP_LOGI(TAG, "Saved Preset Details: %d", tonexData->message.pedalData.presetDataLength);

    // debug
    //ESP_LOGI(TAG, "Preset Data Rx: %d %d", (int)length, (int)index);
    //ESP_LOG_BUFFER_HEXDUMP(TAG, TonexData->Message.PedalData.PresetData, TonexData->Message.PedalData.PresetDataLength, ESP_LOG_INFO);

    return ParsingOk;
}

esp_err_t tonexOne_RequestState(void)
{
	if(!cdcDeviceMounted || cdcDeviceType != CDCDeviceTonexOne)
	{
		ESP_LOGE(TAG, "No Tonex One device mounted.");
		return ESP_FAIL;
	}
	uint16_t outLength;

	// build message
	uint8_t request[] = {0xb9, 0x03, 0x00, 0x82, 0x06, 0x00, 0x80, 0x0b, 0x03, 0xb9, 0x02, 0x81, 0x06, 0x03, 0x0b};

	// add framing
	outLength = tonexOne_AddFraming(request, sizeof(request), framedBuffer);

	// send it
	ESP_LOGD(TAG, "Sent: Request State\n");
	SerialHost.write(framedBuffer, outLength);
	SerialHost.flush();
	return 0;
}

esp_err_t tonexOne_SetActiveSlot(Slot newSlot)
{
	if(!cdcDeviceMounted || cdcDeviceType != CDCDeviceTonexOne)
	{
		ESP_LOGE(TAG, "No Tonex One device mounted.");
		return ESP_FAIL;
	}
	uint16_t framedLength;

	ESP_LOGI(TAG, "Setting slot %d", (int)newSlot);

	// Build message, length to 0 for now                    len LSB  len MSB
	uint8_t message[] = {0xb9, 0x03, 0x81, 0x06, 0x03, 0x82, 0, 0, 0x80, 0x0b, 0x03};

	// set length
	message[6] = tonexData->message.pedalData.stateDataLength & 0xFF;
	message[7] = (tonexData->message.pedalData.stateDataLength >> 8) & 0xFF;

	// save the slot
	tonexData->message.currentSlot = newSlot;

	// modify the buffer with the new slot
	tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_CURRENT_SLOT + 7] = (uint8_t)newSlot;

	// build total message
	memcpy((void *)txBuffer, (void *)message, sizeof(message));
	memcpy((void *)&txBuffer[sizeof(message)], (void *)tonexData->message.pedalData.stateData, tonexData->message.pedalData.stateDataLength);

	// add framing
	framedLength = tonexOne_AddFraming(txBuffer, sizeof(message) + tonexData->message.pedalData.stateDataLength, framedBuffer);

	// send it
	ESP_LOGD(TAG, "Sent: Set Active Slot: %d\n", newSlot);
	SerialHost.write(framedBuffer, framedLength);
	SerialHost.flush();
	return 0;
}

uint16_t tonexOne_GetCurrentActivePreset(void)
{
	uint16_t result = 0;

	switch (tonexData->message.currentSlot)
	{
	case SlotA:
	{
		result = tonexData->message.slotAPreset;
	}
	break;

	case SlotB:
	{
		result = tonexData->message.slotBPreset;
	}
	break;

	case SlotC:
	default:
	{
		result = tonexData->message.slotCPreset;
	}
	break;
	}

	return result;
}

esp_err_t tonexOne_SetPresetInSlot(uint16_t preset, Slot newSlot, uint8_t selectSlot)
{
	if(!cdcDeviceMounted || cdcDeviceType != CDCDeviceTonexOne)
	{
		ESP_LOGE(TAG, "No Tonex One device mounted.");
		return ESP_FAIL;
	}
	uint16_t framedLength;

	ESP_LOGI(TAG, "Setting preset %d in slot %d", (int)preset, (int)newSlot);

	// Build message, length to 0 for now                    len LSB  len MSB
	uint8_t message[] = {0xb9, 0x03, 0x81, 0x06, 0x03, 0x82, 0, 0, 0x80, 0x0b, 0x03};

	// set length
	message[6] = tonexData->message.pedalData.stateDataLength & 0xFF;
	message[7] = (tonexData->message.pedalData.stateDataLength >> 8) & 0xFF;

	// force pedal to Stomp mode. 0 here = A/B mode, 1 = stomp mode
	tonexData->message.pedalData.stateData[TONEX_STATE_OFFSET_START_STOMP_MODE] = 1;

	// make sure direct monitoring is on so sound not muted from USB connection
	tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_DIRECT_MONITOR] = 1;


	// check if setting same preset twice will set bypass
	if (0)
	{
		if (selectSlot && (tonexData->message.currentSlot == newSlot) && (preset == tonexOne_GetCurrentActivePreset()))
		{
			// are we in bypass mode?
			if (tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_BYPASS_MODE] == 1)
			{
					ESP_LOGI(TAG, "Disabling bypass mode");

					// disable bypass mode
					tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_BYPASS_MODE] = 0;
			}
			else
			{
					ESP_LOGI(TAG, "Enabling bypass mode");

					// enable bypass mode
					tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_BYPASS_MODE] = 1;
			}
		}
		else
		{
			// new preset, disable bypass mode to be sure
			tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_BYPASS_MODE] = 0;
		}
	}

	tonexData->message.currentSlot = newSlot;

	// set the preset index into the slot position
	switch (newSlot)
	{
	case SlotA:
	{
		tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_SLOT_A_PRESET] = preset;
	}
	break;

	case SlotB:
	{
		tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_SLOT_B_PRESET] = preset;
	}
	break;

	case SlotC:
	{
		tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_SLOT_C_PRESET] = preset;
	}
	break;
	}

	if (selectSlot)
	{
		// modify the buffer with the new slot
		tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_CURRENT_SLOT] = (uint8_t)newSlot;
	}

	// ESP_LOGI(TAG, "State Data after changes");
	// ESP_LOGD_BUFFER_HEXDUMP(TAG, tonexData->message.pedalData.RawData, tonexData->message.pedalData.Length, ESP_LOGD_INFO);

	// build total message
	memcpy((void *)txBuffer, (void *)message, sizeof(message));
	memcpy((void *)&txBuffer[sizeof(message)], (void *)tonexData->message.pedalData.stateData, tonexData->message.pedalData.stateDataLength);

	// do framing
	framedLength = tonexOne_AddFraming(txBuffer, sizeof(message) + tonexData->message.pedalData.stateDataLength, framedBuffer);

	// Before sending the packet, pad the packet with zeros to the nearest 64-byte boundary
	
	uint8_t padding = 64 - (framedLength % 64);

/*
	if (padding < 64)
	{
		framedBuffer[framedLength] = 0x7e; // add end marker to the end of the packet
		framedLength++;
		memset((void *)&framedBuffer[framedLength], 0x00, padding-2);
		framedLength += padding-2;
	}
	framedBuffer[framedLength] = 0x7e;
	framedLength++;
	*/

	size_t sentBytes = cdc_Transmit(framedBuffer, framedLength);
	//size_t sentBytes = SerialHost.write(framedBuffer, framedLength);

	//size_t sentBytes = SerialHost.write(framedBuffer, 64);
	//while(SerialHost.availableForWrite() < 64)
	{
		//vTaskDelay(1 / portTICK_PERIOD_MS);
	}
	//SerialHost.flush();
	//delay(5);
	
	//vTaskDelay(5 / portTICK_PERIOD_MS);
	//sentBytes = SerialHost.write(&framedBuffer[64], 64);
	//while(SerialHost.availableForWrite() < 64)
	{
		//vTaskDelay(1 / portTICK_PERIOD_MS);
	}
	//SerialHost.flush();
	//vTaskDelay(5 / portTICK_PERIOD_MS);
	//delay(5);
	//sentBytes = SerialHost.write(&framedBuffer[128], 46);
	//SerialHost.flush();
	ESP_LOGE(TAG, "Data Transmit Complete: %d (%d from driver)", (int)framedLength, sentBytes);

	return 0;
}

esp_err_t tonexOne_RequestPresetDetails(uint8_t preset_index, uint8_t full_details)
{
	esp_err_t res = ESP_FAIL;
	uint16_t outLength;

	ESP_LOGI(TAG, "Requesting full preset details for %d", (int)preset_index);

	// build message                                                                                               Preset Full    
	uint8_t request[] = {0xb9, 0x03, 0x81, 0x00, 0x03, 0x82, 0x06, 0x00, 0x80, 0x0b, 0x03, 0xb9, 0x04, 0x0b, 0x01, 0x00,  0x00};  
	request[15] = preset_index;
	request[16] = full_details;     // 0x00 = approx 2k byte summary. 0x01 = approx 30k byte full preset details

	// add framing
	outLength = tonexOne_AddFraming(request, sizeof(request), framedBuffer);

	// send it
	ESP_LOGD(TAG, "Sent: Request Full Preset Details\n");
	SerialHost.write(framedBuffer, outLength);
	SerialHost.flush();
	return res;
}

ParsingStatus tonexOne_ParsePacket(uint8_t *message, uint16_t inlength)
{
	uint16_t out_len = 0;

	ParsingStatus status = tonexOne_RemoveFraming(message, inlength, framedBuffer, &out_len);

	/*
	Serial0.print("De-framed message: ");
	for(uint16_t i=0; i<out_len; i++)
	{
		Serial0.print(framedBuffer[i], HEX);
		Serial0.print(" ");
	}
	Serial0.println();
	*/
	if (status != ParsingOk)
	{
		ESP_LOGD(TAG, "Remove framing failed");
		return ParsingInvalidFrame;
	}

	if (out_len < 5)
	{
		ESP_LOGD(TAG, "Message too short");
		return ParsingInvalidFrame;
	}

	if ((framedBuffer[0] != 0xB9) || (framedBuffer[1] != 0x03))
	{
		ESP_LOGD(TAG, "Invalid header");
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
	} break;

	case 0x0304:
	{
		header.type = PacketStatePresetDetails;
	} break;

	case 0x0303:
	{
		header.type = PacketStatePresetDetailsFull;
	} break;

	case 0x0309:
	{
		header.type = PacketStateParamChanged;
	} break;

	case 0x02:
	{
		header.type = PacketHello;
	} break;

	default:
	{
		ESP_LOGI(TAG, "Unknown type %d", (int)type);
		header.type = PacketUnknown;
	} break;
	};

	header.size = tonexOne_ParseValue(framedBuffer, &index);
	header.unknown = tonexOne_ParseValue(framedBuffer, &index);
	ESP_LOGI(TAG, "TonexOne Parse: type: %d size: %d", (int)header.type, (int)header.size);

	// ESP_LOGI(TAG, "Structure ID: %d", header.type);
	// ESP_LOGI(TAG, "Size: %d", header.size);

	if ((out_len - index) != header.size)
	{
		ESP_LOGD(TAG, "Invalid message size");
		return ParsingInvalidFrame;
	}

	// check message type
	switch (header.type)
	{
	case PacketHello:
	{
		ESP_LOGI(TAG, "Hello response");
		memcpy((void *)&tonexData->message.header, (void *)&header, sizeof(header));
		return ParsingOk;
	}

	case PacketStateUpdate:
	{
		return tonexOne_ParseState(framedBuffer, out_len, index);
	}

	case PacketStatePresetDetails:
	{
		return tonexOne_ParsePresetDetails(framedBuffer, out_len, index);
	}

	case PacketStatePresetDetailsFull:
	{

	} break;

	case PacketStateParamChanged:
	{
		ESP_LOGI(TAG, "Param change confirmation");
		return ParsingOk;
	}

	default:
	{
		ESP_LOGI(TAG, "Unknown structure. Skipping.");
		memcpy((void *)&tonexData->message.header, (void *)&header, sizeof(header));
		return ParsingOk;
	}
	};
}

esp_err_t tonexOne_SendSingleParameter(uint16_t index, float value)
{
    uint16_t framedLength;

    // NOTE: only supported in newer Pedal firmware that came with Editor support!

    // Build message                                         len LSB  len MSB
    uint8_t message[] = {0xb9, 0x03, 0x81, 0x09, 0x03, 0x82, 0x0A,     0x00, 0x80, 0x0B, 0x03};

    // payload           unknown                 param index             4 byte float value
    uint8_t payload[] = {0xB9, 0x04, 0x02, 0x00, 0x00,             0x88, 0x00, 0x00, 0x00, 0x00 };

    // set param index
    payload[4] = index;

    // set param value
    memcpy((void*)&payload[6], (void*)&value, sizeof(value));

    // build total message
    memcpy((void*)txBuffer, (void*)message, sizeof(message));
    memcpy((void*)&txBuffer[sizeof(message)], (void*)payload, sizeof(payload));

    // add framing
    framedLength = tonexOne_AddFraming(txBuffer, sizeof(message) + sizeof(payload), framedBuffer);

    // debug
    //ESP_LOG_BUFFER_HEXDUMP(TAG, FramedBuffer, framed_length, ESP_LOG_INFO);

    // send it
	size_t sentBytes = SerialHost.write(framedBuffer, framedLength);
	SerialHost.flush();
	return 0;
   // return usb_tonex_one_transmit(FramedBuffer, framed_length);
}

esp_err_t tonexOne_ModifyParameter(uint16_t index, float value)
{
    tTonexParameter* param_ptr = NULL;
    esp_err_t res = ESP_FAIL;
     
    if (index >= TONEX_PARAM_LAST)
    {
        ESP_LOGE(TAG, "usb_tonex_one_modify_parameters invalid index %d", (int)index);   
        return ESP_FAIL;
    }
        
    if (tonex_params_get_locked_access(&param_ptr) == ESP_OK)
    {
        ESP_LOGI(TAG, "usb_tonex_one_modify_parameter index: %d name: %s value: %02f", (int)index, param_ptr[index].Name, value);  

        // update the local copy
        memcpy((void*)&param_ptr[index].Value, (void*)&value, sizeof(float));

        tonex_params_release_locked_access();
        res = ESP_OK;
    }

    return res;
}

esp_err_t tonexOne_ModifyGlobal(uint16_t global_val, float value)
{
    esp_err_t res = ESP_FAIL;

    switch (global_val)
    {
        case TONEX_GLOBAL_BPM:
        {
            // modify the BPM value in state packet
            memcpy((void*)&tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_BPM], (void*)&value, sizeof(float));
            res = ESP_OK;
        } break;

        case TONEX_GLOBAL_INPUT_TRIM:
        {
            // modify the input trim value in state
            memcpy((void*)&tonexData->message.pedalData.stateData[TONEX_STATE_OFFSET_START_INPUT_TRIM], (void*)&value, sizeof(float));
            res = ESP_OK;
        } break;

        case TONEX_GLOBAL_CABSIM_BYPASS:
        {
            // modify the cabsim bypass in state
            tonexData->message.pedalData.stateData[TONEX_STATE_OFFSET_START_CAB_BYPASS] = (uint8_t)value;
            res = ESP_OK;
        } break;

        case TONEX_GLOBAL_TEMPO_SOURCE:
        {
            // modify the tempo source value in state
            tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_TEMPO_SOURCE] = (uint8_t)value;
            res = ESP_OK;
        } break;
    }

    return res;
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
	tTonexParameter* param_ptr;

	tonexData->message.header.type = PacketStateUpdate;

	tonexData->message.pedalData.stateDataLength = length - index;
	memcpy((void*)tonexData->message.pedalData.stateData, (void*)&unframed[index], tonexData->message.pedalData.stateDataLength);
	ESP_LOGI(TAG, "Saved Pedal StateData: %d", tonexData->message.pedalData.stateDataLength);

	// save preset details
	tonexData->message.slotAPreset = tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_SLOT_A_PRESET];
	tonexData->message.slotBPreset = tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_SLOT_B_PRESET];
	tonexData->message.slotCPreset = tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_SLOT_C_PRESET];
	tonexData->message.currentSlot = (Slot)tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_CURRENT_SLOT];

	// update global params
	if (tonex_params_get_locked_access(&param_ptr) == ESP_OK)
	{
		 memcpy((void*)&param_ptr[TONEX_GLOBAL_BPM].Value, (void*)&tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_BPM], sizeof(float));
		 memcpy((void*)&param_ptr[TONEX_GLOBAL_INPUT_TRIM].Value, (void*)&tonexData->message.pedalData.stateData[TONEX_STATE_OFFSET_START_INPUT_TRIM], sizeof(float));
		 param_ptr[TONEX_GLOBAL_CABSIM_BYPASS].Value = (float)tonexData->message.pedalData.stateData[TONEX_STATE_OFFSET_START_CAB_BYPASS];
		 param_ptr[TONEX_GLOBAL_TEMPO_SOURCE].Value = (float)tonexData->message.pedalData.stateData[tonexData->message.pedalData.stateDataLength - TONEX_STATE_OFFSET_END_TEMPO_SOURCE];

		 tonex_params_release_locked_access();
	}

	tonexOne_DumpState();
	ESP_LOGI(TAG, "Slot A: %d. Slot B:%d. Slot C:%d. Current slot: %d", (int)tonexData->message.slotAPreset, (int)tonexData->message.slotBPreset, (int)tonexData->message.slotCPreset, (int)tonexData->message.currentSlot);

	// ESP_LOGI(TAG, "State Data Rx: %d %d", (int)length, (int)index);
	// ESP_LOGD_BUFFER_HEXDUMP(TAG, tonexData->message.pedalData.RawData, tonexData->message.pedalData.Length, ESP_LOGD_INFO);

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

uint16_t tonexOne_AddByteWithStuffing(uint8_t *output, uint8_t byte)
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
		outlength += tonexOne_AddByteWithStuffing(&output[outlength], input[byte]);
	}

	// add CRC
	uint16_t crc = tonexOne_CalculateCRC(input, inlength);
	outlength += tonexOne_AddByteWithStuffing(&output[outlength], crc & 0xFF);
	outlength += tonexOne_AddByteWithStuffing(&output[outlength], (crc >> 8) & 0xFF);

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
		ESP_LOGD(TAG, "Invalid Frame (1)");
		return ParsingInvalidFrame;
	}

	for (uint16_t i = 1; i < inlength - 1; ++i)
	{
		if (input[i] == 0x7D)
		{
			if ((i + 1) >= (inlength - 1))
			{
				ESP_LOGD(TAG, "Invalid Escape sequence");
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
		ESP_LOGD(TAG, "Invalid Frame (2)");
		return ParsingInvalidFrame;
	}

	// ESP_LOGI(TAG, "In:");
	// ESP_LOGD_BUFFER_HEXDUMP(TAG, input, inlength, ESP_LOGD_INFO);
	// ESP_LOGI(TAG, "Out:");
	// ESP_LOGD_BUFFER_HEXDUMP(TAG, output, *outlength, ESP_LOGD_INFO);

	uint16_t received_crc = (output[(*outlength) - 1] << 8) | output[(*outlength) - 2];
	(*outlength) -= 2;

	uint16_t calculated_crc = tonexOne_CalculateCRC(output, *outlength);

	if (received_crc != calculated_crc)
	{
		ESP_LOGD(TAG, "Crc mismatch: %X, %X", (int)received_crc, (int)calculated_crc);
		return ParsingCRCError;
	}

	return ParsingOk;
}

#endif