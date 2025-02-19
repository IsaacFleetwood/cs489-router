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

void send_ipv4(ipv4_hdr_t* pkt);
void send_arp(arp_hdr_t* pkt, interface_id_t int_id);

#endif