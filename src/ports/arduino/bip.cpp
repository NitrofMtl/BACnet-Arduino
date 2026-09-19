/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/



#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <Ethernet.h>
#include <Udp.h>
//#include <string.h>
//#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/datalink/bip.h"
//#include "bvlc-arduino.h"
//#include "socketWrapper.h"
//#include "w5100Wrapper.h"


#include <Udp.h>
#include "bacnet_udp_port.h"

#if PRINT_ENABLED | DEBUG
#include <stdio.h> /* for standard i/o, like printing */
#endif

#if defined (ROUTER_DEVICE)
#include "dlcustom.h"
#endif

UDP* bacnetUdp; 


void bip_get_my_address(BACNET_ADDRESS *my_address)
{
    IPAddress ip = Ethernet.localIP();

    my_address->mac_len = 6;

    my_address->mac[0] = ip[0];
    my_address->mac[1] = ip[1];
    my_address->mac[2] = ip[2];
    my_address->mac[3] = ip[3];

    uint16_t port = bacnet_get_udp_port();
    my_address->mac[4] = port >> 8 & 0xFF;
    my_address->mac[5] = port & 0xFF;

    my_address->net = 0;
    my_address->len = 0;
}


void bip_get_broadcast_address(BACNET_ADDRESS *dest)
{
    dest->mac_len = 6;

    dest->mac[0] = 255;
    dest->mac[1] = 255;
    dest->mac[2] = 255;
    dest->mac[3] = 255;

    uint16_t port = bacnet_get_udp_port();
    dest->mac[4] = port >> 8 & 0xFF;
    dest->mac[5] = port & 0xFF;

    dest->net = BACNET_BROADCAST_NETWORK;
    dest->len = 0;
}


bool bip_init(char *ifname)
{
    (void)ifname;

    //udp.begin(47808); // BAC0 //udp init in sketch Setup()

    return true;
}


/** Function to send a packet out the BACnet/IP socket (Annex J).
 * @ingroup DLBIP
 *
 * @param dest [in] Destination address (may encode an IP address and port #).
 * @param npdu_data [in] The NPDU header (Network) information (not used).
 * @param pdu [in] Buffer of data to be sent - may be null (why?).
 * @param pdu_len [in] Number of bytes in the pdu buffer.
 * @return Number of bytes sent on success, negative number on failure.
 */
int bip_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    uint8_t mtu[1500];
    uint16_t mtu_len = 0;

    (void)npdu_data;

    mtu[0] = BVLL_TYPE_BACNET_IP;

    if(dest->mac_len == 0)
    {
        mtu[1] = BVLC_ORIGINAL_BROADCAST_NPDU;
    }
    else
    {
        mtu[1] = BVLC_ORIGINAL_UNICAST_NPDU;
    }

    encode_unsigned16(&mtu[2], pdu_len + 4);

    memcpy(&mtu[4], pdu, pdu_len);

    mtu_len = pdu_len + 4;

    IPAddress ip(
        dest->mac[0],
        dest->mac[1],
        dest->mac[2],
        dest->mac[3]);

    uint16_t port =
        ((uint16_t)dest->mac[4] << 8) |
        dest->mac[5];

    g_bacnet_udp->beginPacket(ip, port);
    g_bacnet_udp->write(mtu, mtu_len);
    g_bacnet_udp->endPacket();

    return mtu_len;
}


/** Implementation of the receive() function for BACnet/IP; receives one
 * packet, verifies its BVLC header, and removes the BVLC header from
 * the PDU data before returning.
 *
 * @param src [out] Source of the packet - who should receive any response.
 * @param pdu [out] A buffer to hold the PDU portion of the received packet,
 *                  after the BVLC portion has been stripped off.
 * @param max_pdu [in] Size of the pdu[] buffer.
 * @param timeout [in] The number of milliseconds to wait for a packet.
 * @return The number of octets (remaining) in the PDU, or zero on failure.
 */
uint16_t bip_receive(
    BACNET_ADDRESS *src, /* source address */
    uint8_t *pdu, /* PDU data */
    uint16_t max_pdu, /* amount of space available in the PDU  */
    unsigned timeout)
{
    (void)timeout;

    if (g_bacnet_udp == nullptr) {
        return 0;
    }

    int packetSize = g_bacnet_udp->parsePacket();

    if(packetSize <= 0)
        return 0;

    uint8_t buffer[1500];

    int len = g_bacnet_udp->read(buffer, sizeof(buffer));

    if(len < 4) return 0;

//Validate BACnet/IP
    if(buffer[0] != BVLL_TYPE_BACNET_IP) return 0;

//Validate BVLC function
    if(buffer[1] != BVLC_ORIGINAL_UNICAST_NPDU &&
    buffer[1] != BVLC_ORIGINAL_BROADCAST_NPDU)
    {
        return 0;
    }

//Extract sender
    IPAddress remoteIp = g_bacnet_udp->remoteIP();
    uint16_t remotePort = bacnet_get_udp_port();

    src->mac_len = 6;

    src->mac[0] = remoteIp[0];
    src->mac[1] = remoteIp[1];
    src->mac[2] = remoteIp[2];
    src->mac[3] = remoteIp[3];

    src->mac[4] = remotePort >> 8;
    src->mac[5] = remotePort & 0xFF;

    
    uint16_t npdu_len = len - 4;

    if(npdu_len > max_pdu)
        npdu_len = max_pdu;

    memcpy(pdu, &buffer[4], npdu_len);

    return npdu_len;
}


#if defined (BACDL_BIP)
struct custom_BIP_packet_t{
    BACNET_ADDRESS dest;
    uint8_t pdu[BIP_MPDU_MAX];
    unsigned pdu_len;
    bool pending;
};

static custom_BIP_packet_t custom_BIP_packet;


uint16_t datalink_custom_BIP_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout)
{
    (void)timeout;

    if (!custom_BIP_packet.pending)
        return 0;

    uint8_t *buffer = custom_BIP_packet.pdu;
    uint16_t len = custom_BIP_packet.pdu_len;

    if (len < 4) {
        custom_BIP_packet.pending = false;
        return 0;
    }

    // Validate BACnet/IP BVLL type
    if (buffer[0] != BVLL_TYPE_BACNET_IP) {
        custom_BIP_packet.pending = false;
        return 0;
    }

    // Validate BVLC function
    if (buffer[1] != BVLC_ORIGINAL_UNICAST_NPDU &&
        buffer[1] != BVLC_ORIGINAL_BROADCAST_NPDU) {
        custom_BIP_packet.pending = false;
        return 0;
    }

    // Extract source address from the captured destination.
    //
    // The captured BIP frame was generated for this destination,
    // so preserve the same BACnet/IP address representation.
    *src = custom_BIP_packet.dest;

    uint16_t npdu_len = len - 4;

    if (npdu_len > max_pdu) {
        custom_BIP_packet.pending = false;
        return 0;
    }

    memcpy(
        pdu,
        &buffer[4],
        npdu_len);

    custom_BIP_packet.pending = false;

    return npdu_len;
}


int custom_BIP_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    uint16_t mtu_len = pdu_len + 4;

    (void)npdu_data;

    if (!dest || !pdu)
        return 0;

    if (mtu_len > sizeof(custom_BIP_packet.pdu))
        return 0;

    if (custom_BIP_packet.pending)
        return 0;

    custom_BIP_packet.dest = *dest;

    custom_BIP_packet.pdu[0] = BVLL_TYPE_BACNET_IP;

    if (dest->mac_len == 0) {
        custom_BIP_packet.pdu[1] =
            BVLC_ORIGINAL_BROADCAST_NPDU;
    } else {
        custom_BIP_packet.pdu[1] =
            BVLC_ORIGINAL_UNICAST_NPDU;
    }

    encode_unsigned16(
        &custom_BIP_packet.pdu[2],
        mtu_len);

    memcpy(
        &custom_BIP_packet.pdu[4],
        pdu,
        pdu_len);

    custom_BIP_packet.pdu_len = mtu_len;
    custom_BIP_packet.pending = true;

    return mtu_len;
}


#endif
