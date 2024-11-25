#ifndef OTA_UPDATING_H_
#define OTA_UPDATING_H_

#include "Arduino.h"

void ota_Begin();
void ota_Loop();

String ota_GetLatestVersion(String url);
void testOTA();
void testOTALoop();

#endif /* OTA_UPDATING_H_ */