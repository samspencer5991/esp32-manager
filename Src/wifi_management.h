#ifndef WIFI_MANAGEMENT_H_
#define WIFI_MANAGEMENT_H_

bool wifi_Connect(const char* hostName, const char* apName, const char* apPassword);
void wifi_Reset();

extern bool wifiConnected;

#endif // WIFI_MANAGEMENT_H_