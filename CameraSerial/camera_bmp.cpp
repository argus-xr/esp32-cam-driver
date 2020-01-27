#include "esp_camera.h"
#include "esp_timer.h"
#include <HardwareSerial.h>

char* HEX_CHARS = "0123456789ABCDEF";

void send_buf(uint8_t* buf, size_t buf_len) {
  Serial.print("buf_len: ");
  Serial.println(buf_len);
  uint8_t line_len = 64;
  char line[line_len + 1];  // Extra slot for \0
  uint8_t line_ptr = 0;
  for (size_t i = 0; i < buf_len; i++) {
    line[line_ptr++] = HEX_CHARS[(buf[i] >> 4) & 0x0f];
    line[line_ptr++] = HEX_CHARS[buf[i] & 0x0f];
    if (line_ptr >= line_len - 1) {
      line[line_ptr++] = '\0';
      Serial.println(line);
      line_ptr = 0;
    }
  }
  if (line_ptr > 0) {
    line[line_ptr++] = '\0';
    Serial.println(line);
  }
}

esp_err_t bmp_handler(){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    int64_t fr_start = esp_timer_get_time();

    fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        Serial.println("Camera capture failed");
        return ESP_FAIL;
    }

    uint8_t * buf = NULL;
    size_t buf_len = 0;
    bool converted = frame2bmp(fb, &buf, &buf_len);
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
    int64_t fr_end = esp_timer_get_time();
    ESP_LOGI(TAG, "BMP: %uKB %ums", (uint32_t)(buf_len/1024), (uint32_t)((fr_end - fr_start)/1000));
    return res;
}
