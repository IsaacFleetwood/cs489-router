
#include <arpa/inet.h>

#include "../include/ethernet.h"
#include "../include/arp.h"
#include "../include/ipv4.h"
#include "../include/devices.h"

void ethernet_handle(pkt_ether_hdr* pkt_ptr, interface_id_t int_id) {

  uint16_t ethertype = ntohs(pkt_ptr->type);
  if(ethertype == ETHERTYPE_IPV4) {
    struct pkt_ipv4_hdr* ip_pkt = (struct pkt_ipv4_hdr*) &pkt_ptr->data;
    
    // Update ip-mac map.
    ip_mac_pair_t sender_pair = {
      .ip_addr = ip_pkt->addr_src,
      .mac_addr = pkt_ptr->src
    };
    update_ip_mac_pair(sender_pair);

    ipv4_handle(ip_pkt, int_id);
  } else if(ethertype == ETHERTYPE_ARP) {
    arp_handle((struct pkt_arp_hdr*) &pkt_ptr->data, int_id, pkt_ptr->src);
  }

}