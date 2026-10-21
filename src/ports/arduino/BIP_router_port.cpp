#include "BIP_router_port.h"


BIP_PeripheralBase* BIP_Port::_activePeripheral = nullptr;

bool  BIP_Port::registerPeripheral(BacnetRouterClass*, BIP_PeripheralBase* peripheral)
{
    if (!peripheral) return false;
    _peripheral = peripheral;
    _activePeripheral = peripheral; //ensure active peripherial valid.
    return true;
}


void BIP_Port::activePeripheralSet()
{
        _activePeripheral = _peripheral;
}


BIP_PeripheralBase& BIP_Port::active()
{
    return *_activePeripheral;
}


