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

/* Manual Configuration of IP Pool */
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
    
    // Allow reuse of local addresses
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
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
    // Ensure we have at least the standard DHCP header length
    if (recv_len < 240) {
        fprintf(stderr, "Received packet too short (%d bytes)\n", recv_len);
        return;
    }

    uint8_t* options = buffer + 240;
    uint8_t message_type = 0;
    int options_len = recv_len - 240;

    // Parse DHCP options safely
    for (int i = 0; i < options_len;) {
        uint8_t option_type = options[i];
        if (option_type == DHCP_END) {
            break;
        }
        // Ensure we have room for option type and length
        if (i + 1 >= options_len) break;
        uint8_t option_len = options[i + 1];
        if (option_type == DHCP_OPTION_MESSAGE_TYPE && i + 2 < options_len) {
            message_type = options[i + 2];
            break;
        }
        i += 2 + option_len;
    }

    if (message_type == DHCP_DISCOVER) { // Discover
        printf("Received DHCP Discover from %s\n", inet_ntoa(client_addr->sin_addr));
        send_dhcp_offer(sockfd, client_addr, buffer);
    } else if (message_type == DHCP_REQUEST) { // Request
        printf("Received DHCP Request from %s\n", inet_ntoa(client_addr->sin_addr));
        struct in_addr requested_ip = extract_requested_ip(buffer, recv_len);
        send_dhcp_ack(sockfd, client_addr, buffer, requested_ip);
    }
}

struct in_addr get_available_ip() {
    for (int i = 0; i < MAX_LEASES; i++) {
        if (!lease_table[i].allocated) {
            lease_table[i].allocated = 1;
            lease_table[i].lease_start = time(NULL);
            lease_table[i].lease_expiry = lease_table[i].lease_start + 1800; // 30 minutes lease
            lease_table[i].ip.s_addr = htonl(ntohl(dhcp_pool_start.s_addr) + i);
            return lease_table[i].ip;
        }
    }
    struct in_addr empty = { .s_addr = inet_addr("0.0.0.0") };
    return empty;
}

struct in_addr extract_requested_ip(uint8_t* buffer, int buf_len) {
    struct in_addr requested_ip = { .s_addr = 0 };
    if (buf_len < 240) return requested_ip;
    uint8_t* options = buffer + 240;
    int options_len = buf_len - 240;

    for (int i = 0; i < options_len;) {
        uint8_t option_type = options[i];
        if (option_type == DHCP_END) {
            break;
        }
        if (i + 1 >= options_len) break;
        uint8_t option_len = options[i + 1];
        if (option_type == DHCP_OPTION_REQUESTED_IP && i + 2 + 4 <= options_len) {
            memcpy(&requested_ip.s_addr, &options[i + 2], 4);
            requested_ip.s_addr = ntohl(requested_ip.s_addr);
            break;
        }
        i += 2 + option_len;
    }
    return requested_ip;
}

void send_dhcp_offer(int sockfd, struct sockaddr_in* client_addr, uint8_t* request_buffer) {
    struct in_addr offered_ip = get_available_ip();
    printf("Sending DHCP Offer (IP: %s)\n", inet_ntoa(offered_ip));

    uint8_t offer_packet[DHCP_BUFFER_SIZE] = {0};
    // Copy the original DHCP header from the request (first 240 bytes)
    memcpy(offer_packet, request_buffer, 240);
    // Set the yiaddr field (offset 16 in DHCP header) to the offered IP
    memcpy(offer_packet + 16, &offered_ip.s_addr, sizeof(offered_ip.s_addr));

    // Append DHCP options
    uint8_t* options = offer_packet + 240;
    *options++ = DHCP_OPTION_MESSAGE_TYPE;
    *options++ = 1;
    *options++ = DHCP_OFFER;
    *options++ = DHCP_OPTION_REQUESTED_IP;
    *options++ = 4;
    memcpy(options, &offered_ip.s_addr, 4);
    options += 4;
    *options++ = DHCP_END;

    sendto(sockfd, offer_packet, DHCP_BUFFER_SIZE, 0, (struct sockaddr*)client_addr, sizeof(*client_addr));
}

void send_dhcp_ack(int sockfd, struct sockaddr_in* client_addr, uint8_t* request_buffer, struct in_addr allocated_ip) {
    printf("Sending DHCP ACK (IP: %s)\n", inet_ntoa(allocated_ip));

    uint8_t ack_packet[DHCP_BUFFER_SIZE] = {0};
    // Copy the original DHCP header from the request
    memcpy(ack_packet, request_buffer, 240);
    // Set the yiaddr field to the allocated IP
    memcpy(ack_packet + 16, &allocated_ip.s_addr, sizeof(allocated_ip.s_addr));

    uint8_t* options = ack_packet + 240;
    *options++ = DHCP_OPTION_MESSAGE_TYPE;
    *options++ = 1;
    *options++ = DHCP_ACK;
    *options++ = DHCP_OPTION_REQUESTED_IP;
    *options++ = 4;
    memcpy(options, &allocated_ip.s_addr, 4);
    options += 4;
    *options++ = DHCP_END;

    sendto(sockfd, ack_packet, DHCP_BUFFER_SIZE, 0, (struct sockaddr*)client_addr, sizeof(*client_addr));
}