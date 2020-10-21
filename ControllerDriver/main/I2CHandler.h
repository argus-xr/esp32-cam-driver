#ifndef I2CHANDLER_H
#define I2CHANDLER_H

#include <vector>
#include <stdint.h>

namespace IH {

void startI2CHandlerTask();
void I2CHandlerTask(void *pvParameters);

void readIMUData();
void sendIMUData();

struct IMUData {
    int16_t aX, aY, aZ;
    int16_t gX, gY, gZ;
    uint64_t timestamp_us; // microseconds
};

}

#endif // I2CHANDLER_H
