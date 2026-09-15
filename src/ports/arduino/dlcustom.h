#pragma once

#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/datalink/dlmstp.h"

#include "ports/arduino/arduino_configs.h"


/*
Here all datalink macro function. to see how much need to be defined.. ?
#define datalink_init dlmstp_init
#define datalink_send_pdu dlmstp_send_pdu
#define datalink_receive dlmstp_receive
#define datalink_cleanup dlmstp_cleanup
#define datalink_get_broadcast_address dlmstp_get_broadcast_address
#define datalink_get_my_address dlmstp_get_my_address
#define datalink_maintenance_timer(s)

*/

#if defined(BACDL_CUSTOM)

uint16_t custom_receive_pdu(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout
);

int custom_send(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len
);


typedef struct {
    BACNET_ADDRESS dest;
    uint8_t pdu[DLMSTP_MPDU_MAX]; //must be macroed....
    unsigned pdu_len;
    bool pending;
}custom_packet_t;

#endif//BACDL_CUSTOM
