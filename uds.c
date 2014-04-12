#include "uds.h"

int msg_send(int fd, char *ip_addr, int port, char *message, int rediscover) {
  uds_message omessage;

  strcpy(omessage.dest_ip_addr, ip_addr);
  omessage.dest_port = port;
  strcpy(omessage.message, message);
  omessage.rediscover = rediscover;

  return uds_send(fd, ODR_PATH, (char *)&omessage, sizeof(omessage));
}

int msg_recv(int fd, char *message, char *ip_addr, int *port) {
  uds_message omessage;
  char recv_from[PATH_MAX];
  int rv;

  rv = uds_recv(fd, recv_from, (char *)&omessage, sizeof(omessage));

  strcpy(message, omessage.message);
  
  strcpy(ip_addr, omessage.src_ip_addr);
  *port = omessage.src_port;

  return rv;
}

uds uds_create(char *path) {
  struct sockaddr_un uds_sock;
  uds uds_info;
  int tmp_fd;
  int fd;

  if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
    perror("unix domain socket error");
  }

  uds_info.fd = fd;

  if (path == NULL) {
    strcpy(uds_info.path, TEMP_FORMAT);
    tmp_fd = mkstemp(uds_info.path);
    printf("info: generated temp file named %s\n", uds_info.path);
    if (close(tmp_fd) == -1) {
      perror("temp file close error");
    }
  } else {
    strcpy(uds_info.path, path);
  }

  if (unlink(uds_info.path) == -1) {
    printf("warning: unlink failed for %s\n", uds_info.path);
  }

  uds_sock.sun_family = AF_UNIX;
  strcpy(uds_sock.sun_path, uds_info.path);

  if (bind(fd, (struct sockaddr *)&uds_sock, sizeof(uds_sock)) == -1) {
    perror("bind error");
  }

  return uds_info;
}

int uds_send(int fd, char *dest_path, char *message, int len) {
  struct sockaddr_un uds_sock;

  uds_sock.sun_family = AF_UNIX;
  strcpy(uds_sock.sun_path, dest_path);

  return sendto(fd, message, len, 0, (struct sockaddr *)&uds_sock, sizeof(uds_sock));
}

int uds_recv(int fd, char *recv_from, char *message, int len) {
  struct sockaddr_un uds_sock;
  socklen_t sock_len;
  int rv;

  sock_len = sizeof(uds_sock);
  rv = recvfrom(fd, message, len, 0, (struct sockaddr *)&uds_sock, &sock_len);

  strcpy(recv_from, uds_sock.sun_path);

  return rv;
}

int uds_destroy(uds *uds_info) {
  int rv;
  if ((rv = unlink(uds_info->path)) == -1) {
    printf("warning: unlink failed for %s\n", uds_info->path);
  }
  return rv;
}
