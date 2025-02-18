#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>

#define MAX_NAPT_ENTRIES 256  // Maximum number of NAT table entries

/*** MAC Address Structure (6 bytes) ***/
typedef struct mac_addr {
  uint8_t bytes[6];
} mac_addr_t;

/*** IPv4 Address Structure (4 bytes) ***/
typedef struct ip_addr {
  uint8_t bytes[4];
} ip_addr_t;

/*** Convert IP Address Struct <-> uint32_t for Easier Processing ***/
#define IP_TO_UINT(ip) \
    ((ip.bytes[0] << 24) | (ip.bytes[1] << 16) | (ip.bytes[2] << 8) | ip.bytes[3])

#define UINT_TO_IP(uint, ip) { \
    ip.bytes[0] = (uint >> 24) & 0xFF; \
    ip.bytes[1] = (uint >> 16) & 0xFF; \
    ip.bytes[2] = (uint >> 8) & 0xFF; \
    ip.bytes[3] = uint & 0xFF; \
}

/*** Port Structure (2 bytes) ***/
typedef struct port {
  uint16_t value;
} port_t;

/*** Network Address and Port Translation (NAPT) Table Entry ***/
typedef struct {
  ip_addr_t lan_ip;     // Private IP (original source)
  port_t lan_port;      // Private port (original source port)
  ip_addr_t public_ip;  // Public IP (router's public IP)
  port_t public_port;   // New assigned port for translation
  ip_addr_t dest_ip;    // Destination IP (external server)
  port_t dest_port;     // Destination port (external server)
  uint32_t timestamp;   // Timestamp for timeout handling
} napt_entry_t;

/*** NAPT Table Structure ***/
typedef struct {
  napt_entry_t entries[MAX_NAPT_ENTRIES];  // Array of NAPT entries
  uint32_t entry_count;                    // Current number of entries
} napt_table_t;

/*** Ethernet Frame Structure ***/
typedef struct {
  mac_addr_t dst_mac;  // Destination MAC address
  mac_addr_t src_mac;  // Source MAC address
  uint16_t ethertype;  // Ethernet frame type (IPv4, ARP, etc.)
} ethernet_hdr_t;

/*** IPv4 header ***/
typedef struct {
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
} ipv4_hdr_t;

#endif