#include "main.h"
#include "networkhandler.h"
#include "camerahandler.h"

#include "argus-netbuffer/BasicMessageProtocol/BasicMessageProtocol.h"

void setup() {
  try {
    delay(5000);
    Serial.begin(115200);

    NH::startNetworkHandlerTask();
    CH::startCameraHandlerTask();

    NetMessageOut* msg = new NetMessageOut(6);
    msg->writeuint8(0);
    msg->writeVarString("Test\\Hello!");
    NH::pushNetMessage(msg); // don't delete msg, it's passed to another task.
  } catch(std::exception e) {
    Serial.printf("Exception in setup(): %s", e.what());
  }
}

void loop() {
  delay(1);
}
