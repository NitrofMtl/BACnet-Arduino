#ifndef ARDUINO_BACNET_CONFIGS_H
#define ARDUINO_BACNET_CONFIGS_H

#include "bacnet/bacdef.h"


//Define all needed for multimode!

//Ethernet configuration
#define BACDL_BIP

//MSTP configuration
#define BACDL_MSTP

//ARCNET configurations
//#defineBACDL_ARCNET

//BIP6 configurations
//#define BACDL_BIP6

//ZIGBEE configurations
//#define BACDL_ZIGBEE

//BSC configurations
//#define BACDL_BSC

#define BACNET_ROUTER

//Router device's configuration
#define ROUTER_DEVICE


#if defined(ROUTER_DEVICE)
//uncomment the port you want to use as local device
#if defined(BACDL_ARCNET)
//#define LOCAL_DEVICE_ARCNET
#endif
#if defined(BACDL_ETHERNET)
//#define LOCAL_DEVICE_ETHERNET
#endif
#if defined(BACDL_BIP)
//#define LOCAL_DEVICE_BIP
#endif
#if defined(BACDL_BIP6)
//#define LOCAL_DEVICE_BIP6
#endif
#if defined(BACDL_MSTP)
#define LOCAL_DEVICE_MSTP
#endif
#if defined(BACDL_ZIGBEE)
//#define LOCAL_DEVICE_ZIGBEE
#endif
#if defined(BACDL_BSC)
//#define LOCAL_DEVICE_BSC
#endif
#endif//ROUTER_DEVICE

#if defined(BACDL_BIP) &&  defined(BACNET_ROUTER)
#define BACNET_ROUTER_BIP
#define BACNET_ROUTER_BIP_PORTS 1
#else
#define BACNET_ROUTER_BIP_PORTS 0
#endif

#if defined(BACDL_ETHERNET) &&  defined(BACNET_ROUTER)
#define BACNET_ROUTER_ETHERNET
#define BACNET_ROUTER_ETHERNET_PORTS 1
#else
#define BACNET_ROUTER_ETHERNET_PORTS 0
#endif

#if defined(BACDL_ARCNET) &&  defined(BACNET_ROUTER)
#define BACNET_ROUTER_ARCNET
#define BACNET_ROUTER_ARCNET_PORTS 1
#else
#define BACNET_ROUTER_ARCNET_PORTS 0
#endif

#if defined(BACDL_BIP6) &&  defined(BACNET_ROUTER)
#define BACNET_ROUTER_BIP6
#define BACNET_ROUTER_BIP6_PORTS 1
#else
#define BACNET_ROUTER_BIP6_PORTS 0
#endif

#if defined(BACDL_MSTP) &&  defined(BACNET_ROUTER)
#define BACNET_ROUTER_MSTP
#define BACNET_ROUTER_MSTP_PORTS 1
#else
#define BACNET_ROUTER_MSTP_PORTS 0
#endif

#if defined(BACDL_ZIGBEE) &&  defined(BACNET_ROUTER)
#define BACNET_ROUTER_ZIGBEE
#define BACNET_ROUTER_ZIGBEE_PORTS 1
#else
#define BACNET_ROUTER_ZIGBEE_PORTS 0
#endif

#if defined(BACDL_BSC) &&  defined(BACNET_ROUTER)
#define BACNET_ROUTER_BSC
#define BACNET_ROUTER_BSC_PORTS 1
#else
#define BACNET_ROUTER_BSC_PORTS 0
#endif

#ifdef BACNET_ROUTER
#define BACNET_ROUTER_MAX_PORTS \
    (BACNET_ROUTER_BIP_PORTS + \
     BACNET_ROUTER_ETHERNET_PORTS + \
     BACNET_ROUTER_ARCNET_PORTS + \
     BACNET_ROUTER_BIP6_PORTS + \
     BACNET_ROUTER_MSTP_PORTS + \
     BACNET_ROUTER_ZIGBEE_PORTS + \
     BACNET_ROUTER_BSC_PORTS)

#endif //BACNET_ROUTER

// for confirmed messages, this is the number of transactions
// that we hold in a queue waiting for timeout.
// Configure to zero if you don't want any confirmed messages
// Configure from 1..255 for number of outstanding confirmed
// requests available.
//Get low for les RAM usage
//255	Maximum/full-size table; huge RAM cost
//127	Plenty for a fairly active client
//31	Good embedded default
//16	Probably plenty for many controllers
//8	Very reasonable small controller
//4	Fine if you have little simultaneous client activity
//2	Possible for a very constrained device
//1	Minimum practical value
#define MAX_TSM_TRANSACTIONS 31

#define MAX_ADDRESS_CACHE 63

#define MAX_SCHEDULES 1

#define BACNET_DAILY_SCHEDULE_TIME_VALUES_SIZE 10


#endif // ARDUINO_BACNET_CONFIGS_H