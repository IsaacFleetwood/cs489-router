#ifndef IPV4_H
#define IPV4_H

#include "ethernet.h"
#include "interfaces.h"
#include "structs.h"

struct pkt_ipv4_hdr {
    uint8_t ihl:4;
    uint8_t ver:4;
    uint8_t tos;
    uint16_t tot_length;
    uint32_t idk_1;
    uint8_t idk_2;
    uint8_t protocol;
    uint16_t checksum;
    ip_addr_t addr_src;
    ip_addr_t addr_dst;
  // TODO: Fill in.
  // https://en.wikipedia.org/wiki/IPv4#/media/File:IPv4_Packet-en.svg
  // NOTE: Don't worry about minutia until it's actually known to be needed.
  //       Many fields will never be touched.

  // structs.h has a "ip_addr_t" type that might be useful.
};

void ipv4_handle(struct pkt_ipv4_hdr* pkt, interface_id_t int_id);

void napt_inc_handle(struct pkt_ipv4_hdr* pkt, interface_id_t int_id);
void napt_out_handle(struct pkt_ipv4_hdr* pkt, interface_id_t int_id);

#endif