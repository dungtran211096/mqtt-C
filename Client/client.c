/**
Fullname: Do Xuan Quy
MSV: 14020634
Description : Client
Use : ./client <IP_SERVER>
*/
#include <stdio.h>
#include "stdlib.h"
#include "sys/socket.h"
#include "string.h"
#include "sys/types.h"
#include "netinet/in.h" // cac struct socket
#include "unistd.h"
#include "arpa/inet.h"
#include "pthread.h"
//************////

// client co 2 luong, luong gui va nhan tin nhan
void *send_thread_func();
void *recv_thread_func();

// main
int main(int argc, char const *argv[])
{
	int socket_desc = 0;
	struct sockaddr_in servaddr;
	socklen_t slen = sizeof(servaddr);
	// kiem tra cac tham so cua chuong trinh
	if (argc != 2){
		printf("No IP server provide\n");
		printf("%s\n", "Use: ./client <IP server>" );
		return 1;
	}
	// tao socket udp
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	// cau hinh cac tham so cua server address
	memset(&servaddr, '0', sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(8888);
	// chuyen doi agument 1 sang dia chi binary va ghi vao servaddr.sin_addr
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0){
		printf("%s\n", "inet_pton error occured");
		return 1;
	}
	// ket noi den server
	
	if (connect(socket_desc, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
		printf("%s\n", "Error in connect");
		return 1;
	}
	else { 
		printf("Connected to server %s:%d\n", inet_ntoa(*(struct in_addr*)&servaddr.sin_addr) , 8888);
	}
	
	// nhap username va gui cho server 
	char username[256];
	printf("%s\n", "Nhap username:");
	fgets(username, sizeof(username), stdin);
	// write(socket_desc, username , sizeof(username));
	sendto(socket_desc, username, sizeof(username), 0,  (struct sockaddr*)&servaddr, slen);
	// nhan lai list user hien tai
	char list_users[1000];
	// read(socket_desc, list_users, sizeof(list_users));
	recvfrom(socket_desc, list_users, sizeof(list_users), 0, (struct sockaddr *) &servaddr, &slen );
	printf("List current users is : %s\n", list_users );
	char send_message[256];
	char recv_message[256];
	//tao thread nhan message va thread viet message
	pthread_t send_thread_id, recv_thread_id;
	if( pthread_create( &send_thread_id , NULL ,  send_thread_func , (void*) &servaddr) < 0 
	 || pthread_create( &recv_thread_id , NULL ,  recv_thread_func , (void*) &servaddr) < 0 )
    	{
      	perror("could not create thread");
      	return 1;
      }
      // doi thread send ket thuc thi ket thuc chuong trinh
      pthread_join(send_thread_id, NULL);
      // pthread_join(recv_thread_id, NULL);
	close(socket_desc);
	return 0;
}

void *send_thread_func(void *saddr)
{
	struct sockaddr servaddr;
	servaddr = *(struct sockaddr*) saddr;
	char send_message[256];
	while(1)
	{	
		printf("%s", "Enter message: ");
		fgets(send_message, sizeof(send_message), stdin);
		send_message[strlen(send_message) - 1] = '\0';
		// write(sock, send_message , sizeof(send_message));
		sendto(socket_desc, send_message, sizeof(send_message), 0,  (struct sockaddr*)servaddr, sizeof(*servaddr));
		if ( strcmp(send_message, "@") == 0 ) {
			printf("%s\n", "End connection");
			break;
		}
	}
	return 0 ;
}

void *recv_thread_func(void *saddr)
{
	struct sockaddr servaddr;
	servaddr = *(struct sockaddr*) saddr;
	while(1){
		char recv_message[256];
		int n = recvfrom(socket_desc, recv_message, sizeof(recv_message), 0, (struct sockaddr *)servaddr, & sizeof(*servaddr) );
		if (n < 256 ) recv_message[n] = '\0';
		printf("Thong bao: %s\n", recv_message);
	}
}