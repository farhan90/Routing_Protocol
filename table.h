#ifndef _TABLE_H_
#define _TABLE_H_

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>

typedef struct table_ent {
  long key;
  time_t last_access;
  void *data;
  int data_len;
  struct table_ent *next;
} table_ent;

typedef struct table {
  long ttl;
  table_ent *ents;
  pthread_mutex_t mutex;
  pthread_t cleanup_thread;
} table;

void table_init(table *t, long ttl);
void table_insert(table *t, long key, void *data, int len, int permanent);
int table_get(table *t, long key, void **data, int refresh_timeout);
int table_contains_value(table *t, long *key, void *data, int len);
int table_delete(table *t, long key);

int _table_delete(table *t, long key, int use_lock);
void _table_cleanup(table *t);

#endif
