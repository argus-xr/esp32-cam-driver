#include "I2CHandler.h"

#include <Wire.h>
#include <Arduino.h>
#include <atomic>

#include <time.h>
#include "argus-netbuffer/netutils.h"
#include "networkhandler.h"

namespace IH {

const uint8_t IMU_addr = 0x68;

TaskHandle_t I2CTaskHandle = NULL;

bool awake = false;
std::vector<IMUData*> storedData;

uint64_t lastSend = 0;

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

            // Configure Accelerometer Sensitivity - Full Scale Range (default +/- 2g)
            Wire.beginTransmission(IMU_addr);
            Wire.write(0x1C);                  //Talk to the ACCEL_CONFIG register (1C hex)
            Wire.write(0x10);                  //Set the register bits as 00010000 (+/- 8g full scale range)
            Wire.endTransmission(true);
            // Configure Gyro Sensitivity - Full Scale Range (default +/- 250deg/s)
            Wire.beginTransmission(IMU_addr);
            Wire.write(0x1B);                   // Talk to the GYRO_CONFIG register (1B hex)
            Wire.write(0x10);                   // Set the register bits as 00010000 (1000deg/s full scale)
            Wire.endTransmission(true);
        }
        if(awake) {
            readIMUData();

            if(esp_timer_get_time() - lastSend > 10000) {
                sendIMUData();
            }
            //Serial.printf("aX = %6d, aY = %6d, aZ = %6d, tmp = %6d, gX = %6d, gY = %6d, gZ = %6d.\n", aX, aY, aZ, tmp, gX, gY, gZ);
            vTaskDelay(pdMS_TO_TICKS( 10 ));
        }
        taskYIELD();
    }
}

void readIMUData() {
    if(!NH::NetworkHandler::getInstance()->isConnected()) {
        return;
    }
    Wire.beginTransmission(IMU_addr);
    Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
    Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
    Wire.requestFrom((int) IMU_addr, (int) 7*2, (int) 1); // request a total of 7*2=14 registers

    IMUData* d = new IMUData();
    
    // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
    d->aX = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
    d->aY = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
    d->aZ = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
    Wire.read(); Wire.read(); // skip these! // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
    d->gX = Wire.read()<<8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
    d->gY = Wire.read()<<8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
    d->gZ = Wire.read()<<8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)

    d->timestamp_us = esp_timer_get_time();
    
    storedData.push_back(d);
}

void sendIMUData() {
    if(!NH::NetworkHandler::getInstance()->isConnected()) {
        return;
    }
    if(storedData.size() > 0) {
        uint64_t num = storedData.size();
        uint8_t varIntLength = ArgusNetUtils::bytesToFitVarInt(num);
        NetMessageOut* msg = NH::NetworkHandler::newMessage(NH::CTSMessageType::IMUData, num * 14 + varIntLength);
        msg->writeVarInt(num);
        for(int i = 0; i < num; ++i) {
            IMUData* d = storedData[i];
            msg->writeVarIntSigned((int64_t) d->aX);
            msg->writeVarIntSigned((int64_t) d->aY);
            msg->writeVarIntSigned((int64_t) d->aZ);
            msg->writeVarIntSigned((int64_t) d->gX);
            msg->writeVarIntSigned((int64_t) d->gY);
            msg->writeVarIntSigned((int64_t) d->gZ);
            msg->writeVarInt(d->timestamp_us);

            //printf("IMU: %6.2f %6.2f %6.2f\n", d->aX * 8.0f * 9.81f / INT16_MAX, d->aY * 8.0f * 9.81f / INT16_MAX, d->aZ * 8.0f * 9.81f / INT16_MAX);

            delete d;
        }
        storedData.clear();

        NH::NetworkHandler::getInstance()->pushNetMessage(msg);
        lastSend = esp_timer_get_time();
    }
}

}