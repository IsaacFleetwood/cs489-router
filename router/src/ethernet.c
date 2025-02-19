
#include <arpa/inet.h>

#include "../include/ethernet.h"
#include "../include/arp.h"
#include "../include/ipv4.h"
#include "../include/devices.h"

void ethernet_handle(ethernet_hdr_t* pkt_ptr, interface_id_t int_id) {

  uint16_t ethertype = ntohs(pkt_ptr->ethertype);
  if(ethertype == ETHERTYPE_IPV4) {
    ipv4_hdr_t* ip_pkt = (ipv4_hdr_t*) (pkt_ptr + 1);
    
    // Update ip-mac map.
    ip_mac_pair_t sender_pair = {
      .ip_addr = ip_pkt->ip_src,
      .mac_addr = pkt_ptr->mac_src
    };
    update_ip_mac_pair(sender_pair);

    ipv4_handle(ip_pkt, int_id);
  } else if(ethertype == ETHERTYPE_ARP) {
    arp_handle((arp_hdr_t*) (pkt_ptr + 1), int_id, pkt_ptr->mac_src);
  }

}