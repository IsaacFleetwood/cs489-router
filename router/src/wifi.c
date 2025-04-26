#include "../include/wifi.h"
#include "../include/ipv4.h"
#include "../include/arp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// =============================
// Wi-Fi packet handler
// =============================
void wifi_handle(pkt_wifi_hdr* pkt_ptr, interface_id_t int_id) {
  // TODO: Read LLC/SNAP Header from 802.11 packet

  // Read in the EtherType field from the SNAP header.
  uint16_t ethertype;
  // And determine the position of the inner network-layer packet.
  uint8_t* inner_pkt_data;

  if (ethertype == ETHERTYPE_IPV4) {
    ipv4_handle((ipv4_hdr_t*) inner_pkt_data, int_id);
  } else if (ethertype == ETHERTYPE_ARP) {
    arp_handle((arp_hdr_t*) inner_pkt_data, int_id, pkt_ptr->addr_src);
  }
}

// =============================
// Wi-Fi interface control
// =============================

// Helper function to run shell commands
static void exec_cmd(const char *cmd) {
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Failed to execute: %s\n", cmd);
    }
}

void wifi_init(const char *interface) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ip link set %s up", interface);
    exec_cmd(cmd);
}

void wifi_set_ip(const char *interface, const char *ip_addr, const char *netmask) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "ip addr add %s/%s dev %s", ip_addr, netmask, interface);
    exec_cmd(cmd);
}
