#ifndef ARP_H
#define ARP_H

#include "ethernet.h"
#include "interfaces.h"

struct pkt_arp_hdr {
    // TODO: Fill in.
    // https://en.wikipedia.org/wiki/Address_Resolution_Protocol
};

void arp_handle(struct pkt_arp_hdr* pkt, interface_id_t int_id, mac_addr_t src);

#endif