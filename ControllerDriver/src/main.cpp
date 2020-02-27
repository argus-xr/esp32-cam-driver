#include "main.h"
#include "networkhandler.h"

#include "argus-netbuffer/BasicMessageProtocol/BasicMessageProtocol.h"

unsigned long loopTime = 0;

unsigned long getLoopTime() {
  return loopTime;
}

void setup() {
  delay(5000);
  Serial.begin(115200);

  NH::startNetworkHandlerTask();

  NetMessageOut* msg = new NetMessageOut(6);
  msg->writeuint8(0);
  msg->writeVarString("Test");
  NH::makeNetworkPacket(msg);
  delete msg;

  /*uint8_t* _data = new uint8_t[5];
  memcpy(_data, "Test", 5);
  NH::makeNetworkPacket(0, _data, 5, 255);
  delete _data;*/
}

void loop() {
  loopTime = millis();

  /*try {
    NH::handleNetworkStuff();
  } catch (const std::exception e) {
    Serial.println(e.what());
  }*/
  delay(1);
}
