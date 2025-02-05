
// TODO: Fill in ARP packet structure in header file.
#include "../include/arp.h"
#include "../include/interfaces.h"

// TODO: Data structure to store IP to Network Interface mapping.
// Note: Must use mutexes for parallelization.

void arp_handle(struct pkt_arp_hdr* pkt, interface_id_t int_id, mac_addr_t src) {
  // ??? - Does the router itself need to handle responding to ARP packets to itself? (Will the Linux Kernel do it for us? Answer: Kinda, it depends.)
  if(interface_get_side(int_id) != INT_SIDE_LAN)
    return; // Don't respond to ARP packets on anything other than the LAN.

  // TODO: Bridge WIFI and Ethernet interfaces for ARP requests.
  // E.g. If ARP request is sent over Ethernet network, asking for Wifi device IP, send the ARP over wifi to the device.
  // Also be sure to let the response get back to the original requester.

  // Steps:
  // Lookup ARP request IP in table
  // - If no result, send it over other networks just to be safe.
  // - If there is a result, check which network it is on.
  //      - If sender and destination are on same network, don't forward it.
  //      - If not on same network, forward it to the network you know it's on.

}
