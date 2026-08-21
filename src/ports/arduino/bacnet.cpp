#include "ports/arduino/bacnet.h"

#include "bacnet/basic/object/device.h"
#include "ports/arduino/bacnet_udp_port.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/basic/npdu/h_npdu.h"
#include "bacnet/basic/service/h_apdu.h"
#include "bacnet/basic/service/h_whois.h"
#include "bacnet/basic/service/h_iam.h"
#include "bacnet/basic/service/h_rp.h"
#include "bacnet/bacapp.h"
#include "bacnet/basic/service/h_wp.h"
#include "bacnet/basic/service/h_cov.h"
#include "bacnet/basic/service/h_rpm.h"
#include "bacnet/basic/service/h_dcc.h"
#include "bacnet/basic/service/h_ts.h"
#include "bacnet/basic/service/h_rd.h"

#include "ports/arduino/device.h"


void bacnet_Poll()
{
    BACNET_ADDRESS src;
    uint8_t rx_buf[512];

    uint16_t len = bip_receive(&src, rx_buf, sizeof(rx_buf), 0);
    if (len > 0) {
        npdu_handler(&src, rx_buf, len);
    }
    bacnet_handle_reinitialize();
    BACnetSchedule::handler();
}


