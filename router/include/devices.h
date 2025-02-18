#ifndef DEVICES_H
#define DEVICES_H

#include <stdbool.h>

#include "structs.h"

typedef struct ip_mac_pair {
    ip_addr_t ip_addr;
    mac_addr_t mac_addr;
} ip_mac_pair_t;

void update_ip_mac_pair(ip_mac_pair_t pair);
bool has_mac_for_ip(ip_addr_t ip);
mac_addr_t get_mac_from_ip(ip_addr_t ip);

void print_pairs();

#endif