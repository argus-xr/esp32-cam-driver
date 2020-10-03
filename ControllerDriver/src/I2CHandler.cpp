#include "I2CHandler.h"

#include <Wire.h>
#include <Arduino.h>

namespace IH {

const uint8_t IMU_addr = 0x68;

TaskHandle_t I2CTaskHandle = NULL;

bool awake = false;

void startI2CHandlerTask() {
xTaskCreateUniversal(I2CHandlerTask, "I2CHandler", 60000, NULL, 0, &I2CTaskHandle, tskNO_AFFINITY);
}

void I2CHandlerTask(void *pvParameters) {
    Wire.begin(GPIO_NUM_15, GPIO_NUM_14);

    while(true) {
        if(!awake) {
            Wire.beginTransmission(IMU_addr);
            Wire.write(0x6B); // PWR_MGMT_1 register
            Wire.write(0); // wake up ya lazy git
            uint8_t error = Wire.endTransmission();
            if(error == 0) {
                awake = true;
                Serial.printf("I2C device found at address %u.\n", IMU_addr);
            } else {
                vTaskDelay(pdMS_TO_TICKS( 100 ));
            }
        }
        if(awake) {
            
            Wire.beginTransmission(IMU_addr);
            Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
            Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
            Wire.requestFrom(IMU_addr, 7*2, true); // request a total of 7*2=14 registers
            
            // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
            int16_t aX = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
            int16_t aY = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
            int16_t aZ = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
            int16_t tmp = Wire.read()<<8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
            int16_t gX = Wire.read()<<8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
            int16_t gY = Wire.read()<<8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
            int16_t gZ = Wire.read()<<8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)

            Serial.printf("aX = %6d, aY = %6d, aZ = %6d, tmp = %6d, gX = %6d, gY = %6d, gZ = %6d.\n", aX, aY, aZ, tmp, gX, gY, gZ);
            vTaskDelay(pdMS_TO_TICKS( 100 ));
        }
        taskYIELD();
    }
}

}