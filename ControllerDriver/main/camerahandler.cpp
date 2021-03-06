#include "camerahandler.h"
#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h" // load up ESP32-Cam pins, but allow switching to other models easily if needed.

#include "networkhandler.h"

#include "argus-netbuffer/BasicMessageProtocol/BasicMessageProtocol.h"
#include "argus-netbuffer/netutils.h"
#include <atomic>

namespace CH {
    TaskHandle_t cameraTaskHandle = NULL;
    std::atomic_bool recordVideo;

    void startCameraHandlerTask() {
        printf("Starting camera handler task.\n");
        xTaskCreate(cameraHandlerTask, "CameraHandler", 60000, nullptr, 0, &cameraTaskHandle);
    }

    void cameraHandlerTask(void *pvParameters) {
        recordVideo.store(false);
        printf("Initializing camera.\n");
        initCamera();

        while(true) {
            if(getRecordVideo()) {
                bmp_handler();
                vTaskDelay(pdMS_TO_TICKS( 50 ));
                //taskYIELD();
            } else {
                vTaskDelay(pdMS_TO_TICKS( 10 ));
            }
        }
    }
    
    void setRecordVideo(bool record) {
        recordVideo.store(record);
    }

    bool getRecordVideo() {
        return recordVideo;
    }

    void initCamera() {
        camera_config_t config;
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer = LEDC_TIMER_0;
        config.pin_d0 = Y2_GPIO_NUM;
        config.pin_d1 = Y3_GPIO_NUM;
        config.pin_d2 = Y4_GPIO_NUM;
        config.pin_d3 = Y5_GPIO_NUM;
        config.pin_d4 = Y6_GPIO_NUM;
        config.pin_d5 = Y7_GPIO_NUM;
        config.pin_d6 = Y8_GPIO_NUM;
        config.pin_d7 = Y9_GPIO_NUM;
        config.pin_xclk = XCLK_GPIO_NUM;
        config.pin_pclk = PCLK_GPIO_NUM;
        config.pin_vsync = VSYNC_GPIO_NUM;
        config.pin_href = HREF_GPIO_NUM;
        config.pin_sscb_sda = SIOD_GPIO_NUM;
        config.pin_sscb_scl = SIOC_GPIO_NUM;
        config.pin_pwdn = PWDN_GPIO_NUM;
        config.pin_reset = RESET_GPIO_NUM;
        config.xclk_freq_hz = 20000000;
        config.jpeg_quality = 20; // lower is higher quality.
        config.pixel_format = PIXFORMAT_JPEG;
        config.frame_size = FRAMESIZE_QVGA;
        //config.fb_count = 2;

        // camera init
        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            printf("Camera init failed with error 0x%x", err);
            return;
        }
		printf("Camera initialized.\n");
    }

    esp_err_t bmp_handler() {
        camera_fb_t * fb = nullptr;
        esp_err_t res = ESP_OK;

        fb = esp_camera_fb_get();
        if (!fb) {
            printf("Camera capture failed.\n");
            return ESP_FAIL;
        }
        
        send_buf(fb->buf, fb->len);

        esp_camera_fb_return(fb);

        return res;
    }

    void send_buf(uint8_t* buf, size_t buf_len) {
        struct timeval tv_now;
        gettimeofday(&tv_now, NULL);
        int64_t time_ms = (int64_t)tv_now.tv_sec * 1000L + (int64_t)tv_now.tv_usec / 1000L;

        printf("Start send_buf. Length: %d, time %llu.\n", buf_len, time_ms);
        uint8_t varIntLength = ArgusNetUtils::bytesToFitVarInt(buf_len);
        NetMessageOut* msg = NH::NetworkHandler::newMessage(NH::CTSMessageType::VideoData, buf_len * 2 + varIntLength + 1 + 200); // +1 for message type length. buf_len * 2 because of a reallocation bug. TODO: Fix the reallocation code.
        msg->writeVarInt(time_ms);
        msg->writeVarInt(buf_len);
        msg->writeByteBlob(buf, buf_len);
        NH::NetworkHandler::getInstance()->pushNetMessage(msg);
        
        printf("End send_buf.\n");
        // msg deleted elsewhere
    }

}
