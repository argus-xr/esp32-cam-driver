#include "networkhandler.h"
#include "queue.h"
#include <WiFi.h>
#include "networkconfig.h"
#include "main.h"
//#include "utility/util.h"

namespace NH {
  
const char* ssid     = NETSSID;
const char* password = NETPASS;
const char* host = NETHOST;

const int serverPort = 11000;

unsigned long waitUntil = 0;

// Use WiFiClient class to create TCP connections
WiFiClient client;

int wStatus = WL_IDLE_STATUS;
QueueArray<struct Packet*> buffer;

#define readBufferSize 1500
uint8_t readBuf[readBufferSize];

uint8_t* inputBuffer = NULL;
uint32_t inputCurrentLength = 0;
uint32_t inputMaxLength = 0;
#define inputSizeStep 64

void handleNetworkStuff() {
  if (waitUntil < getLoopTime()) {
    wStatus = WiFi.status();
  
    if (wStatus != WL_CONNECTED && wStatus != WL_IDLE_STATUS) {
      WiFi.begin(ssid, password);
      Serial.print("Current status: ");
      Serial.print(wStatus);
      Serial.print(". Reconnecting to ");
      Serial.println(ssid);
      waitUntil = getLoopTime() + 500;
    }
    
    if (wStatus == WL_CONNECTED) {
      if (client.connected()) {
        Packet* p = buffer.pop();
        if (p != NULL) {
          int sent = client.write(p->data, p->length);
          Serial.print("Bytes sent: ");
          Serial.println(sent);
          delete[] p->data;
          delete p;
        }
      
        checkMessages();
      } else {
        if (client.connect(host, serverPort)) {
          Serial.print("Connected to ");
          Serial.println(host);
          client.setNoDelay(true); // consider setSync(true): flushes each write, slower but does not allocate temporary memory.
        } else {
          Serial.print("Failed to connect to ");
          Serial.println(host);
        }
      }
    }
  }
}

void pushNetworkPacket(Packet* packet) {
  buffer.enqueue(packet);
}

void makeNetworkPacket(const uint8_t &type, const uint8_t* contents, uint16_t contentLength, const uint8_t &priority) {
  assert(contentLength <= 1500); // TODO: implement a multi-packet message type.
  Packet* p = new Packet();
  
  uint16_t pSize = contentLength + 5;
  p->data = new uint8_t[pSize]; // 2 for ||, 2 for length, 1 for type.
  p->data[0] = '|';
  writeNumberToBuffer<uint16_t>(contentLength, p->data, 1);
  writeNumberToBuffer<uint8_t>(type, p->data, 3);
  memcpy(&p->data[4], contents, contentLength);
  p->data[pSize - 1] = '|';
  p->length = pSize;
  p->priority = priority;
  pushNetworkPacket(p);
}

bool isConnected() {
  return (wStatus == WL_CONNECTED);
}

template <>
void swapEndian<uint32_t>(uint32_t &val) {
    val = htonl(val);
}
template <>
void swapEndian<uint16_t>(uint16_t &val) {
    val = htons(val);
}
template <>
void swapEndian<uint8_t>(uint8_t &val) {
    // do nothing.
}
template <>
void swapEndian<int8_t>(int8_t &val) {
    // do nothing.
}

template <typename T>
void writeNumberToBuffer(T val, uint8_t* buf, const uint16_t &pos) {
  swapEndian(val);
  memcpy(&buf[pos], &val, sizeof(T));
}

template <typename T>
T readNumberFromBuffer(uint8_t* buf, const uint16_t &pos) {
  T result;
  memcpy(&result, &buf[pos], sizeof(T));
  swapEndian(result);
  return result;
}

void checkMessages() {
  bool newData = false;
  while (client.available()) {
    int bytes = client.read(readBuf, readBufferSize);
    if (bytes <= 0) {
      break;
    } else {
      newData = true;
    }
    
    if (inputMaxLength < inputCurrentLength + bytes) { // resize buffer if needed
      uint8_t* tmp = inputBuffer;
      inputMaxLength = inputCurrentLength + bytes + inputSizeStep;
      inputBuffer = new uint8_t[inputMaxLength];
      if (tmp != NULL) {
        memcpy(inputBuffer, tmp, inputCurrentLength);
        delete[] tmp;
      }
    }
    memcpy(inputBuffer + inputCurrentLength, readBuf, bytes); // add newly read data to buffer
    inputCurrentLength += bytes;
  }

  if (newData) {
    // TODO process messages? We could probably call processMessage(inputBuffer, inputCurrentLength) here, but I haven't checked yet and I'm currently just patching the leaks in my PR.
  }
}

void processMessage(uint8_t* message, uint16_t len) {
  if (len < 5) {
    return;
  }
  if (message[0] != '|') {
    return;
  }
  uint16_t msgLen = readNumberFromBuffer<uint16_t>(message, 1);
  // TODO: finish this function. It's supposed to split the receive buffer into discrete messages and pass them into relevant parsing functions.
}

} // namespace NH
