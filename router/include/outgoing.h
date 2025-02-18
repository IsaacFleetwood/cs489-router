#ifndef OUTGOING_H
#define OUTGOING_H

#include "ipv4.h"
#include "arp.h"

struct ip_pseudo_hdr {
  ip_addr_t addr_src;
  ip_addr_t addr_dst;
  uint8_t zero;
  uint8_t proto;
  uint16_t udp_length;
};

void send_ipv4(struct pkt_ipv4_hdr* pkt);
void send_arp(struct pkt_arp_hdr* pkt, interface_id_t int_id);

#endif