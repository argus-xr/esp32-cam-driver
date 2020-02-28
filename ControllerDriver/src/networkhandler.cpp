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

// Use WiFiClient class to create TCP connections
WiFiClient client;

int wStatus = WL_IDLE_STATUS;
QueueArray<struct Packet*> buffer;

#define readBufferSize 1500
uint8_t readBuf[readBufferSize];

BasicMessageBuffer* buf = new BasicMessageBuffer();

void handleNetworkStuff() {
  try {
    wStatus = WiFi.status();

    if (wStatus != WL_CONNECTED && wStatus != WL_IDLE_STATUS) {
      WiFi.begin(ssid, password);
      Serial.print("Current status: ");
      Serial.print(wStatus);
      Serial.print(". Reconnecting to ");
      Serial.println(ssid);
      delay(500);
    }
    
    if (wStatus == WL_CONNECTED) {
      if (client.connected()) {
        if (!buffer.isEmpty()) {
          try {
            Packet* p = buffer.pop();
            if (p != nullptr) {
              if (p->length > 0 && p->data != NULL && p->data != nullptr) {
                Serial.println("Sending message.");
                int sent = client.write(p->data, p->length);
                Serial.print("Bytes sent: ");
                Serial.println(sent);
              }
              if(p->data != NULL) {
                delete[] p->data;
              }
              delete p;
            }
          } catch (std::exception e) {
            Serial.println(e.what());
          }
        }

        try {
          checkMessages();
        } catch (std::exception e) {
          Serial.println(e.what());
        }
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
  } catch (std::exception e) {
    Serial.print("Exception in network handler: ");
    Serial.println(e.what());
  }
}

void pushNetworkPacket(Packet* packet) {
  buffer.enqueue(packet);
}

void makeNetworkPacket(NetMessageOut* msg) {
  Packet* p = new Packet();
  p->length = buf->messageOutToByteArray(p->data, msg);
  pushNetworkPacket(p);
}

bool isConnected() {
  return (wStatus == WL_CONNECTED);
}

void checkMessages() {
  while (client.available()) {
    int bytes = client.read(readBuf, readBufferSize);
    if (bytes <= 0) {
      break;
    } else {
      buf->insertBuffer(readBuf, bytes, true);
      buf->checkMessages();
    }
  }
  NetMessageIn* msg = nullptr;
  do {
    msg = buf->popMessage();
    if(msg != nullptr) {
      processMessage(msg);
    }
    delete msg;
  } while(msg != nullptr);
}

void processMessage(NetMessageIn* msg) {
  uint8_t type = msg->readVarInt();
  Serial.println("Message received.");
  Serial.printf("Length: %d, type: %d.\n", msg->getInternalBufferLength(), type);
  switch(type) {
    case 0:
      std::string text = msg->readVarString();
      Serial.printf("Message: %s\n", text.c_str());
      break;
  }
}
  

TaskHandle_t loopTaskHandle = NULL;

void startNetworkHandlerTask() {
  xTaskCreateUniversal(networkHandlerLoop, "NetworkLoop", 16000, NULL, 0, &loopTaskHandle, CONFIG_ARDUINO_RUNNING_CORE);
}

void networkHandlerLoop(void *pvParameters) {
  while(true) {
    handleNetworkStuff();
    delay(1);
  }
}

} // namespace NH
