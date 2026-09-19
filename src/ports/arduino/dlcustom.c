#include "dlcustom.h"


//In case of reouter with local device here the map for the local datalink
//if a port is enable, the function must be defined
#if defined(ROUTER_DEVICE)


static custom_packet_t MSTP_Custom_Packet;







#if defined(BACDL_MSTP)
uint16_t datalink_custom_dlmstp_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout)
{
        (void)timeout;

    if (!MSTP_Custom_Packet.pending)
        return 0;

    if (!src || !pdu || MSTP_Custom_Packet.pdu_len > max_pdu)
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

    memcpy(pdu, MSTP_Custom_Packet.pdu, MSTP_Custom_Packet.pdu_len);

    unsigned len = MSTP_Custom_Packet.pdu_len;

    MSTP_Custom_Packet.pending = false;
    MSTP_Custom_Packet.pdu_len = 0;

    return len;
}


int custom_dlmstp_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    //(void)dest;
    (void)npdu_data;
    if (!dest || !pdu || pdu_len > DLMSTP_MPDU_MAX)
        return 0;
    if (MSTP_Custom_Packet.pending)
        return 0;

    MSTP_Custom_Packet.dest = *dest;
    memcpy(MSTP_Custom_Packet.pdu, pdu, pdu_len);
    MSTP_Custom_Packet.pdu_len = pdu_len;
    MSTP_Custom_Packet.pending = true;

    return pdu_len;

}

#endif




/*
uint16_t custom_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout
)
{
    (void)timeout;

    if (!MSTP_Custom_Packet.pending)
        return 0;

    if (!src || !pdu || MSTP_Custom_Packet.pdu_len > max_pdu)
        return 0;

    ///*
    // * The CUSTOM packet originated from the local Router Device.
    // * Get the actual source address from the physical local datalink.
    // *
#if defined(LOCAL_DEVICE_MSTP)
    dlmstp_get_my_address(src);
#elif defined(LOCAL_DEVICE_BIP)
    bip_get_my_address(src);
#elif defined(LOCAL_DEVICE_ETHERNET)
    ethernet_get_my_address(src);
    
#endif

    memcpy(pdu, MSTP_Custom_Packet.pdu, MSTP_Custom_Packet.pdu_len);

    unsigned len = MSTP_Custom_Packet.pdu_len;

    MSTP_Custom_Packet.pending = false;
    MSTP_Custom_Packet.pdu_len = 0;

    return len;
}

*/

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


#if defined(LOCAL_DEVICE_MSTP__)
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
    if (MSTP_Custom_Packet.pending)
        return 0;

    MSTP_Custom_Packet.dest = *dest;
    memcpy(MSTP_Custom_Packet.pdu, pdu, pdu_len);
    MSTP_Custom_Packet.pdu_len = pdu_len;
    MSTP_Custom_Packet.pending = true;

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

