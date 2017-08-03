#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <WiFiManager.h>

#include <bigboi_hardware.h>

const char* host = "api.thingspeak.com";

const int UPDATE_DELAY = 300;

// Runtime State
float temperature;

BigBoiHardware hardware;
ESP8266WebServer server(80);

void handleData() {
  String arg;
  if (server.method() == HTTP_GET) {
    String url = "{";
    url += "\"temperature\":";
    if(isnan(temperature)) {
      url += "false";
    } else {
      url += temperature;
    }
    url += "}";
    server.send(200, "application/javascript", url);
  }
}

void thingspeak_log() {
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  String url = "/update?key=";
  url += thingspeak_key;
  url += "&field1=";
  url += temperature;
  Serial.println(host + url);

  delay(10);
  client.print(
    "GET " +
    url +
    " HTTP/1.1\r\n" +
    "Host: " + host + "\r\n" +
    "Connection: close\r\n\r\n");

  while (client.available()) {
    String line = client.readStringUntil('\r');
  }
}

void setup(void)
{
  Serial.begin(115200);
  Serial.println();

  hardware.begin();

  server.on("/data", handleData);

  // Setup network
  char hostname[64];
  snprintf(hostname, 64, "bigboi-%06x", ESP.getChipId());
  wifi_station_set_hostname(hostname);
  WiFi.hostname(hostname);
  ArduinoOTA.begin();
  MDNS.begin(hostname);
  SPIFFS.begin();
  WiFiManager wifiManager;
  wifiManager.autoConnect(hostname);
  MDNS.addService("bigboi", "tcp", 80);
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    server.serveStatic(dir.fileName().c_str(), SPIFFS, dir.fileName().c_str());
  }
  server.serveStatic("/", SPIFFS, "/index.html");
  server.begin();

  hardware.tick(5);
}

unsigned long lastReading = 0;
void loop(void)
{
  server.handleClient();

  unsigned long now = millis();
  if (now - lastReading > UPDATE_DELAY)
  {
    lastReading = now;
    temperature = hardware.getTemperature(0);
    thingspeak_log();
  }
}
