#include <stdio.h>

#include "../include/stub.h"

FILE* file;

void stub_init() {
    file = fopen("./out.pcap", "wb+");
    pcap_hdr_t hdr = {
        .magic_number = 0xa1b2c3d4,
        .version_major = 2,
        .version_minor = 4,
        .thiszone = 0,
        .sigfigs = 0,
        .snaplen = 65535,
        .network = 1,
    };
    fwrite(&hdr, sizeof(pcap_hdr_t), 1, file);
}

void stub_write_pkt(uint8_t* ptr, uint32_t size) {
    pcaprec_hdr_t entry_hdr = {
        .ts_sec = 0,
        .ts_usec = 0,
        .incl_len = size,
        .orig_len = size,
    };
    fwrite(&entry_hdr, sizeof(pcaprec_hdr_t), 1, file);
    fwrite(ptr, size, 1, file);
}