#include "main.h"
#include "networkhandler.h"
#include "camerahandler.h"

#include "argus-netbuffer/BasicMessageProtocol/BasicMessageProtocol.h"

void setup() {
    try {
        vTaskDelay(pdMS_TO_TICKS( 2000 ));
        Serial.begin(115200);
        vTaskDelay(pdMS_TO_TICKS( 1000 ));
        NH::startNetworkHandlerTask();
        vTaskDelay(pdMS_TO_TICKS( 1000 ));
        CH::startCameraHandlerTask();
    } catch(std::exception e) {
        Serial.printf("Exception in setup(): %s", e.what());
    }
}

void loop() {
}
