#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H

#include <Arduino.h>

namespace NH {
  typedef struct Packet {
  	uint8_t* data;
    uint32_t length;
  	uint8_t priority;
  };
  
  void handleNetworkStuff();
  
  void pushNetworkPacket(Packet* packet);
  void makeNetworkPacket(const uint8_t &type, const uint8_t* contents, uint16_t contentLength, const uint8_t &priority);
  bool isConnected();
  
  template <typename T>
  void swapEndian(T &val);
  
  template <typename T>
  void writeNumberToBuffer(T val, uint8_t* buf, const uint16_t &pos);
  
  template <typename T>
  T readNumberFromBuffer(uint8_t* buf, const uint16_t &pos);

  void checkMessages();
  
  void processMessage(uint8_t* message, uint16_t len);
}

#endif
