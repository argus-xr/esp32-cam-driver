#ifndef EEPROMCONFIG_H
#define EEPROMCONFIG_H

#include "nvs_flash.h"
#include <stdint.h>

class EConfig {
    static bool EEPROMInitialized;
    static uint64_t guid;
    
    public:
    static void initEEPROM();
    static void setGuid(uint64_t guid);
    static uint64_t getGuid();
};

#endif // EEPROMCONFIG_H
