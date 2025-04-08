#ifndef DHCP_H
#define DHCP_H

#include <stdint.h>
#include <arpa/inet.h>

// DHCP Constants
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DHCP_BUFFER_SIZE 1024
#define MAX_LEASES 10
#define DHCP_OPTION_MESSAGE_TYPE 53
#define DHCP_OPTION_REQUESTED_IP 50
#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_ACK 5
#define DHCP_NAK 6
#define DHCP_END 255

// Function Declarations
void dhcp_server_start();
void handle_dhcp_packet(int sockfd, struct sockaddr_in* client_addr, uint8_t* buffer, int recv_len);
struct in_addr get_available_ip();
struct in_addr extract_requested_ip(uint8_t* buffer);
void send_dhcp_offer(int sockfd, struct sockaddr_in* client_addr, uint8_t* buffer);
void send_dhcp_ack(int sockfd, struct sockaddr_in* client_addr, struct in_addr allocated_ip, uint8_t* buffer);

#endif // DHCP_H