#include "bacnet_peripheral.h"
#include "bacnet/datalink/datalink.h"

RouterPort::RouterPort(BacnetDatalink portType, uint16_t net)
    : net(net), portType(portType)
{}


void RouterPort::activeDatalinkSet()
{
    datalinkSet();
    activePeripheralSet();
}


#if defined(BACDL_MULTIPLE)
void RouterPort::datalinkSet()
{
    switch (portType){

#ifdef BACNET_ROUTER_BIP
    case BacnetDatalink::BIP: {
        char bipName[] = "bip";
        datalink_set(bipName);
        break;
    }
#endif 
#ifdef BACNET_ROUTER_ETHERNET
    case BacnetDatalink::ETHERNET: {
        char ethernetName[] = "ethernet";
        datalink_set(ethernetName);
        break;
    }
#endif
#ifdef BACNET_ROUTER_ARCNET
    case BacnetDatalink::ARCNET: {
        char arcnetName[] = "arcnet";
        datalink_set(arcnetName);
        break;
    }
#endif
#ifdef BACNET_ROUTER_BIP6
    case BacnetDatalink::BIP6: {
        char bip6Name = "bip6";
        datalink_set(bip6Name);
        break;
    }
#endif
#ifdef BACNET_ROUTER_MSTP
    case BacnetDatalink::MSTP: {
        char mstpName[] = "mstp";
        datalink_set(mstpName);
        break;
    }
#endif
#ifdef BACNET_ROUTER_ZIGBEE
    case BacnetDatalink::ZIGBEE: {
    char zigbeeName = "zigbee";
        datalink_set(zigbeeName);
        break;
    }
#endif
#ifdef BACNET_ROUTER_BSC
    case BacnetDatalink::BSC: {
        char bscName = "bsc";
        datalink_set(bscName);
        break;
    }
#endif
    default:
        break;
    }
}
#else //BACDL_MULTIPLE
void RouterPort::datalinkSet()
{}
#endif //BACDL_MULTIPLE  
