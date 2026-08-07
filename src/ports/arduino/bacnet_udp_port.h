#pragma once

#include <Udp.h>

extern UDP *g_bacnet_udp;

/**
 * @brief Set the UDP interface used by BACnet/IP.
 *
 * Registers the UDP object and local port used by the BACnet/IP
 * transport layer.
 *
 * @param udp Pointer to the Arduino UDP interface.
 * @param port Local BACnet/IP UDP port.
 */
void bacnet_set_udp(UDP *udp, uint16_t port);

/**
 * @brief Get the registered UDP interface.
 *
 * @return Pointer to the UDP interface used by BACnet/IP.
 */
UDP *bacnet_get_udp();

/**
 * @brief Get the configured BACnet/IP UDP port.
 *
 * @return Local UDP port used by the BACnet/IP transport.
 */
uint16_t bacnet_get_udp_port();