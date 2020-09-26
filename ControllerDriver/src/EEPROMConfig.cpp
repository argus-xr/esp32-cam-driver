#include "EEPROMConfig.h"

#define EEPROM_SIZE 8

bool EConfig::EEPROMInitialized = false;

uint64_t EConfig::guid = 0;

void EConfig::initEEPROM() {
    if(!EEPROMInitialized) {
        EEPROM.begin(EEPROM_SIZE);
    }
}

void EConfig::setGuid(uint64_t guid) {
    initEEPROM();
    EEPROM.writeULong64(0, guid);
    EEPROM.commit();
}

uint64_t EConfig::getGuid() {
    initEEPROM();
    if(guid == 0) {
        guid = EEPROM.readULong64(0);
    }
    return guid;
}