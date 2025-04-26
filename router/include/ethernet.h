#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>
#include "interfaces.h"
#include "structs.h"

/*** Ethernet Frame Structure ***/
typedef struct {
  mac_addr_t mac_dst;     // Destination MAC address
  mac_addr_t mac_src;     // Source MAC address
  uint16_t ethertype;     // Ethernet frame type (IPv4, ARP, etc.)
} ethernet_hdr_t;

#define ETHERTYPE_IPV4 (0x0800)
#ifndef ETHERTYPE_ARP
#define ETHERTYPE_ARP (0x0806)
#endif


void ethernet_handle(ethernet_hdr_t* pkt_ptr, interface_id_t int_id);

#endif
