#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "networkhandler.h"
#include <queue>
#include "networkconfig.h"
#include "camerahandler.h" // to enable/disable recording.
#include "esp_heap_caps.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "EEPROMConfig.h"
#include "wifihandler.h"

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

// Use AsyncClient class to create TCP connections
AsyncClient* tcpClient = nullptr;

std::queue<NetMessageOut*> outMessagebuffer;
OutPacket* currentPacket = nullptr;
size_t currentPacketSpot = 0;

#define readBufferSize 1500
uint8_t readBuf[readBufferSize];

BasicMessageBuffer* bmbuf = new BasicMessageBuffer(myMalloc, myFree);

TaskHandle_t networkTaskHandle = NULL;
QueueHandle_t messageQueue = NULL;
const TickType_t xBlockTime = pdMS_TO_TICKS( 200 );

void NetworkHandler::handleNetworkStuff() {
	while (!WiFiHandler::getInstance()->isConnected()) {
		vTaskDelay(pdMS_TO_TICKS( 100 ));
	}

	if(WiFiHandler::getInstance()->isConnected() && (!tcpClient || (!tcpClient->connected() && !tcpClient->connecting()))) {
		tcpConnect();
	}

	if(isConnected()) {
		if(messageQueue) {
			NetMessageOut* msg = nullptr;
			while(xQueueReceive( messageQueue, &msg, 0 )) { // returns true if there's any messages
				if(msg) {
					outMessagebuffer.emplace(msg);
				}
			}
		}
		trySendStuff();
	}

	CH::setRecordVideo(isConnected());
	taskYIELD();
}

void NetworkHandler::trySendStuff() {
    if (!outMessagebuffer.empty()) {
        try {
            //Serial.printf("TrySendStuff start.\n");
            if(!currentPacket) {
                NetMessageOut* msg = outMessagebuffer.front();
                outMessagebuffer.pop();
                if(msg) {
                    OutPacket* pac = bmbuf->messageToOutPacket(msg);
                    delete msg;
                    currentPacket = pac;
                    //Serial.printf("Selected new packet for sending.\n");
                }
            }
            if (currentPacket) {
                int32_t len = currentPacket->getDataLength();
                int32_t bytesToSend = len - currentPacketSpot;
                int32_t clientSpace = tcpClient->space() - 1; // -1 just to make sure we have room. I fear the off-by-one!
                //Serial.printf("Packetspot: %d. Data length: %d. Bytes to write: %d. Space available: %d.\n", currentPacketSpot, len, bytesToSend, clientSpace);
                if(clientSpace < bytesToSend) { // somehow breaks if comparing size_t instead of int32_t
                    //Serial.printf("Truncating bytesToSend (%d) to clientSpace (%d).\n", bytesToSend, clientSpace);
                    bytesToSend = clientSpace;
                }
                if(bytesToSend <= 0) {
                    return;
                } else {
                    //Serial.printf("Before add, sending %d bytes. CurrentPacketSpot is %d out of %u.\n", bytesToSend, currentPacketSpot, len);
                    tcpClient->add((char*) (currentPacket->getData() + currentPacketSpot), bytesToSend);
                    tcpClient->send();
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
        } catch (std::exception &e) {
            Serial.printf("Exception in network handler send loop: %s.\n", e.what());
        }
    }
}

void NetworkHandler::pushNetMessage(NetMessageOut* msg) {
    if(xQueueSend( messageQueue, &msg, xBlockTime) != pdTRUE) {
        printf("Message slipped out of queue!\n");
    }
}

bool NetworkHandler::isConnected() {
	if(!tcpClient) {return false;}
	return tcpClient->connected();
}

void NetworkHandler::processMessage(NetMessageIn* msg) {
    STCMessageType type = (STCMessageType) msg->readVarInt();
    printf("Message received.\n");
    printf("Length: %d, type: %lld.\n", msg->getInternalBufferLength(), (uint64_t) type);
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
    printf("Processing handshake.\n");
    sendHandshake();
}

void NetworkHandler::sendHandshake() {
    uint64_t guid = EConfig::getGuid();
    NetMessageOut* msg = newMessage(CTSMessageType::Handshake, 20);
    msg->writeVarInt(guid);
    std::string contents = msg->debugBuffer();
    printf("Sending handshake - GUID: %llu, contents: %s.\n", guid, contents.c_str());
    pushNetMessage(msg);
}

void NetworkHandler::processDebug(NetMessageIn* msg) {
    std::string text = msg->readVarString();
    printf("Message: %s\n", text.c_str());
}

void NetworkHandler::processSetGUID(NetMessageIn* msg) {
    uint64_t newGUID = msg->readVarInt(); // can't read/write uint64_t right now
    EConfig::setGuid(newGUID);
    printf("Processing SetGUID - GUID: %llu.\n", newGUID);
}

NetMessageOut* NetworkHandler::newMessage(CTSMessageType type, uint64_t length) {
    NetMessageOut* nmo = new NetMessageOut(length, NH::myMalloc, NH::myFree);
    nmo->writeVarInt((uint64_t) type);
    return nmo;
}

void startNetworkHandlerTask() {
    messageQueue = xQueueCreate( 5, sizeof( NetMessageOut* ) );

    xTaskCreatePinnedToCore(&networkHandlerTask, "NetworkTask", 60000, NULL, 5, &networkTaskHandle, tskNO_AFFINITY);
}

void networkHandlerTask(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS( 5000 ));
    tcpInit();
    WiFiHandler::getInstance()->wifiConnect();
    tcpConnect();
    while(true) {
        NetworkHandler::getInstance()->handleNetworkStuff();
        taskYIELD();
    }
}

void tcpInit() {
    tcpClient = new AsyncClient();
    tcpClient->onData(onData, tcpClient);
    tcpClient->onConnect(onConnect, tcpClient);
    tcpClient->onDisconnect(onDisconnect, tcpClient);
    tcpClient->onError(onError, tcpClient);
    tcpClient->onTimeout(onTimeout, tcpClient);
}

void tcpConnect() {
    if(!tcpClient->connected() && !tcpClient->connecting()) {
        Serial.printf("Starting TCP connection.\n");
        tcpClient->connect(NETHOST, serverPort);
        tcpClient->setNoDelay(false);
        vTaskDelay(pdMS_TO_TICKS(100));
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
    } catch (std::exception &e) {
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
