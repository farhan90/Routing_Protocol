#include "table.h"

void table_init(table *t, long ttl) {
  t->ttl = ttl;
  t->ents = NULL;
  pthread_mutex_init(&t->mutex, NULL);
  if (t->ttl > 0) {
    pthread_create(&t->cleanup_thread, NULL, (void * (*)(void *))&_table_cleanup, t);
  }
}

void table_insert(table *t, long key, void *data, int len, int permanent) {
  table_ent *cursor;
  table_ent *ent;

  pthread_mutex_lock(&t->mutex);

  // See if already in the list
  ent = NULL;
  for (cursor = t->ents; cursor != NULL; cursor = cursor->next) {
    if (cursor->key == key) {
      ent = cursor;
      break;
    }
  }

  // If not already in the list
  if (ent == NULL) {
    // Make a new entry
    ent = malloc(sizeof(table_ent));
    ent->data = NULL;
    ent->next = NULL;

    // Put in the list
    if (t->ents == NULL) {
      t->ents = ent;
    } else {
      cursor = t->ents;
      while (cursor->next != NULL) {
        cursor = cursor->next;
      }
      cursor->next = ent;
    }
  }

  // If there is already data here free it
  if (ent->data != NULL) {
    free(ent->data);
  }

  // Update data in ent
  ent->key = key;
  ent->data = data;
  ent->data_len = len;
  ent->last_access = permanent ? INT_MAX : time(NULL);

  pthread_mutex_unlock(&t->mutex);
}

int table_get(table *t, long key, void **data, int refresh_timeout) {
  table_ent *cursor;
  table_ent *ent;
  time_t now;

  // Table with ttl of 0 never caches anything
  if (t->ttl <= 0) {
    return 0;
  }

  pthread_mutex_lock(&t->mutex);

  // Look for entry in list
  ent = NULL;
  for (cursor = t->ents; cursor != NULL; cursor = cursor->next) {
    if (cursor->key == key) {
      ent = cursor;
      break;
    }
  }

  // Entry was either not found, out of date, or found
  now = time(NULL);
  if (ent == NULL) {
    *data = NULL;
  } else if (now - ent->last_access >= t->ttl) {
    _table_delete(t, ent->key, 0);
    *data = NULL;
  } else {
    *data = ent->data;
    if (refresh_timeout && ent->last_access != INT_MAX) {
      ent->last_access = now;
    }
  }

  pthread_mutex_unlock(&t->mutex);

  return *data != NULL;
}

int table_contains_value(table *t, long *key, void *data, int len) {
  table_ent *cursor;
  int found;

  pthread_mutex_lock(&t->mutex);

  found = 0;
  for (cursor = t->ents; cursor != NULL; cursor = cursor->next) {
    if (cursor->data_len == len && memcmp(data, cursor->data, len) == 0) {
      *key = cursor->key;
      found = 1;
      break;
    }
  }

  pthread_mutex_unlock(&t->mutex);

  return found;
}

int _table_delete(table *t, long key, int use_lock) {
  table_ent *cursor;
  table_ent *prev;
  int found;

  if (use_lock) {
    pthread_mutex_lock(&t->mutex);
  }

  prev = NULL;
  found = 0;
  for (cursor = t->ents; cursor != NULL; cursor = cursor->next) {
    if (cursor->key == key) {
      if (prev == NULL) {
        t->ents = cursor->next;
      } else {
        prev->next = cursor->next;
      }
      free(cursor->data);
      free(cursor);
      found = 1;
      break;
    }
    prev = cursor;
  }

  if (use_lock) {
    pthread_mutex_unlock(&t->mutex);
  }

  return found;
}

int table_delete(table *t, long key) {
  return _table_delete(t, key, 1);
}

void _table_cleanup(table *t) {
  table_ent *cursor;
  table_ent *prev;
  table_ent *to_delete;

  // Note: this thread won't exist if ttl <= 0
  while (1) {
    sleep(t->ttl);

    pthread_mutex_lock(&t->mutex);

    printf("table cleaning daemon waking up\n");

    time_t now = time(NULL);

    prev = NULL;
    cursor = t->ents;
    while (cursor != NULL) {
      to_delete = NULL;
      if (now - cursor->last_access >= t->ttl) {
        if (prev == NULL) {
          t->ents = cursor->next;
        } else {
          prev->next = cursor->next;
        }
        to_delete = cursor;
        printf("removing entry with key %ld\n", to_delete->key);

        cursor = cursor->next;
        free(to_delete->data);
        free(to_delete);
      }
      if (to_delete == NULL) {
        prev = cursor;
        cursor = cursor->next;
      }
    }

    printf("table cleaning daemon going back to sleep\n");

    pthread_mutex_unlock(&t->mutex);
  }
}
