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

char invite_cache[256];
// client co 2 luong, luong gui va nhan tin nhan
void *send_thread_func();
void *recv_thread_func();
void sendFile(int sock, char *filename);
void recvFile(int sock, char *filename);
void rFCaTrim( char str[]);
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
	if (connect(socket_desc, (struct sockaddr *)&servaddr, slen) == -1){
		printf("%s\n", "Error in connect");
		return 1;
	}
	else { 
		printf("Connected to server %s:%d\n", inet_ntoa(*(struct in_addr*)&servaddr.sin_addr) , 8888);
		printf("%s\n", "----------------------\n");
	}
	// nhap username va gui cho server 
	char username[256];
	// char channelName[256];
	printf("%s", "Nhap username:");
	fgets(username, sizeof(username), stdin);
	//gui den server username
	write(socket_desc, username , sizeof(username));
	// nhan lai list user hien tai, va in ra man hinh
	char list_users[1000];
	read(socket_desc, list_users, sizeof(list_users));
	printf("%s\n", list_users );

	// *** nhan va gui tin nhan voi server
	//tao thread nhan message va thread viet message
	pthread_t send_thread_id, recv_thread_id;
	if( pthread_create( &send_thread_id , NULL ,  send_thread_func , (void*) &socket_desc) < 0 
	||  pthread_create( &recv_thread_id , NULL ,  recv_thread_func , (void*) &socket_desc) < 0 )
    {
      	perror("could not create thread");
      	return 1;
    }
  	// doi thread send ket thuc thi ket thuc chuong trinh
  	pthread_join(send_thread_id, NULL);
 	 // vi thread nhan message ko the tu ket thuc => huy thread
  	pthread_cancel(recv_thread_id);
	close(socket_desc);
	return 0;
}

void *send_thread_func(void *sockfd)
{
	int sock = *(int *) sockfd;
	char send_message[256];
	printf("%s\n", "\n------------------------------------------------------------------------------");
	printf("| NOTE:\t\t\t\t\t\t\t\t\t     |\n");
	printf("| > Tin nhan dau tien la dang ky channel\t\t\t\t     |\n");
	printf("| > Neu muon chat rieng: $username \t\t\t\t\t     |\n");
	printf("| > Gui file: #filename ... \t\t\t\t\t\t     |\n");
	printf("| > Dong y hoac khong dong y chat : !y hoac !n\t\t\t\t     |\n");

	printf("%s\n", "------------------------------------------------------------------------------\n");
	printf("Cuoc tro chuyen bat dau...\n\n");

	while(1)
	{	
		// nhap tin nhan
		fgets(send_message, sizeof(send_message), stdin);
		send_message[strlen(send_message) - 1] = '\0';

		// gui tin nhan den server
		if (strlen(send_message) > 0 && send_message[0] == '!'){
			char str[256];
			strcpy(str, send_message);
			rFCaTrim(str);
			if (strcmp(str, "list") != 0){
				strcat(send_message, ",");
				strcat(send_message, invite_cache);
			}
		}
		write(sock, send_message , sizeof(send_message));
		// neu user gui '@' thi  dong ket noi
		if( send_message[0] == '@') {
			printf("%s\n", "End connection");
			exit(1)	;
		}
		else if ( send_message[0] == '#' ) {
			memmove(send_message, send_message + 1, strlen(send_message));
    		send_message[strlen(send_message)] = '\0';
    		sendFile(sock, send_message);
    		continue;
		}

	}
	return 0 ;
}

void *recv_thread_func(void *sockfd)
{
	int sock = *(int *) sockfd;
	while(1){

		char recv_message[256];
		int n = read(sock, recv_message , sizeof(recv_message));
		if (n < 256 ) recv_message[n] = '\0';

		if (strlen(recv_message) > 0 && recv_message[0] == '#') {
			printf("File session ... \n");
			memmove(recv_message, recv_message + 1, strlen(recv_message));
			recv_message[strlen(recv_message)] = '\0';
			char *search = ",";
			char *sender = strtok(recv_message, search);
			char *filename = strtok(NULL, search);
			printf("User %s send file %s to you\n", sender, filename );
			// nhan file tu server 
			recvFile(sock, filename);
			printf("recvFile is done !!!\n");
			continue;
		}

		if ( strlen(recv_message) > 0 && recv_message[0] == '!' ) {
			printf("An invite message ... \n");
			memmove(recv_message, recv_message + 1, strlen(recv_message));
			recv_message[strlen(recv_message)] = '\0';
			strcpy(invite_cache, recv_message );
			printf("Set invite_cache = '%s'\n", invite_cache );
			printf("User %s invite you chat with him , accept ? [y/n]\n", recv_message);
			continue;
		}
		printf("%s\n", recv_message);	
	}
}
/////////////////////////////////
void recvFile(int sock, char *filename){
	char data[1024];
	memset(data, '0', sizeof(data));
	/*Check size of file*/
	read(sock, data, strlen(data));
	int fsize = atoi(data);
	printf("Length of file: %d\n", fsize);
	if(fsize == 0){
		printf("%s\n", "File doesnt exist");
		printf("%s\n", "----------------");
		//memset(data, '0', sizeof(data));
		close(sock);
	}
	else{
		FILE * wf = fopen(filename, "ab+");
		if(wf == NULL){
			perror("open file");
			close(sock);
		}
	
		printf("%s\n", "file created");
		int n;
		while (1){
			n = read(sock, data, sizeof(data));
			if (n > 0){
				printf("n: %d\n", n);
				fwrite(data, 1, n, wf);
				fsize -= n;
				if(fsize == 0) break;
			}

			printf("xong\n");
		}
		fclose(wf);

		//memset(data, '0', sizeof(data));
		printf("Downloaded successful filename %s\n", filename );
		printf("%s\n", "--------------");
		
	}
	
}
void sendFile(int sock, char *filename){
	char filesize[1024];
	memset(filesize, 0, sizeof(filesize));
	char data[1024];
	memset(data, 0, sizeof(data));

	FILE *rf = fopen(filename, "rb");
	if(rf == NULL){
		printf("%s\n", "File doesnt exist\n");
		printf("%s\n", "-------------------");
		sprintf(filesize, "0");
		send(sock, filesize, sizeof(data), 0);
		close(sock);
	}
	else{
		memset(filesize, '0', sizeof(filesize));

		/**/
		fseek(rf, 0, SEEK_END);
		int fsize = ftell(rf);
		rewind(rf);
		/****/
		sprintf(filesize, "%d", fsize);
		printf("Size of file: %d\n", fsize);
		send(sock, filesize, sizeof(data), 0);
		memset(filesize, '0', sizeof(filesize));

		while(1) {
			int j = fread(data, 1, 1024 , rf);
			if ( j == 0) {
				data[0] = '\0';
				j = 1;
			}
			int nw = write(sock, data, j);
			if (nw < 1024) break;
		}
		memset(data, '0', sizeof(data));
	}
	fclose(rf);
	printf("Send successful %s\n", filename);
	printf("%s\n", "--------------");
}
void rFCaTrim( char str[]) {
	memmove(str, str + 1, strlen(str));
	str[strlen(str)] = '\0';	
}