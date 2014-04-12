#ifndef PFSOCKET_H_
#define PFSOCKET_H_

#include "hw_addrs.h"
#include "unp.h"
#include <netpacket/packet.h>
#include <net/ethernet.h>
#define NET_PROTO 5080


typedef struct {
  int fd;
  char hw_addr[6];
  int index;
  char ip_addr[INET_ADDRSTRLEN];
}if_info;

if_info *init();
int get_num_interfaces();
void print_addr(char *addr);
void get_my_ip(char *ptr);

#endif
