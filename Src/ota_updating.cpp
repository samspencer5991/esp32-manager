#include "ota_updating.h"
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "ota_pull.h"

#include <time.h>

static const char *url = "https://github.com/Pirate-MIDI/Spin/raw/refs/heads/main/Firmware/.pio/build/spin-v1-x-0/spin_v0.1.0.0.bin";  //state url of your firmware image

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

String ota_GetLatestVersion(String url)
{
	JsonDocument doc;
	WiFiClientSecure client;
	HTTPClient http;
	String response;
	client.setInsecure();
	http.begin(client, url);  // Specify the URL
	int httpCode = http.GET();  // Send the GET request

	if (httpCode > 0)
	{
		// Check for successful response
		if (httpCode == HTTP_CODE_OK)
		{
			// Get the response payload (JSON data)
			response = http.getString();  
			// Print the raw JSON data
			Serial.println("Received JSON:");
			//Serial.println(response);
		}
		else
			return "HTTP GET request failed";

	}
	else
		return "Failed to connect to GitHub";

	http.end();  // Close the connection
	DeserializationError error = deserializeJson(doc, response);
	if (error)
		return error.c_str();
	
	serializeJsonPretty(doc, Serial);

	const char* version = doc["Configurations"][0]["Version"];
	return (String)version;
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

void setClock()
{
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // UTC

  Serial.print(F("Waiting for NTP time sync: "));
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    yield();
    delay(500);
    Serial.print(F("."));
    now = time(nullptr);
  }

  Serial.println(F(""));
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}









