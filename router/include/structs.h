#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>

typedef struct mac_addr {
  uint8_t bytes[6];
} mac_addr_t;

typedef struct ip_addr {
  uint8_t bytes[4];
} ip_addr_t;

#endif