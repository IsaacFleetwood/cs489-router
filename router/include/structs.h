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
  ip_addr_t lan_ip;       // Private IP (original source)
  port_t lan_port;        // Private port (original source port)
  ip_addr_t public_ip;    // Public IP (router's public IP)
  port_t public_port;     // New assigned port for translation
  ip_addr_t dst_ip;       // Destination IP (external server)
  port_t dst_port;        // Destination port (external server)
  uint32_t timestamp;     // Timestamp for timeout handling
} napt_entry_t;

/*** NAPT Table Structure ***/
typedef struct {
  napt_entry_t entries[MAX_NAPT_ENTRIES];  // Array of NAPT entries
  uint32_t entry_count;                    // Current number of entries
} napt_table_t;

/*** Ethernet Frame Structure ***/
typedef struct {
  mac_addr_t mac_dst;     // Destination MAC address
  mac_addr_t mac_src;     // Source MAC address
  uint16_t ethertype;     // Ethernet frame type (IPv4, ARP, etc.)
} ethernet_hdr_t;

/*** UPD Header Structure ***/
typedef struct {
  uint16_t port_src;      // Source port
  uint16_t port_dst;      // Destination port
  uint16_t length;        // Length of UDP header + data
  uint16_t checksum;      // UDP checksum
} udp_hdr_t;

/*** TCP Header Structure ***/
typedef struct {
  uint16_t port_src;      // Source port
  uint16_t port_dst;      // Destination port
  uint32_t seq_num;       // Sequence number
  uint32_t ack_num;       // Acknowledgement number

  uint8_t data_offset:4;  // Data offset (header length) - 4 bits
  uint8_t reserved:3;     // Reserved - 3 bits (must be zero)
  uint8_t ns:1;           // Nonce sum flag (for ECN) - 1 bit     ------------I
                                                                            
  uint8_t cwr:1;          // Congestion Window Reduced (CWR)                  I
  uint8_t ece:1;          // ECN-Echo                                         I
  uint8_t urg:1;          // Urgent pointer field significant                 I
  uint8_t ack:1;          // Acknowledgment field significant                 I    9 bit of Flags
  uint8_t psh:1;          // Push function                                    I
  uint8_t rst:1;          // Reset connection                                 I
  uint8_t syn:1;          // Synchronize sequence numbers                     I
  uint8_t fin:1;          // No more data from sender             ------------I

  uint16_t window_size;   // Window size
  uint16_t checksum;      // TCP checksum
  uint16_t urgent_ptr;    // Urgent pointer
} tcp_hdr_t;

#endif