#include "odr.h"
#include "pfsocket.h"

void print_uds_message(uds_message *message) {
  printf("odr received message:\n");
  printf("\tip_addr:    %s\n", message->ip_addr);
  printf("\tport:       %d\n", message->port);
  printf("\tmessage:    %s\n", message->message);
  printf("\trediscover: %d\n", message->rediscover);
}

void print_addr(char *addr){

  int  prflag = 0;
  int j = 0;
  do {
    if (addr[j] != '\0') {
      prflag = 1;
      break;
    }
  } while (++j < IF_HADDR);

  if(prflag){
    printf("HW addr = ");
    char *ptr = addr;
    j = IF_HADDR;
    do {
      printf("%.2x%s", *ptr++ & 0xff, (j == 1) ? " " : ":");
    } while (--j > 0);
  }
  printf("\n");

}


void select_loop(uds socket,if_info*aList){
  int size=get_num_interfaces();
  int i=0;
  fd_set fset;
  int max_fd;
  int ufd=socket.fd;
  struct sockaddr_ll addr;
  bzero(&addr,sizeof(addr));
  char recv_from[PATH_MAX];
  uds_message message;
  //char *ether_frame;
  //int frame_length=6+6+2+60;
  //ether_frame=malloc(frame_length);// For now only testing with 60 bytes payload
  //memset(ether_frame,0,frame_length);
  char dest_mac[6] ={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  short proto=NET_PROTO;
  char *test="Hello wold!";
  int bytes=0;
  struct timeval timeout;
  timeout.tv_sec=5;
  int ready;
  while(1){
    FD_ZERO(&fset);
    FD_SET(ufd,&fset);
    max_fd=ufd;
    for(i=0;i<size;i++){
      FD_SET(aList[i].fd,&fset);
      printf("The fd to be set is %d\n",aList[i].fd);
      max_fd=max(max_fd,aList[i].fd);
    }
    if((ready=select(max_fd+1,&fset,NULL,NULL,&timeout))>0){
	printf("I have a message\n");
	if(FD_ISSET(ufd,&fset)){
	  uds_recv(socket.fd, recv_from, (char *)&message, sizeof(message));              
	  print_uds_message(&message);
	  addr.sll_family=AF_PACKET;
	  addr.sll_protocol=htons(proto);
	  addr.sll_ifindex=aList[0].index;
	  addr.sll_addr[0]=dest_mac[0];
	  addr.sll_addr[1]=dest_mac[1];
	  addr.sll_addr[2]=dest_mac[2];
	  addr.sll_addr[3]=dest_mac[3];
	  addr.sll_addr[4]=dest_mac[4];
	  addr.sll_addr[5]=dest_mac[5];
	  addr.sll_addr[6]=0x00;
	  addr.sll_addr[7]=0x00;
	  addr.sll_halen=6;
	  addr.sll_hatype=1;
	  addr.sll_pkttype=PACKET_BROADCAST;
	  //memcpy(ether_frame,dest_mac,6);
	  print_addr(dest_mac);
	  print_addr(aList[0].hw_addr);
	  //memcpy(ether_frame+6,aList[0].hw_addr,6);
	  //memcpy(ether_frame+12,&proto,2);
	  //memcpy(ether_frame+14,test,sizeof(test));
	  if ((bytes = sendto (aList[0].fd,test,sizeof(test), 0, (struct sockaddr *) &addr, sizeof (addr))) <= 0) {
	    perror ("sendto() failed");
	    exit (1);
	  }
	  printf("the bytes sent is %d\n",bytes);
	}
	else{
	  printf("In else condition\n");
	  for(i=0;i<size;i++){
	    printf("the value of i is %d\n",i);
	    if(FD_ISSET(aList[i].fd,&fset)){
	      printf("The fd set is %d\n",aList[i].fd);
	      struct sockaddr_ll recv_addr;
	      socklen_t recv_len=sizeof(struct sockaddr_ll);
	      char buff[74];
	      if(recvfrom(aList[i].fd,buff,sizeof(buff),0,(struct sockaddr*)&recv_addr,&recv_len)>0){
		printf("The message received is %s\n",buff);
	      }
	      else{
		printf("No message received\n");
	      }
	    }
	  }
	      
	}
    }
    else if(ready==0){
      printf("the select timed out\n");
      if ((bytes = sendto (aList[0].fd, test, sizeof(test), 0, (struct sockaddr *) &addr, sizeof (addr))) <= 0) {
            perror ("sendto() failed");
          }
          printf("the bytes sent is %d\n",bytes);
	  sleep(1);

      }
    else if(ready==-1){
      perror("Select error\n");
      printf("The error no is %d\n",errno);
      exit(1);
    }
  }     	      
    
}

      

int main(int argc, char **argv) {
  uds socket;
  //uds_message message;
  // char recv_from[PATH_MAX];

  socket = uds_create(ODR_PATH);

  printf("The odr is running\n");
  /*
  while (1) {
    uds_recv(socket.fd, recv_from, (char *)&message, sizeof(message));
    print_uds_message(&message);

    // TODO(mark): Look up port number to file name mapping here instead of always sending\
 to server                                                                                 
   if (strcmp(recv_from, SERVER_PATH) != 0) {
     uds_send(socket.fd, SERVER_PATH, (char *)&message, sizeof(message));
   }
   }*/
  
  if_info* if_list=init();

  select_loop(socket,if_list);
  return 0;
}


