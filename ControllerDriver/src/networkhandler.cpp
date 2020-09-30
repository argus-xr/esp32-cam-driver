#include "networkhandler.h"
#include "queue.h"
#include "networkconfig.h"
#include "main.h"
//#include "utility/util.h"
#include "camerahandler.h" // to enable/disable recording.
#include "EEPROMConfig.h"

#ifdef NETUSEWPA2ENTERPRISE
#include "esp_wpa2.h"
#endif

namespace NH {

void* myMalloc(uint64_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_8BIT);
}
void myFree(void* ptr) {
    heap_caps_free(ptr);
}

NetworkHandler* NetworkHandler::inst = new NetworkHandler();
NetworkHandler* NetworkHandler::getInstance() {
    return inst;
}

const int serverPort = 11000;

// Use WiFiClient class to create TCP connections
WiFiClient client;

int wStatus = WL_IDLE_STATUS;
QueueArray<OutPacket*> buffer;

#define readBufferSize 1500
uint8_t readBuf[readBufferSize];

BasicMessageBuffer* buf = new BasicMessageBuffer(myMalloc, myFree);

TaskHandle_t networkTaskHandle = NULL;
QueueHandle_t messageQueue = NULL;
const TickType_t xBlockTime = pdMS_TO_TICKS( 200 );


void NetworkHandler::handleNetworkStuff() {
    try {
        wStatus = WiFi.status();

        if (wStatus != WL_CONNECTED && wStatus != WL_IDLE_STATUS) { // WL_IDLE_STATUS means it's currently trying to connect.
            wifiConnect();
            vTaskDelay(pdMS_TO_TICKS( 5000 ));
        }

        try {
            if(messageQueue) {
                NetMessageOut* msg = nullptr;
                while(xQueueReceive( messageQueue, &msg, 0 )) { // returns true if there's any messages
                    if(msg) {
                        OutPacket* pac = buf->messageToOutPacket(msg);
                        buffer.enqueue(pac);
                        delete msg;
                    }
                }
            }
        } catch (std::exception e) {
            Serial.printf("Exception in converting NetMessageOut to Packet: %s.\n", e.what());
        }
        
        if (wStatus == WL_CONNECTED) {
            if (client.connected()) {
                if (!buffer.isEmpty()) {
                    try {
                        OutPacket* p = buffer.pop();
                        if (p != nullptr) {
                            uint16_t bytesSent = 0;
                            int sent = 0;
                            do {
                                Serial.printf("Sending some data.\n");
                                uint16_t bytesLeft = p->getDataLength() - bytesSent;
                                if(bytesLeft > 1000) {
                                    bytesLeft = 1000;
                                }
                                uint8_t* buf = (uint8_t*) myMalloc(bytesLeft);
                                memcpy(buf, p->getData() + bytesSent, bytesLeft);
                                //memset(buf, 'A', bytesLeft); // purely for testing purposes.
                                sent = client.write(buf, bytesLeft);
                                if(sent <= 0) {
                                    Serial.printf("Sending failed with EAGAIN/EWOULDBLOCK, bytes: %d.\n", bytesLeft);
                                }
                                myFree(buf);
                                bytesSent += sent;
                                taskYIELD();
                            } while (bytesSent < p->getDataLength() && sent > 0);
                            delete p;


                            struct timeval tv_now;
                            gettimeofday(&tv_now, NULL);
                            int64_t time_us = (int64_t)tv_now.tv_sec * 1000L + (int64_t)tv_now.tv_usec / 1000L;


                            Serial.printf("Sending %d bytes. Time in ms: %llu.\n", bytesSent, time_us);
                        } else {
                            Serial.printf("Null outpacket found.\n");
                        }
                    } catch (std::exception e) {
                        Serial.printf("Exception in network handler send loop: %s.\n", e.what());
                    }
                }

                checkMessages();
            } else {
                tcpConnect();
            }
        }
        CH::setRecordVideo(isConnected());
    } catch (std::exception e) {
        Serial.printf("Exception in network handler: %s.\n", e.what());
    } catch (...) {
        Serial.println("Non-std::exception exception caught.");
    }
}

void NetworkHandler::pushNetMessage(NetMessageOut* msg) {
    if(xQueueSend( messageQueue, &msg, xBlockTime) != pdTRUE) {
        Serial.printf("Message slipped out of queue!\n");
    }
}

bool NetworkHandler::isConnected() {
    return (wStatus == WL_CONNECTED && client.connected());
}

void NetworkHandler::checkMessages() {
    try {
        while (client.available()) {
            int bytes = client.read(readBuf, readBufferSize);
            if (bytes <= 0) {
                break;
            } else {
                buf->insertBuffer(readBuf, bytes, true);
                buf->checkMessages();
            }
        }
        NetMessageIn* msg = nullptr;
        do {
            msg = buf->popMessage();
            if(msg) {
                processMessage(msg);
            }
            delete msg;
        } while(msg);
    } catch (std::exception e) {
        Serial.printf("Exception in checkMessages: %s.\n", e.what());
    }
}

void NetworkHandler::processMessage(NetMessageIn* msg) {
    STCMessageType type = (STCMessageType) msg->readVarInt();
    Serial.println("Message received.");
    Serial.printf("Length: %d, type: %lld.\n", msg->getInternalBufferLength(), (uint64_t) type);
    switch(type) {
        case STCMessageType::Handshake:
            processHandshake(msg);
            break;
        case STCMessageType::SetGUID:
            processSetGUID(msg);
            break;
        case STCMessageType::Debug:
            processDebug(msg);
            break;
        default:
            break;
    }
}

void NetworkHandler::processHandshake(NetMessageIn* msg) {
    Serial.printf("Processing handshake.\n");
    sendHandshake();
}

void NetworkHandler::sendHandshake() {
    uint64_t guid = EConfig::getGuid();
    NetMessageOut* msg = newMessage(CTSMessageType::Handshake, 20);
    msg->writeVarInt((uint64_t) CTSMessageType::Handshake);
    msg->writeVarInt(guid);
    std::string contents = msg->debugBuffer();
    Serial.printf("Sending handshake - GUID: %llu, contents: %s.\n", guid, contents.c_str());
    pushNetMessage(msg);
}

void NetworkHandler::processDebug(NetMessageIn* msg) {
    std::string text = msg->readVarString();
    Serial.printf("Message: %s\n", text.c_str());
}

void NetworkHandler::processSetGUID(NetMessageIn* msg) {
    uint64_t newGUID = msg->readVarInt(); // can't read/write uint64_t right now
    EConfig::setGuid(newGUID);
    Serial.printf("Processing SetGUID - GUID: %llu.\n", newGUID);
}

NetMessageOut* NetworkHandler::newMessage(CTSMessageType type, uint64_t length) {
    NetMessageOut* nmo = new NetMessageOut(length, NH::myMalloc, NH::myFree);
    nmo->writeVarInt((uint64_t) type);
    return nmo;
}

void startNetworkHandlerTask() {
    messageQueue = xQueueCreate( 5, sizeof( NetMessageOut* ) );
    
    initWifi();

    xTaskCreatePinnedToCore(&networkHandlerTask, "NetworkTask", 60000, NULL, 5, &networkTaskHandle, tskNO_AFFINITY);
}

void networkHandlerTask(void *pvParameters) {
    while(true) {
        NetworkHandler::getInstance()->handleNetworkStuff();
        taskYIELD();
    }
}

void initWifi() {
    WiFi.onEvent(onWifiConnect, SYSTEM_EVENT_STA_CONNECTED);
    WiFi.onEvent(onWifiReady, SYSTEM_EVENT_WIFI_READY);
}

void wifiConnect() {
    WiFi.setAutoReconnect(true);
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
        WiFi.begin(NETSSID);
    #else
        WiFi.begin(NETSSID, NETPASS);
    #endif
    Serial.printf("Current status: %d, Reconnecting to %s.\n", wStatus, NETSSID);
}

void onWifiReady(WiFiEvent_t event, WiFiEventInfo_t info) {
    wifiConnect();
}

void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("Connected to WiFi, now connecting to server!\n");
    tcpConnect();
}

void tcpConnect() {
    if (client.connect(NETHOST, serverPort)) {
        Serial.printf("Connected to %s.\n", NETHOST);
        client.setNoDelay(false);
        //client.setNoDelay(true); // consider setSync(true): flushes each write, slower but does not allocate temporary memory.
    } else {
        Serial.printf("Failed to connect to %s.\n", NETHOST);
    }
}

} // namespace NH
