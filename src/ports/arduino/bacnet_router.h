#pragma once


#include "ports/arduino/bacnet.h"
#include "ports/arduino/bacnet_mstp.h"



#define BACNET_ROUTER_MAX_ROUTES 128


enum class BacnetDatalink : uint8_t {
#ifdef BACNET_ROUTER_BIP
    BIP,
#endif 
#ifdef BACNET_ROUTER_ETHERNET
    ETHERNET,
#endif
#ifdef BACNET_ROUTER_ARCNET
    ARCNET,
#endif
#ifdef BACDL_BIP6
    BIP6,
#endif
#ifdef BACNET_ROUTER_MSTP
    MSTP,
#endif
#ifdef BACDL_ZIGBEE
    ZIGBEE,
#endif
#ifdef BACNET_ROUTER_BSC
    BSC,
#endif
};

struct RouterNetwork {
    BacnetDatalink port;
    uint16_t network;
};

class BacnetRouterClass {
    public:
    BacnetRouterClass()
    {  
    }

    template <typename... RouterNetworks>
    void begin(RouterNetworks... networks)
    {
        addPorts(networks...);
    }

    void poll();

    struct RouterPort {
        BACNET_ADDRESS address;
        uint16_t net;
        BacnetDatalink portType;
    };
    struct RouterRoute {
        BACNET_ADDRESS address;
        RouterPort *port;
        uint16_t net;
    };
    struct RouterDevice {
    int8_t portIndex = -1u;
    };
    bool attachRouterDevice(BacnetDatalink portType);
    
    private:
    template <typename... RouterNetworks>
    bool addPorts(RouterNetwork firstNetwork, RouterNetworks... restNetworks)
    {
        if (!addPort(firstNetwork.port, firstNetwork.network))
            return false;
        return addPorts(restNetworks...);
    }
    bool addPorts(RouterNetwork network) { 
        return addPort(network.port, network.network);
    } // Base case for recursion
    bool addPort(BacnetDatalink portType, uint16_t network);
    
    RouterPort *findPort(uint16_t network);
    int sendPdu(
        const RouterPort &port,
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
    void datalinkSet(BacnetDatalink portType);
#if defined(ROUTER_DEVICE)
    bool isRouterDevice(RouterPort *port, BACNET_ADDRESS routeDest);
    bool handleLocalDevice(
        uint16_t snet,
        BACNET_ADDRESS &src,
        uint8_t *apdu,
        uint16_t apduLen
    );
#endif
    


    RouterPort _ports[BACNET_ROUTER_MAX_PORTS] = {};
    uint8_t _portsCount = 0;
    RouterRoute _routes[BACNET_ROUTER_MAX_ROUTES];
    uint16_t _routesCount = 0;
    RouterDevice _routerDevice;


    // router ports/table
};


