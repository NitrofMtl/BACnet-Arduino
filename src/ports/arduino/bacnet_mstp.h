#ifndef PORTS_BACNET_MSTP_H
#define PORTS_BACNET_MSTP_H

#include <Arduino.h>

#include "bacnet/bacdef.h"

#include "ports/arduino/bacnet_core.h"
#include "ports/arduino/bacnet_peripheral.h"

#define BACNET_PRIORITY_NORMAL 16
#define BACNET_PRIORITY_OPERATOR 8
#define BACNET_PRIORITY_ALARM 6
#define BACNET_PRIORITY_EMERGENCY 1
#define BACNET_PRIORITY_RELEASE 0


void init_bacnet_mstp(int8_t mac, uint8_t index = 0);
void mstp_nmax_master_set(uint8_t nmax_master, uint8_t index = 0);

void bacnet_Poll_mstp();

void mstp_callback_send_set(void (*cb)(const uint8_t *p, uint16_t len), uint8_t index = 0);
void mstp_callback_read_set(bool (*cb)(uint8_t *b), uint8_t index = 0);
void mstp_callback_transmitting_set(bool (*cb)(), uint8_t index = 0);
bool mstp_baud_rate_set(uint32_t baud);
bool mstp_baud_rate_set(uint32_t baud, uint8_t index);
void mstp_silence_reset();

uint8_t mstp_ActiveInstanceIndexGet();
void mstp_ActiveInstanceIndexSet(uint8_t index);



#if defined (BACNET_ROUTER)
template <class Peripheral>
class MSTP_port : public RouterPort {
    public:
    MSTP_port(BacnetDatalink datalink, uint16_t network)
    : RouterPort(datalink, network)
    {
    }
    Peripheral& active()
    {
        return *activeInstance;
    }
    bool registerPeripheral(BacnetRouterClass* router, Peripheral *peripheral)
    {
        uint8_t portIndex = router->getPortIndex(net);
        if (!peripheral || portIndex >= BACNET_ROUTER_MSTP_PORTS_COUNT || portIndex < 0)
            return false;

        instances[portIndex] = peripheral;
        return true;
    }
    private:
    static Peripheral *instances[BACNET_ROUTER_MSTP_PORTS_COUNT];
    static Peripheral *activeInstance;
    void activePeripheralSet() override {
        if (portIndex < BACNET_ROUTER_MSTP_PORTS_COUNT && instances[portIndex]) {
            mstp_ActiveInstanceIndexSet(portIndex);
            activeInstance = instances[portIndex];
        }
    } 
};

template <class Peripheral>
Peripheral *MSTP_port<Peripheral>::activeInstance = nullptr;

template <class Peripheral>
Peripheral *MSTP_port<Peripheral>::instances[BACNET_ROUTER_MSTP_PORTS_COUNT] = {};

#endif


#endif // PORTS_BACNET_MSTP_H


