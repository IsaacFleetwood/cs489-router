
#include <netinet/in.h>
#include <stdlib.h>

#include "../include/devices.h"
#include "../include/structs.h"
#include "../include/ethernet.h"
#include "../include/ipv4.h"
#include "../include/stub.h"
#include "../include/arp.h"

void add_ip_mac_pair(ip_addr_t ip, mac_addr_t mac) {
    ip_mac_pair_t pair = {
        .ip_addr = ip,
        .mac_addr = mac
    };
    update_ip_mac_pair(pair);
}

ip_addr_t to_ip(unsigned char b2, unsigned char b3) {
    ip_addr_t ip = {
        .bytes = {192, 168, b2, b3}
    };
    return ip;
}

mac_addr_t to_mac(unsigned char b0) {
    mac_addr_t mac = {
        .bytes = {b0, b0, b0, b0, b0, b0}
    };
    return mac;
}

void testcase_run() {

    uint32_t size = sizeof(ethernet_hdr_t) + 
        sizeof(ipv4_hdr_t) +
        sizeof(udp_hdr_t);
    ethernet_hdr_t* pkt = calloc(1, size);
    pkt->mac_src = to_mac(0x22);
    pkt->mac_dst = to_mac(0x21);
    pkt->ethertype = htons(ETHERTYPE_IPV4);
    ipv4_hdr_t* ip_pkt = (ipv4_hdr_t*) (pkt + 1);
    ip_pkt->ip_src = to_ip(2, 2);
    ip_pkt->ip_dst = to_ip(50, 50);
    ip_pkt->ver_ihl = 0x45;
    ip_pkt->protocol = 0x11;
    ip_pkt->total_length = htons(20 + sizeof(udp_hdr_t));
    udp_hdr_t* udp_pkt = (udp_hdr_t*) (ip_pkt + 1);
    udp_pkt->length = htons(8);
    udp_pkt->port_dst = htons(80);
    udp_pkt->port_src = htons(50001);
    stub_write_pkt((uint8_t*) pkt, size);

    //add_ip_mac_pair(to_ip(0, 1), to_mac(0x01));
    uint32_t sizeArp = sizeof(ethernet_hdr_t) + 
        sizeof(arp_hdr_t);
    ethernet_hdr_t* pktArp = calloc(1, sizeArp);
    pktArp->mac_src = to_mac(0x01);
    pktArp->mac_dst = to_mac(0xFF);
    pktArp->ethertype = htons(ETHERTYPE_ARP);
    arp_hdr_t* arp_pkt = (arp_hdr_t*) (pktArp + 1);
    arp_pkt->sender_ip = to_ip(0, 1);
    arp_pkt->target_ip = to_ip(0, 1);
    arp_pkt->sender_mac = to_mac(0x01);
    arp_pkt->target_mac = to_mac(0x01);
    arp_pkt->operation = htons(0x02);
    arp_pkt->hardware_length = 6;
    arp_pkt->protocol_length = 4;
    arp_pkt->hardware_type = htons(1);
    arp_pkt->protocol_type = htons(0x0800);
    stub_write_pkt((uint8_t*) pktArp, sizeArp);
}