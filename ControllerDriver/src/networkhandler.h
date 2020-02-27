#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H

#include <Arduino.h>
#include "argus-netbuffer/BasicMessageProtocol/BasicMessageProtocol.h"

namespace NH {
  struct Packet {
  	uint8_t* data;
    uint32_t length;
  };
  
  void handleNetworkStuff();
  
  void pushNetworkPacket(Packet* packet);
  void makeNetworkPacket(NetMessageOut* msg);
  bool isConnected();

  void checkMessages();
  
  void processMessage(NetMessageIn* msg);

  void startNetworkHandlerTask();
  void networkHandlerLoop(void *pvParameters);
}

#endif
