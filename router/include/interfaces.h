#ifndef INTERFACES_H
#define INTERFACES_H

#include <stdint.h>

#include "structs.h"

#define INT_TYPE_ETHER (0)
#define INT_TYPE_WIFI (1)
// #define INTERFACE_TYPE_CELL (2)

#define INT_SIDE_LAN (0)
#define INT_SIDE_WAN (1)

typedef uint8_t interface_id_t;

typedef struct interface_config {
  char* name;
  mac_addr_t device_mac_addr;
  ip_addr_t network_ip; // 192.168.0.0
  ip_addr_t interface_ip; // 192.168.0.1
  uint8_t cidr_prefix_len;
  uint8_t type: 2; // Type of interface (ethernet, wifi, etc)
  uint8_t side: 1; // WAN or LAN
} interface_config_t;

extern ip_addr_t wan_network_ip; // Given by DHCP
extern int wan_cidr_prefix_len;  // Given by DHCP
extern ip_addr_t wan_gateway_ip; // Given by DHCP

interface_id_t get_interface_for_ip(ip_addr_t);
interface_config_t* interface_get_config(interface_id_t);
interface_id_t interface_get_wan_id();

ip_addr_t interface_get_subnet_mask(interface_id_t);
ip_addr_t interface_get_ip(interface_id_t);
ip_addr_t interface_get_network_ip(interface_id_t);
uint8_t interface_get_side(interface_id_t);
uint8_t interface_get_type(interface_id_t);
mac_addr_t interface_get_mac_addr(interface_id_t);

#endif