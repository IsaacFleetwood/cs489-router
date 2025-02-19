
#include <bits/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/ipv4.h"
#include "../include/timer.h"
#include "../include/utils.h"
#include "../include/outgoing.h"

pthread_t timer_thread_id;
timer_t timer_id;
pthread_mutex_t timer_mutex;
volatile struct delta_node* delta_list = NULL;

struct pkt_list {
    struct pkt_list* next;
    ipv4_hdr_t* pkt;
};
struct delta_node {
    struct delta_node* next;
    uint32_t msec;
    ip_addr_t ip_addr;
    struct pkt_list* pkt_list;
    uint8_t retry_amt;
};

void timer_init() {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    int s = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if(s != 0)
        perror("Error setting sigmask for non-timer threads.");

    pthread_mutex_lock(&timer_mutex);
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_ptr = &timer_id;
    int res = timer_create(CLOCK_REALTIME, &sev, &timer_id);
    if(res != 0) {
        perror("Unable to create timer.");
        return;
    }

    pthread_mutex_unlock(&timer_mutex);

    s = pthread_create(&timer_thread_id, NULL, timer_thread_main, NULL);
}

void* timer_thread_main(void* _) {

    signal(SIGALRM, timer_handler);

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    int s = pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    if(s != 0)
        perror("Error setting sigmask for timer thread.");

    while(1) {
        sched_yield();
    }

}

void timer_handler() {
    pthread_mutex_lock(&timer_mutex);
    if(delta_list == NULL) {
        pthread_mutex_unlock(&timer_mutex);
        return;
    }
    delta_list->msec = 0;
    while(delta_list != NULL && delta_list->msec == 0) {
        struct delta_node* delta_node = (struct delta_node*) delta_list;
        if(delta_node == NULL) {
            pthread_mutex_unlock(&timer_mutex);
            return;
        }
        delta_list = delta_node->next;
        // If it's been tried many times and no response, throw out all packets.
        // Best effort delivery semantics make this acceptable.
        if(delta_node->retry_amt >= 1) {
            printf("Failed to resolve ARP. Discarding packets...\n");
            struct pkt_list* pkt = delta_node->pkt_list;
            while(pkt != NULL) {
                free(pkt->pkt);
                struct pkt_list* next = pkt->next;
                free(pkt);
                pkt = next;
            }
            free(delta_node);
        } else {
            // Try to resend ARP packet
            interface_id_t int_id = get_interface_for_ip(delta_node->ip_addr);
            arp_hdr_t arp_pkt = {
                .hardware_type = htons(0x0001), // Ethernet
                .protocol_type = htons(0x0800), // IPv4
                .hardware_length = 6,
                .protocol_length = 4,
                .operation = htons(0x01), // Request
                .sender_ip = interface_get_ip(int_id),
                .sender_mac = interface_get_mac_addr(int_id),
                .target_ip = delta_node->ip_addr,
                .target_mac = {.bytes = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}
            };
            send_arp(&arp_pkt, int_id);
            delta_node->retry_amt += 1;

            // Read the wait period to the delta list.
            int total_time = 0;
            struct delta_node** node_src = (struct delta_node**) &delta_list;
            while((*node_src) != NULL) {
                total_time += (*node_src)->msec;
                node_src = &(*node_src)->next;
            }
            if(total_time > 1000)
                total_time = 1000;
            delta_node->msec = 1000 - total_time;
            delta_node->next = NULL;
            *node_src = delta_node;
        }
    }

    if(delta_list == NULL) {
        // Disable timer.
        struct itimerspec time;
        time.it_value.tv_nsec = 0;
        time.it_value.tv_sec = 0;
        time.it_interval.tv_nsec = 0;
        time.it_interval.tv_sec = 0;
        timer_settime(timer_id, 0, &time, NULL);
    } else {
        // Set timer to new value.
        int msec = delta_list->msec;
        struct itimerspec time;
        uint64_t total = msec * 1000000;
        time.it_value.tv_nsec = total % 1000000000;
        time.it_value.tv_sec = total / 1000000000;
        time.it_interval.tv_nsec = 0;
        time.it_interval.tv_sec = 0;
        timer_settime(timer_id, 0, &time, NULL);
    }

    pthread_mutex_unlock(&timer_mutex);
}

void timer_update_packets(ip_addr_t dst) {
    pthread_mutex_lock(&timer_mutex);
    struct delta_node* delta_node = (struct delta_node*) delta_list;
    struct delta_node** node_src = (struct delta_node**) &delta_list;
    while(delta_node != NULL) {
        if(ip_addr_equals(delta_node->ip_addr, dst)) {
            // Remove node from delta list.
            if(delta_node == delta_list) {
                // Set timer to next value.
                if(delta_node->next == NULL) {
                    // Disable timer.
                    struct itimerspec time;
                    time.it_value.tv_nsec = 0;
                    time.it_value.tv_sec = 0;
                    time.it_interval.tv_nsec = 0;
                    time.it_interval.tv_sec = 0;
                    timer_settime(timer_id, 0, &time, NULL);
                } else {
                    // Set timer to new value.
                    int extra_msec = delta_node->next->msec;
                    struct itimerspec time;
                    timer_gettime(timer_id, &time);
                    uint64_t total = time.it_value.tv_nsec + extra_msec * 1000000;
                    time.it_value.tv_nsec = total % 1000000000;
                    time.it_value.tv_sec = total / 1000000000;
                    timer_settime(timer_id, 0, &time, NULL);
                }
            }
            *node_src = delta_node->next;
            if(delta_node->next != NULL)
                delta_node->next->msec += delta_node->msec;
            break;
        }
        node_src = &(delta_node->next);
        delta_node = delta_node->next;
    }
    pthread_mutex_unlock(&timer_mutex);
    if(delta_node == NULL)
        return;
    struct pkt_list* pkt = delta_node->pkt_list;
    while(pkt != NULL) {
        send_ipv4(pkt->pkt);
        free(pkt->pkt);
        struct pkt_list* next = pkt->next;
        free(pkt);
        pkt = next;
    }
    free(delta_node);
}

void timer_add_packet(ipv4_hdr_t* pkt, interface_id_t int_id, ip_addr_t dst) {
    pthread_mutex_lock(&timer_mutex);

    int total_time = 0;
    struct delta_node* delta_node = (struct delta_node*) delta_list;
    while(delta_node != NULL) {
        total_time += delta_node->msec;
        if(ip_addr_equals(delta_node->ip_addr, dst)) {
            struct pkt_list* pkts = delta_node->pkt_list;
            while(pkts->next != NULL) {
                pkts = pkts->next;
            }
            struct pkt_list* next_pkt = malloc(sizeof(struct pkt_list));
            next_pkt->next = NULL;
            next_pkt->pkt = pkt;
            pkts->next = next_pkt;
            pthread_mutex_unlock(&timer_mutex);
            return;
        }
        delta_node = delta_node->next;
    }
    // ARP packet needs to be sent. 1s delay needs to be added to delta list.

    arp_hdr_t arp_pkt = {
      .hardware_type = htons(0x0001), // Ethernet
      .protocol_type = htons(0x0800), // IPv4
      .hardware_length = 6,
      .protocol_length = 4,
      .operation = htons(0x01), // Request
      .sender_ip = interface_get_ip(int_id),
      .sender_mac = interface_get_mac_addr(int_id),
      .target_ip = dst,
      .target_mac = {.bytes = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}
    };
    send_arp(&arp_pkt, int_id);
    
    struct pkt_list* next_pkt = malloc(sizeof(struct pkt_list));
    next_pkt->next = NULL;
    next_pkt->pkt = pkt;
    struct delta_node* new_node = malloc(sizeof(struct delta_node));
    new_node->pkt_list = next_pkt;
    new_node->next = NULL;
    if(total_time > 1000) {
        total_time = 1000;
    }
    new_node->msec = 1000 - total_time;
    new_node->ip_addr = dst;
    new_node->retry_amt = 0;

    if(delta_list == NULL) {
        // Set timer to 1s. Activate it.
        delta_list = new_node;
        struct itimerspec its;
        its.it_value.tv_sec = 1;
        its.it_value.tv_nsec = 0;
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        int res = timer_settime(timer_id, 0, &its, NULL);
        if(res != 0) {
            perror("Unable to set time on timer.");
        }
    } else {
        delta_node = (struct delta_node*) delta_list;
        while(delta_node->next != NULL)
            delta_node = delta_node->next;
        delta_node->next = new_node;
    }
    pthread_mutex_unlock(&timer_mutex);
}