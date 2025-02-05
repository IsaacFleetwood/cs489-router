#ifndef STUB_H
#define STUB_H

#include <stdint.h>

struct pcaphdr {
  uint32_t magic;
  uint16_t major_ver;
  uint16_t minor_ver;
  uint32_t reserved_1;
  uint32_t reserved_2;
  uint32_t snap_len;
  struct {
    uint32_t fcs:4;
    uint32_t link_type:28;
  } link_type;
};

#define LINKTYPE_ETHERNET (1)

struct pkt_rec_hdr {
  uint32_t timestamp;
  uint32_t timestamp_lower;
  uint32_t captured_pkt_len;
  uint32_t original_pkt_len;
};

#endif