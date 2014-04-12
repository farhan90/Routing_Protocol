#include "odrether.h"
#include "odr.h"

void print_uds_message(char *heading, uds_message *message) {
  printf("\n%s:\n", heading);
  printf("\tsrc_ip_addr:    %s\n", message->src_ip_addr);
  printf("\tsrc_port:       %d\n", message->src_port);
  printf("\tdest_ip_addr:   %s\n", message->dest_ip_addr);
  printf("\tdest_port:      %d\n", message->dest_port);
  printf("\tmessage:        %s\n", message->message);
  printf("\trediscover:     %d\n", message->rediscover);
}



void select_loop(uds socket, if_info*aList){

  int size=get_num_interfaces();
  int i=0;
  fd_set fset;
  int max_fd;
  int ufd=socket.fd;
  uds_message message;
  char recv_from[PATH_MAX];
  char dst_mac[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  char *msg="Hello World!\0";
  int bytes;
  //struct timeval timeout;
  //timeout.tv_sec=5;
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
    if((ready=select(max_fd+1,&fset,NULL,NULL,NULL))>0){
      printf("I have a message\n");
      if(FD_ISSET(ufd,&fset)){
		uds_recv(socket.fd, recv_from, (char *)&message, sizeof(message));
		print_uds_message("message rcvd was", &message);
		bytes=send_frame_dgram(aList[0].fd,dst_mac,aList[0].index,(char*)msg,14);
		if(bytes<=0){
			printf("Send frame error\n");
			exit(1);
		}
		printf("The bytes sent is %d\n",bytes);
      }

	  else{
		for(i=0;i<size;i++){
			if(FD_ISSET(aList[i].fd,&fset)){
				char src_addr[6];
				char output[14];
				int len=14;
				bytes=recv_frame_dgram(aList[i].fd,(char*)src_addr,(char*)output,&len);
				if(bytes<=0){
					printf("Recv frame failed\n");
					exit(1);
				}
				printf("The message received is %s\n",output);
				printf("From address\n");
				print_addr(src_addr);
            }
		}
	}
   }
	
    else if(ready==-1){
      perror("Select error\n");
      printf("The error no is %d\n",errno);
      
    }
  }
}



int main(int argc, char **argv) {
  uds socket;
  //uds_message message;                                                                                                                                                                       
  // char recv_from[PATH_MAX];                                                                                                                                                                 

  socket = uds_create(ODR_PATH);

  printf("The odr is running\n");

  if_info* if_list=init();

  select_loop(socket,if_list);
  return 0;
}
