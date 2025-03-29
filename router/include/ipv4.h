#ifndef IPV4_H
#define IPV4_H

#include "ethernet.h"
#include "interfaces.h"
#include "structs.h"

typedef struct {
  /*** First 32-bit Word ***/
  uint8_t ver_ihl;          // Version (4 bits) + IHL (4 bits)
  uint8_t dscp_ecn;         // DSCP (6 bits) + ECN (2 bits)
  uint16_t total_length;    // Total Length (16 bits)

  /*** Second 32-bit Word ***/
  uint16_t identification;  // Packet Identification (16 bits)
  uint16_t flags_offset;    // Flags (3 bits) + Fragment Offset (13 bits)

  /*** Third 32-bit Word ***/
  uint8_t ttl;              // Time to Live (8 bits)
  uint8_t protocol;         // Protocol (TCP, UDP, etc.) (8 bits)
  uint16_t checksum;        // Header Checksum (16 bits)

  /*** Fourth & Fifth 32-bit Words ***/
  ip_addr_t ip_src;         // Source IP Address (32 bits)
  ip_addr_t ip_dst;         // Destination IP Address (32 bits)
} __attribute__((packed)) ipv4_hdr_t;

typedef struct {
  ip_addr_t ip_dst;
  port_t port_src;
  port_t port_dst;
  uint8_t protocol;
} __attribute__((packed)) napt_extern_key_t;

typedef struct {
  ip_addr_t ip_src;
  port_t port_src;
  uint8_t protocol;
} __attribute__((packed)) napt_intern_key_t;

void napt_init();
void ipv4_handle(ipv4_hdr_t* pkt, interface_id_t int_id);

void napt_inc_handle(ipv4_hdr_t* pkt, interface_id_t int_id);
void napt_out_handle(ipv4_hdr_t* pkt, interface_id_t int_id);

#endif