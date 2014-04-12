#ifndef _ODR_H_
#define _ODR_H_

#include "common.h"
#include "uds.h"
#include "table.h"
#include "pfsocket.h"
#include "queue.h"
#include "odrether.h"

#include <signal.h>

#define PORT_TABLE_TTL 5 * 60

#define RREQ_TYPE 0
#define RREP_TYPE 1
#define APP_TYPE 2

#include "odr.h"

typedef struct odr_header {
  char src_addr[32];
  char dest_addr[32];
  int type;
  int hop_count;
  int rediscover;
} odr_header;

typedef struct odr_app_message {
  odr_header header;
  uds_message message;
} odr_app_message;

typedef struct odr_route_message {
  odr_header header;
  int bid;
  int should_rrep;
} odr_route_message;

typedef struct routing_table_entry {
  int iface_index;
  int hop_count;
  int bid;
  char dest_addr[6];
} routing_table_entry;

void on_sigint(int signo);
void listen_loop();

void print_uds_message(char *heading, uds_message *message);

void open_odr_uds_socket();
void close_odr_uds_socket();

void populate_port_table();
int get_src_port(char *recv_from);

void deliver_uds_message(uds_message *message);
void handle_uds_message();

void open_packet_sockets();
void fill_header(odr_header *header, char *src_addr, char *dest_addr, int type, int hop_count,int rediscover);
long ip_addr_to_table_key(char *ip_addr);
void init_routing_table();
void init_bid_table();
if_info *if_info_from_index(int index);
void on_application_message(odr_app_message *message, int iface_index, char *src_addr);
void on_rreq_message(odr_route_message *message, int iface_index, char *src_addr);
void on_rrep_message(odr_route_message *message, int iface_index, char *src_addr);
int highest_bid_from_src_addr(char *src_addr);
int consider_adding_to_table(
    char *dest_addr, int hop_count, int iface_index, char *dest_hw_addr, int bid, int *old_bid);
void broadcast_rreq(odr_route_message *message, int except_index);
void send_rrep(char *src_addr, char *dest_addr, int iface_index, char *dest_hw_addr,int rediscover);
void forward_message(odr_header *header, int iface_index, char *dest_hw_addr, int len);
void send_queued_message(int type, void *message, char *dest_addr, int iface_index);
void handle_odr_message(if_info *iface);
void print_send_info(char *src, char *dest, char *dest_hwaddr, int interface, int msg_type);
void ip_to_vm(char *ip,char *myaddr, char *res);
#endif
