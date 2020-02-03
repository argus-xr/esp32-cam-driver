#include "main.h"
#include "networkhandler.h"

unsigned long loopTime = 0;

unsigned long getLoopTime() {
  return loopTime;
}

void initialize() {
  Serial.begin(115200);

  Packet* p = new Packet();
  p->data = new uint8_t[5];
  memcpy(p->data, "Test", 5);
  p->length = 5;
  p->priority = 255;
  pushNetworkPacket(p);
}

void runLoop() {
  loopTime = millis();

  handleNetworkStuff();
  delay(1);
}
