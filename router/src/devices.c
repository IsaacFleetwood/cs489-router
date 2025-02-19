
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/devices.h"
#include "../include/utils.h"

ip_mac_pair_t* ip_mac_map_arr = NULL;
int ip_mac_map_alloc_amt = 0;
int ip_mac_map_amt = 0;
pthread_mutex_t ip_mac_map_mutex;

void update_ip_mac_pair(ip_mac_pair_t pair) {
  pthread_mutex_lock(&ip_mac_map_mutex);
  // Attempt to update existing mapping.
  for(int i = 0; i < ip_mac_map_amt; i++) {
    if(ip_addr_equals(pair.ip_addr, ip_mac_map_arr[i].ip_addr)) {
      ip_mac_map_arr[i].mac_addr = pair.mac_addr;
      pthread_mutex_unlock(&ip_mac_map_mutex);
      return;
    }
  }
  // If it has gotten to this point, it isn't in the map yet.
  // So add it to the map.

  // If there is no space left. Reallocate the array with more space.
  if(ip_mac_map_amt >= ip_mac_map_alloc_amt) {
    if(ip_mac_map_alloc_amt == 0)
      ip_mac_map_alloc_amt = 5;
    ip_mac_map_arr = realloc(ip_mac_map_arr, (ip_mac_map_alloc_amt * 2) * sizeof(ip_mac_pair_t));
  }
  // By this point, more space will be available, so add it to the end.
  ip_mac_map_arr[ip_mac_map_amt] = pair;
  ip_mac_map_amt += 1;
  pthread_mutex_unlock(&ip_mac_map_mutex);
}

bool has_mac_for_ip(ip_addr_t ip) {
  pthread_mutex_lock(&ip_mac_map_mutex);
  for(int i = 0; i < ip_mac_map_amt; i++) {
    if(ip_addr_equals(ip, ip_mac_map_arr[i].ip_addr)) {
      pthread_mutex_unlock(&ip_mac_map_mutex);
      return true;
    }
  }
  pthread_mutex_unlock(&ip_mac_map_mutex);
  return false;
}

mac_addr_t get_mac_from_ip(ip_addr_t ip) {
  pthread_mutex_lock(&ip_mac_map_mutex);
  mac_addr_t mac = { 0 };
  for(int i = 0; i < ip_mac_map_amt; i++) {
    if(ip_addr_equals(ip, ip_mac_map_arr[i].ip_addr)) {
      mac = ip_mac_map_arr[i].mac_addr;
      break;
    }
  }
  pthread_mutex_unlock(&ip_mac_map_mutex);
  return mac;
}

void print_pairs() {
  for(int i = 0; i < ip_mac_map_amt; i++) {
    char* str_ip = to_string_ip(ip_mac_map_arr[i].ip_addr);
    char* str_mac = to_string_mac(ip_mac_map_arr[i].mac_addr);
    printf("%s: %s\n", str_ip, str_mac);
    free(str_ip);
    free(str_mac);
  }
}