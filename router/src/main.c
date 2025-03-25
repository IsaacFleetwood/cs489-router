#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/ethernet.h"
#include "../include/timer.h"
#include "../include/stub.h"
#include "../include/stub.h"


#define ETHER_MTU (1518)

// struct mac_addr {
//     unsigned char bytes[6];
// };

char* mac_addr_to_string(struct mac_addr mac_addr) {
    char* mac_addr_str = malloc(18);
    for(int i = 0; i < 6; i++) {
        sprintf(&mac_addr_str[i*3], "%02x", mac_addr.bytes[i]);
        if(i == 5) {
            mac_addr_str[i*3+2] = '\0';
        } else {
            mac_addr_str[i*3+2] = ':';
        }
    }
    return mac_addr_str;
}

// #define TEST_ENABLED
// #define DRIVER_ENABLED

int driver_main(int argc, char** argv);
void testcase_run();

int main(int argc, char** argv) {
  timer_init();

  // #ifdef STUB_ENABLED
  // stub_init();
  // #endif

  // #ifdef TEST_ENABLED
  // testcase_run();
  // return 0;
  // #endif

  // #ifdef DRIVER_ENABLED
  // return driver_main(argc, argv);
  // #endif

  // The network interfaces are:
  // lo and enp0s1 on my mac's VM

  // void ethernet_handle(ethernet_hdr_t* pkt_ptr, interface_id_t int_id)

  char* ifname = "lo";
  int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

  if (sockfd < 0) {
    perror("socket");
    return 1;
  }

  struct sockaddr_ll sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sll_family = PF_PACKET;
	sockaddr.sll_protocol = htons(ETH_P_ALL);
	sockaddr.sll_ifindex = if_nametoindex(ifname);

	if(bind(sockfd, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) < 0) {
		printf("Unable to bind to device.\n");
		return 1;
	}

  char* buffer = malloc(ETHER_MTU);
  
  if (buffer == NULL) {
    perror("malloc");
    return 1;
  }

  while(1) {
		int bytes_rec = read(sockfd, buffer, ETHER_MTU);

    ethernet_hdr_t* eth_pkt = (ethernet_hdr_t*) buffer;
		uint16_t ethertype = ntohs(eth_pkt->ethertype);

    char* mac_src_str = mac_addr_to_string(eth_pkt->mac_src);
		char* mac_dst_str = mac_addr_to_string(eth_pkt->mac_dst);

    printf("Src: %s Dst: %s Type: %#06x\n", mac_src_str, mac_dst_str, ethertype);

    free(mac_src_str);
    free(mac_dst_str);

    interface_id_t intf_id = sockaddr.sll_ifindex; 

    printf("1\n");
    ethernet_handle(eth_pkt, intf_id);
  }

  free(buffer);
  close(sockfd);
  
  return 0;

  // https://stackoverflow.com/questions/4139405/how-can-i-get-to-know-the-ip-address-for-interfaces-in-c
  // struct ifaddrs *addrs = {0};
  // struct ifaddrs *tmp = {0};

  // getifaddrs(&addrs);
  // tmp = addrs;

  // while (tmp) {
  //   // printf("tmp entry\n");
  //   if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET)
  //     printf("%s\n", tmp->ifa_name);

  //   tmp = tmp->ifa_next;
  // }

  // freeifaddrs(addrs);
}
