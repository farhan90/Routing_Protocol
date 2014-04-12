#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdlib.h>
#include "odr.h"

typedef struct qnode {
  void *data;
  struct qnode *next;
} qnode;

void queue_add(qnode **queue, void *data);
void queue_send_to(
  qnode **queue, 
  char *dest_addr, 
  char *dest_hw_addr, 
  int iface_index, 
  void (*sendfn)(int, void *, char *, int));

#endif
