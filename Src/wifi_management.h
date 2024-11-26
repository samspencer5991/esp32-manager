#ifndef WIFI_MANAGEMENT_H_
#define WIFI_MANAGEMENT_H_

#include "stdint.h"

uint8_t wifi_Connect(const char* hostName, const char* apName, const char* apPassword);
uint8_t wifi_ConnectionStatus();
uint8_t wifi_CheckConnectionPing();
void wifi_Loop();
void wifi_ResetSettings();

extern uint8_t wifiEnabled;

#endif // WIFI_MANAGEMENT_H_