#ifndef _TONEXONE_H_
#define _TONEXONE_H_

#include "Adafruit_TinyUSB.h"
#include "usb_cdc_handling.h"

#define TONEX_ONE_PID 0x00D1
#define TONEX_ONE_VID 0x1963


// MIDI Mapping
#define TONEX_ONE_DEFAULT_MIDI_CHANNEL 9

// Tonex One Commands
#define TONEX_ONE_PREVIOUS_PRESET_CC	32
#define TONEX_ONE_NEXT_PRESET_CC			33



//Adafruit_USBH_CDC SerialHost;

void tonexOne_SendHello();
void tonexOne_HandleReceivedData(char *rxData, uint16_t len);
void tonexOne_Process();
void tonexOne_SendGoToPreset(uint8_t presetNum);
void tonexOne_SendNextPreset();
void tonexOne_SendPreviousPreset();

#endif // _TONEXONE_H_