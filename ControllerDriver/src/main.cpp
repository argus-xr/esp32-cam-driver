#include "main.h"
#include "networkhandler.h"
#include "camerahandler.h"

#include "argus-netbuffer/BasicMessageProtocol/BasicMessageProtocol.h"

void setup() {
    try {
        delay(2000);
        Serial.begin(115200);

        NH::startNetworkHandlerTask();
        CH::startCameraHandlerTask();
    } catch(std::exception e) {
        Serial.printf("Exception in setup(): %s", e.what());
    }
}

void loop() {
    delay(1000);
}
