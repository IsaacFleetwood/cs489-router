#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

#include "structs.h"

ip_addr_t apply_mask(ip_addr_t mask, ip_addr_t ip);
bool ip_addr_equals(ip_addr_t ip1, ip_addr_t ip2);

#endif