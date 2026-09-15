#include "dlcustom.h"


//In case of reouter with local device here the map for the local datalink
//if a port is enable, the function must be defined
#if defined(ROUTER_DEVICE)


static custom_packet_t Custom_Packet;


uint16_t custom_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout
)
{
    (void)timeout;

    if (!Custom_Packet.pending)
        return 0;

    if (!src || !pdu || Custom_Packet.pdu_len > max_pdu)
        return 0;

    /*
     * The CUSTOM packet originated from the local Router Device.
     * Get the actual source address from the physical local datalink.
     */
#if defined(LOCAL_DEVICE_MSTP)
    dlmstp_get_my_address(src);
#elif defined(LOCAL_DEVICE_BIP)
    bip_get_my_address(src);
#elif defined(LOCAL_DEVICE_ETHERNET)
    ethernet_get_my_address(src);
    /* etc. */
#endif

    memcpy(pdu, Custom_Packet.pdu, Custom_Packet.pdu_len);

    unsigned len = Custom_Packet.pdu_len;

    Custom_Packet.pending = false;
    Custom_Packet.pdu_len = 0;

    return len;
}



#if defined(LOCAL_DEVICE_ARCNET)
int custom_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len
)
{}*/ 
#endif


#if defined(LOCAL_DEVICE_ETHERNET)
int custom_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{}*/
#endif


#if defined(LOCAL_DEVICE_BIP)
int custom_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{}
#endif


#if defined(LOCAL_DEVICE_BIP6)
int custom_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{}*/
#endif


#if defined(LOCAL_DEVICE_MSTP)
int custom_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    //(void)dest;
    (void)npdu_data;
    if (!dest || !pdu || pdu_len > DLMSTP_MPDU_MAX)
        return 0;
    if (Custom_Packet.pending)
        return 0;

    Custom_Packet.dest = *dest;
    memcpy(Custom_Packet.pdu, pdu, pdu_len);
    Custom_Packet.pdu_len = pdu_len;
    Custom_Packet.pending = true;

    return pdu_len;
}
#endif


#if defined(LOCAL_DEVICE_ZIGBEE)
int custom_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{}*/
#endif


#if defined(LOCAL_DEVICE_BSC)
int custom_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{}*/ 
#endif
#endif//ROUTER_DEVICE

/*
void bacnet_router_custom_get_broadcast_address(
    BACNET_ADDRESS *dest)
{
    datalinkSet(_ports[_routerDevice.portIndex].portType);

    datalink_get_broadcast_address(dest);

    datalinkSet(BacnetDatalink::CUSTOM);
}*/