#ifndef TIMER_H
#define TIMER_H

#include <pthread.h>

#include "ipv4.h"

void timer_init();
void* timer_thread_main(void*);
void timer_handler();
void timer_add_packet(struct pkt_ipv4_hdr* pkt, interface_id_t int_id, ip_addr_t dst);
void timer_update_packets(ip_addr_t dst);
extern volatile struct delta_node* delta_list;
extern pthread_mutex_t timer_mutex;

#endif