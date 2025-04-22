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
struct sockaddr_in broad_addr;

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
    // inet_ntoa uses the same buffer every time, so must print separately.
    printf("DHCP Pool Range: %s - ", inet_ntoa(dhcp_pool_start));
    printf("%s\r\n", inet_ntoa(dhcp_pool_end));
}

/* Validate IP address functions */
int is_ip_in_pool(struct in_addr ip) {
    uint32_t ip_val = (ip.s_addr);
    return ip_val >= (dhcp_pool_start.s_addr) &&
           ip_val <= (dhcp_pool_end.s_addr);
}

int is_ip_available(struct in_addr ip) {
    for (int i = 0; i < MAX_LEASES; i++) {
        if (lease_table[i].allocated &&
            lease_table[i].ip.s_addr == ip.s_addr &&
            time(NULL) < lease_table[i].lease_expiry) {
            return 0; // IP is already leased
        }
    }
    return 1; // IP is available
}

void dhcp_server_start() {
    init_dhcp_pool();

    // Setup broadcast address to be used when responding.
    memset(&broad_addr, 0, sizeof(broad_addr));
    broad_addr.sin_family = AF_INET;
    broad_addr.sin_port = htons(68); // DHCP client port
    broad_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST); // 255.255.255.255


    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    uint8_t buffer[DHCP_BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    const char *iface = "veth-router";
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, iface, strlen(iface)) < 0) {
        perror("setsockopt(SO_BINDTODEVICE)");
        exit(EXIT_FAILURE);
    }


    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;//inet_addr("192.168.1.1");//INADDR_ANY;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int broadcastEnable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    printf("DHCP Server running on port %d\r\n", DHCP_SERVER_PORT);

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
                                                                                            //________DORA______
    if (message_type == DHCP_DISCOVER) {                                                    //                  | Discover
        printf("Received DHCP Discover from %s\r\n", inet_ntoa(client_addr->sin_addr));       //                  |
        send_dhcp_offer(sockfd, client_addr, buffer);                                       //                  | Offer
    } else if (message_type == DHCP_REQUEST) {
        printf("Received DHCP Request from %s\r\n", inet_ntoa(client_addr->sin_addr));
    
        struct in_addr requested_ip = extract_requested_ip(buffer);
        printf("DHCP Request requested %s\r\n", inet_ntoa(requested_ip));
        
        printf("%d %d\r\n", is_ip_in_pool(requested_ip), is_ip_available(requested_ip));
        if (!is_ip_in_pool(requested_ip) || is_ip_available(requested_ip)) {
            send_dhcp_nak(sockfd, client_addr, buffer);
        } else {
            send_dhcp_ack(sockfd, client_addr, requested_ip, buffer);
        }
    }    
}

struct in_addr get_available_ip() {
    cleanup_expired_leases();

    for (int i = 0; i < MAX_LEASES; i++) {
        if (!lease_table[i].allocated) {
            lease_table[i].allocated = 1;
            lease_table[i].lease_start = time(NULL);
            lease_table[i].lease_expiry = lease_table[i].lease_start + 180000;
            lease_table[i].ip.s_addr = ntohl(ntohl(dhcp_pool_start.s_addr) + i);
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
            break;
        }
        
        options += 2 + option_len;
    }
    return requested_ip;
}

void send_dhcp_offer(int sockfd, struct sockaddr_in* client_addr, uint8_t* buffer) {
    struct in_addr offered_ip = get_available_ip();
    printf("Sending DHCP Offer (IP: %s)\r\n", inet_ntoa(offered_ip));

    uint8_t offer_packet[DHCP_BUFFER_SIZE] = {0};
    memcpy(offer_packet, buffer, 240);

    offer_packet[0] = 0x02;
    memcpy(&offer_packet[16], &offered_ip.s_addr, 4); // yiaddr
    struct in_addr server_addr; inet_aton("192.168.1.1", &server_addr);
    memcpy(&offer_packet[24], &server_addr, 4); // giaddr
    struct in_addr gateway; inet_aton("192.168.1.1", &gateway);
    memcpy(&offer_packet[24], &gateway, 4); // giaddr
    
    uint8_t* options = offer_packet + 240 * 4;
    *(options++) = DHCP_OPTION_MESSAGE_TYPE;
    *(options++) = 1;
    *(options++) = DHCP_OFFER;

    *(options++) = DHCP_OPTION_REQUESTED_IP;
    *(options++) = 4;
    memcpy(options, &offered_ip.s_addr, 4);
    options += 4;

    *(options++) = DHCP_OPTION_LEASE_TIME;
    *(options++) = 4;
    uint32_t lease_time = htonl(18000);
    memcpy(options, &lease_time, 4);
    options += 4;

    *(options++) = DHCP_OPTION_SUBNET_MASK;
    *(options++) = 4;
    struct in_addr subnet_mask; inet_aton("255.255.255.0", &subnet_mask);
    memcpy(options, &subnet_mask, 4);
    options += 4;

    *(options++) = DHCP_OPTION_ROUTER;
    *(options++) = 4;
    struct in_addr router; inet_aton("192.168.1.1", &router);
    memcpy(options, &router, 4);
    options += 4;

    *(options++) = DHCP_OPTION_DNS;
    *(options++) = 4;
    struct in_addr dns; inet_aton("8.8.8.8", &dns);
    memcpy(options, &dns, 4);
    options += 4;

    *(options++) = DHCP_END;

    int res = sendto(sockfd, offer_packet, ((size_t) options - (size_t) offer_packet), 0, (struct sockaddr*) &broad_addr, sizeof(broad_addr));
    if(res == -1) {
        perror("sendto");
    }
}

void send_dhcp_ack(int sockfd, struct sockaddr_in* client_addr, struct in_addr allocated_ip, uint8_t* buffer) {
    printf("Sending DHCP ACK (IP: %s)\r\n", inet_ntoa(allocated_ip));

    uint8_t ack_packet[DHCP_BUFFER_SIZE] = {0};
    memcpy(ack_packet, buffer, 240);

    ack_packet[0] = 0x02;
    memcpy(&ack_packet[16], &allocated_ip.s_addr, 4); // yiaddr

    uint8_t* options = ack_packet + 240 * 4;
    *(options++) = DHCP_OPTION_MESSAGE_TYPE;
    *(options++) = 1;
    *(options++) = DHCP_ACK;

    *(options++) = DHCP_OPTION_REQUESTED_IP;
    *(options++) = 4;
    memcpy(options, &allocated_ip.s_addr, 4);
    options += 4;

    *(options++) = DHCP_OPTION_LEASE_TIME;
    *(options++) = 4;
    uint32_t lease_time = htonl(18000);
    memcpy(options, &lease_time, 4);
    options += 4;

    *(options++) = DHCP_OPTION_SUBNET_MASK;
    *(options++) = 4;
    struct in_addr subnet_mask; inet_aton("255.255.255.0", &subnet_mask);
    memcpy(options, &subnet_mask, 4);
    options += 4;

    *(options++) = DHCP_OPTION_ROUTER;
    *(options++) = 4;
    struct in_addr router; inet_aton("192.168.1.1", &router);
    memcpy(options, &router, 4);
    options += 4;

    *(options++) = DHCP_OPTION_DNS;
    *(options++) = 4;
    struct in_addr dns; inet_aton("8.8.8.8", &dns);
    memcpy(options, &dns, 4);
    options += 4;

    *(options++) = DHCP_END;

    int res = sendto(sockfd, ack_packet, ((size_t) options - (size_t) ack_packet), 0, (struct sockaddr*) &broad_addr, sizeof(broad_addr));
    if(res == -1) {
        perror("sendto");
    }
}

void send_dhcp_nak(int sockfd, struct sockaddr_in* client_addr, uint8_t* buffer) {
    printf("Sending DHCP NAK\r\n");

    uint8_t nak_packet[DHCP_BUFFER_SIZE] = {0};
    memcpy(nak_packet, buffer, 240 * 4); // Copy original BOOTP header for xid and chaddr

    nak_packet[0] = 0x02;

    uint8_t* options = nak_packet + 240 * 4;
    *(options++) = DHCP_OPTION_MESSAGE_TYPE;
    *(options++) = 1;
    *(options++) = DHCP_NAK;
    *(options++) = DHCP_END;

    sendto(sockfd, nak_packet, ((size_t) options - (size_t) nak_packet), 0,
        (struct sockaddr*) &broad_addr, sizeof(broad_addr));
}

void cleanup_expired_leases() {
    time_t now = time(NULL);

    for (int i = 0; i < MAX_LEASES; i++) {
        if (lease_table[i].allocated && lease_table[i].lease_expiry <= now) {
            printf("Reclaiming expired lease: %s\r\n", inet_ntoa(lease_table[i].ip));
            lease_table[i].allocated = 0;
            lease_table[i].lease_start = 0;
            lease_table[i].lease_expiry = 0;
            memset(&lease_table[i].ip, 0, sizeof(struct in_addr));
        }
    }
}
