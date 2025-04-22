
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#include "../include/structs.h"
#include "../include/outgoing.h"
#include "../include/interfaces.h"
#include "../include/stub.h"
#include "../include/devices.h"
#include "../include/utils.h"
#include "../include/timer.h"

uint16_t calculate_checksum(void* ptr, int len) {
  uint32_t checksum = 0;
  if(len % 2 == 1)
    ((uint8_t*) ptr)[len] = 0;
  for(int i = 0; i < (len + 1) / 2; i++) {
    checksum += ((uint16_t*) ptr)[i];
  }
  while(checksum > 0xFFFF) {
    checksum = (checksum & 0xFFFF) + (checksum >> 16);
  }
  return ~((uint16_t) checksum);
}

void send_ipv4(ipv4_hdr_t* pkt) {

  ip_addr_t dst_ip = pkt->ip_dst; // Read IPv4 destination address.
  interface_id_t int_id = get_interface_for_ip(dst_ip);
  
  pkt->checksum = 0;
  pkt->checksum = calculate_checksum(pkt, (pkt->ver_ihl & 0xF) * 4);

  // TODO: UDP and TCP packets calculate checksum over the IP header.
  //       So you will have to calculate the checksum for them.
  // (Except when UDP checksum is disabled)
  uint8_t transport_proto = pkt->protocol;
  if(transport_proto == 0x11) { // UDP
    udp_hdr_t* udp_pkt = ((void*) pkt) + ((pkt->ver_ihl & 0xF) * 4);
    if(udp_pkt->checksum != 0) {
      struct ip_pseudo_hdr pseudo_hdr = {
        .addr_src = pkt->ip_src,
        .addr_dst = pkt->ip_dst,
        .zero = 0,
        .proto = pkt->protocol,
        .udp_length = udp_pkt->length
      };
      int pseudo_checksum = calculate_checksum(&pseudo_hdr, sizeof(struct ip_pseudo_hdr));
      udp_pkt->checksum = ~pseudo_checksum;
      int checksum = calculate_checksum(udp_pkt, (ntohs(udp_pkt->length)));
      if(checksum == 0) checksum = 0xffff;
      udp_pkt->checksum = checksum;
    }
  } else if(transport_proto == 0x06) {
    tcp_hdr_t* tcp_pkt = ((void*) pkt) + ((pkt->ver_ihl & 0xF) * 4);
    uint16_t tcp_len = ntohs(pkt->total_length) - ((pkt->ver_ihl & 0xF) * 4);
    struct ip_pseudo_hdr pseudo_hdr = {
      .addr_src = pkt->ip_src,
      .addr_dst = pkt->ip_dst,
      .zero = 0,
      .proto = pkt->protocol,
      .udp_length = htons(tcp_len)
    };
    int pseudo_checksum = calculate_checksum(&pseudo_hdr, sizeof(struct ip_pseudo_hdr));
    tcp_pkt->checksum = ~pseudo_checksum;
    int checksum = calculate_checksum(tcp_pkt, (tcp_len));
    tcp_pkt->checksum = checksum;
  }
  ip_addr_t local_dst_ip;
  if(interface_get_side(int_id) == INT_SIDE_LAN) {
    local_dst_ip = dst_ip;
  } else if (interface_get_side(int_id) == INT_SIDE_WAN) {
    ip_addr_t network_ip = wan_network_ip;
    ip_addr_t subnet_mask = create_subnet_mask(wan_cidr_prefix_len);
    ip_addr_t dst_masked = apply_mask(subnet_mask, dst_ip);
    if(ip_addr_equals(dst_masked, network_ip)) {
      local_dst_ip = dst_ip;
    } else {
      local_dst_ip = wan_gateway_ip;
    }
  }

  if(!has_mac_for_ip(local_dst_ip)) {
    uint16_t len = ntohs(pkt->total_length);
    ipv4_hdr_t* new_pkt = malloc(len);
    memcpy(new_pkt, pkt, len);
    // Add outgoing IPv4 packet to ARP request timer.
    timer_add_packet(new_pkt, int_id, local_dst_ip);
    return;
  }

  // Get mac address of local interface.
  mac_addr_t src_mac = interface_get_mac_addr(int_id);
  // Get associated mac address for dst ip.
  mac_addr_t dst_mac = get_mac_from_ip(local_dst_ip);



  if(interface_get_type(int_id) == INT_TYPE_ETHER) {
    uint16_t ethertype = htons(0x0800);
    uint16_t len = ntohs(pkt->total_length);
    uint32_t size = len + sizeof(ethernet_hdr_t);

    // TODO: Use some sort of buffer instead of malloc.
    ethernet_hdr_t* new_ether_pkt = malloc(size);
    memcpy(new_ether_pkt + 1, pkt, len);

    new_ether_pkt->mac_dst = dst_mac;
    new_ether_pkt->mac_src = src_mac;
    new_ether_pkt->ethertype = ethertype;

    #ifdef STUB_ENABLED
      stub_write_pkt((uint8_t*) new_ether_pkt, size);
    #else
      int res = write(interface_get_config(int_id)->fd, new_ether_pkt, size);
      if (res == -1) {
				perror("sendto failed");
			}
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

void send_arp(arp_hdr_t* pkt, interface_id_t int_id) {

  mac_addr_t src_mac = pkt->sender_mac;
  mac_addr_t dst_mac = {.bytes = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

  if(interface_get_type(int_id) == INT_TYPE_ETHER) {
    uint16_t ethertype = htons(0x0806);
    uint16_t len = sizeof(arp_hdr_t);
    uint32_t size = len + sizeof(ethernet_hdr_t);

    // TODO: Use some sort of buffer instead of malloc.
    ethernet_hdr_t* new_ether_pkt = malloc(size);
    memcpy(new_ether_pkt + 1, pkt, len);

    new_ether_pkt->mac_dst = dst_mac;
    new_ether_pkt->mac_src = src_mac;
    new_ether_pkt->ethertype = ethertype;

    #ifdef STUB_ENABLED
      stub_write_pkt((uint8_t*) new_ether_pkt, size);
    #else
      int res = write(interface_get_config(int_id)->fd, new_ether_pkt, size);
      if (res == -1) {
				perror("sendto failed");
			}
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