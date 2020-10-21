#ifndef NETWORKCONFIG_H
#define NETWORKCONFIG_H

// Copy this file and rename it to networkconfig.h

#define NETSSID "Dummy"
#define NETPASS "DummyPass"
#define NETHOST "192.168.1.100"
// #define NETWPA2ENTID "SomeID" // uncomment and fill in if using WPA2-Enterprise
// #define NETWPA2ENTUSER "SomeUser"

#if defined(NETWPA2ENTID) && defined(NETWPA2ENTUSER)
#define NETUSEWPA2ENTERPRISE
#endif

#endif