#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H

#include <Arduino.h>

typedef struct Packet {
	uint8_t* data;
  uint32_t length;
	uint8_t priority;
};

void handleNetworkStuff();
void pushNetworkPacket(Packet* packet);
bool isConnected();

#endif
