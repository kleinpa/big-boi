#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "private.h"
#include "http_index.h"

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

// State
float temperature;
float setpoint = 20;
float epsilon = 0.2;
int chiller = 0;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(GPIO_ONE_WIRE);
DallasTemperature thermometers(&oneWire);

ESP8266WebServer server(80);

void setup(void)
{
  Serial.begin(115200);
  Serial.println();

  thermometers.begin();
  pinMode(GPIO_CHILLER, OUTPUT);
  digitalWrite(GPIO_CHILLER, GPIO_CHILLER_OFF);

  server.on("/data", handleData);
  server.on("/", []() {
    server.send(200, "text/html", http_index);
  } );
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
    url += setpoint;
    url += ",\"epsilon\":";
    url += epsilon;
    url += "}";
    server.send(200, "application/javascript", url);
  } else if (server.method() == HTTP_POST) {
    String arg, msg;
    if(arg = server.arg("setpoint"))
    {
      setpoint = arg.toFloat();
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
  
    if (!chiller && temperature > setpoint + epsilon) set_chiller(true);
    if ( chiller && temperature < setpoint - epsilon) set_chiller(false);
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
  url += setpoint;
  url += "&field4=";
  url += epsilon;
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
