#include <DallasTemperature.h>

class BigBoiHardware
{
private:
  long cooldown;
public:
  void begin();
  void update();
  void setStatus(int x);
  void setCompressor(int x);
  void setFan(int x);
  int getCompressor();
  float getTemperature(DeviceAddress x);
  bool getThermostatAddress(uint8_t* a, int i);
  void tick(int n);
};
