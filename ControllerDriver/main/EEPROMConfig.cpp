#include "EEPROMConfig.h"

#define EEPROM_SIZE 8

bool EConfig::EEPROMInitialized = false;

uint64_t EConfig::guid = 0;

void EConfig::initEEPROM() {
    if(!EEPROMInitialized) {
        //Initialize NVS
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
          ESP_ERROR_CHECK(nvs_flash_erase());
          ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);
    }
}

void EConfig::setGuid(uint64_t newGuid) {
    initEEPROM();
    nvs_handle_t handle;
    nvs_open("config", NVS_READWRITE, &handle);
    nvs_set_u64(handle, "GUID", newGuid);
    nvs_commit(handle);
    nvs_close(handle);
    guid = newGuid;
}

uint64_t EConfig::getGuid() {
    initEEPROM();
    if(guid == 0) {
        nvs_handle_t handle;
    	nvs_open("config", NVS_READONLY, &handle);
    	nvs_get_u64(handle, "GUID", &guid);
    	nvs_close(handle);
    }
    return guid;
}
