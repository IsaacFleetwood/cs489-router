#include <stdio.h>
#include <ifaddrs.h>

#include "../include/stub.h"

#define DRIVER_ENABLED
int driver_main(int argc, char** argv);

int main(int argc, char** argv) {

  #ifdef STUB_ENABLED
  stub_init();
  #endif
  #ifdef DRIVER_ENABLED
  return driver_main(argc, argv);
  #endif

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
