#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H

#include <Arduino.h>

namespace CH {

    void startCameraHandlerTask();
    void cameraHandlerTask(void *pvParameters);

    void setRecordVideo(bool record);
    bool getRecordVideo();

    bool getMutex();
    void giveMutex();

    void initCamera();
    void send_buf(uint8_t* buf, size_t buf_len);
    esp_err_t bmp_handler();

}

#endif // CAMERAHANDLER_H