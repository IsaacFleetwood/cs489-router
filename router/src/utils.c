
#include "../include/utils.h"
#include <stdio.h>

ip_addr_t create_subnet_mask(uint8_t cidr_prefix_len) {
  ip_addr_t mask = {0, 0, 0, 0};
  for(int i = 0; i < 3; i++) {
    if(cidr_prefix_len <= 8) {
      mask.bytes[i] = ((int8_t) -1) << (8 - cidr_prefix_len);
      break;
    } else {
      mask.bytes[i] = 0xff;
      cidr_prefix_len -= 8;
    }
  }
  return mask;
}

int prefix_len_get(ip_addr_t mask) {
  int prefix_len = 0;
  for(int j = 0; j < 4; j++) {
    int val = mask.bytes[j];
    int byte_len = 8;
    while(!(val & 1)) {
      byte_len -= 1;
      val = val >> 1;
    }
    prefix_len += byte_len;
    if(byte_len != 8)
      return prefix_len;
  }
}

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

char* to_string_ip(ip_addr_t ip) {
  char* res;
  asprintf(
    &res,
    "%d.%d.%d.%d",
    ip.bytes[0],
    ip.bytes[1],
    ip.bytes[2],
    ip.bytes[3]
  );
  return res;
}

char* to_string_mac(mac_addr_t mac) {
  char* res;
  asprintf(
    &res,
    "%02x:%02x:%02x:%02x:%02x:%02x",
    mac.bytes[0],
    mac.bytes[1],
    mac.bytes[2],
    mac.bytes[3],
    mac.bytes[4],
    mac.bytes[5]
  );
  return res;
}
