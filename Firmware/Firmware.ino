#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "private.h"
#include "http_content.h"

const char* host = "api.thingspeak.com";
/* Values set in "private.h" */
// const char* ssid = "home";
// const char* password = "P4aaw0rd55";
// const char* thingspeak_key = "XFMVXWGM1l24K55I";

/* GPIO Mapping */
const int GPIO_ONE_WIRE = 2;
const int GPIO_CHILLER = 0;
const int GPIO_CHILLER_ON = LOW;
const int GPIO_CHILLER_OFF = HIGH;

const int UPDATE_DELAY = 15500;
const int TIMEOUT_NETWORK = 5000;
const int TIMEOUT_THERMOMETER = 5000;

const int THERMOMETER_MIN = -2;
const int THERMOMETER_MAX = 37;

// Runtime State
float temperature;
int chiller = 0;

// Persistent State
int STATE_VERSION = 0;
struct State {
  char id[16];
  int version;
  float setpoint;
  float epsilon;
} state;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(GPIO_ONE_WIRE);
DallasTemperature thermometers(&oneWire);

ESP8266WebServer server(80);

void setup(void)
{
  EEPROM.begin(sizeof(state));
  loadState();

  Serial.begin(115200);
  Serial.println();

  thermometers.begin();
  pinMode(GPIO_CHILLER, OUTPUT);
  digitalWrite(GPIO_CHILLER, GPIO_CHILLER_OFF);

  server.on("/data", handleData);
  load_http_content(server);
  server.begin();
}

void handleData() {
  if (server.method() == HTTP_GET) {
    String url = "{";
    url += "\"temperature\":";
    url += temperature;
    url += ",\"chiller\":";
    url += chiller?"true":"false";
    url += ",\"setpoint\":";
    url += state.setpoint;
    url += ",\"epsilon\":";
    url += state.epsilon;
    url += "}";
    server.send(200, "application/javascript", url);
  } else if (server.method() == HTTP_POST) {
    String arg, msg;
    if(arg = server.arg("setpoint"))
    {
      state.setpoint = arg.toFloat();
      saveState();
      server.send(200, "application/javascript", "{\"result\":\"ok\"}");
    }
  }
}

void connect_wifi(){
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long start = millis();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(50);
      if(millis() - start > TIMEOUT_NETWORK){
        Serial.println("Timeout connecting to network.");
        return;
      }
    }
    Serial.println(WiFi.localIP());
  }
}
unsigned long lastReading = 0;
void loop(void)
{
  connect_wifi();
  server.handleClient();

  unsigned long now = millis();
  if(now - lastReading > UPDATE_DELAY)
  {
    lastReading = now;
    do {
      if(millis() - now > TIMEOUT_THERMOMETER) {
        set_chiller(false);
        Serial.println("Timeout reading from thermometer.");
        return;  
      }
      delay(15);
      temperature = get_temperature();
    } while (temperature < THERMOMETER_MIN || temperature > THERMOMETER_MAX);
  
    if (!chiller && temperature > state.setpoint + state.epsilon) set_chiller(true);
    if ( chiller && temperature < state.setpoint - state.epsilon) set_chiller(false);
    thingspeak_log();
  }
}

float get_temperature()
{
  thermometers.requestTemperatures(); // Send the command to get temperatures
  return thermometers.getTempCByIndex(0);
}

float set_chiller(bool c)
{
  chiller = c;
  digitalWrite(GPIO_CHILLER, chiller ? GPIO_CHILLER_ON : GPIO_CHILLER_OFF);
}

void thingspeak_log(){
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
  url += "&field2=";
  url += chiller?"1":"0";
  url += "&field3=";
  url += state.setpoint;
  url += "&field4=";
  url += state.epsilon;
  Serial.println(host + url);

  delay(10);
  client.print(
    "GET " +
    url +
    " HTTP/1.1\r\n" +
    "Host: " + host + "\r\n" +
    "Connection: close\r\n\r\n");

  while(client.available()){
    String line = client.readStringUntil('\r');
  }
}

char STATE_ID[16] = {0x32, 0x07, 0x0b, 0x4f, 0x08, 0x3c, 0x42, 0x25, 0xa6, 0x09, 0x0c, 0x35, 0x97, 0x92, 0x16, 0x9c};

void loadState() {

  EEPROM.get(0, state);
  if(!(/*memcmp(STATE_ID, state.id, 16) && */STATE_VERSION == state.version)) {
    // Fails ID check, reinitialize
    memcpy(state.id, STATE_ID, 16);
    state.version = STATE_VERSION;
    state.setpoint = 8;
    state.epsilon = 0.2;
    saveState();
  }
}

void saveState() {
  EEPROM.put(0, state);
  yield();
  EEPROM.commit();
}
