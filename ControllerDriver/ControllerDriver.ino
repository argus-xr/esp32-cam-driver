/*
    This sketch sends data via HTTP GET requests to data.sparkfun.com service.

    You need to get streamId and privateKey at data.sparkfun.com and paste them
    below. Or just customize this script to talk to other HTTP servers.

*/

#include <WiFi.h>
#include "networkconfig.h"

const char* ssid     = NETSSID; // set up your networkconfig.h for these values, see networkconfig_sample.h
const char* password = NETPASS;
const char* host = NETHOST;

void setup()
{
  Serial.begin(115200);
  delay(10);
}

int value = 0;

void loop()
{
  int wStatus = WiFi.status();

  if(wStatus != WL_CONNECTED && wStatus != WL_IDLE_STATUS) {
    WiFi.begin(ssid, password);
    Serial.println("Reconnecting to wifi.");
    delay(500);
    wStatus = WiFi.status();
  }
  if(wStatus == WL_CONNECTED) {
    ++value;
  
    Serial.print("connecting to ");
    Serial.println(host);
  
    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int serverPort = 11000;
    if (!client.connect(host, serverPort)) {
      Serial.println("connection failed");
      return;
    }
  
    // This will send the request to the server
    client.print("Test");
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }
  
    // Read all the lines of the reply from server and print them to Serial
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
  
    Serial.println();
    Serial.println("closing connection");
  
    delay(5000);
  }
}
