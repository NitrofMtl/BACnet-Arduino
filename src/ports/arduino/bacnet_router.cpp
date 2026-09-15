#include "ports/arduino/bacnet_router.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/npdu.h"

#include "ports/arduino/dlcustom.h"

#include "bacnet_debug.h"

void printBacnetAddress(char* tag, const BACNET_ADDRESS &addr)
{
    Serial.print(tag);
    Serial.print(": mac_len=");
    Serial.print(addr.mac_len);
    Serial.print(" MAC: ");
    for (uint8_t i = 0; i < addr.mac_len; i++) {
        Serial.print(addr.mac[i], HEX);
        if (i < addr.mac_len - 1) Serial.print(":");
    }
    Serial.print(", NET: ");
    Serial.print(addr.net);
    Serial.print(", LEN: ");
    Serial.print(addr.len);
    Serial.print(", ADR: ");
    for (uint8_t i = 0; i < addr.len; i++) {
        Serial.print(addr.adr[i], HEX);
        if (i < addr.len - 1) Serial.print(":");
    }
    Serial.println("-----");
}

constexpr size_t TX_BUFFER_SIZE = max(DLMSTP_MPDU_MAX, BIP_MPDU_MAX);
uint8_t _txBuffer[TX_BUFFER_SIZE] = {};


bool BacnetRouterClass::addPort(BacnetDatalink portType, uint16_t network)
{
    BACNET_ADDRESS addr = {};
    switch (portType){

#ifdef BACNET_ROUTER_BIP
        case BacnetDatalink::BIP:
            bip_get_my_address(&addr);
            break;
#endif 
#ifdef BACNET_ROUTER_ETHERNET
        case BacnetDatalink::ETHERNET:
            ethernet_get_my_address(&addr);
            break;
#endif
#ifdef BACNET_ROUTER_ARCNET
        case BacnetDatalink::ARCNET:
            arcnet_get_my_address(&addr);
            break;
#endif
#ifdef BACNET_ROUTER_BIP6
        case BacnetDatalink::BIP6:
            bip6_get_my_address(&addr);
            break;
#endif
#ifdef BACNET_ROUTER_MSTP
        case BacnetDatalink::MSTP:
            dlmstp_get_my_address(&addr);
            break;
#endif
#ifdef BACNET_ROUTER_ZIGBEE
        case BacnetDatalink::ZIGBEE:
            bzll_get_my_address(&addr);
            break;
#endif
#ifdef BACNET_ROUTER_BSC
        case BacnetDatalink::BSC:
            bsc_get_my_address(&addr);
            break;
#endif
    default:
        return false;
    }
    if (_portsCount >= BACNET_ROUTER_MAX_PORTS) return false; //slots already full
    if ( network == 0 || network == BACNET_BROADCAST_NETWORK ) return false;

    for (uint8_t i = 0; i < _portsCount; i++) {
        if (_ports[i].portType == portType) return false; //port type already exists
        if (_ports[i].net == network) return false; // network number already exists
    }
    uint8_t slot = _portsCount;
    RouterPort rp{addr, network, portType};
    _ports[slot] = rp;

    _portsCount++;

    return true;
}


void BacnetRouterClass::poll()
{
    static uint8_t pdu[512];

#if defined(ROUTER_DEVICE)
    RouterPort &localPort = _ports[_routerDevice.portIndex];
    datalinkSet(BacnetDatalink::CUSTOM);

    BACNET_ADDRESS src = {};
    uint16_t len = datalink_receive(
        &src,
        pdu,
        sizeof(pdu),
        0);
        //BACNET_ADDRESS src = {};

    if (len > 0) {
src.mac_len = 1;
src.mac[0] = 0;
src.net = 0;
src.len = 0;

uint8_t testPdu[] = {
    0x01, 0x20, 0xFF, 0xFF, 0x00, 0xFF, 0x10, 0x00,
    0xC4, 0x02, 0x00, 0x00, 0x02, 0x22, 0x01, 0xE0,
    0x91, 0x03, 0x22, 0x01, 0x04
};

Serial.println("Injecting Router Device I-Am test");

printBacnetAddress("src:", src);

Serial.print("TEST PDU len: ");
Serial.println(sizeof(testPdu));

Serial.print("TEST PDU: ");
for (uint16_t i = 0; i < sizeof(testPdu); i++) {
    if (testPdu[i] < 0x10)
        Serial.print('0');
    Serial.print(testPdu[i], HEX);
    Serial.print(' ');
}
Serial.println();

routingNpduHandler(
    2000,
    src,
    testPdu,
    sizeof(testPdu));
/*        Serial.println("Receive from custom local device");
        Serial.print("CUSTOM RX len: ");
        Serial.println(len);

        Serial.print("CUSTOM RX PDU: ");

        for (uint16_t i = 0; i < len; i++) {
            if (pdu[i] < 0x10)
                Serial.print("0");

            Serial.print(testPdu[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        printBacnetAddress("src: ", src);

        routingNpduHandler(
            localPort.net,
            src,
            pdu,
            len);*/
    }

#endif

    for (uint8_t i = 0; i < _portsCount; i++) {
        RouterPort &port = _ports[i];

        datalinkSet(port.portType);

        BACNET_ADDRESS src = {};
        uint16_t len = datalink_receive(
            &src,
            pdu,
            sizeof(pdu),
            0);

        if (len > 0) {
                    Serial.print("regular receive RX len: ");
                    Serial.println(len);

                    Serial.print("RX PDU: ");

                    for (uint16_t i = 0; i < len; i++) {
                        if (pdu[i] < 0x10)
                            Serial.print("0");

                        Serial.print(pdu[i], HEX);
                        Serial.print(" ");
                    }
                    Serial.println();
                    printBacnetAddress("src: ", src);

            routingNpduHandler(
                port.net,
                src,
                pdu,
                len);
        }
    }
}


bool BacnetRouterClass::attachRouterDevice(BacnetDatalink portType) {
    for (uint8_t i = 0; i < _portsCount; i++) {
        if (_ports[i].portType == portType) {
            _routerDevice.portIndex = i;
            return true;
        }
    }
    return false;
}


BacnetRouterClass::RouterPort *BacnetRouterClass::findPort(uint16_t network)
{
    for (uint8_t i = 0; i < _portsCount; i++) {
        if (_ports[i].net == network)
            return &_ports[i];
    }

    return nullptr;
}


BACNET_ADDRESS BacnetRouterClass::getBroadcastAddress() const
{
    BACNET_ADDRESS dest = {};

    dest.mac_len = 0;
    dest.net = BACNET_BROADCAST_NETWORK;
    dest.len = 0;

    return dest;
}


int BacnetRouterClass::sendPdu(
    const RouterPort &port,
    BACNET_ADDRESS &dest,
    BACNET_NPDU_DATA &npdu,
    uint8_t *pdu,
    uint16_t pduLen)
{
    datalinkSet(port.portType);
    return datalink_send_pdu(&dest, &npdu, pdu, pduLen);
}


int BacnetRouterClass::encodeRoutedPdu(
    uint8_t *pdu,
    BACNET_ADDRESS &dest,
    BACNET_ADDRESS &src,
    uint16_t snet,
    BACNET_NPDU_DATA &npdu,
    uint8_t *apdu,
    uint16_t apduLen)
{
    BACNET_ADDRESS routerSrc = {};

    if (!routedSrcAddress(routerSrc, snet, src))
        return 0;

    int npduLen =
        npdu_encode_pdu(pdu, &dest, &routerSrc, &npdu);

    if (npduLen <= 0)
        return 0;

    if ((npduLen + apduLen) > TX_BUFFER_SIZE)
        return 0;

    if (apdu && apduLen)
        memcpy(&pdu[npduLen], apdu, apduLen);

    return npduLen + apduLen;
}


bool BacnetRouterClass::routedSrcAddress(
    BACNET_ADDRESS &routerSrc,
    uint16_t snet,
    const BACNET_ADDRESS &src)
{
    RouterPort *port = findPort(snet);

    if (!port) return false;

    if (src.net) {
        // Routed packet.
        // Downstream route learning can be added later.
        routerSrc.net = src.net;
        routerSrc.len = src.len;

        for (uint8_t i = 0; i < MAX_MAC_LEN; i++)
            routerSrc.adr[i] = src.adr[i];

    } else {
        // Packet originated on one of our directly connected networks.
        routerSrc.net = snet;
        routerSrc.len = src.mac_len;

        for (uint8_t i = 0; i < MAX_MAC_LEN; i++)
            routerSrc.adr[i] = src.mac[i];
    }
    return true;
}


bool BacnetRouterClass::routeApdu(
    uint16_t snet,
    BACNET_ADDRESS &src,
    BACNET_ADDRESS &dest,
    BACNET_NPDU_DATA &npdu,
    uint8_t *apdu,
    uint16_t apduLen)
{
    Serial.println("routeApdu");
    // Global broadcast
    if (dest.net == BACNET_BROADCAST_NETWORK) {
        return routeBroadcastApdu(snet, src, npdu, apdu, apduLen);
    }
    Serial.println("not a broadcast");
    printBacnetAddress("dest: ", dest);
    Serial.print("dest.net = ");
Serial.println(dest.net);
    // No DNET/DADR: destination is on the network
    // the packet arrived from.
    if (dest.net == 0) {
        if (snet == _ports[_routerDevice.portIndex].net) {
            datalinkSet(
            _ports[_routerDevice.portIndex].portType);
            apdu_handler(&src, apdu, apduLen);
            return true;
        }

        return false;
    }

    // Destination is a specific network.
    RouterPort *port = findPort(dest.net);

    if (!port) {
        return routeUnknownApdu(
            snet, src, dest, npdu, apdu, apduLen);
    }

    // Directly connected destination.
    if (port->net != snet) {

        return routeDirectApdu(
            snet, src, dest, npdu, apdu, apduLen);
        }
    // Explicitly addressed to the same network we received from.
    if (handleLocalDevice(snet, src, dest, apdu, apduLen)) {
        return true;
    }

    // It wasn't our local device.
    return false;
}


bool BacnetRouterClass::routeBroadcastApdu(
        uint16_t snet,
        BACNET_ADDRESS &src,
        BACNET_NPDU_DATA &npdu,
        uint8_t *apdu,
        uint16_t apduLen)
{
    BACNET_ADDRESS npduDest = getBroadcastAddress();
    BACNET_ADDRESS routerSrc = {};
    Serial.println("routeBroadcastApdu");
    if (npdu.hop_count == 0)
        return false;

    npdu.hop_count--;

    if (!routedSrcAddress(routerSrc, snet, src))
        return false;

    const int pduLen = encodeRoutedPdu(
        _txBuffer,
        npduDest,
        routerSrc,
        snet,
        npdu,
        apdu,
        apduLen);

    if (pduLen <= 0)
        return false;

    bool sent = false;

    for (uint8_t i = 0; i < _portsCount; i++) {
        RouterPort &port = _ports[i];
        
#if defined (ROUTER_DEVICE)
        //Broadcast for local device
        if (_routerDevice.portIndex < _portsCount) {
            if (port.net == snet) {
                datalinkSet(port.portType);
                apdu_handler(&src, apdu, apduLen);
            } else {
                datalinkSet(BacnetDatalink::CUSTOM);
                apdu_handler(&src, apdu, apduLen);
            } 
        }
#endif

        if (port.net == snet)
            continue;

        BACNET_ADDRESS datalinkDest = {};

        datalinkSet(port.portType);
        datalink_get_broadcast_address(&datalinkDest);
        Serial.print("broadcast route apdu send, snet: ");
        Serial.println(snet);
        printBacnetAddress("dest:", datalinkDest);
        Serial.print("pdu len: ");
        Serial.println(pduLen);
        if (sendPdu(port, datalinkDest, npdu, _txBuffer, pduLen)) {
            sent = true;
        }
    }

    return sent;
}


bool BacnetRouterClass::routeDirectApdu(
    uint16_t snet,
    BACNET_ADDRESS &src,
    BACNET_ADDRESS &dest,
    BACNET_NPDU_DATA &npdu,
    uint8_t *apdu,
    uint16_t apduLen)
{
    RouterPort *port = nullptr;
    BACNET_ADDRESS routeDest = dest;
    BACNET_ADDRESS routeSrc = src;

    port = findPort(routeDest.net);
    if (port) {
        for (uint8_t i = 0; i < routeDest.len; i++) {
            routeDest.mac[i] = routeDest.adr[i];
        }
        routeDest.mac_len = routeDest.len;
    }
    else{
        RouterRoute* route = findRoute(routeDest.net);
        if (!route) {
            return false;
        }
        port = route->port;
        for (uint8_t i = 0; i < route->address.mac_len; i++) {
            routeDest.mac[i] = route->address.mac[i];
        }
        routeDest.mac_len = route->address.mac_len;
    }

    if (npdu.hop_count == 0) return false;

    npdu.hop_count--;

    const int pduLen = encodeRoutedPdu(
        _txBuffer,
        dest,
        src,
        snet,
        npdu,
        apdu,
        apduLen);

    return sendPdu(
        *port,
        routeDest,
        npdu,
        _txBuffer,
        pduLen) > 0;
}


bool BacnetRouterClass::routeUnknownApdu(
    uint16_t snet,
    BACNET_ADDRESS &src,
    BACNET_ADDRESS &dest,
    BACNET_NPDU_DATA &npdu,
    uint8_t *apdu,
    uint16_t apduLen)
{
    BACNET_ADDRESS remoteDest = dest;
    BACNET_ADDRESS routerSrc = {};

    if (npdu.hop_count == 0)
        return false;

    npdu.hop_count--;

    if (!routedSrcAddress(routerSrc, snet, src))
        return false;

    // No specific destination MAC is known.
    remoteDest.mac_len = 0;

    const int pduLen = encodeRoutedPdu(
        _txBuffer,
        remoteDest,
        routerSrc,
        snet,
        npdu,
        apdu,
        apduLen);

    if (pduLen <= 0)
        return false;

    bool sent = false;

    for (uint8_t i = 0; i < _portsCount; i++) {
        RouterPort &port = _ports[i];

        if (port.net == snet)
            continue;

        if (sendPdu(
                port,
                remoteDest,
                npdu,
                _txBuffer,
                pduLen) > 0) {
            sent = true;
        }
    }

    // TODO: request a router to the unknown network.
    // sendWhoIsRouterToNetwork(0, dest.net);

    return sent;
}


void BacnetRouterClass::routingNpduHandler(
    uint16_t snet,
    BACNET_ADDRESS &src,
    uint8_t *pdu,
    uint16_t pduLen)
{
    int apduOffset = 0;
    BACNET_ADDRESS dest = {};
    BACNET_NPDU_DATA npduData = {};

    if (!pdu || pduLen == 0)
        return;

    if (pdu[0] != BACNET_PROTOCOL_VERSION)
        return;

    apduOffset = bacnet_npdu_decode(
        pdu,
        pduLen,
        &dest,
        &src,
        &npduData);

    if (apduOffset <= 0 || apduOffset > pduLen)
        return;

    /*
     * Network layer control messages are handled by the router
     * when they are addressed to this network or broadcast.
     */
    if (npduData.network_layer_message) {
        if ((dest.net == 0) ||
            (dest.net == BACNET_BROADCAST_NETWORK)) {
            networkControlHandler(
                snet,
                src,
                npduData,
                &pdu[apduOffset],
                pduLen - apduOffset);
        }

        return;
    }

    /*
     * A global broadcast must not be delivered as a confirmed
     * service request to the application.
     */
    if ((dest.net == BACNET_BROADCAST_NETWORK) &&
        ((pdu[apduOffset] & 0xF0) ==
         PDU_TYPE_CONFIRMED_SERVICE_REQUEST)) {
        // ConfirmedBroadcastReceived / IDLE.
        return;
    }

    /*
     * Route the NPDU.
     *
     * DNET == 0 means the destination is on our local network.
     * Other DNET values are either directly connected, globally
     * broadcast, or reachable through a learned route.
     */
    routeApdu(
        snet,
        src,
        dest,
        npduData,
        &pdu[apduOffset],
        pduLen - apduOffset);

    return;
}


void BacnetRouterClass::networkControlHandler(
    uint16_t snet,
    BACNET_ADDRESS &src,
    BACNET_NPDU_DATA &npduData,
    uint8_t *npdu,
    uint16_t npduLen)
{
    if (!npdu || npduLen == 0)
        return;

    switch (npduData.network_message_type) {

        case NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK:
            Serial.println("NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK");
            whoIsRouterToNetworkHandler(
                snet, src, npduData, npdu, npduLen);
            break;

        case NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK:
            Serial.println("NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK");
            processIAmRouterToNetwork(snet, src, npdu, npduLen);
            break;

        case NETWORK_MESSAGE_I_COULD_BE_ROUTER_TO_NETWORK:
            // Do nothing.
            break;

        case NETWORK_MESSAGE_REJECT_MESSAGE_TO_NETWORK:
            //if (npduLen >= 3) {
            //    decode_unsigned16(&npdu[1], &dnet);

                // Optional debug handling here.
            //}
            break;

        case NETWORK_MESSAGE_ROUTER_BUSY_TO_NETWORK:
        case NETWORK_MESSAGE_ROUTER_AVAILABLE_TO_NETWORK:
            // Traffic congestion control not supported.
            break;

        case NETWORK_MESSAGE_INIT_RT_TABLE:
            processInitializeRoutingTable(
                snet, src, npdu, npduLen);

            sendInitializeRoutingTableAck(snet, nullptr);
            break;

        case NETWORK_MESSAGE_INIT_RT_TABLE_ACK:
            // Not currently supported.
            break;

        case NETWORK_MESSAGE_ESTABLISH_CONNECTION_TO_NETWORK:
        case NETWORK_MESSAGE_DISCONNECT_CONNECTION_TO_NETWORK:
            // PTP half-router control not supported.
            break;

        default:
            sendRejectMessageToNetwork(
                snet,
                &src,
                NETWORK_REJECT_UNKNOWN_MESSAGE_TYPE,
                0);
            break;
    }
}


bool BacnetRouterClass::addRoute(
        uint16_t snet,
        uint16_t net,
        const BACNET_ADDRESS &addr)
{
    // A directly connected network doesn't need a route entry.
    if (findPort(net))
        return false;

    // Don't add the same network twice.
    if (findRoute(net))
        return false;

    // The route must be reachable through one of our ports.
    RouterPort *port = findPort(snet);
    if (!port)
        return false;

    if (_routesCount >= BACNET_ROUTER_MAX_ROUTES)
        return false;

    RouterRoute &route = _routes[_routesCount];

    route.net = net;
    route.address = addr;
    route.port = port;

    _routesCount++;

    return true;
}


BacnetRouterClass::RouterRoute *BacnetRouterClass::findRoute(uint16_t network)
{
    for (uint16_t i = 0; i < _routesCount; i++) {
        if (_routes[i].net == network)
            return &_routes[i];
    }

    return nullptr;
}


void BacnetRouterClass::processIAmRouterToNetwork(
    uint16_t snet,
    const BACNET_ADDRESS &src,
    const uint8_t *npdu,
    uint16_t npduLen)
{
    uint16_t offset = 0;

    while (npduLen >= 2) {
        uint16_t dnet = 0;

        decode_unsigned16(&npdu[offset], &dnet);

        addRoute(snet, dnet, src);

        offset += 2;
        npduLen -= 2;
    }
}


void BacnetRouterClass::whoIsRouterToNetworkHandler(
    uint16_t snet,
    const BACNET_ADDRESS &src,
    BACNET_NPDU_DATA &npduData,
    const uint8_t *npdu,
    uint16_t npduLen)
{
        (void)src;
    (void)npduData;

    if (!npdu || npduLen < 2) {
        sendIAmRouterToNetwork(snet, 0);
        return;
    }

    uint16_t network = 0;
    decode_unsigned16(npdu, &network);

    RouterPort *port = findPort(network);
    RouterRoute *route = findRoute(network);

    if (port) {
        // Directly connected network.
        if (port->net != snet) {
            sendIAmRouterToNetwork(snet, network);
        }
        return;
    }

    if (route) {
        // Network is reachable through another router.
        if (route->port->net != snet) {
            sendIAmRouterToNetwork(snet, network);
        }
        return;
    }

    // Unknown network: ask the other ports.
    for (uint8_t i = 0; i < _portsCount; i++) {
        if (_ports[i].net != snet) {
            sendWhoIsRouterToNetwork(
                _ports[i].net, network);
        }
    }
}


void BacnetRouterClass::sendWhoIsRouterToNetwork(
    uint16_t snet,
    uint16_t dnet,
    BACNET_ADDRESS *src)
{
    RouterPort *port = findPort(snet);
    if (!port) return;

    BACNET_ADDRESS dest = getBroadcastAddress();
    BACNET_NPDU_DATA npduData{};
    const bool dataExpectingReply = false;

    npdu_encode_npdu_network(
        &npduData,
        NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK,
        dataExpectingReply,
        MESSAGE_PRIORITY_NORMAL);

    int pduLen = npdu_encode_pdu(
        _txBuffer,
        &dest,
        src,
        &npduData);

    if (pduLen <= 0)
        return;

    if (dnet) {
        pduLen += encode_unsigned16(
            &_txBuffer[pduLen],
            dnet);
    }

    sendPdu(
        *port,
        dest,
        npduData,
        _txBuffer,
        pduLen);
}


void BacnetRouterClass::sendIAmRouterToNetwork(
    uint16_t snet,
    uint16_t net)
{
    RouterPort *port = findPort(snet);
    if (!port)
        return;

    BACNET_ADDRESS dest = getBroadcastAddress();
    BACNET_NPDU_DATA npduData{};

    npdu_encode_npdu_network(
        &npduData,
        NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK,
        false,
        MESSAGE_PRIORITY_NORMAL);

    int pduLen = npdu_encode_pdu(
        _txBuffer,
        &dest,
        nullptr,
        &npduData);

    if (pduLen <= 0)
        return;

    if (net) {
        if ((pduLen + 2) > TX_BUFFER_SIZE)
            return;

        pduLen += encode_unsigned16(
            &_txBuffer[pduLen],
            net);
    } else {
        // Add directly connected networks reachable
        // through ports other than the source port.
        for (uint8_t i = 0; i < _portsCount; i++) {
            if (_ports[i].net == snet)
                continue;

            if ((pduLen + 2) > TX_BUFFER_SIZE)
                break;

            pduLen += encode_unsigned16(
                &_txBuffer[pduLen],
                _ports[i].net);
        }

        // Add learned networks reachable through
        // ports other than the source port.
        for (uint16_t i = 0; i < _routesCount; i++) {
            if (!_routes[i].port)
                continue;

            if (_routes[i].port->net == snet)
                continue;

            if ((pduLen + 2) > TX_BUFFER_SIZE)
                break;

            pduLen += encode_unsigned16(
                &_txBuffer[pduLen],
                _routes[i].net);
        }
    }

    sendPdu(
        *port,
        dest,
        npduData,
        _txBuffer,
        pduLen);
}


/**

* Process an Initialize-Routing-Table network message.
*
* This is the router-class adaptation of the upstream
* npdu_init_routing_table_process() function. It is kept here rather
* than added to the local NPDU core because the router uses its own
* C++ routing table (_ports[] / _routes[]) and does not use the
* upstream DNET linked-list/callback mechanism.
*
* Only the DNET entries are relevant to the router. Port ID and
* Port Info fields are parsed and skipped according to the BACnet
* network message format.
*/
void BacnetRouterClass::processInitializeRoutingTable(
    uint16_t snet,
    const BACNET_ADDRESS &src,
    const uint8_t *npdu,
    uint16_t npduLen)
{
    if (!npdu || npduLen < 1)
        return;

    uint8_t netCount = npdu[0];

    if (netCount == 0)
        return;

    uint16_t offset = 1;

    while (netCount--) {
        // DNET(2) + PortID(1) + PortInfoLen(1)
        if ((npduLen - offset) < 4)
            return;

        uint16_t dnet = 0;
        offset += decode_unsigned16(&npdu[offset], &dnet);

        // Port ID
        offset++;

        // Port information length
        uint8_t portInfoLen = npdu[offset++];

        // Port information
        if ((npduLen - offset) < portInfoLen)
            return;

        offset += portInfoLen;

        addRoute(snet, dnet, src);
    }
}


void BacnetRouterClass::sendInitializeRoutingTableAck(
    uint16_t snet,
    const BACNET_ADDRESS *dst)
{
    RouterPort *port = findPort(snet);
    if (!port)
        return;

    BACNET_ADDRESS dest{};

    if (dst)
        dest = *dst;
    else
        dest = getBroadcastAddress();

    BACNET_NPDU_DATA npduData{};

    npdu_encode_npdu_network(
        &npduData,
        NETWORK_MESSAGE_INIT_RT_TABLE_ACK,
        false,
        MESSAGE_PRIORITY_NORMAL);

    int pduLen = npdu_encode_pdu(
        _txBuffer,
        &dest,
        nullptr,
        &npduData);

    if (pduLen <= 0)
        return;

    // Number of ports included in the routing table.
    if ((pduLen + 1) > TX_BUFFER_SIZE)
        return;

    _txBuffer[pduLen++] = _portsCount;

    // Encode each directly connected port.
    for (uint8_t i = 0; i < _portsCount; i++) {
        if ((pduLen + 4) > TX_BUFFER_SIZE)
            break;

        pduLen += encode_unsigned16(
            &_txBuffer[pduLen],
            _ports[i].net);

        // Port ID
        _txBuffer[pduLen++] = i + 1;

        // Port Info Length = 0
        _txBuffer[pduLen++] = 0;
    }

    sendPdu(
        *port,
        dest,
        npduData,
        _txBuffer,
        pduLen);
}


void BacnetRouterClass::sendRejectMessageToNetwork(
    uint16_t snet,
    const BACNET_ADDRESS *dst,
    uint8_t rejectReason,
    uint16_t dnet)
{
    RouterPort *port = findPort(snet);
    if (!port)
        return;

    BACNET_ADDRESS dest{};

    if (dst)
        dest = *dst;
    else
        dest = getBroadcastAddress();

    BACNET_NPDU_DATA npduData{};

    npdu_encode_npdu_network(
        &npduData,
        NETWORK_MESSAGE_REJECT_MESSAGE_TO_NETWORK,
        false,
        MESSAGE_PRIORITY_NORMAL);

    int pduLen = npdu_encode_pdu(
        _txBuffer,
        &dest,
        nullptr,
        &npduData);

    if (pduLen <= 0)
        return;

    // Reject reason
    if (pduLen + 1 > TX_BUFFER_SIZE)
        return;

    _txBuffer[pduLen++] = rejectReason;

    // DNET is optional.
    if (dnet) {
        if (pduLen + 2 > TX_BUFFER_SIZE)
            return;

        pduLen += encode_unsigned16(
            &_txBuffer[pduLen],
            dnet);
    }

    sendPdu(
        *port,
        dest,
        npduData,
        _txBuffer,
        pduLen);
}


void BacnetRouterClass::datalinkSet(BacnetDatalink portType)
{
    switch (portType){

#ifdef BACNET_ROUTER_BIP
    case BacnetDatalink::BIP:
        datalink_set(static_cast<char*>("bip"));
        break;
#endif 
#ifdef BACNET_ROUTER_ETHERNET
    case BacnetDatalink::ETHERNET:
        datalink_set(static_cast<char*>("ethernet"));
        break;
#endif
#ifdef BACNET_ROUTER_ARCNET
    case BacnetDatalink::ARCNET:
        datalink_set(static_cast<char*>("arcnet"));
        break;
#endif
#ifdef BACNET_ROUTER_BIP6
    case BacnetDatalink::BIP6:
        datalink_set(static_cast<char*>("bip6"));
        break;
#endif
#ifdef BACNET_ROUTER_MSTP
    case BacnetDatalink::MSTP:
        datalink_set(static_cast<char*>("mstp"));
        break;
#endif
#ifdef BACNET_ROUTER_ZIGBEE
    case BacnetDatalink::ZIGBEE:
        datalink_set(static_cast<char*>("zigbee"));

        break;
#endif
#ifdef BACNET_ROUTER_BSC
    case BacnetDatalink::BSC:
        datalink_set(static_cast<char*>("bsc"));

        break;
#endif
#ifdef BACNET_ROUTER_CUSTOM
    case BacnetDatalink::CUSTOM:
        datalink_set(static_cast<char*>("custom"));
        break;
#endif
    default:
        break;
    }
}

bool BacnetRouterClass::handleLocalDevice(
    uint16_t snet,
    BACNET_ADDRESS &src,
    const BACNET_ADDRESS &dest,
    uint8_t *apdu,
    uint16_t apduLen)
{
    Serial.println("handleLocalDevice");
    for (int i = 0; i < _portsCount; i++) {
        BACNET_ADDRESS &local = _ports[i].address;

        if (dest.net == _ports[i].net &&
            dest.len == local.mac_len &&
            memcmp(dest.adr, local.mac, dest.len) == 0) {
                datalinkSet(BacnetDatalink::CUSTOM);
                apdu_handler(&src, apdu, apduLen);
                return true;
        }
    }
    return false;
}



/*
custom datalink TO DO:
-add custom datalink to the list
-add get my address custom XXX not needed
-add send_pdu costom
-add datalink receive

-Sequence:

    -outgoing packet:
        -datalinkset to custom
        -apdu handler sendPdu
            -payload store into buffer
        -poll set datalink to custom
        -datalink receive decode packet (is datalink could be same as local? see sequence fort snet)
        -package decode and handler by router

    -incomming packet:
        -route handeling could follow normal path until loacl mac is found, then call apduHandler (already implemented???)



*/