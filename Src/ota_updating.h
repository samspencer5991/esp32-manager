#ifndef OTA_UPDATING_H_
#define OTA_UPDATING_H_

#include "Arduino.h"

void ota_Begin();
void ota_Process();

String ota_GetLatestVersion(String url);

#endif /* OTA_UPDATING_H_ */