#include "odr.h"

//-----Global-Vars-----//

table port_table;
uds uds_socket;
int next_port;

table routing_table;
int route_ttl;
qnode *message_queue = NULL;
int next_bid = 0;

table bid_table;

if_info *if_list;
int num_interfaces;
char my_ip[INET_ADDRSTRLEN];

//-----Common-Stuff-----//

void on_sigint(int signo) {
  close_odr_uds_socket();
  exit(1);
}

void listen_loop() {
  fd_set readset;
  int maxfd;
  int nready;
  int i;

  // Get max fd for select
  maxfd = uds_socket.fd;
  for (i = 0; i < num_interfaces; i++) {
    maxfd = max(maxfd, if_list[i].fd);
  }

  while (1) {
    FD_ZERO(&readset);
    FD_SET(uds_socket.fd, &readset);
    for (i = 0; i < num_interfaces; i++) {
      FD_SET(if_list[i].fd, &readset);
    }

    nready = select(maxfd + 1, &readset, NULL, NULL, NULL);
    if (nready > 0) {
      if (FD_ISSET(uds_socket.fd, &readset)) {
        handle_uds_message();
      }
      for (i = 0; i < num_interfaces; i++) {
        if (FD_ISSET(if_list[i].fd, &readset)) {
          handle_odr_message(&if_list[i]);
        }
      }
    } else if (nready == 1 && errno == EINTR) {
      continue;
    } else {
      perror("select error");
    }
  }
}

//-----UDS-Layer-Stuff-----//

void print_uds_message(char *heading, uds_message *message) {
  printf("\n%s:\n", heading);
  printf("\tsrc_ip_addr:    %s\n", message->src_ip_addr);
  printf("\tsrc_port:       %d\n", message->src_port);
  printf("\tdest_ip_addr:   %s\n", message->dest_ip_addr);
  printf("\tdest_port:      %d\n", message->dest_port);
  printf("\tmessage:        %s\n", message->message);
  printf("\trediscover:     %d\n", message->rediscover);
}

void open_odr_uds_socket() {
  uds_socket = uds_create(ODR_PATH);
  signal(SIGINT, &on_sigint);
}

void close_odr_uds_socket() {
  uds_destroy(&uds_socket);
}

void populate_port_table() {
  table_init(&port_table, PORT_TABLE_TTL);
  table_insert(&port_table, SERVER_PORT, SERVER_PATH, strlen(SERVER_PATH), 1);

  next_port = SERVER_PORT + 1;
}

void deliver_uds_message(uds_message *message) {
  char *dest_path;

  // Lookup the destination UDS path for this dest port
  if (table_get(&port_table, (long)message->dest_port, (void **)&dest_path, 1)) {
    // Send the message to the local application
    print_uds_message("delivering the following to an application on this node", message);
    uds_send(uds_socket.fd, dest_path, (char *)message, sizeof(uds_message));
  } else {
    print_uds_message("the application this message was inteded timed out", message);
  }
}

void handle_uds_message() {
  uds_message message;
  odr_app_message odr_message;
  char recv_from[PATH_MAX];
  int src_port;

  uds_recv(uds_socket.fd, recv_from, (char *)&message, sizeof(message));

  // Get the source port for this connection
  src_port = get_src_port(recv_from);

  // Put the source port and ip in the message
  message.src_port = src_port;
  strcpy(message.src_ip_addr, my_ip);

  print_uds_message("incoming message processed from an application on this node", &message);

  // If this is a local host message, deliver it right away
  if (strcmp(message.src_ip_addr, message.dest_ip_addr) == 0) {
    deliver_uds_message(&message);
  } else {
    fill_header(&odr_message.header, message.src_ip_addr, message.dest_ip_addr, APP_TYPE, -1,message.rediscover);
    odr_message.message = message;
    on_application_message(&odr_message, -1, NULL);
  }
}

int get_src_port(char *recv_from) {
  long src_port;
  char *recv_from_table_ent;
  int was_found;
  
  was_found = table_contains_value(&port_table, &src_port, recv_from, strlen(recv_from));
  
  if (!was_found) {
    recv_from_table_ent = malloc(strlen(recv_from) + 1);
    strcpy(recv_from_table_ent, recv_from);

    src_port = (long)next_port;

    table_insert(
      &port_table, 
      src_port, 
      recv_from_table_ent, 
      strlen(recv_from_table_ent),
      0);

    next_port++;
  }

  return (int)src_port;
}

//-----ODR-Layer-Stuff-----//

void open_packet_sockets() {
  if_list = init();
  get_my_ip(my_ip);
  num_interfaces = get_num_interfaces();
}

void fill_header(odr_header *header, char *src_addr, char *dest_addr, int type, int hop_count,int rediscover) {
  strcpy(header->src_addr, src_addr);
  strcpy(header->dest_addr, dest_addr);
  header->type = type;
  header->hop_count = hop_count;
  header->rediscover=rediscover;
}

long ip_addr_to_table_key(char *ip_addr) {
  struct sockaddr_in addr;
  
  if (!inet_pton(AF_INET, ip_addr, &addr.sin_addr)) {
    perror("error converting ip address to table key");
  }

  return (long)addr.sin_addr.s_addr;
}

void init_routing_table() {
  table_init(&routing_table, route_ttl);
}

void init_bid_table() {
  table_init(&bid_table, PORT_TABLE_TTL);
}

if_info *if_info_from_index(int index) {
  int i;
  for (i = 0; i < num_interfaces; i++) {
    if (if_list[i].index == index) {
      return &if_list[i];
    }
  }
  return NULL;
}

void on_application_message(odr_app_message *message, int iface_index, char *src_addr) {
  odr_app_message *odr_message_copy;
  odr_route_message rreq_message;
  routing_table_entry *entry;
  long table_key;

  message->header.hop_count++;

  // Determine if the destination address route is known
  table_key = ip_addr_to_table_key(message->header.dest_addr);
  if (strcmp(message->header.dest_addr, my_ip) == 0) {
    printf("application message has arrived from outside world\n");
    deliver_uds_message(&message->message);
  }
  else if (table_get(&routing_table, table_key, (void **)&entry, 1) && message->header.rediscover==0) {
    printf("application destination found in table, forwarding to next router\n");
    forward_message(
      &message->header, 
      entry->iface_index, 
      entry->dest_addr, 
      sizeof(odr_app_message));
  } else {

    int temp_rediscover=message->header.rediscover;
    if(message->header.rediscover==1){
      if(table_get(&routing_table, table_key, (void **)&entry, 1)){
	printf("deleting an entry from routing table due to force discovery\n");
        table_delete(&routing_table,table_key);
      }
      //Need to change app message header rediscover to 0 so intermediate 
      //nodes dont repeat the process when the receive app message
      message->header.rediscover=0;
    }

    printf("application destination not in table, queuing and broadcasting rreq\n");

    // Route not known add it to the queue
    odr_message_copy = malloc(sizeof(odr_app_message));
    memcpy(odr_message_copy, message, sizeof(odr_app_message));
    queue_add(&message_queue, odr_message_copy);

    // And start a broadcast
    fill_header(
      &rreq_message.header, 
      my_ip, 
      message->header.dest_addr, 
      RREQ_TYPE, 
      0,
      temp_rediscover);

    rreq_message.bid = next_bid++;
    rreq_message.should_rrep = 1;

    broadcast_rreq(&rreq_message, iface_index);
  }

  consider_adding_to_table(
    message->header.src_addr,
    message->header.hop_count,
    iface_index,
    src_addr,
    -1,
    NULL);
}

void on_rreq_message(odr_route_message *message, int iface_index, char *src_addr) {
  routing_table_entry *entry;
  long table_key;
  int old_bid;

  message->header.hop_count++;

  table_key = ip_addr_to_table_key(message->header.dest_addr);
  if (strcmp(message->header.dest_addr, my_ip) == 0) {
    printf("rreq with bid %d has reached its destination, ", message->bid);
    if (message->should_rrep 
        && highest_bid_from_src_addr(message->header.src_addr) < message->bid) {
      printf("sending rrep\n");
      send_rrep(message->header.dest_addr, message->header.src_addr, iface_index, src_addr,message->header.rediscover);
      message->should_rrep = 0;
    } else {
      printf("not sending rrep, should_rrep == 0 or already sent rrep for this rreq\n");
    }
  } else if (table_get(&routing_table, table_key, (void **)&entry, 1)&& message->header.rediscover==0) {
    printf("rreq request was found in table, ");
    if (message->should_rrep) {
      printf("sending rrep\n");
      send_rrep(message->header.dest_addr, message->header.src_addr, iface_index, src_addr,0);
      message->should_rrep = 0;
    } else {
      printf("not sending rrep, should_rrep == 0\n");
    }
  } else {
    if(message->header.rediscover==1){
      long temp_key=ip_addr_to_table_key(message->header.src_addr);
      //look for the for the src addr of rreq message in table
      if(table_get(&routing_table, temp_key, (void **)&entry, 1)){
        printf("rreq request src addr was found in table but forced to update\n");
        table_delete(&routing_table,temp_key);
      }
    }
  }

  if (route_ttl == 0 && (message->should_rrep == 0 || highest_bid_from_src_addr(message->header.src_addr) >= message->bid)) {
    return;
  }

  if (consider_adding_to_table(
        message->header.src_addr, 
        message->header.hop_count, 
        iface_index,
        src_addr,
        message->bid, 
        &old_bid)) {
    if (old_bid != message->bid) {
      broadcast_rreq(message, iface_index);
    }
  }
}

void on_rrep_message(odr_route_message *message, int iface_index, char *src_addr) {
  odr_route_message *odr_message_copy;
  odr_route_message rreq_message;
  routing_table_entry *entry;
  long table_key;

  message->header.hop_count++;

  table_key = ip_addr_to_table_key(message->header.dest_addr);
  if (strcmp(message->header.dest_addr, my_ip) == 0) {
    queue_send_to(
      &message_queue, 
      message->header.src_addr, 
      src_addr, 
      iface_index,
      &send_queued_message);
  } else if (table_get(&routing_table, table_key, (void **)&entry, 1)) {
    printf("rrep destination was found in table, forwarding\n");
    forward_message(
      &message->header, 
      entry->iface_index, 
      entry->dest_addr, 
      sizeof(odr_route_message));
    //delete old rrep entry if rediscover=1;
    if(message->header.rediscover==1){
      printf("rrep source has been deleted due to rediscovery\n");
      long temp_key=ip_addr_to_table_key(message->header.src_addr);
      table_delete(&routing_table,temp_key);
    }
  } else {
    printf("rrep destination was not found in table, queuing and broadcasting rreq\n");
    odr_message_copy = malloc(sizeof(odr_route_message));
    memcpy(odr_message_copy, message, sizeof(odr_route_message));
    queue_add(&message_queue, odr_message_copy);

    fill_header(
      &rreq_message.header, 
      my_ip, 
      message->header.dest_addr, 
      RREQ_TYPE, 
      0,
      0);

    rreq_message.bid = next_bid++;
    rreq_message.should_rrep = 1;

    broadcast_rreq(&rreq_message, iface_index);
  }

  consider_adding_to_table(
    message->header.src_addr,
    message->header.hop_count,
    iface_index,
    src_addr,
    -1,
    NULL);
}

int highest_bid_from_src_addr(char *src_addr) {
  int *p;
  long table_key;

  table_key = ip_addr_to_table_key(src_addr);
  if (table_get(&bid_table, table_key, (void **)&p, 0)) {
    return *p;
  }
  return -1;
}

int consider_adding_to_table(
    char *dest_addr, int hop_count, int iface_index, char *dest_hw_addr, int bid, int *old_bid) {
  routing_table_entry *entry;
  long table_key;
  int found;
  int *p;
  int i;
  char myaddr[1024];
  char dest_char[1024];
  gethostname(myaddr,1024);
  ip_to_vm(dest_addr,myaddr,dest_char);

  if (hop_count <= 0 || dest_hw_addr == NULL || iface_index < 0) {
    return 0;
  }

  table_key = ip_addr_to_table_key(dest_addr);

  if (bid != -1) {
    p = malloc(sizeof(int));
    *p = bid;
    table_insert(&bid_table, table_key, p, sizeof(int), 0);
  }

  if (route_ttl <= 0) {
    return 1;
  }

  found = table_get(&routing_table, table_key, (void **)&entry, 1);

  if (found && old_bid != NULL) {
    printf("The route IP with destinatinon address %s found in table\n",dest_char);
    *old_bid = entry->bid;
  }

  if (!found || entry->hop_count >= hop_count) {
    entry = malloc(sizeof(entry));
    entry->hop_count = hop_count;
    entry->iface_index = iface_index;
    entry->bid = bid;
    for (i = 0; i < 6; i++) {
      entry->dest_addr[i] = dest_hw_addr[i];
    }
    if(!found)
      printf("The route IP with destination address %s not found in table, adding to the table\n",dest_char);
    else
      printf("The old entry with hop count %d is being replaced with new hop count %d\n",entry->hop_count,hop_count);
    table_insert(&routing_table, table_key, entry, sizeof(routing_table_entry), 0);

    return 1;
  }

  return 0;
}

void broadcast_rreq(odr_route_message *message, int except_index) {
  char broadcast[6];
  int i;

  for (i = 0; i < 6; i++) {
    broadcast[i] = 0xFF;
  }

  for (i = 0; i < num_interfaces; i++) {
    if (except_index != if_list[i].index) {
      forward_message(&message->header, if_list[i].index, broadcast, sizeof(odr_route_message));
    }
  }
}

void send_rrep(char *src_addr, char *dest_addr, int iface_index, char *dest_hw_addr,int rediscover) {
  odr_route_message message;

  fill_header(&message.header, src_addr, dest_addr, RREP_TYPE, 0,rediscover);
  if(rediscover==1){
    printf("Sending an rrep with rediscovery\n");
  }

  forward_message(&message.header, iface_index, dest_hw_addr, sizeof(odr_route_message));
}

void forward_message(odr_header *header, int iface_index, char *dest_hw_addr, int len) {
  if_info *iface;

  iface = if_info_from_index(iface_index);

  send_frame_dgram(
    iface->fd,
    dest_hw_addr,
    iface->index,
    header,
    len);

  print_send_info(header->src_addr,header->dest_addr,dest_hw_addr,iface_index,header->type);

}

void send_queued_message(int type, void *message, char *dest_addr, int iface_index) {
  switch (type) {
    case APP_TYPE:
      printf("sending queued application message\n");
      forward_message(message, iface_index, dest_addr, sizeof(odr_app_message));
      break;
    case RREP_TYPE:
      printf("sending queued rrep message\n");
      forward_message(message, iface_index, dest_addr, sizeof(odr_route_message));
      break;
  }
}

void handle_odr_message(if_info *iface) {
  odr_header *header;
  char buffer[MAX_ETH_FRAME];
  char src_addr[6];
  int nread;

  nread = MAX_ETH_FRAME;
  recv_frame_dgram(iface->fd, src_addr, buffer, &nread);

  header = (odr_header *)buffer;

  switch (header->type) {
    case APP_TYPE:
      on_application_message((odr_app_message *)buffer, iface->index, src_addr);
      break;
    case RREQ_TYPE:
      on_rreq_message((odr_route_message *)buffer, iface->index, src_addr);
      break;
    case RREP_TYPE:
      on_rrep_message((odr_route_message *)buffer, iface->index, src_addr);
      break;
  }
}

void print_send_info(char *src, char *dest, char *dest_hwaddr, int interface, int msg_type) {
  char myaddr[1024],src_char[1024],dst_char[1024];
 
  gethostname(myaddr,1024);

  ip_to_vm(src,myaddr,src_char);
  ip_to_vm(dest,myaddr,dst_char);

  char *type;
  switch(msg_type){
  case APP_TYPE:
    type="APP";
    break;
  case RREQ_TYPE:
    type="RREQ";
    break;
  case RREP_TYPE:
    type="RREP";
    break;
      
  }
  
  printf("ODR at node %s: sending frame hdr src %s dest iface %d ", myaddr, myaddr, interface);
  print_addr(dest_hwaddr);
  printf("ODR msg type %s src %s dest %s\n", type, src_char, dst_char);
  printf("\n");
} 

void ip_to_vm(char *ip,char *myaddr,char *res){

  struct hostent *hst;
  struct in_addr addr;
  if(strcmp(ip,my_ip)==0){
    strcpy(res,myaddr);
  }

  else{
    if(inet_pton(AF_INET,ip,&addr)<0){
      printf("Error resolving vm name for ip %s\n",ip);
      res=NULL;
      return;
    }
    hst=gethostbyaddr(&addr,sizeof(addr),AF_INET);
    if(hst==NULL){
      printf("Error getting the vm name\n");
      res=NULL;
      return;
    }
    strcpy(res,hst->h_name);
  }
}



//-----Entry-Point-----//

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("please provide a staleness argument in seconds\n");
    exit(1);
  }

  route_ttl = atoi(argv[1]);
  printf("using a staleness of %d seconds\n", route_ttl);

  open_odr_uds_socket();
  populate_port_table();

  open_packet_sockets();
  init_routing_table();
  init_bid_table();

  listen_loop();

  close_odr_uds_socket();
  return 0;
}
