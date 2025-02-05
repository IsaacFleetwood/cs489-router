#include <stdio.h>
#include <ifaddrs.h>

#define DRIVER_ENABLED (1)
int driver_main(int argc, char** argv);

int main(int argc, char** argv) {

	if(DRIVER_ENABLED) {
		return driver_main(argc, argv);
	}

	struct ifaddrs *addrs,*tmp;

	getifaddrs(&addrs);
	tmp = addrs;

	while (tmp)
	{
		  if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET)
		      printf("%s\n", tmp->ifa_name);

		  tmp = tmp->ifa_next;
	}

	freeifaddrs(addrs);
}
