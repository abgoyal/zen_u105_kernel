/*****************************************************************************/
/*                                                                           */
/*                     Ittiam 802.11 MAC SOFTWARE                            */
/*                                                                           */
/*                  ITTIAM SYSTEMS PVT LTD, BANGALORE                        */
/*                           COPYRIGHT(C) 2005                               */
/*                                                                           */
/*  This program  is  proprietary to  Ittiam  Systems  Private  Limited  and */
/*  is protected under Indian  Copyright Law as an unpublished work. Its use */
/*  and  disclosure  is  limited by  the terms  and  conditions of a license */
/*  agreement. It may not be copied or otherwise  reproduced or disclosed to */
/*  persons outside the licensee's organization except in accordance with the*/
/*  terms  and  conditions   of  such  an  agreement.  All  copies  and      */
/*  reproductions shall be the property of Ittiam Systems Private Limited and*/
/*  must bear this notice in its entirety.                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  File Name         : event_parser_ap.c                                    */
/*                                                                           */
/*  Description       : This file contains the event parser functions for    */
/*                      AP mode.                                             */
/*                                                                           */
/*  List of Functions : process_host_rx_msdu_ap                              */
/*                                                                           */
/*  Issues            : None                                                 */
/*                                                                           */
/*****************************************************************************/

#ifdef BSS_ACCESS_POINT_MODE

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

#include "common.h"
#include "core_mode_if.h"
#include "management_ap.h"
#include "prot_if.h"
#include "host_if.h"
#include "sme_ap.h"

/*****************************************************************************/
/*                                                                           */
/*  Function Name : process_host_rx_msdu_ap                                  */
/*                                                                           */
/*  Description   : This function parses the HOST_RX_MSDU event, prepares a  */
/*                  WLAN transmit data request and calls the MAC API.        */
/*                                                                           */
/*  Inputs        : 1) Pointer to the MAC structure                          */
/*                  2) Pointer to the HOST RX event buffer                   */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : The buffer and offset given to the function is to the    */
/*                  ethernet header. The source and destination addresses    */
/*                  are extracted from the ethernet header. If the           */
/*                  destination address is a group address or an associated  */
/*                  STA the frame is further processed. The SNAP is added,   */
/*                  if required. The WLAN Tx data request structure is       */
/*                  prepared and appropriate API called with this as input.  */
/*                                                                           */
/*  Outputs       : None                                                     */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*****************************************************************************/
void process_host_rx_msdu_ap(mac_struct_t *mac, UWORD8* event)
{
    UWORD8        host_type      = 0;
    UWORD8        eth_hdr_offset = 0;
    UWORD8        buffer_ofst    = 0;
    UWORD16       rx_pkt_len     = 0;
    UWORD16       eth_type       = 0;
    UWORD8        *buffer_addr   = 0;
    UWORD8        *eth_hdr       = 0;
    UWORD8        *snap_hdr      = 0;
    wlan_tx_req_t wlan_tx_req    = {{0},};

	TROUT_FUNC_ENTER;
    /* Extract the buffer address of the packet received from the host and   */
    /* the length of the packet.                                             */
    buffer_addr = ((host_rx_event_msg_t *)event)->buffer_addr;
    buffer_ofst = ((host_rx_event_msg_t *)event)->pkt_ofst;
    rx_pkt_len  = ((host_rx_event_msg_t *)event)->rx_pkt_len;
	host_type   = ((host_rx_event_msg_t *)event)->host_if_type;

	eth_hdr_offset = get_eth_hdr_offset(host_type);
    /* Extract the ethernet header address from the ethernet header. */
    eth_hdr  = buffer_addr + buffer_ofst + eth_hdr_offset;

    /* Set the destination address field in the WLAN Tx Request structure. */
    mac_addr_cpy(wlan_tx_req.da, eth_hdr);

    /* For AP, the source address should be sent with the frame. */
    mac_addr_cpy(wlan_tx_req.sa, eth_hdr + 6);

    /* For all the packets coming for the Host side, the status of the port */
    /* for the destination needs to be verified. This is a security check   */
    /* So the Ignore Port Status flag is set to False                       */
    wlan_tx_req.ignore_port = BFALSE;

    /* Frame coming from the host can be aggregated. */
    wlan_tx_req.dont_aggr = BFALSE;

    /* SNAP header needs to be set for IP/ARP packets. Note that there is    */
    /* sufficient space allocated for SNAP and MAC header in the buffer.     */

    /* Extract the type of the ethernet packet and set the SNAP header       */
    /* contents. Also set the data pointer field in the WLAN Tx Request      */
    /* structure, as required, to the correct value.                         */
    eth_type = ((eth_hdr[ETH_PKT_TYPE_OFFSET] << 8) |
                 eth_hdr[ETH_PKT_TYPE_OFFSET + 1]);

    wlan_tx_req.eth_type = eth_type;
#if 0// dumy add 120702
    if((eth_type == ARP_TYPE)   ||
       (eth_type == IP_TYPE)    ||
       (eth_type == ONE_X_TYPE) ||
       (eth_type == VLAN_TYPE)  ||
       (eth_type == LLTD_TYPE))
#else
	if(1)
#endif
    {
        /* The SNAP header is set before the ethernet payload.               */
        /*                                                                   */
        /* +--------+--------+--------+----------+---------+---------------+ */
        /* | DSAP   | SSAP   | UI     | OUI      | EthType | EthPayload    | */
        /* +--------+--------+--------+----------+---------+---------------+ */
        /* | 1 byte | 1 byte | 1 byte | 3 bytes  | 2 bytes | x bytes       | */
        /* +--------+--------+--------+----------+---------+---------------+ */
        /* <----------------  SNAP Header  ---------------->                 */
        /* <------------------------ 802.11 Payload -----------------------> */
        snap_hdr = buffer_addr + buffer_ofst + eth_hdr_offset +
                        ETHERNET_HDR_LEN - SNAP_HDR_LEN;

        /* Set the data pointer to the start of SNAP header (since this is   */
        /* included as the data payload for 802.11 packets.                  */
        wlan_tx_req.data = snap_hdr;

        *snap_hdr++ = 0xAA;
        *snap_hdr++ = 0xAA;
        *snap_hdr++ = 0x03;
        *snap_hdr++ = 0x00;
        *snap_hdr++ = 0x00;
        *snap_hdr++ = 0x00;

        /* Set the data length parameter to the MAC data length only (does   */
        /* not include headers)                                              */
        wlan_tx_req.data_len = rx_pkt_len - ETHERNET_HDR_LEN + SNAP_HDR_LEN;

        /* Note that the Ethernet Type field is already set in the ethernet  */
        /* header and follows this.                                          */
    }
    else
    {
        /* Set the data length parameter to the MAC data length only (does   */
        /* not include headers)                                              */
        wlan_tx_req.data_len = rx_pkt_len - ETHERNET_HDR_LEN;

        /* In case SNAP header is not present the data pointer points to the */
        /* ethernet payload.                                                 */
        wlan_tx_req.data = buffer_addr + buffer_ofst + eth_hdr_offset +
                           ETHERNET_HDR_LEN;
    }

    /* Set the buffer pointer field in the WLAN Tx Request to the start      */
    /* address of the buffer.                                                */
    wlan_tx_req.buffer_addr = buffer_addr;

    /* Set all fields for the transmit structure */
    if(is_group(wlan_tx_req.da) == BTRUE)
    {
        wlan_tx_req.service_class = BCAST_NO_ACK;
    }
    else
    {
        wlan_tx_req.service_class = g_ack_policy;
    }

    /* Packet classification is done only if QoS is enabled */
    wlan_tx_req.priority = get_priority(eth_hdr, eth_type);

    /* Transmit the packet at default rate */
    wlan_tx_req.min_basic_rate = BFALSE;

    /* Call the wlan_tx_data() API to transmit the packet. Note that the     */
    /* buffer for the request structure is not freed here. It should be      */
    /* freed after the request has been processed.                           */
    wlan_tx_data(mac, (UWORD8 *)&wlan_tx_req);
    TROUT_FUNC_EXIT;
}

#endif /* BSS_ACCESS_POINT_MODE */
