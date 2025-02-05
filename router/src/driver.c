#include <stdio.h>
#include <stdlib.h>
#include "../include/ethernet.h"
#include "../include/stub.h"

int driver_main(int argc, char** argv) {
  if(argc < 2) {
    printf("Invalid arguments: %s <pcap file>\n", argv[0]);
    return 1;
  }
  FILE* file = fopen(argv[1], "r");
  if(file == NULL) {
    printf("Invalid file. File does not exist\n");
    return 1;
  }
  int amt_read;

  struct pcaphdr file_header;
  amt_read = fread(&file_header, sizeof(struct pcaphdr), 1, file);
  if(amt_read == 0) {
    printf("Unable to read file header. Note: The file must be a pcap file.\n");
    return 1;
  }

  uint8_t* buffer = malloc(file_header.snap_len);
  // If 'f' is not set, assume no CRC bytes. Otherwise, read FCS to determine number of CRC bytes.
  int fcs_size = (file_header.link_type.fcs & 0x1) ? (file_header.link_type.fcs >> 1) : 0;

  interface_id_t int_id = 0;

  int index = 0;
  struct pkt_rec_hdr record_header;
  // Read the captured packet record header
  while((amt_read = fread(&record_header, sizeof(struct pkt_rec_hdr), 1, file)) != 0) {
    // Print packet information
    printf("Packet %d: timestamp= %#08x subtimestamp= %#08x actual length= %d captured_length= %d\n",
      index + 1, record_header.timestamp, record_header.timestamp_lower, record_header.original_pkt_len, record_header.captured_pkt_len);
    fread(buffer, record_header.captured_pkt_len, 1, file);
    ethernet_handle((pkt_ether_hdr*) buffer, int_id);
    index += 1;
  }
  return 0;
}
