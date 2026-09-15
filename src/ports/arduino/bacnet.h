/**
 * @file bacnet.h
 * @brief Main BACnet/IP interface for Arduino.
 *
 * This header provides the primary API for initializing and running the
 * BACnet/IP stack on Arduino-compatible platforms.
 *
 * Include this file in applications using BACnet/IP. Additional transport
 * headers may be included to enable other BACnet datalinks, such as MS/TP.
 */

#ifndef PORTS_BACNET_H
#define PORTS_BACNET_H

#include <Arduino.h>

//arduino libs
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <NTPClient.h>

//ports lib
#include "ports/arduino/bacnet_udp_port.h"
#include "ports/arduino/bacnet_core.h"

#define BACNET_PRIORITY_NORMAL 16
#define BACNET_PRIORITY_OPERATOR 8
#define BACNET_PRIORITY_ALARM 6
#define BACNET_PRIORITY_EMERGENCY 1
#define BACNET_PRIORITY_RELEASE 0

/**
 * @brief Process pending BACnet communication tasks.
 *
 * Handles incoming BACnet messages and processes the required protocol
 * services.
 *
 * This function must be called periodically by the application main loop
 * when using BACnet/IP or other polled datalink implementations.
 *
 * @note The exact transport processing depends on the configured BACnet
 * datalink.
 */
void bacnet_Poll();


#endif // PORTS_BACNET_H


