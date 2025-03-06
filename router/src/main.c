#include <stdio.h>
#include <ifaddrs.h>
#include <pthread.h>

#include "../include/dhcp.h"
#include "../include/timer.h"
#include "../include/stub.h"
#include "../include/stub.h"

// #define TEST_ENABLED
#define DRIVER_ENABLED

int driver_main(int argc, char** argv);
void testcase_run();
void* dhcp_thread(void* arg);

int main(int argc, char** argv) {
  printf("Starting router...\n");

  timer_init();

  #ifdef STUB_ENABLED
  stub_init();
  #endif

  #ifdef TEST_ENABLED
  testcase_run();
  return 0;
  #endif

  // ** Start DHCP Server in a Separate Thread **
  pthread_t dhcp_tid;
  if (pthread_create(&dhcp_tid, NULL, dhcp_thread, NULL) != 0) {
      perror("Failed to start DHCP server");
  }

  #ifdef DRIVER_ENABLED
  return driver_main(argc, argv);
  #endif

  // https://stackoverflow.com/questions/4139405/how-can-i-get-to-know-the-ip-address-for-interfaces-in-c
  struct ifaddrs *addrs,*tmp;

  getifaddrs(&addrs);
  tmp = addrs;

  while (tmp) {
    if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET)
      printf("%s\n", tmp->ifa_name);

    tmp = tmp->ifa_next;
  }

  freeifaddrs(addrs);
}

// ** Function to Run DHCP Server in a Separate Thread **
void* dhcp_thread(void* arg) {
  dhcp_server_start();
  return NULL;
}