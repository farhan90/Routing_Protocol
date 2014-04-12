#include "queue.h"

void queue_add(qnode **queue, void *data) {
  qnode *to_add;
  qnode *cursor;

  to_add = malloc(sizeof(qnode));
  to_add->data = data;
  to_add->next = NULL;

  if (*queue) {
    cursor = *queue;
    while (cursor->next != NULL) {
      cursor = cursor->next;
    }
    cursor->next = to_add;
  } else {
    *queue = to_add;
  }
}

void queue_send_to(
    qnode **queue, 
    char *dest_addr, 
    char *dest_hw_addr, 
    int iface_index, 
    void (*sendfn)(int, void *, char *, int)) {
  qnode *cursor;
  qnode *prev;
  qnode *to_delete;
  odr_header *header;

  prev = NULL;
  cursor = *queue;
  while (cursor != NULL) {
    header = (odr_header *)cursor->data;
    if (strcmp(header->dest_addr, dest_addr) == 0) {
      sendfn(header->type, cursor->data, dest_hw_addr, iface_index);

      if (prev == NULL) {
        *queue = cursor->next;
      } else {
        prev->next = cursor->next;
      }

      to_delete = cursor;
      cursor = cursor->next;

      free(to_delete->data);
      free(to_delete);
    } else {
      prev = cursor;
      cursor = cursor->next;
    }
  }
}
