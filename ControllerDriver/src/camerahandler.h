#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H

#include <Arduino.h>

namespace CH {

    void startCameraHandlerTask();
    void cameraHandlerLoop(void *pvParameters);

}

#endif // CAMERAHANDLER_H