#include "odrether.h"


int send_frame_dgram(int fd,char *dst,int if_index,void*message,int msglen){
  int bytes;
  struct sockaddr_ll addr;
  bzero(&addr,sizeof(addr));

  addr.sll_family=AF_PACKET;
  addr.sll_protocol=htons(NET_PROTO);
  addr.sll_ifindex=if_index;
  addr.sll_addr[0]=dst[0];
  addr.sll_addr[1]=dst[1];
  addr.sll_addr[2]=dst[2];
  addr.sll_addr[3]=dst[3];
  addr.sll_addr[4]=dst[4];
  addr.sll_addr[5]=dst[5];
  addr.sll_addr[6]=0x00;
  addr.sll_addr[7]=0x00;
  addr.sll_halen=6;
  addr.sll_hatype=1;
  //addr.sll_pkttype=PACKET_BROADCAST;
  bytes = sendto (fd,message,msglen, 0, (struct sockaddr *) &addr, sizeof(addr));
  if(bytes<=0){
    perror("sendto failed:");
    printf("The error no is %d\n",errno);
  }
  return bytes;
}


int recv_frame_dgram(int fd,char *recv,void *output,int *outlen){
  
  int bytes;
  void *buff=malloc(*outlen);
  struct sockaddr_ll recv_addr;
  socklen_t recv_len=sizeof(struct sockaddr_ll);
  bytes=recvfrom(fd,buff,*outlen,0,(struct sockaddr*)&recv_addr,&recv_len);
  if(bytes<=0){
    perror("Recvfrom failed");
    printf("The errno is %d\n",errno);
  }
  memcpy(recv,(char*)recv_addr.sll_addr,6);
  memcpy(output,buff,*outlen);
  free(buff);
  return bytes;
} 



int send_frame(int fd, char *src, char *dst,int if_index,void *message,int msglen){
  
  int bytes;
  struct sockaddr_ll addr;
  char *ether_frame;
  ether_frame=malloc(MAX_ETH_FRAME);// For now only testing with 60 bytes payload
  memset(ether_frame,0,MAX_ETH_FRAME);

  bzero(&addr,sizeof(addr));

  addr.sll_family=AF_PACKET;
  addr.sll_protocol=htons(NET_PROTO);
  addr.sll_ifindex=if_index;
  addr.sll_addr[0]=dst[0];
  addr.sll_addr[1]=dst[1];
  addr.sll_addr[2]=dst[2];
  addr.sll_addr[3]=dst[3];
  addr.sll_addr[4]=dst[4];
  addr.sll_addr[5]=dst[5];
  addr.sll_addr[6]=0x00;
  addr.sll_addr[7]=0x00;
  addr.sll_halen=6;
  addr.sll_hatype=1;
  
  memcpy(ether_frame,dst,6);
  memcpy(ether_frame+6,src,6);
  ether_frame[12]=NET_PROTO/256;
  ether_frame[13]=NET_PROTO%256;
  
  memcpy(ether_frame+ETH_LEN,message,msglen);
  bytes = sendto (fd,ether_frame,MAX_ETH_FRAME, 0, (struct sockaddr *) &addr, sizeof (addr));
  
  free(ether_frame);
  return bytes;
}

int recv_frame(int fd,char *recv, void *output,int *outlen){
  int len;
  struct sockaddr_ll recv_addr;
  socklen_t recv_len=sizeof(struct sockaddr_ll);
  char *buff=malloc(MAX_ETH_FRAME);
  
  len=recvfrom(fd,buff,sizeof(buff),MSG_TRUNC,(struct sockaddr*)&recv_addr,&recv_len);
  memcpy(recv,(char*)recv_addr.sll_addr,6);
  memcpy(output,buff,*outlen);
  free(buff);
  return len;

}
