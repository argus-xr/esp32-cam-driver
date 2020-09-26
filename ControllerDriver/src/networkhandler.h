#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H

#include <Arduino.h>
#include "argus-netbuffer/BasicMessageProtocol/BasicMessageProtocol.h"


namespace NH {
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
        
        void pushNetMessage(NetMessageOut* msg);
        void makeNetworkPacket(NetMessageOut* msg);
        bool isConnected();

        void checkMessages();
        
        void processMessage(NetMessageIn* msg);

        esp_err_t bmp_handler();

        void processHandshake(NetMessageIn* msg);

        void sendHandshake();

        void processDebug(NetMessageIn* msg);
        
        void processSetGUID(NetMessageIn* msg);
            
    };

    void startNetworkHandlerTask();
    void networkHandlerTask(void *pvParameters);
}

#endif
