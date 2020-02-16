#include "main.h"
#include "networkhandler.h"

unsigned long loopTime = 0;

unsigned long getLoopTime() {
  return loopTime;
}

void initialize() {
  Serial.begin(115200);

  uint8_t* _data = new uint8_t[5];
  memcpy(_data, "Test", 5);
  NH::makeNetworkPacket(0, _data, 5, 255);
  delete _data;
}

void runLoop() {
  loopTime = millis();

  NH::handleNetworkStuff();
  delay(1);
}
