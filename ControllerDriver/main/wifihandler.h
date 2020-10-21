#ifndef WIFIHANDLER_H
#define WIFIHANDLER_H

/*#include "argus-netbuffer/BasicMessageProtocol/BasicMessageProtocol.h"
#include <asyncTCP.h>*/

#include <stdint.h>
#include <WiFi.h>


class WiFiHandler {
	protected:
	bool initialized = false;
	void wifiInit();
	static WiFiHandler* inst;

	public:
	volatile bool connected = false; // should be protected, but that breaks the callback setting it.
	static WiFiHandler* getInstance();
	void wifiConnect();
	bool isConnected();
};

#endif
