#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

#include "structs.h"

ip_addr_t create_subnet_mask(uint8_t cidr_prefix_len);
ip_addr_t apply_mask(ip_addr_t mask, ip_addr_t ip);
bool ip_addr_equals(ip_addr_t ip1, ip_addr_t ip2);

char* to_string_ip(ip_addr_t ip);
char* to_string_mac(mac_addr_t mac);

#endif