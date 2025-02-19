#ifndef ARP_H
#define ARP_H

#include "ethernet.h"
#include "interfaces.h"

struct pkt_arp_hdr {
  uint16_t hardware_type;
  uint16_t protocol_type;
  uint8_t hardware_length;
  uint8_t protocol_length;
  uint16_t operation;
  mac_addr_t sender_mac;
  ip_addr_t sender_ip;
  mac_addr_t target_mac;
  ip_addr_t target_ip;
};

void arp_handle(struct pkt_arp_hdr* pkt, interface_id_t int_id, mac_addr_t src);

#endif