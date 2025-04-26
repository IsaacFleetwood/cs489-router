#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include "../include/dhcp.h"


typedef struct {
    struct in_addr ip;
    uint8_t mac[6];
    time_t lease_start;
    time_t lease_expiry;
    int allocated;
} dhcp_lease_t;

#define LEASE_TIME 1800     // Lease time: 30 minutes

// Separate lease tables
static dhcp_lease_t lease_table_wired[MAX_LEASES];
static dhcp_lease_t lease_table_wifi[MAX_LEASES];

// Pool starts and ends
struct in_addr dhcp_pool_start_wired;
struct in_addr dhcp_pool_end_wired;
struct in_addr dhcp_pool_start_wifi;
struct in_addr dhcp_pool_end_wifi;

struct sockaddr_in broad_addr;

/* Initialize DHCP IP pools */
void init_dhcp_pool() {
    // Wired pool
    if (inet_aton("192.168.1.100", &dhcp_pool_start_wired) == 0) { perror("Invalid wired start IP"); exit(EXIT_FAILURE); }
    if (inet_aton("192.168.1.110", &dhcp_pool_end_wired) == 0) { perror("Invalid wired end IP"); exit(EXIT_FAILURE); }

    // Wi-Fi pool
    if (inet_aton("192.168.2.100", &dhcp_pool_start_wifi) == 0) { perror("Invalid wifi start IP"); exit(EXIT_FAILURE); }
    if (inet_aton("192.168.2.110", &dhcp_pool_end_wifi) == 0) { perror("Invalid wifi end IP"); exit(EXIT_FAILURE); }

    printf("Wired DHCP Pool: %s - %s\n", inet_ntoa(dhcp_pool_start_wired), inet_ntoa(dhcp_pool_end_wired));
    printf("Wi-Fi DHCP Pool: %s - %s\n", inet_ntoa(dhcp_pool_start_wifi), inet_ntoa(dhcp_pool_end_wifi));
}

/* Cleanup expired leases */
void cleanup_expired_leases(int is_wifi) {
    time_t now = time(NULL);
    dhcp_lease_t *lease_table = is_wifi ? lease_table_wifi : lease_table_wired;

    for (int i = 0; i < MAX_LEASES; i++) {
        if (lease_table[i].allocated && lease_table[i].lease_expiry <= now) {
            lease_table[i].allocated = 0;
            memset(&lease_table[i].ip, 0, sizeof(struct in_addr));
        }
    }
}

/* Get available IP */
struct in_addr get_available_ip(int is_wifi) {
    cleanup_expired_leases(is_wifi);

    dhcp_lease_t *lease_table = is_wifi ? lease_table_wifi : lease_table_wired;
    struct in_addr start_ip = is_wifi ? dhcp_pool_start_wifi : dhcp_pool_start_wired;

    for (int i = 0; i < MAX_LEASES; i++) {
        if (!lease_table[i].allocated) {
            lease_table[i].allocated = 1;
            lease_table[i].lease_start = time(NULL);
            lease_table[i].lease_expiry = lease_table[i].lease_start + LEASE_TIME;
            lease_table[i].ip.s_addr = ntohl(ntohl(start_ip.s_addr) + i);
            return lease_table[i].ip;
        }
    }

    struct in_addr empty = { .s_addr = inet_addr("0.0.0.0") };
    return empty;
}

/* Extract requested IP */
struct in_addr extract_requested_ip(uint8_t* buffer) {
    struct in_addr requested_ip = { .s_addr = 0 };
    uint8_t* options = buffer + 240;

    while (*options != DHCP_END && options < buffer + DHCP_BUFFER_SIZE) {
        uint8_t option_type = options[0];
        uint8_t option_len = options[1];

        if (option_type == DHCP_OPTION_REQUESTED_IP) {
            memcpy(&requested_ip.s_addr, &options[2], 4);
            break;
        }

        options += 2 + option_len;
    }
    return requested_ip;
}

/* Handle incoming DHCP packet */
void handle_dhcp_packet(int sockfd, struct sockaddr_in* client_addr, uint8_t* buffer, int recv_len, int is_wifi) {
    uint8_t* options = buffer + 240;
    uint8_t message_type = 0;

    while (*options != DHCP_END && options < buffer + recv_len) {
        uint8_t option_type = options[0];
        uint8_t option_len = options[1];

        if (option_type == DHCP_OPTION_MESSAGE_TYPE) {
            message_type = options[2];
            break;
        }

        options += 2 + option_len;
    }

    if (message_type == DHCP_DISCOVER) {
        struct in_addr offered_ip = get_available_ip(is_wifi);
        send_dhcp_offer(sockfd, client_addr, buffer, offered_ip, is_wifi);
    } else if (message_type == DHCP_REQUEST) {
        struct in_addr requested_ip = extract_requested_ip(buffer);

        if (requested_ip.s_addr == 0) {
            send_dhcp_nak(sockfd, client_addr, buffer);
        } else {
            send_dhcp_ack(sockfd, client_addr, requested_ip, buffer, is_wifi);
        }
    }
}

/* Send DHCP Offer */
void send_dhcp_offer(int sockfd, struct sockaddr_in* client_addr, uint8_t* buffer, struct in_addr offered_ip, int is_wifi) {
    uint8_t offer_packet[DHCP_BUFFER_SIZE] = {0};
    memcpy(offer_packet, buffer, 240);

    offer_packet[0] = 0x02;                             // Boot Reply
    memcpy(&offer_packet[16], &offered_ip.s_addr, 4);   // yiaddr

    struct in_addr router_ip;
    inet_aton(is_wifi ? "192.168.2.1" : "192.168.1.1", &router_ip);

    uint8_t* options = offer_packet + 240 * 4;
    *(options++) = DHCP_OPTION_MESSAGE_TYPE;
    *(options++) = 1;
    *(options++) = DHCP_OFFER;

    *(options++) = DHCP_OPTION_LEASE_TIME;
    *(options++) = 4;
    uint32_t lease_time = htonl(LEASE_TIME);
    memcpy(options, &lease_time, 4);
    options += 4;

    *(options++) = DHCP_OPTION_SUBNET_MASK;
    *(options++) = 4;
    struct in_addr subnet_mask; inet_aton("255.255.255.0", &subnet_mask);
    memcpy(options, &subnet_mask, 4);
    options += 4;

    *(options++) = DHCP_OPTION_ROUTER;
    *(options++) = 4;
    memcpy(options, &router_ip, 4);
    options += 4;

    *(options++) = DHCP_OPTION_DNS;
    *(options++) = 4;
    struct in_addr dns; inet_aton("8.8.8.8", &dns);
    memcpy(options, &dns, 4);
    options += 4;

    *(options++) = DHCP_END;

    sendto(sockfd, offer_packet, (size_t)(options - offer_packet), 0, (struct sockaddr*) &broad_addr, sizeof(broad_addr));
}

/* Send DHCP ACK */
void send_dhcp_ack(int sockfd, struct sockaddr_in* client_addr, struct in_addr allocated_ip, uint8_t* buffer, int is_wifi) {
    uint8_t ack_packet[DHCP_BUFFER_SIZE] = {0};
    memcpy(ack_packet, buffer, 240);

    ack_packet[0] = 0x02; // Boot Reply
    memcpy(&ack_packet[16], &allocated_ip.s_addr, 4); // yiaddr

    struct in_addr router_ip;
    inet_aton(is_wifi ? "192.168.2.1" : "192.168.1.1", &router_ip);

    uint8_t* options = ack_packet + 240 * 4;
    *(options++) = DHCP_OPTION_MESSAGE_TYPE;
    *(options++) = 1;
    *(options++) = DHCP_ACK;

    *(options++) = DHCP_OPTION_LEASE_TIME;
    *(options++) = 4;
    uint32_t lease_time = htonl(LEASE_TIME);
    memcpy(options, &lease_time, 4);
    options += 4;

    *(options++) = DHCP_OPTION_SUBNET_MASK;
    *(options++) = 4;
    struct in_addr subnet_mask; inet_aton("255.255.255.0", &subnet_mask);
    memcpy(options, &subnet_mask, 4);
    options += 4;

    *(options++) = DHCP_OPTION_ROUTER;
    *(options++) = 4;
    memcpy(options, &router_ip, 4);
    options += 4;

    *(options++) = DHCP_OPTION_DNS;
    *(options++) = 4;
    struct in_addr dns; inet_aton("8.8.8.8", &dns);
    memcpy(options, &dns, 4);
    options += 4;

    *(options++) = DHCP_END;

    sendto(sockfd, ack_packet, (size_t)(options - ack_packet), 0, (struct sockaddr*) &broad_addr, sizeof(broad_addr));
}

/* Send DHCP NAK */
void send_dhcp_nak(int sockfd, struct sockaddr_in* client_addr, uint8_t* buffer) {
    uint8_t nak_packet[DHCP_BUFFER_SIZE] = {0};
    memcpy(nak_packet, buffer, 240 * 4);

    nak_packet[0] = 0x02; // Boot Reply

    uint8_t* options = nak_packet + 240 * 4;
    *(options++) = DHCP_OPTION_MESSAGE_TYPE;
    *(options++) = 1;
    *(options++) = DHCP_NAK;
    *(options++) = DHCP_END;

    sendto(sockfd, nak_packet, (size_t)(options - nak_packet), 0, (struct sockaddr*) &broad_addr, sizeof(broad_addr));
}

/* Start DHCP Server */
void dhcp_server_start() {
    printf("[Router] Starting DHCP server...\n");

    init_dhcp_pool(); // Setup wired and Wi-Fi pools

    memset(&broad_addr, 0, sizeof(broad_addr));
    broad_addr.sin_family = AF_INET;
    broad_addr.sin_port = htons(68);
    broad_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    int sockfd_wired, sockfd_wifi;
    struct sockaddr_in addr_wired, addr_wifi;
    uint8_t buffer[DHCP_BUFFER_SIZE];

    // Wired socket
    sockfd_wired = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_wired < 0) { perror("socket wired"); exit(EXIT_FAILURE); }
    const char *iface_wired = "veth-router";
    if (setsockopt(sockfd_wired, SOL_SOCKET, SO_BINDTODEVICE, iface_wired, strlen(iface_wired)) < 0) {
        perror("setsockopt wired"); exit(EXIT_FAILURE);
    }

    memset(&addr_wired, 0, sizeof(addr_wired));
    addr_wired.sin_family = AF_INET;
    addr_wired.sin_addr.s_addr = INADDR_ANY;
    addr_wired.sin_port = htons(DHCP_SERVER_PORT);

    if (bind(sockfd_wired, (struct sockaddr*)&addr_wired, sizeof(addr_wired)) < 0) {
        perror("bind wired"); close(sockfd_wired); exit(EXIT_FAILURE);
    }

    // Wi-Fi socket
    sockfd_wifi = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_wifi < 0) { perror("socket wifi"); exit(EXIT_FAILURE); }
    const char *iface_wifi = "wlan0";
    if (setsockopt(sockfd_wifi, SOL_SOCKET, SO_BINDTODEVICE, iface_wifi, strlen(iface_wifi)) < 0) {
        perror("setsockopt wifi"); exit(EXIT_FAILURE);
    }

    memset(&addr_wifi, 0, sizeof(addr_wifi));
    addr_wifi.sin_family = AF_INET;
    addr_wifi.sin_addr.s_addr = INADDR_ANY;
    addr_wifi.sin_port = htons(DHCP_SERVER_PORT);

    if (bind(sockfd_wifi, (struct sockaddr*)&addr_wifi, sizeof(addr_wifi)) < 0) {
        perror("bind wifi"); close(sockfd_wifi); exit(EXIT_FAILURE);
    }

    int broadcastEnable = 1;
    setsockopt(sockfd_wired, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
    setsockopt(sockfd_wifi, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    printf("[DHCP] Server running on veth-router and wlan0!\n");

    fd_set readfds;
    socklen_t len = sizeof(struct sockaddr_in);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd_wired, &readfds);
        FD_SET(sockfd_wifi, &readfds);

        int maxfd = (sockfd_wired > sockfd_wifi) ? sockfd_wired : sockfd_wifi;

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(sockfd_wired, &readfds)) {
            int recv_len = recvfrom(sockfd_wired, buffer, DHCP_BUFFER_SIZE, 0, NULL, &len);
            if (recv_len > 0) {
                handle_dhcp_packet(sockfd_wired, NULL, buffer, recv_len, 0); // 0 = wired
            }
        }
        if (FD_ISSET(sockfd_wifi, &readfds)) {
            int recv_len = recvfrom(sockfd_wifi, buffer, DHCP_BUFFER_SIZE, 0, NULL, &len);
            if (recv_len > 0) {
                handle_dhcp_packet(sockfd_wifi, NULL, buffer, recv_len, 1); // 1 = wifi
            }
        }
    }

    close(sockfd_wired);
    close(sockfd_wifi);
}
