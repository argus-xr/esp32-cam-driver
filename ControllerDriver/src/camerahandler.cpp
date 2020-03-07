#include "camerahandler.h"
#include "esp_camera.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h" // load up ESP32-Cam pins, but allow switching to other models easily if needed.

#include "networkhandler.h"

#include "argus-netbuffer/BasicMessageProtocol/BasicMessageProtocol.h"
#include "argus-netbuffer/netutils.h"

namespace CH {

    SemaphoreHandle_t semaphore = NULL;

    TaskHandle_t cameraTaskHandle = NULL;
    void startCameraHandlerTask() {  
        xTaskCreateUniversal(cameraHandlerTask, "CameraHandler", 16000, NULL, 0, &cameraTaskHandle, CONFIG_ARDUINO_RUNNING_CORE);
    }

    void cameraHandlerTask(void *pvParameters) {
        semaphore = xSemaphoreCreateMutex();

        initCamera();

        while(true) {
            bmp_handler();
            delay(5000);
        }
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
        config.jpeg_quality = 0; // lower is higher quality.
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
        //initial sensors are flipped vertically and colors are a bit saturated
        if (s->id.PID == OV3660_PID) {
            s->set_vflip(s, 1);//flip it back
            s->set_brightness(s, 1);//up the blightness just a bit
            s->set_saturation(s, -2);//lower the saturation
        }
        //drop down frame size for higher initial frame rate
        //s->set_framesize(s, FRAMESIZE_QVGA);
        s->set_framesize(s, FRAMESIZE_QQVGA);
    }

    esp_err_t bmp_handler() {
        camera_fb_t * fb = NULL;
        esp_err_t res = ESP_OK;
        //int64_t fr_start = esp_timer_get_time();

        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            Serial.println("Camera capture failed");
            return ESP_FAIL;
        }

        uint8_t * buf = NULL;
        size_t buf_len = 0;
        //bool converted = frame2bmp(fb, &buf, &buf_len);
        bool converted = frame2jpg(fb, 0, &buf, &buf_len);
        esp_camera_fb_return(fb);
        if(!converted){
            ESP_LOGE(TAG, "BMP conversion failed");
            Serial.println("BMP conversion failed");
            return ESP_FAIL;
        }

        //res = httpd_resp_set_type(req, "image/x-windows-bmp")
        //   || httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.bmp")
        //   || httpd_resp_send(req, (const char *)buf, buf_len);

        send_buf(buf, buf_len);
        free(buf);
        //int64_t fr_end = esp_timer_get_time();
        ESP_LOGI(TAG, "BMP: %uKB %ums", (uint32_t)(buf_len/1024), (uint32_t)((fr_end - fr_start)/1000));
        return res;
    }

    void send_buf(uint8_t* buf, size_t buf_len) {
        uint8_t varIntLength = ArgusNetUtils::bytesToFitVarInt(buf_len);
        NetMessageOut* msg = new NetMessageOut(buf_len * 2 + varIntLength + 1); // +1 for message type length. buf_len * 2 because of a reallocation bug. TODO: Fix the reallocation code.
        msg->writeVarInt(1);
        msg->writeVarInt(buf_len);
        Serial.printf("Binary blob size: %d, VarInt length: %d.\n", buf_len, varIntLength);
        Serial.printf("Before writing binary blob, length: %d.\n", msg->getContentLength());
        Serial.printf("Available heap space: %d.\n", esp_get_minimum_free_heap_size());
        delay(1);
        uint8_t* copyTestBuf = new uint8_t[buf_len];
        memcpy(copyTestBuf, buf, buf_len);
        delete[] copyTestBuf;
        //msg->writeByteBlob(buf, buf_len > 25000? 25000 : buf_len);
        Serial.printf("Available heap space: %d.\n", esp_get_minimum_free_heap_size());
        msg->writeByteBlob(buf, buf_len);
        Serial.printf("After writing binary blob, length: %d.\n", msg->getContentLength());
        NH::pushNetMessage(msg);
        //delete msg;
    }

}