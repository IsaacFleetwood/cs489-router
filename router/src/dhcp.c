#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
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

static dhcp_lease_t lease_table[MAX_LEASES];

struct in_addr dhcp_pool_start;
struct in_addr dhcp_pool_end;

void init_dhcp_pool() {
    if (inet_aton("192.168.1.100", &dhcp_pool_start) == 0) {
        perror("Invalid start IP");
        exit(EXIT_FAILURE);
    }
    if (inet_aton("192.168.1.110", &dhcp_pool_end) == 0) {
        perror("Invalid end IP");
        exit(EXIT_FAILURE);
    }
    printf("DHCP Pool Range: %s - %s\n", inet_ntoa(dhcp_pool_start), inet_ntoa(dhcp_pool_end));
}

void dhcp_server_start();
void handle_dhcp_packet(int sockfd, struct sockaddr_in* client_addr, uint8_t* buffer, int recv_len);
struct in_addr get_available_ip();
struct in_addr extract_requested_ip(uint8_t* buffer);
void send_dhcp_offer(int sockfd, struct sockaddr_in* client_addr, uint8_t* buffer);
void send_dhcp_ack(int sockfd, struct sockaddr_in* client_addr, struct in_addr allocated_ip);

void dhcp_server_start() {
    init_dhcp_pool();

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    uint8_t buffer[DHCP_BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("DHCP Server running on port %d\n", DHCP_SERVER_PORT);

    socklen_t len = sizeof(client_addr);
    while (1) {
        int recv_len = recvfrom(sockfd, buffer, DHCP_BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &len);
        if (recv_len > 0) {
            handle_dhcp_packet(sockfd, &client_addr, buffer, recv_len);
        }
    }
    close(sockfd);
}

void handle_dhcp_packet(int sockfd, struct sockaddr_in* client_addr, uint8_t* buffer, int recv_len) {
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
        printf("Received DHCP Discover from %s\n", inet_ntoa(client_addr->sin_addr));
        send_dhcp_offer(sockfd, client_addr, buffer);
    } else if (message_type == DHCP_REQUEST) {
        printf("Received DHCP Request from %s\n", inet_ntoa(client_addr->sin_addr));
        struct in_addr requested_ip = extract_requested_ip(buffer);
        send_dhcp_ack(sockfd, client_addr, requested_ip);
    }
}

struct in_addr get_available_ip() {
    for (int i = 0; i < MAX_LEASES; i++) {
        if (!lease_table[i].allocated) {
            lease_table[i].allocated = 1;
            lease_table[i].lease_start = time(NULL);
            lease_table[i].lease_expiry = lease_table[i].lease_start + 1800;
            lease_table[i].ip.s_addr = htonl(ntohl(dhcp_pool_start.s_addr) + i);
            return lease_table[i].ip;
        }
    }
    struct in_addr empty = { .s_addr = inet_addr("0.0.0.0") };
    return empty;
}

struct in_addr extract_requested_ip(uint8_t* buffer) {
    struct in_addr requested_ip = { .s_addr = 0 };
    uint8_t* options = buffer + 240;

    while (*options != DHCP_END && options < buffer + DHCP_BUFFER_SIZE) {
        uint8_t option_type = options[0];
        uint8_t option_len = options[1];

        if (option_type == DHCP_OPTION_REQUESTED_IP) {
            memcpy(&requested_ip.s_addr, &options[2], 4);
            requested_ip.s_addr = ntohl(requested_ip.s_addr);
            break;
        }
        
        options += 2 + option_len;
    }
    return requested_ip;
}

void send_dhcp_offer(int sockfd, struct sockaddr_in* client_addr, uint8_t* buffer) {
    struct in_addr offered_ip = get_available_ip();
    printf("Sending DHCP Offer (IP: %s)\n", inet_ntoa(offered_ip));

    uint8_t offer_packet[DHCP_BUFFER_SIZE] = {0};
    memcpy(offer_packet, buffer, 240);
    
    uint8_t* options = offer_packet + 240;
    *options++ = DHCP_OPTION_MESSAGE_TYPE;
    *options++ = 1;
    *options++ = DHCP_OFFER;

    *options++ = DHCP_OPTION_REQUESTED_IP;
    *options++ = 4;
    memcpy(options, &offered_ip.s_addr, 4);
    options += 4;

    *options++ = DHCP_END;

    sendto(sockfd, offer_packet, sizeof(offer_packet), 0, (struct sockaddr*)client_addr, sizeof(*client_addr));
}

void send_dhcp_ack(int sockfd, struct sockaddr_in* client_addr, struct in_addr allocated_ip) {
    printf("Sending DHCP ACK (IP: %s)\n", inet_ntoa(allocated_ip));

    uint8_t ack_packet[DHCP_BUFFER_SIZE] = {0};
    memcpy(ack_packet, client_addr, sizeof(struct sockaddr_in)); // Ensure correct client data

    uint8_t* options = ack_packet + 240;

    *options++ = DHCP_OPTION_MESSAGE_TYPE;
    *options++ = 1;
    *options++ = DHCP_ACK;

    *options++ = DHCP_OPTION_REQUESTED_IP;
    *options++ = 4;
    memcpy(options, &allocated_ip.s_addr, 4);
    options += 4;

    *options++ = DHCP_END;

    sendto(sockfd, ack_packet, sizeof(ack_packet), 0, (struct sockaddr*)client_addr, sizeof(*client_addr));
}
