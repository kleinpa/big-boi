#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <WiFiManager.h>

#include <bigboi_hardware.h>

const char* host = "api.thingspeak.com";

const int UPDATE_DELAY = 15500;
const int TIMEOUT_NETWORK = 5000;
const int TIMEOUT_THERMOMETER = 5000;

// Runtime State
float temperature;
float compressorTemperature;
int chiller = 0;

// Persistent State
int STATE_VERSION = 2;
struct State {
  char id[16];
  int version;
  float setpoint;
  float epsilon;
  float compressorLimit;
  DeviceAddress fridgeThermometer;
  DeviceAddress compressorThermometer;
  char thingspeak_key[64];
} state;

BigBoiHardware hardware;

ESP8266WebServer server(80);

char STATE_ID[16] = {0x32, 0x07, 0x0b, 0x4f, 0x08, 0x3c, 0x42, 0x25, 0xa6, 0x09, 0x0c, 0x35, 0x97, 0x92, 0x16, 0x9c};

void saveState() {
  EEPROM.put(0, state);
  yield();
  EEPROM.commit();
}

void loadState() {

  EEPROM.get(0, state);
  if (!(/*memcmp(STATE_ID, state.id, 16) && */STATE_VERSION == state.version)) {
    // Fails ID check, reinitialize
    memcpy(state.id, STATE_ID, 16);
    state.version = STATE_VERSION;
    state.setpoint = 8;
    state.epsilon = 0.2;
    state.compressorLimit = 100;
    saveState();
  }
}

void deviceAddressToChar(DeviceAddress a, char c[17])
{
  snprintf(c, 17, "%02X%02X%02X%02X%02X%02X%02X%02X", a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
}

void stringToDeviceAddress(String s, DeviceAddress a)
{
  for(int i = 0; i < 8; i++){
    a[i] = strtoul(s.substring(i*2, i*2+2).c_str(), NULL, 16);
  }
}

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
    url += ",\"compressorTemperature\":";
    if(isnan(compressorTemperature)) {
      url += "false";
    } else {
      url += compressorTemperature;
    }
    url += ",\"chiller\":";
    url += chiller ? "true" : "false";
    url += ",\"setpoint\":";
    url += state.setpoint;
    url += ",\"epsilon\":";
    url += state.epsilon;
    url += ",\"compressorLimit\":";
    url += state.compressorLimit;
    url += ",\"fridgeThermometer\":\"";
    char c[17];
    deviceAddressToChar(state.fridgeThermometer, c);
    url += c;
    url += "\",\"compressorThermometer\":\"";
    deviceAddressToChar(state.compressorThermometer, c);
    url += c;
    url += "\"";
    if ((arg = server.arg("thermometers")) && arg.length() > 0) {
        url += ",\"thermometers\":[";
        DeviceAddress a;
        char c[17];
        int i = 0;
        while(hardware.getThermostatAddress(a, i++)) {
          deviceAddressToChar(a, c);
          if(i != 1) url += ",";
          url += "\"";
          url += c;
          url += "\"";
        }
        url += "]";
    }
    url += "}";
    server.send(200, "application/javascript", url);
  } else if (server.method() == HTTP_POST) {
    String msg;
    if ((arg = server.arg("setpoint")) && arg.length() > 0)
    {
      state.setpoint = arg.toFloat();
      saveState();
      server.send(200, "application/javascript", "{\"result\":\"ok\"");
    }
    if ((arg = server.arg("epsilon")) && arg.length() > 0)
    {
      state.epsilon = arg.toFloat();
      saveState();
      server.send(200, "application/javascript", "{\"result\":\"ok\"}");
    }
    if ((arg = server.arg("compressorLimit")) && arg.length() > 0)
    {
      state.compressorLimit = arg.toFloat();
      saveState();
      server.send(200, "application/javascript", "{\"result\":\"ok\"}");
    }
    if ((arg = server.arg("fridgeThermometer")) && arg.length() > 0)
    {
      stringToDeviceAddress(arg, state.fridgeThermometer);
      saveState();
      server.send(200, "application/javascript", "{\"result\":\"ok\"}");
    }
    if ((arg = server.arg("compressorThermometer")) && arg.length() > 0)
    {
      stringToDeviceAddress(arg, state.compressorThermometer);
      saveState();
      server.send(200, "application/javascript", "{\"result\":\"ok\"}");
    }
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
  url += state.thingspeak_key;
  url += "&field1=";
  url += temperature;
  url += "&field2=";
  url += hardware.getCompressor() ? "1" : "0";
  url += "&field3=";
  url += state.setpoint;
  url += "&field4=";
  url += state.epsilon;
  url += "&field5=";
  url += compressorTemperature;
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

  EEPROM.begin(sizeof(state));
  loadState();

  chiller = false;

  hardware.tick(3); // Hello!
}

unsigned long lastReading = 0;
void loop(void)
{
  server.handleClient();

  unsigned long now = millis();
  if (now - lastReading > UPDATE_DELAY)
  {
    temperature = hardware.getTemperature(state.fridgeThermometer);
    compressorTemperature = hardware.getTemperature(state.compressorThermometer);
    lastReading = millis();

    String s = ""; s += temperature;
    Serial.printf("Got reading: %s\n", s.c_str());

    if (!isnan(temperature) && !isnan(compressorTemperature) && compressorTemperature < state.compressorLimit)
    {
      if (temperature > state.setpoint + state.epsilon) hardware.setCompressor(true);
      if (temperature < state.setpoint - state.epsilon) hardware.setCompressor(false);
    } else {
      hardware.setCompressor(false);
    }
    thingspeak_log();
  }
}
