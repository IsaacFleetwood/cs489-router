
#include <netinet/in.h>
#include <pthread.h>

#include "../include/arp.h"
#include "../include/interfaces.h"
#include "../include/devices.h"
#include "../include/utils.h"
#include "../include/timer.h"

// TODO: Data structure to store IP to Network Interface mapping.
// Note: Must use mutexes for parallelization.


void arp_handle(struct pkt_arp_hdr* pkt, interface_id_t int_id, mac_addr_t src) {
  // ??? - Does the router itself need to handle responding to ARP packets to itself? (Will the Linux Kernel do it for us? Answer: Kinda, it depends.)

  if(ntohs(pkt->protocol_type) != 0x0800)
    return;
  if(ntohs(pkt->hardware_type) != 0x0001)
    return;

  ip_addr_t zero_ip = {0};
  if(ip_addr_equals(pkt->sender_ip, zero_ip))
    return;
  ip_mac_pair_t sender_pair = {
    .ip_addr = pkt->sender_ip,
    .mac_addr = pkt->sender_mac
  };
  update_ip_mac_pair(sender_pair);
  timer_update_packets(pkt->sender_ip);

  if(interface_get_side(int_id) != INT_SIDE_LAN)
    return; // Don't respond to ARP packets on anything other than the LAN.



}
