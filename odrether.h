#ifndef ODRETHER_H_
#define ODRETHER_H_

#include "pfsocket.h"

#define ETH_LEN 14
#define MAX_ETH_FRAME 1500

int send_frame_dgram(int fd, char *dst,int if_index, void *message, int msglen);
int send_frame(int fd, char *src,char *dst, int if_index,void *message, int msglen);
int recv_frame(int fd, char *recv,void*output,int *outlen);
int recv_frame_dgram(int fd, char *recv,void *output,int *outlen);

#endif
