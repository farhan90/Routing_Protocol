#ifndef _UDS_H_
#define _UDS_H_

#include "common.h"

typedef struct uds {
  int fd;
  char path[PATH_MAX];
} uds;

int msg_send(int fd, char *ip_addr, int port, char *message, int rediscover);
int msg_recv(int fd, char *message, char *ip_addr, int *port);

uds uds_create(char *path);
int uds_send(int fd, char *dest_path, char *message, int len);
int uds_recv(int fd, char *recv_from, char *message, int len);
int uds_destroy(uds *uds_info);

#endif
