#include "ports/arduino/bacnet_udp_port.h"

UDP *g_bacnet_udp = nullptr;

static uint16_t bacnetUdpPort = 0;

//static UDP *g_bacnet_udp;

void bacnet_set_udp(UDP *udp, uint16_t port)
{
    g_bacnet_udp = udp;
    bacnetUdpPort = port;
}


inline UDP *bacnet_get_udp()
{
    return g_bacnet_udp;
}


uint16_t bacnet_get_udp_port()
{
    return bacnetUdpPort;
}