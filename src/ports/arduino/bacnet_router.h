#pragma once


#include "ports/arduino/bacnet.h"
#include "ports/arduino/bacnet_mstp.h"
#include "ports/arduino/bacnet_peripheral.h"
#include "BIP_router_port.h"

#define NO_ROUTER_DEVICE 0xFFu

#define BACNET_ROUTER_MAX_ROUTES 128





template <typename T>
constexpr T constexpr_max(T value)
{
    return value;
}

template <typename T, typename... Ts>
constexpr T constexpr_max(T first, Ts... rest)
{
    //T remaining = constexpr_max(rest...);
    //return first > remaining ? first : remaining;
    return first > constexpr_max(rest...)
        ? first
        : constexpr_max(rest...);
}

struct RouterNetwork {
    BacnetDatalink port;
    uint16_t network;
};

class BacnetRouterClass {
    public:
    BacnetRouterClass() {}

    template <typename First, typename Second, typename... Rest>
    bool begin(First *firstPort, Second *secondPort, Rest*... restPorts)
    {
        if (!addPort(firstPort))
            return false;

        return begin(secondPort, restPorts...);
    }
    bool begin(RouterPort *port)
    {
        return addPort(port);
    }
    void poll();
    struct RouterRoute {
        BACNET_ADDRESS address;
        RouterPort *port;
        uint16_t net;
    };
    int8_t getPortIndex(uint16_t network);

    private:
     bool addPort(RouterPort *port);
    RouterPort *findPort(uint16_t network);
    int sendPdu(
        RouterPort &port,
        BACNET_ADDRESS &dest,
        BACNET_NPDU_DATA &npdu,
        uint8_t *pdu,
        uint16_t pduLen
    );
    BACNET_ADDRESS getBroadcastAddress() const;
    int encodeRoutedPdu(
        uint8_t *pdu,
        BACNET_ADDRESS &dest,
        BACNET_ADDRESS &src,
        uint16_t snet,
        BACNET_NPDU_DATA &npdu,
        uint8_t *apdu,
        uint16_t apduLen
    );
    bool routeApdu(
        uint16_t snet,
        BACNET_ADDRESS &src,
        BACNET_ADDRESS &dest,
        BACNET_NPDU_DATA &npdu,
        uint8_t *apdu,
        uint16_t apduLen
    );
    bool routeBroadcastApdu(
        uint16_t snet,
        BACNET_ADDRESS &src,
        BACNET_NPDU_DATA &npdu,
        uint8_t *apdu,
        uint16_t apduLen
    );
    bool routeDirectApdu(
        uint16_t snet,
        BACNET_ADDRESS &src,
        BACNET_ADDRESS &dest,
        BACNET_NPDU_DATA &npdu,
        uint8_t *apdu,
        uint16_t apduLen
    );
    bool routedSrcAddress(
        BACNET_ADDRESS &routerSrc,
        uint16_t snet,
        const BACNET_ADDRESS &src
    );
    bool routeUnknownApdu(
        uint16_t snet,
        BACNET_ADDRESS &src,
        BACNET_ADDRESS &dest,
        BACNET_NPDU_DATA &npdu,
        uint8_t *apdu,
        uint16_t apduLen
    );
    void routingNpduHandler(
        uint16_t snet,
        BACNET_ADDRESS &src,
        uint8_t *pdu,
        uint16_t pduLen
    );
    void networkControlHandler(
        uint16_t snet,
        BACNET_ADDRESS &src,
        BACNET_NPDU_DATA &npdu,
        uint8_t *pdu,
        uint16_t pduLen
    );
    bool addRoute(
        uint16_t snet,
        uint16_t net,
        const BACNET_ADDRESS &addr
    );
    RouterRoute *findRoute(uint16_t network);
    void processIAmRouterToNetwork(
        uint16_t snet,
        const BACNET_ADDRESS &src,
        const uint8_t *npdu,
        uint16_t npduLen
    );
    void whoIsRouterToNetworkHandler(
        uint16_t snet,
        const BACNET_ADDRESS &src,
        BACNET_NPDU_DATA &npduData,
        const uint8_t *npdu,
        uint16_t npduLen
    );
    void sendWhoIsRouterToNetwork(
        uint16_t snet,
        uint16_t dnet,
        BACNET_ADDRESS *src = nullptr
    );
    void sendIAmRouterToNetwork(
        uint16_t snet,
        uint16_t net
    );
    void processInitializeRoutingTable(
        uint16_t snet,
        const BACNET_ADDRESS &src,
        const uint8_t *npdu,
        uint16_t npduLen
    );
    void sendInitializeRoutingTableAck(
        uint16_t snet,
        const BACNET_ADDRESS *dst
    );
    void sendRejectMessageToNetwork(
        uint16_t snet,
        const BACNET_ADDRESS *dst,
        uint8_t rejectReason,
        uint16_t dnet
    );
#if defined(ROUTER_DEVICE)
    bool isRouterDevice(RouterPort *port, BACNET_ADDRESS routeDest);
    bool handleLocalDevice(
        uint16_t snet,
        BACNET_ADDRESS &src,
        uint8_t *apdu,
        uint16_t apduLen
    );
#endif

    RouterPort *_ports[BACNET_ROUTER_MAX_PORTS] = {};
    RouterRoute _routes[BACNET_ROUTER_MAX_ROUTES] {};
    uint16_t _routesCount = 0;
    uint8_t _portsCount = 0;


    // router ports/table
};


void printBacnetAddress(char* tag, const BACNET_ADDRESS &addr);


#if defined (BACNET_ROUTER)
#include "bacnet_mstp_router_impl.h"
#endif