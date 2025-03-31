#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>

#include "../include/ipv4.h"
#include "../include/interfaces.h"
#include "../include/structs.h"
#include "../include/utils.h"
#include "../include/outgoing.h"
#include "../include/hashmap.h"

port_t to_port(uint16_t port) {
  return (port_t) {.value = ntohs(port)};
}
uint16_t from_port(port_t port) {
  return htons(port.value);
}

// TODO: Data structure for storing ip-port tuples.
// Keep track of which ip-port pairs to expect data from, so the firewall is open for them.
// Aswell, keep track of where the data needs to go (which local device), whenever it comes back into the network.

hashmap_t napt_extern_hashmap;
hashmap_t napt_intern_hashmap;

size_t napt_extern_key_hash(void* key_ptr) {
  return hashmap_cyclic_hash(key_ptr, sizeof(napt_extern_key_t));
}
size_t napt_intern_key_hash(void* key_ptr) {
  return hashmap_cyclic_hash(key_ptr, sizeof(napt_intern_key_t));
}

void napt_init() {
  // TODO: Consolidate into 1 entry table, with 2 key lookup maps? Maybe?
  napt_extern_hashmap = hashmap_init(napt_extern_key_hash, sizeof(napt_extern_key_t), sizeof(napt_entry_t));
  napt_intern_hashmap = hashmap_init(napt_intern_key_hash, sizeof(napt_intern_key_t), sizeof(napt_entry_t));
}

// TODO: If packet size is > Ethernet's MTU. It will need to be fragmented.
// This will happen when WiFi packets need to be sent over Ethernet.

void ipv4_handle(ipv4_hdr_t* pkt, interface_id_t int_id) {

  if(interface_get_side(int_id) == INT_SIDE_WAN) {
    if(!ip_addr_equals(pkt->ip_dst, interface_get_ip(int_id))) {
      // Check if the destination is our router. If not, disregard packet.
      // TODO: Do mac address checking on link-layer level also?
      return;
    }
    napt_inc_handle(pkt, int_id);
    return;
  }

  interface_config_t* int_config = interface_get_config(int_id);
  
  interface_id_t out_int_id = get_interface_for_ip(pkt->ip_dst);
  // If they are on the same network, disregard the packet.
  if(out_int_id == int_id) {
    return;
  // Otherwise, if it is going to another LAN port, bridge
  } else if(interface_get_side(out_int_id) == INT_SIDE_LAN) {
    // Forward packet to out interface on LAN.
    // No need to do NAPT, because its on the LAN side.
    send_ipv4(pkt);
  } else {
    // If it reaches here, it is going from LAN to WAN.
    // So do NAPT.
    napt_out_handle(pkt, int_id);
  }
}

void napt_out_handle(ipv4_hdr_t* pkt, interface_id_t int_id) {
  // Assume that the packet is already determined to be leaving the network
  ip_addr_t ip_src = pkt->ip_src;
  ip_addr_t ip_dst = pkt->ip_dst;

  // Calculate the offset for the transport layer (TCP/UDP)
  uint8_t* transport_ptr = (uint8_t*)pkt + ((pkt->ver_ihl & 0x0F) * 4); // Extract IHL

  port_t port_src;
  port_t port_dst;
  uint8_t protocol = pkt->protocol;

  // Check if the protocol is TCP (6) or UDP (17) and extract the ports
  if (protocol == 6) {  // TCP
    tcp_hdr_t* tcp_hdr = (tcp_hdr_t*)transport_ptr;
    port_src = to_port(tcp_hdr->port_src);
    port_dst = to_port(tcp_hdr->port_dst);
  } else if (protocol == 17) {  // UDP
    udp_hdr_t* udp_hdr = (udp_hdr_t*)transport_ptr;
    port_src = to_port(udp_hdr->port_src);
    port_dst = to_port(udp_hdr->port_dst);
  } else {
    // TODO: Handle ICMP
    // Not TCP or UDP, drop packet
    return;
  }

  // Generate a new source port (random high port)
  ip_addr_t public_ip = interface_get_ip(interface_get_wan_id());
  port_t new_port_src;

  // Check if there's already a mapping that was made.
  napt_intern_key_t intern_key = {.ip_src = ip_src, .port_src = port_src, .protocol = protocol};
  napt_entry_t* existing_res;
  if((existing_res = hashmap_get(&napt_intern_hashmap, &intern_key))) {
    // If so, just reuse the existing mapping
    new_port_src = existing_res->public_port;
    // TODO: Update the expiry timestamp.
  } else {
    // If not already existing, add a new one.
    int attempts = 0;
    while(attempts < 10) {
      // Value is already in host order. Don't use to_port, which would change the byte order.
      new_port_src.value = (rand() % (60000 - 1024)) + 1024;
      printf("%d %d\n", new_port_src.value, htons(new_port_src.value));
      napt_extern_key_t extern_key = {.ip_dst = ip_dst, .port_src = new_port_src, .port_dst = port_dst, .protocol = protocol};
      if(!hashmap_contains(&napt_extern_hashmap, &extern_key)) {
        napt_entry_t value;
        value.lan_ip = ip_src;
        value.lan_port = port_src;
        value.public_ip = public_ip;
        value.public_port = new_port_src;
        value.dst_ip = ip_dst;
        value.dst_port = port_dst;
        value.timestamp = 0; // TODO: Setup timestamp for expiry
        hashmap_insert(&napt_extern_hashmap, &extern_key, &value);
        napt_intern_key_t intern_key = {.ip_src = ip_src, .port_src = port_src, .protocol = protocol};
        hashmap_insert(&napt_intern_hashmap, &intern_key, &value);
        break;
      }
      attempts += 1;
    }
    if(!(attempts < 10)) {
      // Drop the packet if unable to find a port to send it.
      return;
    }
  }

  // Modify packet - Change Source IP to Public IP
  pkt->ip_src = public_ip;

  // Update Transport Layer Header with New Source Port
  if (protocol == 6) { // TCP
    tcp_hdr_t* tcp_hdr = (tcp_hdr_t*)transport_ptr;
    tcp_hdr->port_src = from_port(new_port_src);
  } else if (protocol == 17) { // UDP
    udp_hdr_t* udp_hdr = (udp_hdr_t*)transport_ptr;
    udp_hdr->port_src = from_port(new_port_src);
  }

  // Checksum is handled in outgoing.c in send_ipv4() function

  // Forward packet to WAN
  send_ipv4(pkt);
}


// Handle incoming packets from WAN
void napt_inc_handle(ipv4_hdr_t* pkt, interface_id_t int_id) {

  // Calculate the offset for the transport layer (TCP/UDP)
  uint8_t* transport_ptr = (uint8_t*)pkt + ((pkt->ver_ihl & 0x0F) * 4); // Extract IHL

  port_t port_src;
  port_t port_dst;
  uint8_t protocol = pkt->protocol;

  // Extract destination port for TCP or UDP
  if (protocol == 6) { // TCP
    tcp_hdr_t* tcp_hdr = (tcp_hdr_t*)transport_ptr;
    port_dst = to_port(tcp_hdr->port_dst);
    port_src = to_port(tcp_hdr->port_src);
  } else if (protocol == 17) { // UDP
    udp_hdr_t* udp_hdr = (udp_hdr_t*)transport_ptr;
    port_dst = to_port(udp_hdr->port_dst);
    port_src = to_port(udp_hdr->port_src);
  } else {
    // TODO: Handle ICMP
    // Not TCP or UDP, drop packet
    return;
  }

  // Check the NAPT table for a matching public IP and port (for the associated protocol)
  napt_extern_key_t extern_key = {.ip_dst = pkt->ip_src, .port_src = port_dst, .port_dst = port_src, .protocol = protocol};
  napt_entry_t* entry = hashmap_get(&napt_extern_hashmap, &extern_key);
  if(entry == NULL) {
    // If no match found, drop the packet (firewall behavior)
    return;
  }
  
  // Restore the original LAN IP and port
  pkt->ip_dst = entry->lan_ip;

  if (protocol == 6) { // TCP
    tcp_hdr_t* tcp_hdr = (tcp_hdr_t*)transport_ptr;
    tcp_hdr->port_dst = from_port(entry->lan_port);
  } else if (protocol == 17) { // UDP
    udp_hdr_t* udp_hdr = (udp_hdr_t*)transport_ptr;
    udp_hdr->port_dst = from_port(entry->lan_port);
  }

  // Checksum is handled in outgoing.c in send_ipv4() function

  // Forward the packet to the LAN
  send_ipv4(pkt);
}