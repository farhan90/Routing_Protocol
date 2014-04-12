#include "pfsocket.h"

if_info *list;
int num_if;
char ip_addr[INET_ADDRSTRLEN];

int get_num_interfaces(){
  num_if=0;
  struct hwa_info *hwa,*hwahead;
  for(hwahead=hwa=Get_hw_addrs();hwa!=NULL;hwa=hwa->hwa_next)
    num_if++;

  num_if=num_if-2;
  free_hwa_info(hwahead);
  return num_if;
}


void  setup_list(){
  
  get_num_interfaces();
  struct hwa_info *hwa, *hwahead;
  int i=0;
  int j=0;
  struct sockaddr *sa;
  list=(if_info*)malloc(num_if*sizeof(if_info));
  for(hwahead=hwa=Get_hw_addrs();hwa!=NULL;hwa=hwa->hwa_next){
     if(strcmp(hwa->if_name,"lo")!=0){
       if(strcmp(hwa->if_name,"eth0")==0){
	 if((sa=hwa->ip_addr)!=NULL){
	 char *temp=Sock_ntop_host(sa,sizeof(*sa));
	 strncpy(ip_addr,temp,INET_ADDRSTRLEN);
	 continue;
	 }
       }
      list[i].index=hwa->if_index;
       if((sa=hwa->ip_addr)!=NULL){
	  char *temp=Sock_ntop_host(sa,sizeof(*sa));
	strncpy(list[i].ip_addr,temp,INET_ADDRSTRLEN);
      }
      for(j=0;j<IF_HADDR;j++){
	list[i].hw_addr[j]=hwa->if_haddr[j];
      }
      i++;
     }
     
  }
  free_hwa_info(hwahead);
}

void setup_fd(){
  int i=0;
  int fd;
  struct sockaddr_ll addr;
  short proto=NET_PROTO;
  for(i=0;i<num_if;i++){
   if((fd=socket(AF_PACKET,SOCK_DGRAM,htons(proto)))==-1){
      printf("The errno is %d\n",errno);
      err_sys("Socket error");
    }

    bzero(&addr,sizeof(addr));
    addr.sll_family=AF_PACKET;
    addr.sll_protocol=htons(proto);
    addr.sll_ifindex=list[i].index;

    if(bind(fd,(SA*)&addr,sizeof(addr))==-1){
      err_sys("Bind error");
    }

    list[i].fd=fd;
  }

}

void print_list(){
  int i=0;

  for(i=0;i<num_if;i++){
    printf("The ip addr is %s\n",list[i].ip_addr);
    printf("The fd bound to it is %d\n",list[i].fd);
    printf("The index bound to is %d\n",list[i].index);
    int  prflag = 0;
    int j = 0;
     do {
         if (list[i].hw_addr[j] != '\0') {
	 prflag = 1;
	 break;
	 }
      } while (++j < IF_HADDR);

     if(prflag){
       printf("HW addr = ");
       char *ptr = list[i].hw_addr;
       j = IF_HADDR;
       do {
	 printf("%.2x%s", *ptr++ & 0xff, (j == 1) ? " " : ":");
       } while (--j > 0);
     }
     printf("\n");
  }

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


if_info * init (){

  setup_list();
  setup_fd();
  print_list();
  return list;
}

 void get_my_ip(char *ptr){
   strncpy(ptr,ip_addr,INET_ADDRSTRLEN);
 }
