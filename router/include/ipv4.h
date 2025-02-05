#ifndef IPV4_H
#define IPV4_H

#include "ethernet.h"
#include "interfaces.h"

struct pkt_ipv4_hdr {
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