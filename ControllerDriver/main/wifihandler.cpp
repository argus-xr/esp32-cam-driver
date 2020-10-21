#include "wifihandler.h"
#include "networkconfig.h"

#ifdef NETUSEWPA2ENTERPRISE
#include "esp_wpa2.h"
#endif


void onWifiReady(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("Wifi interface ready, trying to connect.\n");
    WiFiHandler::getInstance()->wifiConnect();
}

void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("Connected to WiFi, now connecting to server!\n");
}

WiFiHandler* WiFiHandler::inst = new WiFiHandler();
WiFiHandler* WiFiHandler::getInstance() {
    return inst;
}

void WiFiHandler::wifiInit() {
    //WiFi.onEvent(onWifiConnect, SYSTEM_EVENT_STA_CONNECTED);
    //WiFi.onEvent(onWifiReady, SYSTEM_EVENT_WIFI_READY);
}

void WiFiHandler::wifiConnect() {
	if(WiFi.status() != WL_CONNECTED) {
		try {
			printf("Connecting to %s.\n", NETSSID);
			WiFi.setAutoReconnect(true);
			wl_status_t result;
			vTaskDelay(pdMS_TO_TICKS(100));
			#ifdef NETUSEWPA2ENTERPRISE
				WiFi.disconnect(true);
				esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)NETWPA2ENTID, strlen(NETWPA2ENTID));
				esp_wifi_sta_wpa2_ent_set_username((uint8_t *)NETWPA2ENTUSER, strlen(NETWPA2ENTUSER));
				esp_wifi_sta_wpa2_ent_set_password((uint8_t *)NETPASS, strlen(NETPASS));
				Serial.println("Set enterprise username/pass/ident");
				vTaskDelay(pdMS_TO_TICKS( 50 ));
				esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();
				Serial.println("Created default enterprise config");
				vTaskDelay(pdMS_TO_TICKS( 50 ));
				WiFi.mode(WIFI_STA);
				esp_wifi_sta_wpa2_ent_enable(&config);
				Serial.println("Loaded enterprise config");
				vTaskDelay(pdMS_TO_TICKS( 50 ));
				result = WiFi.begin(NETSSID);
			#else
				result = WiFi.begin(NETSSID, NETPASS);
			#endif
				if(result == WL_CONNECTED) {
					printf("Connected!\n");
				} else {
					printf("WiFi status: %d.\n", result);
				}
		} catch (std::exception &e) {
			printf("Exception: %s\n", e.what());
		}
	}
}

bool WiFiHandler::isConnected() {
	return WiFi.status() == WL_CONNECTED;
}
