#include <stdio.h>
#include <ifaddrs.h>

#include "../include/timer.h"
#include "../include/stub.h"
#include "../include/stub.h"

// #define TEST_ENABLED
#define DRIVER_ENABLED
int driver_main(int argc, char** argv);
void testcase_run();

int main(int argc, char** argv) {


  timer_init();

  #ifdef STUB_ENABLED
  stub_init();
  #endif

  #ifdef TEST_ENABLED
  testcase_run();
  return 0;
  #endif

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
