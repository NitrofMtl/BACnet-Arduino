#ifndef PORTS_BACNET_MSTP_H
#define PORTS_BACNET_MSTP_H

#include <Arduino.h>

#include "bacnet/bacdef.h"



#include "ports/arduino/bacnet_core.h"

#define BACNET_PRIORITY_NORMAL 16
#define BACNET_PRIORITY_OPERATOR 8
#define BACNET_PRIORITY_ALARM 6
#define BACNET_PRIORITY_EMERGENCY 1
#define BACNET_PRIORITY_RELEASE 0



void init_bacnet_mstp(int8_t mac);
void mstp_nmax_master_set(uint8_t nmax_master);

void bacnet_Poll_mstp();

void mstp_callback_send_set(void (*cb)(const uint8_t *p, uint16_t len));
void mstp_callback_read_set(bool (*cb)(uint8_t *b));
void mstp_callback_transmitting_set(bool (*cb)());
bool mstp_baud_rate_set(uint32_t baud);
void mstp_silence_reset();




#endif // PORTS_BACNET_MSTP_H


