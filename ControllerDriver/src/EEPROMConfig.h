#ifndef EEPROMCONFIG_H
#define EEPROMCONFIG_H

#include "EEPROM.h"

class EConfig {
    static bool EEPROMInitialized;
    static uint64_t guid;
    static void initEEPROM();
    
    public:
    static void setGuid(uint64_t guid);
    static uint64_t getGuid();
};

#endif // EEPROMCONFIG_H