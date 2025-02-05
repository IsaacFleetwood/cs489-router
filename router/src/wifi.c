
#include "../include/wifi.h"
#include "../include/ipv4.h"
#include "../include/arp.h"

// TODO: Figure out how to put WiFi receiver into "access point mode" (not sure specific name)

void wifi_handle(pkt_wifi_hdr* pkt_ptr, interface_id_t int_id) {
    // TODO: Read LLC/SNAP Header from 802.11 packet
    // Read in the EtherType field from the SNAP header.
    uint16_t ethertype;
    // And determine the position of the inner network-layer packet.
    uint8_t* inner_pkt_data;
	if(ethertype == ETHERTYPE_IPV4) {
		ipv4_handle((struct pkt_ipv4_hdr*) inner_pkt_data, int_id);
	} else if(ethertype == ETHERTYPE_ARP) {
		arp_handle((struct pkt_arp_hdr*) inner_pkt_data, int_id, pkt_ptr->addr_src);
	}
}