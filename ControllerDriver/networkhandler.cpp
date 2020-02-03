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
      
        /*// This will send the request to the server
        client.print("Test");
        unsigned long timeout = millis();
        while (client.available() == 0) {
          if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
          }
        }*/
      
        // Read all the lines of the reply from server and print them to Serial
        while (client.available()) {
          String line = client.readStringUntil('\r');
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

bool isConnected() {
  return (wStatus == WL_CONNECTED);
}
