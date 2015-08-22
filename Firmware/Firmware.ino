#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include "private.h"

const char* host = "api.thingspeak.com";

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

float temperature;
float setpoint = 15.55;
float epsilon = 0.5;

int compressor = 0;

void setup(void)
{
  // start serial port
  Serial.begin(115200);
  Serial.println();
  // Start up the library
  sensors.begin();
  pinMode(0, OUTPUT);
}

void connect_wifi(){
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long start = millis();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(50);
      if(millis() - start > 5000){
        Serial.println("Timeout connecting to network.");
        return;
      }
    }
    Serial.println(WiFi.localIP());
  }
}

void loop(void)
{
  delay(15000);
  connect_wifi();
  temperature = get_temperature();
  if(temperature < -25){
    compressor = 0;
    set_compressor();
    return;
  }
  if (!compressor && temperature > setpoint + epsilon) compressor = 1;
  if ( compressor && temperature < setpoint - epsilon) compressor = 0;
  set_compressor();
  thingspeak_log();
}

float get_temperature()
{
  sensors.requestTemperatures(); // Send the command to get temperatures
  return sensors.getTempCByIndex(0);
}

float set_compressor()
{
  digitalWrite(0, !compressor);
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
  url += compressor?"1":"0";
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
