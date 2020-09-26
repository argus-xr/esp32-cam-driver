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
    EEPROM.writeULong64(0, guid);
}

uint64_t EConfig::getGuid() {
    if(guid == 0) {
        guid = EEPROM.readULong64(0);
    }
    return guid;
}