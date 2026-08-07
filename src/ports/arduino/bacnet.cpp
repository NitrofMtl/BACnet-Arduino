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


/**
 * @brief Retrieve local BACnet datalink address
 *
 * Returns the local device datalink address used by the BACnet stack
 * for communication on the network layer.
 *
 * This function MUST be implemented by each port depending on
 * its datalink type (IP, MS/TP, UART, etc.).
 *
 * @param[out] my_address Pointer to BACNET_ADDRESS structure to fill
 *
 * @note The returned address is used by the stack for routing
 *       and message filtering.
 */
void datalink_get_my_address(BACNET_ADDRESS *my_address)//"bacnet/datalink/
{
   IPAddress ip = Ethernet.localIP();

    my_address->mac_len = 6;

    my_address->mac[0] = ip[0];
    my_address->mac[1] = ip[1];
    my_address->mac[2] = ip[2];
    my_address->mac[3] = ip[3];

    //my_address->mac[4] = (bacnetUdpPort >> 8) & 0xFF;
    //my_address->mac[5] = bacnetUdpPort & 0xFF;
    my_address->mac[4] = (bacnet_get_udp_port() >> 8) & 0xFF;
    my_address->mac[5] = bacnet_get_udp_port() & 0xFF;

    my_address->net = 0;
    my_address->len = 0;

    memset(my_address->adr, 0, MAX_MAC_LEN);
}


//static BACNET_CHARACTER_STRING Device_Name;

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


