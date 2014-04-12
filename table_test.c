#include "table.h"

int main() {
  table t;

  // Make a table with a ttl of 3 seconds
  table_init(&t, 3);

  // Sleep for 1 second
  sleep(1);

  // The data to store in the table (could be any type)
  // NOTE: All data that goes in the table should be malloced since the table will auto
  //       free any data when it comes time to take it out of the table
  char *tmp_filename = malloc(sizeof(char) * 32);
  strcpy(tmp_filename, "/tmp/blahblahblah");

  // The key to map to the data (needs to be casted to a long before adding)
  int port_number = 32123;

  // Put 32123 -> "/tmp/blahblahblah" mapping into the table, 0 means it is NOT a permanent entry
  table_insert(&t, (long)port_number, tmp_filename, strlen(tmp_filename), 0);

  // Get the element in the table that has a key of 32123 (returns 0 when not found)
  char *p;
  if (table_get(&t, (long)port_number, (void **)&p, 0)) {
    // Print the value
    printf("data was found %s\n", p);
  }

  // Sleep for 4 seconds
  sleep(4);

  // Now since the value has timed out, the return value is 0
  if (table_get(&t, (long)port_number, (void **)&p, 0)) {
    // This won't happen since it is now stale
    printf("data was found %s\n", p);
  } else {
    // This will happen
    printf("data was stale\n");
  }

  return 0;
}
