#ifndef TONEXONE_INTERFACE_H_
#define TONEXONE_INTERFACE_H_
#ifdef USE_TONEX_ONE
#include "stdint.h"

void tonexOne_InterfaceInit();
void tonexOne_SendGoToPreset(uint8_t presetNum);
void tonexOne_SendPreviousPreset();
void tonexOne_SendNextPreset();
void tonexOne_SendParameter(uint16_t parameter, float value);

#endif
#endif // TONEXONE_INTERFACE_H_