#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>

typedef struct mac_addr {
  uint8_t bytes[6];
} mac_addr_t;

typedef struct ip_addr {
  uint8_t bytes[4];
} ip_addr_t;

struct pkt_udp_hdr {
    uint16_t port_src;
    uint16_t port_dst;
    uint16_t length;
    uint16_t checksum;
};
struct pkt_tcp_hdr {
    uint32_t idk_1;
    uint32_t idk_2;
    uint32_t idk_3;
    uint32_t idk_4;
    uint16_t checksum;
    uint32_t idk_5;
};

#endif