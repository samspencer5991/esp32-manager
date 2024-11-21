#ifndef DEVICE_API_H_
#define DEVICE_API_H_

#include "stdint.h"

#define USB_PACKET_DELIMITER								"~"
#define STRING_TERMINATOR									"\0"

#define USB_VALID_COMMAND_STRING							"ok"
#define USB_INVALID_COMMAND_STRING						"invalid command"
#define USB_TX_BUF_OVERFLOW_STRING						"buffer overflowed"
#define USB_INVALID_TERMINATOR_STRING					"invalid packet terminator"

#define USB_COMMAND_STRING									"command"

#define USB_TRUE_STRING										"true"
#define USB_FALSE_STRING									"false"

#define USB_CONTROL_COMMAND_TYPE_STRING 				"CTRL"
#define USB_DATA_REQUEST_TYPE_STRING					"DREQ"
#define USB_DATA_RESPONSE_TYPE_STRING					"DRES"
#define USB_CHECK_TYPE_STRING								"CHCK"
#define USB_DATA_TX_REQUEST_TYPE_STRING				"DTXR"
#define USB_DATA_SET_TYPE_STRING							"DSET"
#define USB_RESET_TYPE_STRING								"RSET"


#define USB_REFRESH_LEDS_STRING							"refreshLeds"
#define USB_REFRESH_DISPLAY_STRING						"refreshDisplay"
#define USB_MIDI_CHANNEL_STRING							"midiChannel"
#define USB_ENTER_BOOTLOADER_STRING						"enterBootloader"
#define USB_RESTART_STRING									"deviceRestart"
#define USB_FACTORY_RESET_STRING							"factoryReset"
#define USB_TOGGLE_FOOTSWITCH_STRING					"toggleFootswitch"
#define USB_BANK_UP_STRING									"bankUp"
#define USB_BANK_DOWN_STRING								"bankDown"
#define USB_GO_TO_BANK_STRING								"goToBank"
#define USB_CURRENT_BANK_STRING							"currentBank"
#define USB_BANK_STRING										"bank"
#define USB_FOOTSWITCH_STRING								"footswitch"
#define USB_FOOTSWITCHES_STRING							"footswitches"
#define USB_SWITCH_STRING									"switch"
#define USB_SWITCH_GROUPS_STRING							"switchGroups"
#define USB_FOOTSWITCH_MODES_STRING						"footswitchModes"
#define USB_GLOBAL_SETTINGS_STRING						"globalSettings"
#define USB_BANK_SETTINGS_STRING							"bankSettings"
#define USB_BANK_ID_STRING									"bankId"
#define USB_SWITCH_TOGGLE_STRING							"toggle"
#define USB_SWITCH_TAP_TEMPO_STRING						"tapTempo"
#define USB_SWITCH_MOMENTARY_STRING						"momentary"
#define USB_SWITCH_SEQUENTIAL_STRING					"sequential"
#define USB_SWITCH_SEQUENTIAL_LINKED_STRING			"sequentialLinked"
#define USB_SWITCH_SCROLLING_STRING						"scrolling"
#define USB_SWITCH_SCROLLING_LINKED_STRING			"scrollingLinked"
#define USB_SWITCH_DOUBLE_PRESS_TOGGLE_STRING		"doublePressToggle"
#define USB_SWITCH_HOLD_TOGGLE_STRING					"holdToggle"
#define USB_SWITCH_DOUBLE_PRESS_MOMENTARY_STRING	"doublePressMomentary"
#define USB_SWITCH_HOLD_MOMENTARY_STRING				"holdMomentary"
#define USB_MIDI_CLOCKS_STRING							"midiClocks"
#define USB_MIDI_CLOCK_TEMPOS_STRING					"midiClockTempos"
#define USB_TEMPO_STRING									"tempo"
#define USB_PRIMARY_STATE_STRING							"primaryState"
#define USB_SECONDARY_STATE_STRING						"secondaryState"
#define USB_PRIMARY_MODE_STRING							"primaryMode"
#define USB_SECONDARY_MODE_STRING						"secondaryMode"
#define USB_PRIMARY_LED_MODE_STRING						"primaryLedMode"
#define USB_SECONDARY_LED_MODE_STRING					"secondaryLedMode"
#define USB_PRIMRARY_COLOUR_STRING						"primaryColor"
#define USB_SECONDARY_COLOUR_STRING						"secondaryColor"
#define USB_IS_PRIMARY_SWITCH_STRING					"isPrimary"
#define USB_BROADCAST_MODE_STRING						"broadcastMode"
#define USB_RESPOND_TO_STRING								"respondTo"
#define USB_RESPONSE_TYPE_STRING							"responseType"
#define USB_STEP_INTERVAL_STRING							"stepInterval"
#define USB_MIN_SCROLL_LIMIT_STRING						"minScrollLimit"
#define USB_MAX_SCROLL_LIMIT_STRING						"maxScrollLimit"
#define USB_SEQUENTIAL_PATTERN_STRING					"sequentialPattern"
#define USB_SEQUENTIAL_REPEAT_STRING					"sequentialRepeat"
#define USB_SEQUENTIAL_SEND_MODE_STRING				"sequentialSendMode"
#define USB_LINKED_SWITCH_STRING							"linkedSwitch"
#define USB_NUM_SEQUENTIAL_MESSAGES_STRING			"numSequentialMessages"
#define USB_NUM_STEPS_STRING								"numSteps"
#define USB_STEPS_STRING									"steps"
#define USB_SEQUENTIAL_LABEL_STRING						"label"
#define USB_SEQUENTIAL_COLOUR_STRING					"color"
#define USB_CURRENT_STEP_STRING							"currentStep"
#define USB_MIDI_CLOCK_STRING								"midiClock"
#define USB_SMART_TYPE_STRING								"smartType"

#define USB_LED_MODE_ON_OFF_STRING						"onOff"
#define USB_LED_MODE_DIM_STRING							"dim"

#define USB_GROUP_BROADCAST_TX_RX_STRING				"txRx"
#define USB_GROUP_BROADCAST_TX_STRING					"tx"
#define USB_GROUP_BROADCAST_RX_STRING					"rx"
#define USB_GROUP_BROADCAST_NONE_STRING				"none"

#define USB_GROUP_RESPOND_TO_ON_OFF_STRING			"onOff"
#define USB_GROUP_RESPOND_TO_ON_STRING					"on"
#define USB_GROUP_RESPOND_TO_OFF_STRING				"off"

#define USB_GROUP_RESPONSE_TYPE_OR_STRING				"or"
#define USB_GROUP_RESPONSE_TYPE_AND_STRING			"and"
#define USB_GROUP_RESPONSE_TYPE_TOGGLE_STRING		"toggle"
#define USB_GROUP_RESPONSE_TYPE_ON_STRING				"on"
#define USB_GROUP_RESPONSE_TYPE_OFF_STRING			"off"

#define USB_SEQUENTIAL_PATTERN_FORWARD_STRING		"forward"
#define USB_SEQUENTIAL_PATTERN_REVERSE_STRING		"reverse"
#define USB_SEQUENTIAL_PATTERN_PENDULUM_STRING		"pendulum"
#define USB_SEQUENTIAL_PATTERN_RANDOM_STRING			"random"

#define USB_SEQUENTIAL_REPEAT_ALL_STRING				"all"
#define USB_SEQUENTIAL_REPEAT_LAST2_STRING			"last2"
#define USB_SEQUENTIAL_REPEAT_LAST3_STRING			"last3"

#define USB_SEQUENTIAL_SEND_MODE_ALWAYS_STRING		"always"
#define USB_SEQUENTIAL_SEND_MODE_SECONDARY_STRING	"secondary"
#define USB_SEQUENTIAL_SEND_MODE_PRIMARY_STRING		"primary"

#define USB_STATE_STRING									"state"

#define USB_MIDI_OUTPUTS_STRING							"outputs"
#define USB_DIVISION_STRING								"division"
#define USB_FOOTSWITCH1_COMMAND_STRING					"fs1"
#define USB_FOOTSWITCH2_COMMAND_STRING					"fs2"
#define USB_FOOTSWITCH3_COMMAND_STRING					"fs3"
#define USB_FOOTSWITCH4_COMMAND_STRING					"fs4"
#define USB_FOOTSWITCH5_COMMAND_STRING					"fs5"
#define USB_FOOTSWITCH6_COMMAND_STRING					"fs6"

#define USB_FLEXIPORTS_STRING								"flexiports"
#define USB_FLEXIPORTS_MODE_STRING						"mode"
#define USB_FLEXIPORTS_CLOCK_STRING						"clock"
#define USB_FLEXIPORTS_AUX_SENSITIVITY_STRING		"auxSensitivity"
#define USB_AUX_SENSITIVITY_LOW_STRING					"low"
#define USB_AUX_SENSITIVITY_MED_STRING					"medium"
#define USB_AUX_SENSITIVITY_HIGH_STRING				"high"
#define USB_FLEXIPORTS_AUX_HOLD_TIME_STRING			"auxHoldTime"
#define USB_FLEXI1_MODE_STRING							"flexi1Mode"
#define USB_FLEXI2_MODE_STRING							"flexi2Mode"
#define USB_FLEXI1_CLOCK_STRING							"flexi1Clock"
#define USB_FLEXI2_CLOCK_STRING							"flexi2Clock"
#define USB_FLEXI_UNASSIGNED_STRING						"unassigned"
#define USB_FLEXI_MIDI_OUT_TYPEA_STRING				"midiOutA"
#define USB_FLEXI_MIDI_OUT_TYPEB_STRING				"midiOutB"
#define USB_FLEXI_MIDI_OUT_TIP_STRING					"midiOutTip"
#define USB_FLEXI_MIDI_OUT_RING_STRING					"midiOutRing"
#define USB_FLEXI_DEVICE_LINK_STRING					"deviceLink"
#define USB_FLEXI_EXP_DUAL_STRING						"expDual"
#define USB_FLEXI_EXP_SINGLE_STRING						"expSingle"
#define USB_FLEXI_SWITCH_IN_STRING						"switchIn"
#define USB_FLEXI_SWITCH_OUT_STRING						"switchOut"
#define USB_FLEXI_TAP_TEMPO_STRING						"tapTempo"
#define USB_FLEXI_PULSE_OUT_STRING						"pulseOut"
#define USB_FLEXI_VOLTAGE_OUT_STRING					"voltageOut"
#define USB_FLEXI_RELAY_OUT_STRING						"relayOut"
#define USB_FLEXI_FAV_SWITCH_STRING						"favSwitch"

#define USB_MIDI_CLOCK_A_TEMPO							"midiClockATempo"
#define USB_MIDI_CLOCK_B_TEMPO							"midiClockBTempo"

#define USB_UI_MODE_STRING									"uiMode"
#define USB_SIMPLE_UI_MODE_STRING						"simple"
#define USB_EXTENDED_UI_MODE_STRING						"extended"
#define USB_ZERO_INDEX_BANKS_STRING						"zeroIndexBanks"
#define USB_PRESERVE_TOGGLE_STATES_STRING				"preserveToggleStates"
#define USB_PRESERVE_SCROLLING_STATES_STRING			"preserveScrollingStates"
#define USB_PRESERVE_SEQUENTIAL_STATES_STRING		"preserveSequentialStates"
#define USB_SEND_STATES_STRING							"sendStates"
#define USB_BANK_TEMPLATE_INDEX_STRING					"bankTemplateIndex"
#define USB_BANK_PC_OUTPUTS_STRING						"bankPcMidiOutputs"
#define USB_CUSTOM_LED_COLOURS_STRING					"customLedColors"
#define USB_COLOUR_CUSTOM1_STRING						"custom1"
#define USB_COLOUR_CUSTOM2_STRING						"custom2"
#define USB_COLOUR_CUSTOM3_STRING						"custom3"
#define USB_COLOUR_CUSTOM4_STRING						"custom4"
#define USB_COLOUR_CUSTOM5_STRING						"custom5"
#define USB_COLOUR_CUSTOM6_STRING						"custom6"
#define USB_COLOUR_CUSTOM7_STRING						"custom7"
#define USB_COLOUR_CUSTOM8_STRING						"custom8"
#define USB_COLOUR_CUSTOM9_STRING						"custom9"
#define USB_COLOUR_CUSTOM10_STRING						"custom10"
#define USB_COLOUR_CUSTOM11_STRING						"custom11"
#define USB_COLOUR_CUSTOM12_STRING						"custom12"

#define USB_DEVICE_NAME_STRING							"deviceName"
#define USB_PROFILE_ID_STRING								"profileId"
#define USB_BOOT_MESSAGES_STRING							"bootMessages"
#define USB_DEVICE_LINK_STRING							"deviceLink"
#define USB_DEVICE_LINK_ROLE_STRING						"role"
#define USB_BANK_NAV_STRING								"bankNav"
#define USB_MIDI_STREAM_STRING							"midiStream"
#define USB_DEVICE_LINK_MAIN_STRING						"main"
#define USB_DEVICE_LINK_SATELLITE_STRING				"satellite"

#define USB_BANK_DOWN_TRIGGER_STRING					"bankDownTrigger"
#define USB_BANK_UP_TRIGGER_STRING						"bankUpTrigger"
#define USB_BANK_FS1_STRING								"fs1"
#define USB_BANK_FS2_STRING								"fs2"
#define USB_BANK_FS3_STRING								"fs3"
#define USB_BANK_FS4_STRING								"fs4"
#define USB_BANK_FS5_STRING								"fs5"
#define USB_BANK_FS6_STRING								"fs6"
#define USB_BANK_FS1_FS2_STRING							"fs1Fs2"
#define USB_BANK_FS2_FS3_STRING							"fs2Fs3"
#define USB_BANK_FS2_FS5_STRING							"fs2Fs5"
#define USB_BANK_FS3_FS6_STRING							"fs3Fs6"
#define USB_BANK_FS4_FS5_STRING							"fs4Fs5"
#define USB_BANK_FS5_FS6_STRING							"fs5Fs6"
#define USB_BANK_FS2_FS4_STRING							"fs2Fs4"
#define USB_BANK_FS3_FS4_STRING							"fs3Fs4"
#define USB_BANK_FS1_HOLD_STRING							"fs1Hold"
#define USB_BANK_FS2_HOLD_STRING							"fs2Hold"
#define USB_BANK_FS3_HOLD_STRING							"fs3Hold"
#define USB_BANK_NONE_STRING								"none"

#define USB_EXP_STRING										"exp"
#define USB_MESSAGES_STRING								"messages"
#define USB_NUM_MESSAGES_STRING							"numMessages"
#define USB_MIN_LIMIT_STRING								"minLimit"
#define USB_MAX_LIMIT_STRING								"maxLimit"
#define USB_SWEEP_STRING									"sweep"
#define USB_STATUS_BYTE_STRING							"statusByte"
#define USB_DATA_BYTE1_STRING								"dataByte1"
#define USB_DATA_BYTE2_STRING								"dataByte2"
#define USB_LINEAR_SWEEP_STRING							"linear"
#define USB_LOG_SWEEP_STRING								"log"
#define USB_ANTI_LOG_SWEEP_STRING						"reverseLog"
#define USB_INV_LINEAR_SWEEP_STRING						"invLinear"
#define USB_INV_LOG_SWEEP_STRING							"invLog"
#define USB_INV_ANTI_LOG_SWEEP_STRING					"invReverseLog"

#define USB_TRIGGER_VALUE_STRING							"triggerValue"

#define USB_NULL_STRING										"null"
#define USB_FLEXI1_STRING									"flexi1"
#define USB_FLEXI2_STRING									"flexi2"
#define USB_DIN5_STRING										"din5"
#define USB_USB_STRING										"usb"
#define USB_MIDI0_STRING									"midi0"
#define USB_BLE_STRING										"ble"
#define USB_MIDI1_STRING									"midi1"
#define USB_MIDI2_STRING									"midi2"
#define USB_MIDI3_STRING									"midi3"
#define USB_MIDI4_STRING									"midi4"
#define USB_MIDI5_STRING									"midi5"

#define USB_MIDI0_THRU_HANDLES_STRING					"midi0ThruHandles"
#define USB_FLEXI1_THRU_HANDLES_STRING					"flexi1ThruHandles"
#define USB_FLEXI2_THRU_HANDLES_STRING					"flexi2ThruHandles"
#define USB_MIDI0_THRU_HANDLES_STRING					"midi0ThruHandles"
#define USB_USB_THRU_HANDLES_STRING						"usbThruHandles"

#define USB_14_DIVISION_STRING							"1/4"
#define USB_14T_DIVISION_STRING							"1/4T"
#define USB_14DOT_DIVISION_STRING						"1/4."
#define USB_18_DIVISION_STRING							"1/8"
#define USB_18T_DIVISION_STRING							"1/8T"
#define USB_18DOT_DIVISION_STRING						"1/8."
#define USB_116_DIVISION_STRING							"1/16"
#define USB_116T_DIVISION_STRING							"1/16T"
#define USB_116DOT_DIVISION_STRING						"1/16."

#define USB_LFO_STRING										"lfo"
#define USB_FREQUENCY_STRING								"frequency"
#define USB_TRIGGER_STRING									"trigger"
#define USB_WAVEFORM_STRING								"waveform"
#define USB_LFO_MOD_PARAMETER_STRING					"modParameter"
#define USB_LFO_MOD_SOURCE_STRING						"modSource"
#define USB_LFO_RESOLUTION_STRING						"resolution"
#define USB_LFO_RESET_STOP_STRING						"resetOnStop"
#define USB_LFO_CLOCK_STRING								"clock"
#define USB_LFO_SINE_WAVE_STRING							"sine"
#define USB_LFO_TRIANGLE_WAVE_STRING					"triangle"
#define USB_LFO_SAW_WAVE_STRING							"saw"
#define USB_LFO_RAMP_WAVE_STRING							"ramp"
#define USB_LFO_SQUARE_WAVE_STRING						"square"
#define USB_LFO_RANDOM_WAVE_STRING						"random"
#define USB_MIDICLOCKA_STRING								"midiClockA"
#define USB_MIDICLOCKB_STRING								"midiClockB"
#define USB_LFO_INDEPENDENT_SOURCE						"free"

#define USB_SMART_SWITCH_ON_STRING							"switchOn"
#define USB_SMART_SWITCH_OFF_STRING							"switchOff"
#define USB_SMART_SWITCH_TOGGLE_STRING						"switchToggle"
#define USB_SMART_RESET_SEQUENTIAL_STEP_STRING			"sequentialResetStep"
#define USB_SMART_INCREMENT_SEQUENTIAL_STEP_STRING		"sequentialIncrementStep"
#define USB_SMART_DECREMENT_SEQUENTIAL_STEP_STRING		"sequentialDecrementStep"
#define USB_SMART_GOTO_SEQUENTIAL_STEP_STRING			"sequentialGoToStep"
#define USB_SMART_QUEUE_NEXT_SEQUENTIAL_STEP_STRING	"sequentialQueueNextStep"
#define USB_SMART_QUEUE_SEQUENTIAL_STEP_STRING			"sequentialQueueStep"
#define USB_SMART_RESET_SCROLLING_STEP_STRING			"scrollingResetStep"
#define USB_SMART_INCREMENT_SCROLLING_STEP_STRING		"scrollingIncrementStep"
#define USB_SMART_DECREMENT_SCROLLING_STEP_STRING		"scrollingDecrementStep"
#define USB_SMART_GOTO_SCROLLING_STEP_STRING				"scrollingGoToStep"
#define USB_SMART_QUEUE_NEXT_SCROLLING_STEP_STRING		"scrollingQueueNextStep"
#define USB_SMART_QUEUE_SCROLLING_STEP_STRING			"scrollingQueueStep"
#define USB_SMART_BANK_UP_STRING								"bankUp"
#define USB_SMART_BANK_DOWN_STRING							"bankDown"
#define USB_SMART_GOTO_BANK_STRING							"goToBank"
#define USB_SMART_GOTO_LAST_BANK_STRING					"jumpToLastBank"
#define USB_SMART_INCREMENT_EXP_STEP_STRING				"incrementExpStep"
#define USB_SMART_DECREMENT_EXP_STEP_STRING				"decrementExpStep"
#define USB_SMART_GOTO_EXP_STEP_STRING						"goToExpStep"
#define USB_SMART_TRS_SWITCH_OUT_STRING					"trsSwitchOut"
#define USB_SMART_TRS_PULSE_OUT_STRING						"trsPulseOut"
#define USB_SMART_UI_MODE_STRING								"uiMode"

// Global settings string label macros
#define USB_TOGGLE_ON_MESSAGES_STRING					"toggleOnMessages"
#define USB_TOGGLE_OFF_MESSAGES_STRING					"toggleOffMessages"
#define USB_SECONDARY_TOGGLE_ON_MESSAGES_STRING		"secondaryToggleOnMessages"
#define USB_SECONDARY_TOGGLE_OFF_MESSAGES_STRING	"secondaryToggleOffMessages"
#define USB_TOGGLE_OFF_MESSAGES_STRING					"toggleOffMessages"
#define USB_PRESS_MESSAGES_STRING						"pressMessages"
#define USB_RELEASE_MESSAGES_STRING						"releaseMessages"
#define USB_DOUBLE_PRESS_MESSAGES_STRING				"doublePressMessages"
#define USB_HOLD_MESSAGES_STRING							"holdMessages"
#define USB_HOLD_RELEASE_MESSAGES_STRING				"holdReleaseMessages"
#define USB_SEQUENTIAL_MESSAGES_STRING					"sequentialMessages"
#define USB_SCROLLING_MESSAGES_STRING					"scrollingMessages"
#define USB_AUX_MESSAGES_STRING							"auxMessages"
#define USB_AUX_GLOBAL_MESSAGES_STRING					"auxGlobalMessages"
#define USB_BANK_MESSAGES_STRING							"bankMessages"
#define USB_EXP_MESSAGES_STRING							"expMessages"
#define USB_EXP_LADDER_MESSAGES_STRING					"expLadderMessages"

#define USB_TOGGLE_ON_MESSAGE_STRING					"toggleOnMessage"
#define USB_TOGGLE_OFF_MESSAGE_STRING					"toggleOffMessage"
#define USB_PRESS_MESSAGE_STRING							"pressMessage"
#define USB_RELEASE_MESSAGE_STRING						"releaseMessage"
#define USB_DOUBLE_PRESS_MESSAGE_STRING				"doublePressMessage"
#define USB_HOLD_MESSAGE_STRING							"holdMessage"
#define USB_HOLD_RELEASE_MESSAGE_STRING				"holdReleaseMessage"
#define USB_BANK_NAME_STRING								"bankName"
#define USB_BANK_ID_STRING									"bankId"

#define USB_HOLD_TIME_STRING								"holdTime"
#define USB_BOOT_DELAY_STRING								"bootDelay"

#define USB_BANK_INDEX_STRING								"bankIndex"

#define USB_FS_NAME_STRING									"name"

#define USB_SWITCH_OUT_OPEN_STRING						"open"
#define USB_LED_BRIGHTNESS_STRING						"ledBrightness"

#define USB_TIP_STRING										"tip"
#define USB_RING_STRING										"ring"
#define USB_TIP_RING_STRING								"tipRing"

#define USB_LED_OFF_STRING									"off"
#define USB_LED_LOW_STRING									"low"
#define USB_LED_MED_STRING									"med"
#define USB_LED_HIGH_STRING								"high"
#define USB_TX_DATA_LABEL_SIZE							50
#define DISABLE_CONTROL_WHEN_COM_OPEN					0

#define USB_TOGGLE_ON_STRING								"toggleOn"
#define USB_TOGGLE_OFF_STRING								"toggleOff"
#define USB_SECONDARY_TOGGLE_ON_STRING					"secondaryToggleOn"
#define USB_SECONDARY_TOGGLE_OFF_STRING				"secondaryToggleOff"
#define USB_PRESS_STRING									"press"
#define USB_RELEASE_STRING									"release"
#define USB_DOUBLE_PRESS_STRING							"doublePress"
#define USB_HOLD_STRING										"hold"
#define USB_HOLD_RELEASE_STRING							"holdRelease"

#define USB_PRIMARY_STRING									"primary"
#define USB_SECONDARY_STRING								"secondary"

#define USB_CDC_TRANSPORT			0
#define MIDI_TRANSPORT				1


extern const char termChar[];
extern const char commaChar[];
extern const char newLineChar[];

// Type of command last received
typedef enum
{
	ControlCommand,
	DataRequestCommand,
	DataTxRequestCommand,
	CheckCommand,
	MidiCommand,
	ResetCommand,
	NoCommand
} DeviceApiCommand;

// The state of the device API
typedef enum
{
	DeviceApiReady,		// API is in a reset state to receive new commands
	DeviceApiBusy,			// API is busy processing a command
	DeviceApiNewCommand,	// API has received a new command and is waiting on follow up data
	DeviceApiRxData,		// API is ready to receive data packet
	DeviceApiError			// API is in an error state
}	DeviceApiState;

// Type of data request last received
typedef enum
{
	DeviceApiNoData,
	DeviceApiGlobalSettings,
	DeviceApiBankSettings
} DeviceApiDataType;

extern DeviceApiCommand receivedCommand;
extern DeviceApiDataType apiDataType;

DeviceApiState deviceApi_Handler(char* appData, uint8_t transport);

#endif /* DEVICE_API_H_ */