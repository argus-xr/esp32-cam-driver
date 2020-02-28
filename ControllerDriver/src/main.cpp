#include "main.h"
#include "networkhandler.h"

#include "argus-netbuffer/BasicMessageProtocol/BasicMessageProtocol.h"

void setup() {
  delay(5000);
  Serial.begin(115200);

  NH::startNetworkHandlerTask();

  NetMessageOut* msg = new NetMessageOut(6);
  msg->writeuint8(0);
  msg->writeVarString("Test\\Hello!");
  NH::pushNetMessage(msg); // don't delete msg, it's passed to another task.
}

void loop() {
  delay(1);
}
