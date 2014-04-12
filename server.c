#include "server.h"

void vm_to_ip(char *vm,char *res);

int main(int argc, char **argv) {
  uds socket;
  char recv_buff[UDS_MESSAGE_MAX];
  char recv_from_ip[IP_ADDR_MAX];
  int recv_from_port;
  time_t timestamp;
  char *timestr;

  char hostname[1024];
  char recv_from_hostname[1024];
  char my_ip[1024];
  struct sockaddr_in cliaddr;
  struct hostent *client;

  socket = uds_create(SERVER_PATH);

  gethostname(hostname, 1024);

  //get_my_ip(my_ip);
  vm_to_ip(hostname,my_ip);

  printf("server listening on uds path %s\n", SERVER_PATH);

  while (1) {
    // Read a message
    msg_recv(socket.fd, recv_buff, recv_from_ip, &recv_from_port);
    
    // Figure out which vm name it came from
    if (strcmp(recv_from_ip, my_ip) == 0) {
      strcpy(recv_from_hostname, hostname);
    } else {
      inet_pton(AF_INET, recv_from_ip, &cliaddr.sin_addr);
      client = gethostbyaddr(&cliaddr.sin_addr, sizeof(cliaddr.sin_addr), AF_INET);
      strcpy(recv_from_hostname, client->h_name);
    }
    
    // Print the message with vm names
    printf("server at node %s responding to request from node %s\n", hostname, recv_from_hostname);

    // Create the timestamp
    timestamp = time(NULL);
    timestr = ctime(&timestamp);

    // Send the timestamp
    msg_send(socket.fd, recv_from_ip, recv_from_port, timestr, 0);
  }

  return 0;
}

void vm_to_ip(char *vm,char *res){
  struct hostent *hst;
  struct in_addr **addr;
  hst=gethostbyname(vm);
  if(hst==NULL){
    printf("Error resolving vm name to ip\n");
    res=NULL;
    return ;
  }

 addr=(struct in_addr**)hst->h_addr_list;
 strcpy(res,inet_ntoa(*addr[0]));

}
