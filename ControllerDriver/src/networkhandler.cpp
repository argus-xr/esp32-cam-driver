#include "networkhandler.h"
#include "queue.h"
#include <WiFi.h>
#include "networkconfig.h"
#include "main.h"
//#include "utility/util.h"
#include "camerahandler.h" // to enable/disable recording.
#include "EEPROMConfig.h"

#ifdef NETUSEWPA2ENTERPRISE
#include "esp_wpa2.h"
#endif

namespace NH {

NetworkHandler* NetworkHandler::inst = new NetworkHandler();
NetworkHandler* NetworkHandler::getInstance() {
    return inst;
}

const int serverPort = 11000;

// Use WiFiClient class to create TCP connections
WiFiClient client;

int wStatus = WL_IDLE_STATUS;
QueueArray<struct Packet*> buffer;

#define readBufferSize 1500
uint8_t readBuf[readBufferSize];

BasicMessageBuffer* buf = new BasicMessageBuffer();

TaskHandle_t networkTaskHandle = NULL;
QueueHandle_t messageQueue = NULL;
const TickType_t xBlockTime = pdMS_TO_TICKS( 200 );


void NetworkHandler::handleNetworkStuff() {
    try {
        wStatus = WiFi.status();

        if (wStatus != WL_CONNECTED && wStatus != WL_IDLE_STATUS) {
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
            Serial.print("Current status: ");
            Serial.print(wStatus);
            Serial.print(". Reconnecting to ");
            Serial.println(NETSSID);
            vTaskDelay(pdMS_TO_TICKS( 5000 ));
        }

        if(messageQueue != NULL) {
            NetMessageOut* msg = NULL;
            while(xQueueReceive( messageQueue, &msg, 0 )) { // returns true if there's any messages
                if(msg != NULL && msg != nullptr) {
                    makeNetworkPacket(msg);
                    delete msg;
                }
            }
        }
        
        if (wStatus == WL_CONNECTED) {
            if (client.connected()) {
                if (!buffer.isEmpty()) {
                    try {
                        Packet* p = buffer.pop();
                        if (p != nullptr) {
                            if (p->length > 0 && p->data != NULL && p->data != nullptr) {
                                uint16_t bytesSent = 0;
                                int sent = 0;
                                do {
                                    uint16_t bytesLeft = p->length - bytesSent;
                                    uint8_t* buf = new uint8_t[bytesLeft];
                                    memcpy(buf, p->data + bytesSent, bytesLeft);
                                    sent = client.write(buf, bytesLeft);
                                    Serial.printf("Sending %d bytes.\n", sent);
                                    bytesSent += sent;
                                } while (bytesSent < p->length && sent > 0);
                            }
                            if(p->data != NULL) {
                                delete[] p->data;
                            }
                            delete p;
                        }
                    } catch (std::exception e) {
                        Serial.println(e.what());
                    }
                }

                try {
                    checkMessages();
                } catch (std::exception e) {
                    Serial.println(e.what());
                }
            } else {
                if (client.connect(NETHOST, serverPort)) {
                    Serial.print("Connected to ");
                    Serial.println(NETHOST);
                    client.setNoDelay(true); // consider setSync(true): flushes each write, slower but does not allocate temporary memory.
                } else {
                    Serial.print("Failed to connect to ");
                    Serial.println(NETHOST);
                }
            }
        }
        CH::setRecordVideo(isConnected());
    } catch (std::exception e) {
        Serial.print("Exception in network handler: ");
        Serial.println(e.what());
    } catch (...) {
        Serial.println("Non-std::exception exception caught.");
    }
}

void NetworkHandler::pushNetMessage(NetMessageOut* msg) {
    xQueueSend( messageQueue, &msg, xBlockTime );
}

void NetworkHandler::makeNetworkPacket(NetMessageOut* msg) {
    Packet* p = new Packet();
    p->length = buf->messageOutToByteArray(p->data, msg);
    buffer.enqueue(p);
}

bool NetworkHandler::isConnected() {
    return (wStatus == WL_CONNECTED && client.connected());
}

void NetworkHandler::checkMessages() {
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
    NetMessageOut* msg = new NetMessageOut(10);
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

void startNetworkHandlerTask() {
    messageQueue = xQueueCreate( 5, sizeof( NetMessageOut* ) );

    xTaskCreatePinnedToCore(&networkHandlerTask, "NetworkTask", 60000, NULL, 0, &networkTaskHandle, tskNO_AFFINITY);
}

void networkHandlerTask(void *pvParameters) {
    while(true) {
        NetworkHandler::getInstance()->handleNetworkStuff();
        vTaskDelay(pdMS_TO_TICKS( 10 ));
    }
}

} // namespace NH
