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
    MSTP_port(BacnetDatalink datalink, uint16_t network, Peripheral *peripheral)
    : RouterPort(datalink, network)
    {
        if (!peripheral) return;

        for (uint8_t i = 0; i < BACNET_MSTP_PORTS_COUNT; i++) {
            if (instances[i] == nullptr) {
                
                _portIndex = i;
                instances[_portIndex] = peripheral;
                break;
            }
        }
    }
    Peripheral& active()
    {
        return *activeInstance;
    }
    bool isActive() {
        uint8_t index = mstp_ActiveInstanceIndexGet();
        if (instances[index] && instances[index] == activeInstance) return true;
        return false;
    }
    uint8_t instanceIndexGet() { return _portIndex; }
    bool isReady() { return _portIndex != 255; }
    private:
    uint8_t _portIndex = 255;
    static Peripheral *instances[BACNET_ROUTER_MSTP_PORTS_COUNT];
    static Peripheral *activeInstance;
    void activePeripheralSet() override {
        if (_portIndex < BACNET_ROUTER_MSTP_PORTS_COUNT && instances[_portIndex]) {
            mstp_ActiveInstanceIndexSet(_portIndex);
            activeInstance = instances[_portIndex];
        }
    } 
};

template <class Peripheral>
Peripheral *MSTP_port<Peripheral>::activeInstance = nullptr;

template <class Peripheral>
Peripheral *MSTP_port<Peripheral>::instances[BACNET_ROUTER_MSTP_PORTS_COUNT] = {};

#endif


#endif // PORTS_BACNET_MSTP_H


