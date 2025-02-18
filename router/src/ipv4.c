
#include "../include/ipv4.h"
#include "../include/interfaces.h"
#include "../include/structs.h"
#include "../include/utils.h"
#include "../include/outgoing.h"

#define ROUTER_PUBLIC_IP 0xC0A80001 // place holder for now

// TODO: Data structure for storing ip-port tuples.
// Keep track of which ip-port pairs to expect data from, so the firewall is open for them.
// Aswell, keep track of where the data needs to go (which local device), whenever it comes back into the network.

static napt_table_t napt_table = { .entry_count = 0};

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
  uint32_t src_ip = IP_TO_UINT(pkt->src_ip);
  uint16_t src_port = pkt->protocol == 6 ? ((transport_hdr_t*)pkt)->src_port : 0; // Handle TCP/UDP only
  uint32_t dest_ip = IP_TO_UINT(pkt->dst_ip);
  uint16_t dest_port = pkt->protocol == 6 ? ((transport_hdr_t*)pkt)->dest_port : 0;

  // Generate new public port
  uint16_t new_src_port = (rand() % (60000 - 1024) + 1024);

  // Store the mapping in the NAPT table
  if (napt_table.entry_count < MAX_NAPT_ENTRIES) {
    napt_entry_t* entry = &napt_table.entries[napt_table.entry_count++];
    entry->lan_ip = pkt->src_ip;
    entry->lan_port.value = src_port;
    UNIT_TO_IP(ROUTER_PUBLIC_IP, entry->public_ip);
    entry->public_port.value = new_src_port;
    entry->dest_ip = pkt->dst_ip;
    entry->dest_port.value = dest_port;
    entry->timestamp = get_time(); // TODO: Implement this function
  }

  // Modify the packet to reflect the new public IP and port
  UNIT_TO_IP(ROUTER_PUBLIC_IP, pkt->src_ip);
  ((transport_hdr_t*)pkt)->src_port = new_src_port;
  pkt->checksum = calculate_checksum(pkt);  // TODO: Implement this function
}

// Handle incoming packets from WAN
void napt_inc_handle(struct pkt_ipv4_hdr* pkt, interface_id_t int_id) {
  uint32_t dest_ip = IP_TO_UINT(pkt->dst_ip);
  uint16_t dest_port = pkt->protocol = 6 ? ((transport_hdr_t*)pkt)->dest_port : 0;

  // Check table for ip-port pair.
  for (int i = 0; i < napt_table.entry_count; i++) {
    if (IP_TO_UINT(napt_table.entries[i].public_ip) == dest_ip && 
        napt_table.entries[i].dest_port.value == dest_port) {

          // Restore the original IP and port
          pkt->dst_ip = napt_table.entries[i].lan_ip;
          ((transport_hdr_t*)pkt)->dest_port = napt_table.entries[i].lan_port.value;
          pkt->checksum = calculate_checksum(pkt);  // TODO: Implement this function

          // Send the packet towards the LAN side to the IP that sent the original outgoing packet
          send_packet_to_lan(pkt);  // TODO: Implement this function
          return;
    }
  }

  // If not found, then throw out the packet (firewall behavior)
  // If found, then send the packet towards the LAN side to the IP that sent the original outgoing packet.
}