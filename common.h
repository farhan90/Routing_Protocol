#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "me.h"

#define IP_ADDR_MAX 16
#define UDS_MESSAGE_MAX 512

#define ODR_PATH "/tmp/" ME "-odr"
#define SERVER_PATH "/tmp/" ME "-server"
#define TEMP_FORMAT "/tmp/" ME "-XXXXXX"

#define SERVER_PORT 0

// UDS application level message
typedef struct uds_message {
  char src_ip_addr[IP_ADDR_MAX];
  int src_port;
  char dest_ip_addr[IP_ADDR_MAX];
  int dest_port;
  char message[UDS_MESSAGE_MAX];
  int rediscover;
} uds_message;

#endif
