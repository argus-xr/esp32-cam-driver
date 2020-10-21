#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <exception>
#include "networkhandler.h"
#include "camerahandler.h"
#include "I2CHandler.h"

#include "wifihandler.h"
#include "Arduino.h"

extern "C" void app_main(void) {
    try {
    	initArduino();
        vTaskDelay(pdMS_TO_TICKS( 2000 ));
        NH::startNetworkHandlerTask();
        vTaskDelay(pdMS_TO_TICKS( 1000 ));
        IH::startI2CHandlerTask();
        vTaskDelay(pdMS_TO_TICKS( 1000 ));
        CH::startCameraHandlerTask();
	} catch (std::exception &e) {
        printf("Exception in setup(): %s", e.what());
    }
}
