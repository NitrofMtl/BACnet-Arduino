#ifndef BACNET_PERIPHERAL_H
#define BACNET_PERIPHERAL_H

#include "bacnet.h"

enum class BacnetDatalink : uint8_t {
#ifdef BACNET_ROUTER_BIP
    BIP,
#endif 
#ifdef BACNET_ROUTER_ETHERNET
    ETHERNET,
#endif
#ifdef BACNET_ROUTER_ARCNET
    ARCNET,
#endif
#ifdef BACDL_BIP6
    BIP6,
#endif
#ifdef BACNET_ROUTER_MSTP
    MSTP,
#endif
#ifdef BACDL_ZIGBEE
    ZIGBEE,
#endif
#ifdef BACNET_ROUTER_BSC
    BSC,
#endif
};

class BacnetRouterClass;


class RouterPort {
    public:
    RouterPort(BacnetDatalink portType, uint16_t net);
    friend class BacnetRouterClass;

    //private:
    void activeDatalinkSet();
    uint16_t net;
    BacnetDatalink portType;
    
    protected:
    virtual void activePeripheralSet() {};
    
    
    void datalinkSet();
};


#endif