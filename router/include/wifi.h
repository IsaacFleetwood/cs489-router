#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>
#include "interfaces.h"
#include "structs.h"

struct pkt_wifi_hdr {
  uint16_t frame_control;
  uint16_t duration;
  mac_addr_t addr_src;
  mac_addr_t addr_dst;
  mac_addr_t addr_transm;
  uint16_t seq_control;
  mac_addr_t addr_recvr;
  uint16_t qos_control;
  uint32_t ht_control;
};
typedef struct pkt_wifi_hdr pkt_wifi_hdr;

void wifi_handle(pkt_wifi_hdr* pkt_ptr, interface_id_t int_id);

#endif