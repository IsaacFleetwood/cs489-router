
#include <arpa/inet.h>

#include "../include/outgoing.h"
#include "../include/interfaces.h"

void send_ipv4(struct pkt_ipv4_hdr* pkt) {

    ip_addr_t dst_ip; // Read IPv4 destination address.
    interface_id_t int_id = get_interface_for_ip(dst_ip);
    
    ip_addr_t src_ip = interface_get_ip(int_id);
    // Set pkt.src_ip to src_ip value determined above?

    // TODO: UDP and TCP packets calculate checksum over the IP header.
    //       So you will have to calculate the checksum for them.
    // (Except when UDP checksum is disabled)

    // Get mac address of local interface.
    mac_addr_t src_mac = interface_get_mac_addr(int_id);
    mac_addr_t dst_mac; // Get associated mac address for dst ip.

    if(interface_get_type(int_id) == INT_TYPE_ETHER) {
        // TODO: Construct Ethernet frame
        uint16_t ethertype = htons(0x0800);

        return;
    } else if(interface_get_type(int_id) == INT_TYPE_WIFI) {
        // Construct WiFi frame

        return;
    } else {
        // Unknown interface type.
    }
    // TODO: Figure out how to share socket to be able to send.
}

void send_arp(struct pkt_arp_hdr* pkt, interface_id_t int_id) {

    mac_addr_t src_mac;
    mac_addr_t dst_mac;

    if(interface_get_type(int_id) == INT_TYPE_ETHER) {
        // TODO: Construct Ethernet frame
        uint16_t ethertype = htons(0x0806);

        return;
    } else if(interface_get_type(int_id) == INT_TYPE_WIFI) {
        // Construct WiFi frame

        return;
    } else {
        // Unknown interface type.
    }
    // TODO: Figure out how to share socket to be able to send.
}