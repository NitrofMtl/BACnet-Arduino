#pragma onve

#include "bacnet_mstp.h"

template <class Peripheral>
bool MSTP_port<Peripheral>::registerPeripheral(
    BacnetRouterClass* router,
    Peripheral* peripheral)
{
    if (!router || !peripheral)
        return false;

    int8_t portIndex = router->getPortIndex(net);

    if (portIndex < 0 ||
        portIndex >= BACNET_ROUTER_MSTP_PORTS_COUNT)
        return false;

    instances[portIndex] = peripheral;
    return true;
}