#include "networkhandler.h"
#include "queue.h"
#include <WiFi.h>
#include "networkconfig.h"
#include "main.h"

const char* ssid     = NETSSID;
const char* password = NETPASS;
const char* host = NETHOST;

const int serverPort = 11000;

unsigned long waitUntil = 0;

// Use WiFiClient class to create TCP connections
WiFiClient client;

int wStatus = WL_IDLE_STATUS;
QueueArray<struct Packet*> buffer;

void handleNetworkStuff() {
  if(waitUntil < getLoopTime()) {
    wStatus = WiFi.status();
  
    if(wStatus != WL_CONNECTED && wStatus != WL_IDLE_STATUS) {
      WiFi.begin(ssid, password);
      Serial.print("Current status: ");
      Serial.print(wStatus);
      Serial.print(". Reconnecting to ");
      Serial.println(ssid);
      waitUntil = getLoopTime() + 500;
    }
    
    if(wStatus == WL_CONNECTED) {
      if(client.connected()) {
        Packet* p = buffer.pop();
        if(p != NULL) {
          int sent = client.write(p->data, p->length);
          Serial.print("Bytes sent: ");
          Serial.println(sent);
          delete[] p->data;
          delete p;
        }
      
        // Read all the lines of the reply from server and print them to Serial
        while (client.available()) {
          String line = client.readStringUntil('\0');
          Serial.print(line);
        }
      } else {
        if(client.connect(host, serverPort)) {
          Serial.print("Connected to ");
          Serial.println(host);
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

template <typename T>
void swapEndian(T &val) {
    union U {
        T val;
        std::array<std::uint8_t, sizeof(T)> raw;
    } src, dst;

    src.val = val;
    std::reverse_copy(src.raw.begin(), src.raw.end(), dst.raw.begin());
    val = dst.val;
}

template <typename T>
inline void swapEndianIfNeeded(T &val) {
  // ifdef to determine byte order? ESP32 and Arduino is little-endian and needs a byte swap for network byte order.
  swapEndian(val);
}

template <typename T>
void writeNumberToBuffer(T val, uint8_t* buf, const uint16_t &pos) {
  swapEndianIfNeeded(val);
  memcpy(&buf[pos], (uint8_t*) &val, sizeof(T));
}
