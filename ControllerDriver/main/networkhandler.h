#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H

#include "argus-netbuffer/BasicMessageProtocol/BasicMessageProtocol.h"

#include <asyncTCP.h>

#include <stdint.h>


namespace NH {
	void* myMalloc(uint64_t size);
	void myFree(void* ptr);

    struct Packet {
        uint8_t* data;
        uint32_t length;
    };

    enum class CTSMessageType { // client to server
        Handshake,
        GUIDResponse,
        VideoData = 20,
        IMUData,
        Debug = 10000,
    };

    enum class STCMessageType { // server to client
        Handshake,
        SetGUID,
        Debug = 10000,
    };

    class NetworkHandler {
        protected:
        static NetworkHandler* inst;

        public:
        static NetworkHandler* getInstance();
        
        void handleNetworkStuff();

        void trySendStuff();
        
        void pushNetMessage(NetMessageOut* msg);
        void makeNetworkPacket(NetMessageOut* msg);
        bool isConnected();
        
        void processMessage(NetMessageIn* msg);

        void processHandshake(NetMessageIn* msg);

        void sendHandshake();

        void processDebug(NetMessageIn* msg);
        
        void processSetGUID(NetMessageIn* msg);
        
        static NetMessageOut* newMessage(CTSMessageType type, uint64_t length);
    };

    void startNetworkHandlerTask();
    void networkHandlerTask(void *pvParameters);

    void onData(void *arg, AsyncClient *client, void *data, size_t len);
    void onConnect(void *arg, AsyncClient *client);
    void onDisconnect(void *arg, AsyncClient *client);
    void onError(void *arg, AsyncClient *client, int8_t error);
    void onTimeout(void *arg, AsyncClient *client, uint32_t time);
    void tcpConnect();
    void tcpInit();
}

#endif
