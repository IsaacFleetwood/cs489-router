
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#include "../include/structs.h"
#include "../include/outgoing.h"
#include "../include/interfaces.h"
#include "../include/stub.h"

mac_addr_t get_mac_for_ip(ip_addr_t ip_addr) {
  // TODO
  return (mac_addr_t) {
    .bytes = {0, 0, 0, 0, 0, 0},
  };
}

uint16_t calculate_checksum(void* ptr, int len) {
  uint32_t checksum = 0;
  for(int i = 0; i < len; i++) {
    checksum += ((uint16_t*) ptr)[i];
    //printf("%04x\n", ((uint16_t*) ptr)[i]);
  }
  while(checksum > 0xFFFF) {
    checksum = (checksum & 0xFFFF) + (checksum >> 16);
  }
  return ~((uint16_t) checksum);
}

struct ip_pseudo_hdr {
  ip_addr_t addr_src;
  ip_addr_t addr_dst;
  uint8_t zero;
  uint8_t proto;
  uint16_t udp_length;
};

void send_ipv4(struct pkt_ipv4_hdr* pkt) {

  ip_addr_t dst_ip = pkt->addr_dst; // Read IPv4 destination address.
  interface_id_t int_id = get_interface_for_ip(dst_ip);
  
  ip_addr_t src_ip = interface_get_ip(int_id);
  pkt->addr_src = src_ip;

  pkt->checksum = 0;
  pkt->checksum = calculate_checksum(pkt, pkt->ihl * 2);

  // TODO: UDP and TCP packets calculate checksum over the IP header.
  //       So you will have to calculate the checksum for them.
  // (Except when UDP checksum is disabled)
  uint8_t transport_proto = pkt->protocol;
  if(transport_proto == 0x11) { // UDP
    struct pkt_udp_hdr* udp_pkt = ((void*) pkt) + (pkt->ihl * 4);
    if(udp_pkt->checksum != 0) {
      struct ip_pseudo_hdr pseudo_hdr = {
        .addr_src = pkt->addr_src,
        .addr_dst = pkt->addr_dst,
        .zero = 0,
        .proto = pkt->protocol,
        .udp_length = udp_pkt->length
      };
      int pseudo_checksum = calculate_checksum(&pseudo_hdr, sizeof(struct ip_pseudo_hdr) / 2);
      udp_pkt->checksum = ~pseudo_checksum;
      int checksum = calculate_checksum(udp_pkt, ntohs(udp_pkt->length) / 2);
      if(checksum == 0) checksum = 0xffff;
      udp_pkt->checksum = checksum;
    }
  } else if(transport_proto == 0x06) {
    struct pkt_tcp_hdr* tcp_pkt = ((void*) pkt) + (pkt->ihl * 4);
    uint16_t tcp_len = ntohs(pkt->tot_length) - (pkt->ihl*4);
    struct ip_pseudo_hdr pseudo_hdr = {
      .addr_src = pkt->addr_src,
      .addr_dst = pkt->addr_dst,
      .zero = 0,
      .proto = pkt->protocol,
      .udp_length = htons(tcp_len)
    };
    int pseudo_checksum = calculate_checksum(&pseudo_hdr, sizeof(struct ip_pseudo_hdr) / 2);
    tcp_pkt->checksum = ~pseudo_checksum;
    int checksum = calculate_checksum(tcp_pkt, tcp_len / 2);
    tcp_pkt->checksum = checksum;
  }

  // Get mac address of local interface.
  mac_addr_t src_mac = interface_get_mac_addr(int_id);
  // Get associated mac address for dst ip.
  mac_addr_t dst_mac = get_mac_for_ip(dst_ip);

  if(interface_get_type(int_id) == INT_TYPE_ETHER) {
    uint16_t ethertype = htons(0x0800);
    uint16_t len = ntohs(pkt->tot_length);
    uint32_t size = len + sizeof(struct pkt_ether_hdr);

    // TODO: Use some sort of buffer instead of malloc.
    struct pkt_ether_hdr* new_ether_pkt = malloc(size);
    memcpy(new_ether_pkt + 1, pkt, len);

    new_ether_pkt->dst = dst_mac;
    new_ether_pkt->src = src_mac;
    new_ether_pkt->type = ethertype;

    #ifdef STUB_ENABLED
    stub_write_pkt((uint8_t*) new_ether_pkt, size);
    #else
    // TODO: Output to actual ethernet interface.
    #endif

    free(new_ether_pkt);
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