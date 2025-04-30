#ifndef WIFI_MANAGEMENT_H_
#define WIFI_MANAGEMENT_H_

#include "stdint.h"

void wifi_ProcessTask(void* parameter);


uint8_t wifi_Connect(const char* hostName, const char* apName, const char* apPassword);
uint8_t wifi_Disconnect();
uint8_t wifi_ConnectionStatus();
uint8_t wifi_CheckConnectionPing();

void wifi_ResetSettings();

//__weak void wifi_ConnectedCallback

#endif // WIFI_MANAGEMENT_H_