
#include "../include/ipv4.h"
#include "../include/interfaces.h"

// TODO: Data structure for storing ip-port tuples.
// Keep track of which ip-port pairs to expect data from, so the firewall is open for them.
// Aswell, keep track of where the data needs to go (which local device), whenever it comes back into the network.

// TODO: If packet size is > Ethernet's MTU. It will need to be fragmented.
// This will happen when WiFi packets need to be sent over Ethernet.

void ipv4_handle(struct pkt_ipv4_hdr* pkt, interface_id_t int_id) {

  // TODO: Check protocol.
  // Only handle packets of desired protocol. (TCP, UDP, ICMP for starters.)

  if(interface_get_side(int_id) == INT_SIDE_WAN) {
    // TODO: Check if IP is actually our public ip (could be a neighbor's)
    napt_inc_handle(pkt, int_id);
    return;
  }

  ip_addr_t src; // TODO: Read these from the packet.
  ip_addr_t dst;

  interface_config_t* int_config = interface_get_config(int_id);
  
  interface_id_t out_int_id = get_interface_for_ip(dst);
  // If they are on the same network, disregard the packet.
  if(out_int_id == int_id) {
    return;
  // Otherwise, if it is going to another LAN port, bridge
  } else if(interface_get_side(out_int_id) == INT_SIDE_LAN) {
    // Forward packet to out interface on LAN.
    // No need to do NAPT, because its on the LAN side.

    // TODO: Send packet over "out_int_id" LAN interface.
  } else {
    // If it reaches here, it is going from LAN to WAN.
    // So do NAPT.
    napt_out_handle(pkt, int_id);
  }
}

// Handle outgoing packets from LAN to WAN
void napt_out_handle(struct pkt_ipv4_hdr* pkt, interface_id_t int_id) {
  // Assume that we already know the packet is going out of the network (subnet mask is already checked)
}

// Handle incoming packets from WAN
void napt_inc_handle(struct pkt_ipv4_hdr* pkt, interface_id_t int_id) {
  // Check table for ip-port pair.
  // If not found, then throw out the packet (firewall behavior)
  // If found, then send the packet towards the LAN side to the IP that sent the original outgoing packet.
}