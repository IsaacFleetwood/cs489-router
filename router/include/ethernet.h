#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>
#include "interfaces.h"
#include "structs.h"

struct pkt_ether_hdr {
  mac_addr_t dst;
  mac_addr_t src;
  uint16_t type; // NOTE: Must use ntohs to convert to host-byte order.
  uint8_t data[0];
};
typedef struct pkt_ether_hdr pkt_ether_hdr;

#define ETHERTYPE_IPV4 (0x0800)
#define ETHERTYPE_ARP (0x0806)

void ethernet_handle(pkt_ether_hdr* pkt_ptr, interface_id_t int_id);

#endif
