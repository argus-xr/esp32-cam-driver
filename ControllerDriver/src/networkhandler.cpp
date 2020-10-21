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
//WiFiClient client;
AsyncClient* tcpClient = nullptr;

int wStatus = WL_IDLE_STATUS;
QueueArray<NetMessageOut*> outMessagebuffer;
OutPacket* currentPacket = nullptr;
size_t currentPacketSpot = 0;

#define readBufferSize 1500
uint8_t readBuf[readBufferSize];

BasicMessageBuffer* bmbuf = new BasicMessageBuffer(myMalloc, myFree);

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
                        outMessagebuffer.enqueue(msg);
                    }
                }
            }
        } catch (std::exception e) {
            Serial.printf("Exception in converting NetMessageOut to Packet: %s.\n", e.what());
        }
        
        if (wStatus == WL_CONNECTED) {
            if (tcpClient->connected()) {
                trySendStuff();
            }
        }
        CH::setRecordVideo(isConnected());
    } catch (std::exception e) {
        Serial.printf("Exception in network handler: %s.\n", e.what());
    } catch (...) {
        Serial.println("Non-std::exception exception caught.");
    }
}

void NetworkHandler::trySendStuff() {
    size_t bytesLeft = tcpClient->space();
    if(bytesLeft > 1000) {
        bool send = false;
        while ((!outMessagebuffer.isEmpty() || currentPacket) && bytesLeft > 0) {
            try {
                if(!currentPacket) {
                    NetMessageOut* msg = outMessagebuffer.pop();
                    if(msg) {
                        OutPacket* pac = bmbuf->messageToOutPacket(msg);
                        delete msg;
                        currentPacket = pac;
                    }
                }
                if (currentPacket) {
                    int32_t len = currentPacket->getDataLength();
                    int32_t bytesToSend = len - currentPacketSpot;
                    int32_t clientSpace = bytesLeft - 1; // -1 just to make sure we have room. I fear the off-by-one!
                    //Serial.printf("Packetspot: %d. Data length: %d. Bytes to write: %d. Space available: %d.\n", currentPacketSpot, len, bytesToSend, clientSpace);
                    if(clientSpace < bytesToSend) { // somehow breaks if comparing size_t instead of int32_t
                        //Serial.printf("Truncating bytesToSend (%d) to clientSpace (%d).\n", bytesToSend, clientSpace);
                        bytesToSend = clientSpace;
                    }
                    if(bytesToSend <= 0) {
                        break;
                    } else {
                        //Serial.printf("Before add, sending %d bytes. CurrentPacketSpot is %d out of %u.\n", bytesToSend, currentPacketSpot, len);
                        tcpClient->add((char*) (currentPacket->getData() + currentPacketSpot), bytesToSend);
                        bytesLeft -= bytesToSend;
                        send = true;
                        //Serial.printf("After send.\n");
                        currentPacketSpot += bytesToSend;
                        if(currentPacketSpot >= len) {
                            //Serial.printf("Deleting packet.\n");
                            delete currentPacket;
                            currentPacket = nullptr;
                            currentPacketSpot = 0;
                        }
                    }

                    //Serial.printf("Sending data.\n");
                }
            } catch (std::exception e) {
                Serial.printf("Exception in network handler send loop: %s.\n", e.what());
            }
        }
        if(send) {
            tcpClient->send();
        }
    }
}

void NetworkHandler::pushNetMessage(NetMessageOut* msg) {
    if(xQueueSend( messageQueue, &msg, xBlockTime) != pdTRUE) {
        Serial.printf("Message slipped out of queue!\n");
    }
}

bool NetworkHandler::isConnected() {
    return (wStatus == WL_CONNECTED && tcpClient->connected());
    //return (wStatus == WL_CONNECTED && client.connected());
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
    tcpClient = new AsyncClient();
    tcpClient->onData(onData, tcpClient);
    tcpClient->onConnect(onConnect, tcpClient);
    tcpClient->onDisconnect(onDisconnect, tcpClient);
    tcpClient->onError(onError, tcpClient);
    tcpClient->onTimeout(onTimeout, tcpClient);

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
    Serial.printf("Wifi interface ready, trying to connect.\n");
    wifiConnect();
}

void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("Connected to WiFi, now connecting to server!\n");
    vTaskDelay(pdMS_TO_TICKS(5000));
    tcpConnect();
}

void tcpConnect() {
    if(!tcpClient->connected() && !tcpClient->connecting()) {
        Serial.printf("Starting TCP connection.\n");
        tcpClient->connect(NETHOST, serverPort);
        tcpClient->setNoDelay(false);
    }
}

void onData(void *arg, AsyncClient *client, void *data, size_t len) {
    try {
        if (len <= 0) {
            return;
        } else {
            bmbuf->insertBuffer((uint8_t*) data, len, true);
            bmbuf->checkMessages();
        }
        NetMessageIn* msg = nullptr;
        do {
            msg = bmbuf->popMessage();
            if(msg) {
                NetworkHandler::getInstance()->processMessage(msg);
            }
            delete msg;
        } while(msg);
    } catch (std::exception e) {
        Serial.printf("Exception in checkMessages: %s.\n", e.what());
    }
    tcpClient->ack(len);
}

void onConnect(void *arg, AsyncClient *client) {
    Serial.printf("TCP connected.\n");
}

void onDisconnect(void *arg, AsyncClient *client) {
    Serial.printf("TCP disconnected.\n");
    vTaskDelay(pdMS_TO_TICKS(500));
    tcpConnect();
}

void onError(void *arg, AsyncClient *client, int8_t error) {
    Serial.printf("LWIP error %d: %s.\n", error, tcpClient->errorToString(error));
    switch(error) {
        case ERR_ABRT:
        vTaskDelay(pdMS_TO_TICKS(500));
        tcpConnect();
        break;
        default:
        break;
    }
}

void onTimeout(void *arg, AsyncClient *client, uint32_t time) {
    Serial.printf("TCP timeout.\n");
}

} // namespace NH
