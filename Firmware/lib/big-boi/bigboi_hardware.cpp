#include <OneWire.h>
#include <DallasTemperature.h>
#include "bigboi_hardware.h"

#ifndef DEBUG_ESP_PORT
#define DEBUG_ESP_PORT Serial
#endif

#define DEBUG(...) DEBUG_ESP_PORT.printf( "[hardware] " __VA_ARGS__ )

// Pin configuration
const uint8_t pinStatus = 14;
const uint8_t pinFan = 13;
const uint8_t pinCompressor = 12;

const uint8_t pinOneWire = 5;

// Constants
const int thermometerMax = 85;
const int thermometerMin = -20;
const int compressorCooldown = 60*1000;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(pinOneWire);
DallasTemperature thermometers(&oneWire);

void BigBoiHardware::begin()
{
  pinMode(pinStatus, OUTPUT);
  pinMode(pinFan, OUTPUT);
  pinMode(pinCompressor, OUTPUT);

  setStatus(0);
  setCompressor(0);
  setFan(0);

  thermometers.begin();
  thermometers.setResolution(11);
}

void BigBoiHardware::setStatus(int x)
{
  DEBUG("Turning status LED %s\n", x?"on":"off");
  digitalWrite(pinStatus, x);
}

void BigBoiHardware::setFan(int x)
{
  DEBUG("Turning fan %s\n", x?"on":"off");
  digitalWrite(pinFan, x);
}

void BigBoiHardware::setCompressor(int x)
{
  if(digitalRead(pinCompressor) != !!x)
  {
    if(x) {
      if(millis() > cooldown)
      {
        DEBUG("Turning compressor on\n");
        digitalWrite(pinCompressor, HIGH);
      }
      else {
        DEBUG("Compressor still cooling down.\n");
      }
    }
    else
    {
      DEBUG("Turning compressor off\n");
      cooldown = millis() + compressorCooldown;
      digitalWrite(pinCompressor, LOW);
    }
  }
}

int BigBoiHardware::getCompressor()
{
  return digitalRead(pinCompressor);
}

float BigBoiHardware::getTemperature(DeviceAddress x)
{
  thermometers.requestTemperatures(); // Send the command to get temperatures
  float f = thermometers.getTempC((uint8_t*)x);
  if(f > thermometerMax || f < thermometerMin) return NAN;
  return f;
}

bool BigBoiHardware::getThermostatAddress(uint8_t* a, int i) {
  return thermometers.getAddress(a, i);
}

void BigBoiHardware::tick(int n){
  int t = 80;
  int r = digitalRead(pinCompressor);
  for(int i = 0; i < n; i++) {
    digitalWrite(pinCompressor, HIGH);
    delay(t);
    digitalWrite(pinCompressor, LOW);
    delay(t);
  }
  digitalWrite(r, LOW);
}
