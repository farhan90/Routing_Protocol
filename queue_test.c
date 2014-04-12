#include "queue.h"

odr_app_message *make_app_msg(char *src_addr, char *dest_addr) {
  odr_app_message *msg = malloc(sizeof(odr_app_message));
  strcpy(msg->header.src_addr, src_addr);
  strcpy(msg->header.dest_addr, dest_addr);
  msg->header.type = APP_TYPE;
  return msg;
}

odr_route_message *make_rreq_msg(char *src_addr, char *dest_addr) {
  odr_route_message *msg = malloc(sizeof(odr_route_message));
  strcpy(msg->header.src_addr, src_addr);
  strcpy(msg->header.dest_addr, dest_addr);
  msg->header.type = RREQ_TYPE; 
  return msg;
}

odr_route_message *make_rrep_msg(char *src_addr, char *dest_addr) {
  odr_route_message *msg = malloc(sizeof(odr_route_message));
  strcpy(msg->header.src_addr, src_addr);
  strcpy(msg->header.dest_addr, dest_addr);
  msg->header.type = RREP_TYPE; 
  return msg;
}

void sendfn(int type, void *message) {
  printf("Should send message with type %d\n", type);
}

int main() {
  qnode *queue = NULL;

  queue_add(&queue, make_app_msg("127.0.0.1", "192.168.1.123"));
  queue_add(&queue, make_rreq_msg("127.0.0.1", "192.168.1.123"));
  queue_add(&queue, make_rrep_msg("127.0.0.1", "192.168.1.123"));

  printf("Sending to 192.168.1.123\n");
  queue_send_to(&queue, "192.168.1.123", &sendfn);

  queue_add(&queue, make_app_msg("127.0.0.1", "192.168.1.124"));
  queue_add(&queue, make_rreq_msg("127.0.0.1", "192.168.1.125"));
  queue_add(&queue, make_rrep_msg("127.0.0.1", "192.168.1.126"));  

  printf("Sending to 192.168.1.125\n");
  queue_send_to(&queue, "192.168.1.125", &sendfn);

  queue_add(&queue, make_app_msg("127.0.0.1", "192.168.1.126"));
  queue_add(&queue, make_rreq_msg("127.0.0.1", "192.168.1.126"));
  queue_add(&queue, make_rrep_msg("127.0.0.1", "192.168.1.126"));  

  printf("Sending to 192.168.1.126\n");
  queue_send_to(&queue, "192.168.1.126", &sendfn);

  printf("Sending to 192.168.1.124\n");
  queue_send_to(&queue, "192.168.1.124", &sendfn);

  queue_send_to(&queue, "192.168.1.1", &sendfn);

  return 0;
}
