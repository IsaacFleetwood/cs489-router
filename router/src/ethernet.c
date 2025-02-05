
#include <arpa/inet.h>

#include "../include/ethernet.h"
#include "../include/arp.h"
#include "../include/ipv4.h"

void ethernet_handle(pkt_ether_hdr* pkt_ptr, interface_id_t int_id) {

  uint16_t ethertype = ntohs(pkt_ptr->type);
  if(ethertype == ETHERTYPE_IPV4) {
    ipv4_handle((struct pkt_ipv4_hdr*) &pkt_ptr->data, int_id);
  } else if(ethertype == ETHERTYPE_ARP) {
    arp_handle((struct pkt_arp_hdr*) &pkt_ptr->data, int_id, pkt_ptr->src);
  }

}