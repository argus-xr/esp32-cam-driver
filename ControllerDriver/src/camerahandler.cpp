#include "camerahandler.h"

namespace CH {

    TaskHandle_t loopTaskHandle = NULL;
    void startCameraHandlerTask() {  
        //messageQueue = xQueueCreate( 5, sizeof( NetMessageOut* ) );
        xTaskCreateUniversal(cameraHandlerLoop, "NetworkLoop", 16000, NULL, 0, &loopTaskHandle, CONFIG_ARDUINO_RUNNING_CORE);
    }

    void cameraHandlerLoop(void *pvParameters) {
        while(true) {
            // do stuff.
            delay(1);
        }
    }
    
}