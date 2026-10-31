#ifndef BACNET_PERIPHERAL_H
#define BACNET_PERIPHERAL_H

#include "arduino_configs.h"

enum class BacnetDatalink : uint8_t {
#ifdef BACDL_BIP
    BIP,
#endif 
#ifdef BACDL__ETHERNET
    ETHERNET,
#endif
#ifdef BACDL_ARCNET
    ARCNET,
#endif
#ifdef BACDL_BIP6
    BIP6,
#endif
#ifdef BACDL_MSTP
    MSTP,
#endif
#ifdef BACDL_ZIGBEE
    ZIGBEE,
#endif
#ifdef BACDL_BSC
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