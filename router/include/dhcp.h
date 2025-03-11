#ifndef DHCP_H
#define DHCP_H

#include <stdint.h>
#include <arpa/inet.h>

// DHCP Constants
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DHCP_BUFFER_SIZE 1024

// Function Declarations
void dhcp_server_start();  // Starts the DHCP server
void process_dhcp_request(int sockfd, struct sockaddr_in* client_addr, uint8_t* buffer, int recv_len);

#endif // DHCP_H