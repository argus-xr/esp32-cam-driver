#include "main.h"
#include "networkhandler.h"

unsigned long loopTime = 0;

unsigned long getLoopTime() {
  return loopTime;
}

void setup() {
  Serial.begin(115200);

  uint8_t* _data = new uint8_t[5];
  memcpy(_data, "Test", 5);
  NH::makeNetworkPacket(0, _data, 5, 255);
  delete _data;
}

void loop() {
  loopTime = millis();

  NH::handleNetworkStuff();
  delay(1);
}
