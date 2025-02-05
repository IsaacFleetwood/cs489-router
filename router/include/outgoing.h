#ifndef OUTGOING_H
#define OUTGOING_H

#include "ipv4.h"
#include "arp.h"

void send_ipv4(struct pkt_ipv4_hdr* pkt);
void send_arp(struct pkt_arp_hdr* pkt, interface_id_t int_id);

#endif