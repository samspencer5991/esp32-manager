#ifndef OTA_UPDATING_H_
#define OTA_UPDATING_H_

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

void ota_Begin();
void ota_Loop();

#endif /* OTA_UPDATING_H_ */