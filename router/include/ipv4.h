#ifndef IPV4_H
#define IPV4_H

#include "ethernet.h"
#include "interfaces.h"

struct pkt_ipv4_hdr {
  // TODO: Fill in.
  // https://en.wikipedia.org/wiki/IPv4#/media/File:IPv4_Packet-en.svg
  // NOTE: Don't worry about minutia until it's actually known to be needed.
  //       Many fields will never be touched.

  // structs.h has a "ip_addr_t" type that might be useful.

  /*** First 32-bit Word ***/
  uint8_t version:4;   // IPv4 Version (4 bits)
  uint8_t ihl:4;       // Internet Header Length (IHL) (4 bits)
  uint8_t dscp:6;      // Differentiated Services Code Point (DSCP - 6 bits)
  uint8_t ecn:2;       // Explicit Congestion Notification (ECN - 2 bits)

  /*** Second 32-bit Word ***/
  uint16_t total_length;  // Total Length (16 bits)

  /*** Third 32-bit Word ***/
  uint16_t identification; // Packet Identification (16 bits)
  
  /*** Fourth 32-bit Word ***/
  uint16_t flags:3;      // Flags (3 bits: Reserved, DF, MF)
  uint16_t fragment_offset:13;  // Fragment Offset (13 bits)

  /*** Fifth 32-bit Word ***/
  uint8_t ttl;            // Time to Live (8 bits)
  uint8_t protocol;       // Protocol (TCP, UDP, etc.) (8 bits)
  uint16_t checksum;      // Header Checksum (16 bits)

  /*** Sixth & Seventh 32-bit Words ***/
  ip_addr_t src_ip;       // Source IP Address (32 bits)
  ip_addr_t dst_ip;       // Destination IP Address (32 bits)
};

void ipv4_handle(struct pkt_ipv4_hdr* pkt, interface_id_t int_id);

void napt_inc_handle(struct pkt_ipv4_hdr* pkt, interface_id_t int_id);
void napt_out_handle(struct pkt_ipv4_hdr* pkt, interface_id_t int_id);

#endif