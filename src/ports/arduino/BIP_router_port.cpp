#include "BIP_router_port.h"


BIP_PeripheralBase* BIP_Port::_activePeripheral = nullptr;


BIP_Port::BIP_Port(BacnetDatalink portType, uint16_t net, BIP_PeripheralBase* peripheral)
: RouterPort(portType, net)
{
    if (!peripheral) return;
    _peripheral = peripheral;
    _activePeripheral = peripheral; //ensure active peripherial valid.
}


void BIP_Port::activePeripheralSet()
{
    _activePeripheral = _peripheral;
}


BIP_PeripheralBase* BIP_Port::active()
{
    return _activePeripheral;
}


#if !defined(BACNET_ROUTER)
BIP_Port BIP(BacnetDatalink::BIP, 0, nullptr);
#endif