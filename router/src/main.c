#include <arpa/inet.h>
#include <bits/pthreadtypes.h>
#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include "../include/ethernet.h"
#include "../include/timer.h"
#include "../include/stub.h"
#include "../include/utils.h"

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

void* thread_start(void* arg);

int main(int argc, char** argv) {

  napt_init();
  timer_init();

  #ifdef STUB_ENABLED
    stub_init();
    #ifdef TEST_ENABLED
      testcase_run();
      return 0;
    #endif
  #endif


  #ifdef DRIVER_ENABLED
    return driver_main(argc, argv);
  #endif

  // https://stackoverflow.com/questions/4139405/how-can-i-get-to-know-the-ip-address-for-interfaces-in-c
  struct ifaddrs *addrs = {0};
  struct ifaddrs *tmp = {0};

  getifaddrs(&addrs);
  tmp = addrs;
  interface_config_t* wan_config = interface_get_config(interface_get_wan_id());

  while (tmp) {
    if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
      if(strcmp(wan_config->name, tmp->ifa_name) != 0) {
        tmp = tmp->ifa_next;
        continue;
      }

      // gets info
      ip_addr_t ip = *((ip_addr_t*) &((struct sockaddr_in*) tmp->ifa_addr)->sin_addr);
      ip_addr_t mask = *((ip_addr_t*) &((struct sockaddr_in*) tmp->ifa_netmask)->sin_addr);
      ip_addr_t ip_network = apply_mask(mask, ip);
      ip_addr_t ip_gateway = ip_network;
      ip_gateway.bytes[3] |= 1;
      // sets up WAN stuff
      wan_gateway_ip = ip_gateway;
      wan_cidr_prefix_len = prefix_len_get(mask);
      wan_network_ip = ip_network;
      wan_config->interface_ip = ip;
    }
    if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET) {
      mac_addr_t mac = *((mac_addr_t*) &((struct sockaddr_ll*) tmp->ifa_addr)->sll_addr);
      for(int i = 0; i < interface_get_amt(); i++) {
        interface_config_t* config = interface_get_config(i);
        if(strcmp(tmp->ifa_name, config->name) != 0)
          continue;
        config->device_mac_addr = mac;
        break;
      }
    }
    tmp = tmp->ifa_next;
  }

  freeifaddrs(addrs);

  pthread_t threads[interface_get_amt()];
  pthread_attr_t attr;
  for(int int_id = 0; int_id < interface_get_amt(); int_id++) {
    int res = pthread_create(&threads[int_id], &attr, thread_start, (void*) ((size_t) int_id));
  }

  for(int int_id = 0; int_id < interface_get_amt(); int_id++) {
    pthread_join(threads[int_id], NULL);
  }
  
  return 0;
}

void* thread_start(void* arg) {
  // get interface info
  interface_id_t int_id = (interface_id_t) ((size_t) arg);
  // config get
  interface_config_t* config = interface_get_config(int_id);

  // set up raw socket
  char* ifname = config->name;
  int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

  if (sockfd < 0) {
    perror("socket");
    return NULL;
  }

  // more setup
  struct sockaddr_ll sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sll_family = PF_PACKET;
	sockaddr.sll_protocol = htons(ETH_P_ALL);
	sockaddr.sll_ifindex = if_nametoindex(ifname);

	if(bind(sockfd, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) < 0) {
		printf("Unable to bind to device.\n");
		return NULL;
	}
  // config mod
  config->fd = sockfd;

  // go thru all of the interfaces
  // wait for each's fd to be set up
  int check_int_id = 0;
  for(int check_int_id = 0; check_int_id < interface_get_amt(); check_int_id++) {
    // config get
    interface_config_t* config = interface_get_config(check_int_id);
    while(config->fd == 0) {
      // interesting!
      sched_yield();
    }
  }

  // set up buffer
  char* buffer = malloc(ETHER_MTU);
  
  if (buffer == NULL) {
    perror("malloc");
    return NULL;
  }

  while(1) {
    // read packet
		int bytes_rec = read(sockfd, buffer, ETHER_MTU);

    ethernet_hdr_t* eth_pkt = (ethernet_hdr_t*) buffer;
		uint16_t ethertype = ntohs(eth_pkt->ethertype);

    // handle packet
    // MT-WORK: not really any problems up until this pt.
    // Starts out singlethreaded so no concurrency issues
    // Then each thread sets up the fd for its own interface
    // so again no overlap as far as I can tell.
    ethernet_handle(eth_pkt, int_id);
  }

  free(buffer);
  close(sockfd);

  return NULL;
}