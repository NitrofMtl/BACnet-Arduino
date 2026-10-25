#ifndef ARDUINO_BACNET_CONFIGS_H
#define ARDUINO_BACNET_CONFIGS_H

#include "bacnet/bacdef.h"

//Define all needed for multimode!

//Ethernet configuration
#define BACDL_BIP //<--- Uncomment

//MSTP configuration
#define BACDL_MSTP //<--- Uncomment

//ARCNET configurations
//#define BACDL_ARCNET //<--- Uncomment

//BIP6 configurations
//#define BACDL_BIP6 //<--- Uncomment

//ZIGBEE configurations
//#define BACDL_ZIGBEE //<--- Uncomment

//BSC configurations
//#define BACDL_BSC //<--- Uncomment

//Router configuration
#define BACNET_ROUTER //<--- Uncomment

#ifdef BACNET_ROUTER
//Router device's configuration
#define ROUTER_DEVICE //<--- Uncomment
#endif

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
#define MAX_TSM_TRANSACTIONS 15//31

#define MAX_ADDRESS_CACHE 63

#define MAX_SCHEDULES 1

#define BACNET_DAILY_SCHEDULE_TIME_VALUES_SIZE 10


#define BACNET_ROUTER_BIP_PORTS_COUNT 2
#define BACNET_ROUTER_ETHERNET_PORTS_COUNT 1
#define BACNET_ROUTER_ARCNET_PORTS_COUNT 1
#define BACNET_ROUTER_BIP6_PORTS_COUNT 1
#define BACNET_ROUTER_MSTP_PORTS_COUNT 1
#define BACNET_ROUTER_ZIGBEE_PORTS_COUNT 1
#define BACNET_ROUTER_BSC_PORTS_COUNT 1

#if defined(BACDL_BIP)
#if defined(BACNET_ROUTER)
#define BACNET_ROUTER_BIP
#define BACNET_BIP_PORTS_COUNT BACNET_ROUTER_BIP_PORTS_COUNT
#else
#define BACNET_BIP_PORTS_COUNT 1
#endif
#else
#define BACNET_BIP_PORTS_COUNT 0
#endif

#if defined(BACDL_ETHERNET)
#if defined(BACNET_ROUTER)
#define BACNET_ROUTER_ETHERNET
#define BACNET_ETHERNET_PORTS_COUNT BACNET_ROUTER_ETHERNET_PORTS_COUNT
#else
#define BACNET_ETHERNET_PORTS_COUNT 1
#endif
#else
#define BACNET_ETHERNET_PORTS_COUNT 0
#endif

#if defined(BACDL_ARCNET)
#if defined(BACNET_ROUTER)
#define BACNET_ROUTER_ARCNET
#define BACNET_ARCNET_PORTS_COUNT BACNET_ROUTER_ARCNET_PORTS_COUNT
#else
#define BACNET_ARCNET_PORTS_COUNT 1
#endif
#else
#define BACNET_ARCNET_PORTS_COUNT 0
#endif

#if defined(BACDL_BIP6)
#if defined(BACNET_ROUTER)
#define BACNET_ROUTER_BIP6
#define BACNET_BIP6_PORTS_COUNT BACNET_ROUTER_BIP6_PORTS_COUNT
#else
#define BACNET_BIP6_PORTS_COUNT 1
#endif
#else
#define BACNET_BIP6_PORTS_COUNT 0
#endif

#if defined(BACDL_MSTP)
#if defined(BACNET_ROUTER)
#define BACNET_ROUTER_MSTP
#define BACNET_MSTP_PORTS_COUNT BACNET_ROUTER_MSTP_PORTS_COUNT
#else
#define BACNET_MSTP_PORTS_COUNT 1
#endif
#else
#define BACNET_MSTP_PORTS_COUNT 0
#endif

#if defined(BACDL_ZIGBEE)
#if defined(BACNET_ROUTER)
#define BACNET_ROUTER_ZIGBEE
#define BACNET_ZIGBEE_PORTS_COUNT BACNET_ROUTER_ZIGBEE_PORTS_COUNT
#else
#define BACNET_ZIGBEE_PORTS_COUNT 1
#endif
#else
#define BACNET_ZIGBEE_PORTS_COUNT 0
#endif

#if defined(BACDL_BSC)
#if defined(BACNET_ROUTER)
#define BACNET_ROUTER_BSC
#define BACNET_BSC_PORTS_COUNT BACNET_ROUTER_BSC_PORTS_COUNT
#else
#define BACNET_BSC_PORTS_COUNT 1
#endif
#else
#define BACNET_BSC_PORTS_COUNT 0
#endif





#if defined(BACDL_BIP)
#define BACNET_BIP_MPDU_MAX BIP_MPDU_MAX
#else
#define BACNET_BIP_MPDU_MAX 0
#endif

#if defined(BACDL_ARCNET)
#define BACNET_ARCNET_MPDU_MAX ARCNET_MPDU_MAX
#else
#define BACNET_ARCNET_MPDU_MAX 0
#endif

#if defined(BACDL_BIP6)
#define BACNET_BIP6_MPDU_MAX BIP6_MPDU_MAX
#else
#define BACNET_BIP6_MPDU_MAX 0
#endif

#if defined(BACDL_MSTP)
#define BACNET_DLMSTP_MPDU_MAX DLMSTP_MPDU_MAX
#else
#define BACNET_DLMSTP_MPDU_MAX 0
#endif

//Stack do not provide default macro for those transport
//User MUST define one if enabling those transports
#if !defined(BACDL_ETHERNET)
#define BACNET_ETHERNET_MPDU_MAX 0
#endif

#if !defined(BACDL_ZIGBEE)
#define BACNET_ZIGBEE_MPDU_MAX 0
#endif

#if !defined(BACDL_BSC)
#define BACNET_BSC_MPDU_MAX 0
#endif


#define BACNET_ROUTER_MAX_PORTS \
    (BACNET_BIP_PORTS_COUNT + \
     BACNET_ETHERNET_PORTS_COUNT + \
     BACNET_ARCNET_PORTS_COUNT + \
     BACNET_BIP6_PORTS_COUNT + \
     BACNET_MSTP_PORTS_COUNT + \
     BACNET_ZIGBEE_PORTS_COUNT + \
     BACNET_BSC_PORTS_COUNT)


#endif // ARDUINO_BACNET_CONFIGS_H