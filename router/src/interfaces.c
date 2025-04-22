

#include "../include/interfaces.h"
#include "../include/utils.h"

// TODO: Fill in mac addresses and interface IPs.
// TODO: Move to configurable file/db? Add management interface where you can change this?
// TODO: Use mutexes to guard this data structure. (Depending on when/if it is modified)
interface_config_t interface_arr[] = {
  {
    .name = "veth-router",
    .device_mac_addr = {{0, 0, 0, 0, 0, 0}},
    .network_ip = {192,168,1,0},
    .cidr_prefix_len = 24,
    .interface_ip = {192,168,1,1},
    .side = INT_SIDE_LAN,
    .type = INT_TYPE_ETHER,
  },/*
  {
    .name = "wlo0",
    .device_mac_addr = {{0, 0, 0, 0, 0, 0}},
    .network_ip = {192, 168, 2, 0},
    .cidr_prefix_len = 24,
    .interface_ip = {192,168,2,1},
    .side = INT_SIDE_LAN,
    .type = INT_TYPE_WIFI,
  },*/
  {
    .name = "veth-hostroute",
    .device_mac_addr = {{0, 0, 0, 0, 0, 0}},
    .network_ip = {0,0,0,0},
    .cidr_prefix_len = 0,
    .interface_ip = {192,168,0,2},
    .side = INT_SIDE_WAN,
    .type = INT_TYPE_ETHER,
  },
};

ip_addr_t wan_network_ip = {192,168,0,0}; // Given by DHCP
int wan_cidr_prefix_len = 24; // Given by DHCP
ip_addr_t wan_gateway_ip = {192,168,0,1}; // Given by DHCP

mac_addr_t interface_mac_addrs[] = {
    {{0, 0, 0, 0, 0, 0}},
    {{0, 0, 0, 0, 0, 0}},
};

size_t interface_get_amt() {
    return (sizeof(interface_arr)) / sizeof(interface_config_t);
}

interface_config_t* interface_get_config(interface_id_t int_id) {
    return &interface_arr[int_id];
}
interface_id_t interface_get_wan_id() {
    return 1; 
}

uint8_t interface_get_side(interface_id_t int_id) {
    return interface_get_config(int_id)->side;
}

uint8_t interface_get_type(interface_id_t int_id) {
    return interface_get_config(int_id)->type;
}

ip_addr_t interface_get_ip(interface_id_t int_id) {
    return interface_get_config(int_id)->interface_ip;
}

ip_addr_t interface_get_network_ip(interface_id_t int_id) {
    return interface_get_config(int_id)->network_ip;
}

ip_addr_t interface_get_subnet_mask(interface_id_t int_id) {
  uint8_t cidr_prefix_len = interface_get_config(int_id)->cidr_prefix_len;
  return create_subnet_mask(cidr_prefix_len);
}

interface_id_t get_interface_for_ip(ip_addr_t ip) {
  interface_id_t int_id;
  int8_t highest_cidr_prefix_len = -1;
  for(uint8_t i = 0; i < (sizeof(interface_arr) / sizeof(interface_config_t)); i += 1) {
    interface_config_t* config = &interface_arr[i];
    if(config->cidr_prefix_len <= highest_cidr_prefix_len)
      continue;
    ip_addr_t mask = interface_get_subnet_mask(i);
    if(ip_addr_equals(apply_mask(mask, ip), config->network_ip)) {
      int_id = i;
      highest_cidr_prefix_len = config->cidr_prefix_len;
    }
  }
  return int_id;
}

mac_addr_t interface_get_mac_addr(interface_id_t int_id) {
    return interface_arr[int_id].device_mac_addr;
}