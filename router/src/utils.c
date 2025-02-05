
#include "../include/utils.h"

ip_addr_t apply_mask(ip_addr_t mask, ip_addr_t ip) {
    ip_addr_t res = {
        .bytes[0] = mask.bytes[0] & ip.bytes[0],
        .bytes[1] = mask.bytes[1] & ip.bytes[1],
        .bytes[2] = mask.bytes[2] & ip.bytes[2],
        .bytes[3] = mask.bytes[3] & ip.bytes[3],
    };
    return res;
}

bool ip_addr_equals(ip_addr_t ip1, ip_addr_t ip2) {
    return
        ip1.bytes[0] == ip2.bytes[0] &&
        ip1.bytes[1] == ip2.bytes[1] &&
        ip1.bytes[2] == ip2.bytes[2] &&
        ip1.bytes[3] == ip2.bytes[3];
}