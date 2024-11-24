#include "ota_updating.h"
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include "WiFi.h"
#include <HTTPClient.h>


void onOTAStart();
void onOTAProgress(size_t current, size_t final);
void onOTAEnd(bool success);

AsyncWebServer server(80);
unsigned long ota_progress_millis = 0;

void ota_Begin()
{
	if(WiFi.status() != WL_CONNECTED)
	{
		Serial.println("error: WiFi not connected.");
		return;
	}
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(200, "text/plain", "Hi! This is ElegantOTA AsyncDemo.");
	});

	ElegantOTA.begin(&server);    // Start ElegantOTA
	// ElegantOTA callbacks
	ElegantOTA.onStart(onOTAStart);
	ElegantOTA.onProgress(onOTAProgress);
	ElegantOTA.onEnd(onOTAEnd);

	server.begin();
	Serial.println("HTTP server started");
}

void ota_Loop()
{
	ElegantOTA.loop();
}

void ota_GetLatestVersion()
{
	
}

void onOTAStart()
{
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final)
{
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success)
{
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

int DownloadJson(const char* URL, String& payload)
{
	HTTPClient http;
	http.begin(URL);

	// Send HTTP GET request
	int httpResponseCode = http.GET();

	if (httpResponseCode == 200)
	{
		payload = http.getString();
	}

	// Free resources
	http.end();
	return httpResponseCode;
}
