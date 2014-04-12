#include "client.h"

uds uds_socket;

char hostname[1024];
char input[1024];
char dest_addr[INET_ADDRSTRLEN];
struct hostent *servent;
struct sockaddr_in servaddr;

int nready;
int ntimeouts;
struct timeval timeout;
char recv_buffer[UDS_MESSAGE_MAX];
char recv_addr[INET_ADDRSTRLEN];
int recv_port;
fd_set readset;

void on_sigint(int signo) {
  uds_destroy(&uds_socket);
  exit(1);
}

void send_loop() {
  printf("client at node %s sending request to server at %s\n", hostname, input);
  msg_send(uds_socket.fd, dest_addr, SERVER_PORT, "time?", 0);

  ntimeouts = 0;
  while (1) {
    FD_ZERO(&readset);
    FD_SET(uds_socket.fd, &readset);

    timeout.tv_sec = TIMEOUT_SECONDS;
    timeout.tv_usec = 0;

    nready = select(uds_socket.fd + 1, &readset, NULL, NULL, &timeout);
    if (nready > 0) {
      msg_recv(uds_socket.fd, recv_buffer, recv_addr, &recv_port);
      printf("client at node %s received from server at %s %s\n", hostname, input, recv_buffer);
      return;
    } else if (nready == 0) {
      printf("client at node %s timed out when sending to server %s\n", hostname, input);
      if (ntimeouts == 0) {
        printf("sending again and forcing rediscovery\n");
        msg_send(uds_socket.fd, dest_addr, SERVER_PORT, "time?", 1);
        ntimeouts++;
      } else {
        printf("second time out, giving up\n");
        return;
      }
    } else if (nready == -1 && errno == EINTR) {
      continue;
    } else {
      printf("select error\n");
    }
  }
}

void choose_vm() {
  printf("\nchoose a server vm (enter the string \"vmX\" where X is on [1, 10]): ");
  if (scanf("%s", input) < 1) {
    return;
  }
  printf("you chose %s\n", input);

  servent = gethostbyname(input);
  if (servent == NULL) {
    printf("that server's ip address wasn't found\n");
    return;
  }

  servaddr.sin_addr = *((struct in_addr *)(*(servent->h_addr_list)));
  inet_ntop(AF_INET, &servaddr.sin_addr, dest_addr, INET_ADDRSTRLEN);
  printf("the server is at ip address %s\n", dest_addr);

  send_loop();
}

int main(int argc, char **argv) {
  signal(SIGINT, &on_sigint);

  uds_socket = uds_create(NULL);

  gethostname(hostname, 1024);

  while (1) {
    choose_vm();
  }

  uds_destroy(&uds_socket);

  return 0;
}
