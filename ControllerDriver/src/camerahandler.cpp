#include "camerahandler.h"
#include "esp_camera.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h" // load up ESP32-Cam pins, but allow switching to other models easily if needed.

#include "networkhandler.h"

#include "argus-netbuffer/BasicMessageProtocol/BasicMessageProtocol.h"
#include "argus-netbuffer/netutils.h"
#include <atomic>

namespace CH {

    SemaphoreHandle_t semaphore = NULL;

    TaskHandle_t cameraTaskHandle = NULL;
    std::atomic_bool recordVideo;

    void startCameraHandlerTask() {  
        xTaskCreateUniversal(cameraHandlerTask, "CameraHandler", 16000, NULL, 0, &cameraTaskHandle, tskNO_AFFINITY);
    }

    void cameraHandlerTask(void *pvParameters) {
        semaphore = xSemaphoreCreateMutex();

        recordVideo.store(false);
        initCamera();

        while(true) {
            if(getRecordVideo()) {
                bmp_handler();
                vTaskDelay(pdMS_TO_TICKS( 2000 ));
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

    bool getMutex() {
        if(semaphore != NULL) {
            return (xSemaphoreTake( semaphore, ( TickType_t ) 10 ) == pdTRUE );
        } else {
            return false;
        }
    }

    void giveMutex() {
        if (semaphore != NULL) {
            xSemaphoreGive( semaphore );
        }
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
        //config.pixel_format = PIXFORMAT_RGB888;
        //init with high specs to pre-allocate larger buffers
        config.frame_size = FRAMESIZE_QVGA;
        config.fb_count = 1;

        // camera init
        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            Serial.printf("Camera init failed with error 0x%x", err);
            return;
        }
        
        sensor_t * s = esp_camera_sensor_get();
        //s->set_colorbar(s, 1);
        //initial sensors are flipped vertically and colors are a bit saturated
        if (s->id.PID == OV3660_PID) {
            s->set_vflip(s, 1);//flip it back
            s->set_brightness(s, 1);//up the blightness just a bit
            s->set_saturation(s, -2);//lower the saturation
        }
        //drop down frame size for higher initial frame rate
        //s->set_framesize(s, FRAMESIZE_QVGA);
        //s->set_framesize(s, FRAMESIZE_QQVGA);
    }

    esp_err_t bmp_handler() {
        camera_fb_t * fb = NULL;
        esp_err_t res = ESP_OK;

        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            return ESP_FAIL;
        }

        
        send_buf(fb->buf, fb->len);

        esp_camera_fb_return(fb);

        return res;
    }

    void send_buf(uint8_t* buf, size_t buf_len) {
        Serial.printf("Start send_buf. Length: %d\n", buf_len);
        uint8_t varIntLength = ArgusNetUtils::bytesToFitVarInt(buf_len);
        NetMessageOut* msg = NH::NetworkHandler::newMessage(NH::CTSMessageType::VideoData, buf_len + varIntLength + 1 + 200); // +1 for message type length. buf_len * 2 because of a reallocation bug. TODO: Fix the reallocation code.
        msg->writeVarInt(buf_len);
        msg->writeByteBlob(buf, buf_len);
        NH::NetworkHandler::getInstance()->pushNetMessage(msg);
        // msg deleted elsewhere
    }

}