/**
Desciption : MQTT Broker Server
Use : ./server
*/
#include "sys/socket.h"
#include "stdio.h"
#include "unistd.h"
#include "netinet/in.h"
#include "sys/types.h"
#include "stdlib.h"
#include "string.h"
#include "arpa/inet.h"
#include "inttypes.h"
#include "ctype.h"
#include "pthread.h"
#include "sys/file.h"
//***    ***////
//**** cac funtion // ****
void sendMessagetoChannel(char message[] , int cur_index);
void revcMessagefromChannel(int cur_index);
void sendFile(int sock, char *filename);
void recvFile(int sock, char *filename);
void sendFiletoChannel(char *filename, int cur_index);
void sendUserList(int sock , int cur_index);
int findIndex ();
void setPrivateChannel(int index1, int index2 );
int findUserIndexbyName(char *name);
//**//********************

//support max 1000 clients
#define MAX_CLI 1000

// cau truc cua channel
typedef struct {
	int type ; // loai channel : 1: private 2: public
	char name[256]; // ten channel
} Channel;

//struct users
typedef struct {
	char name[256];
	int sockfd;
	Channel channel;
	int useFlag;
} User;

// danh sach users
User users[MAX_CLI];
// bien toan cuc i, chi so dat den hien tai cua list users
int i;
// bien mutex thread
pthread_mutex_t	counter_mutex = PTHREAD_MUTEX_INITIALIZER;
//ham xu ly
void *connection_handler();

int main(int argc, char const *argv[])
{
	int socket_desc = 0 ; // socket lang nghe
	struct sockaddr_in cliaddr, servaddr; // client and server address
	socklen_t clilen; // length of client socket 
	// tao socket lang nghe 
	if ( (socket_desc = socket(AF_INET, SOCK_STREAM, 0) ) < 0){
		perror("Error in create socket");
		exit(1);
	}
	else {
		puts("Socket created");
	}
	// thiet lap gia tri cho cac truong cua socket
	memset(&servaddr, '0', sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(8888);
	// bind agument to serv address
	if (bind(socket_desc, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
		perror("Error in bind()");
		exit(2);
	}
	else {
		puts("Bind successful");
	}
	// listen to client connect
	listen(socket_desc, 100);
	
	while(1){
		clilen = sizeof(cliaddr);
		int *iptr = malloc(sizeof(int));
		*iptr = accept(socket_desc, (struct sockaddr *)&cliaddr, &clilen);
		if (*iptr == -1){
			perror("Error connection to client");
			exit(3);
		}
		else printf("A new client is connect to server:\n");
		printf("Client port is :%d\n", ntohs(cliaddr .sin_port));
		char *z = inet_ntoa(*(struct in_addr *)&cliaddr.sin_addr.s_addr);
		printf("Client IP Adress is: %s\n", z);
		pthread_t thread_id;
		if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) iptr) < 0)
      	{
        	perror("could not create thread");
        	return 1;
    	}
	}
	return 0;
}
void *connection_handler(void *connfd)
{
		// chuyen thread thanh doc lap
		pthread_detach(pthread_self()); 
		// lay socket descript tu thread
		int sock = *(int *) connfd;
		// khai bao cac bien
		char username[256]; 
		char channelName[256];
		// doc username tu client 
		read(sock, username, sizeof(username));
		username[strlen(username) - 1] = '\0';
		printf("Client username : %s\n", username);



		// khoa danh sach users
		pthread_mutex_lock(&counter_mutex);
		//**** tim index de luu users
		int cur_index = findIndex();
		// in ra index tim duoc 
		printf("Current index is  %d\n", cur_index );
		// dien thong tin vao slot tim duoc
		strcpy(users[cur_index].name, username);
		users[cur_index].name[strlen(username)]='\0';
		users[cur_index].useFlag = 1;
		users[cur_index].sockfd = sock;
		// in ra danh sach users tren server hien tai
		printf("user[%d]: %s\n", cur_index ,users[cur_index].name );
		pthread_mutex_unlock(&counter_mutex);



		// tao message bao gom cac user va channel cho users
		int j;
		sendUserList(users[cur_index].sockfd, cur_index);
		// nhan dang ky kenh tu client
		//***************************************************
		// Lua chon chat rieng voi user hoac join vao 1 kenh chat
		read(sock, channelName, sizeof(channelName));


		if (strlen(channelName) > 0 && channelName[0] == '$'){
			printf("User want to private channel\n");
			memmove(channelName, channelName + 1, strlen(channelName));
			channelName[strlen(channelName) - 1 ] = '\0';
			printf("That username is '%s'\n", channelName);
	
			// find user with the name client want
			pthread_mutex_lock(&counter_mutex);
			for (j = 0; j < i; j++){
				printf("Check user : user[%d].name = '%s'\n", j , users[j].name);
				if (strcmp(channelName, users[j].name) == 0 ){
					// gui yeu cau den user kia
					char invite_mess[256];
					sprintf(invite_mess, "!%s", users[cur_index].name);
					write(users[j].sockfd, invite_mess, sizeof(invite_mess));
				}
			}
			pthread_mutex_unlock(&counter_mutex);	
		}
		else { 
			// khi nguoi dung muon join vao nhom 
			channelName[strlen(channelName) - 1 ] = '\0';
			strcpy(users[cur_index].channel.name, channelName);
			users[cur_index].channel.type = 0;
			printf("public channel '%s'\n", users[cur_index].channel.name);
		}
		//***************************************************
		// xu ly cac tin nhan ma users gui den
		char message[256];
    	while(1)
		{	
			// doc tin nhan tu client
			int n = read(sock, message, sizeof(message));
			// neu client gui '@' thi dong ket noi
			if (strcmp(message, "@") == 0) {
				printf("User %s end connect\n", users[cur_index].name);
				users[cur_index].useFlag = 0;
				close(sock);
				break;
			}

			if ( strlen(message) > 0 && message[0] == '!' ){
				printf("xxxxx\n");
				printf("message = '%s'\n", message );
				memmove(message, message + 1, strlen(message));
				message[strlen(message)] = '\0';
				printf("Message after trim = '%s'\n", message );
				/////
				if (message[0] == 'y') {
					//find user
					strtok(message, ",");
					char *invite_user = strtok(NULL, ",");
					printf("invite user = '%s'\n", invite_user );
					int inv_user_index = findUserIndexbyName( invite_user );
					if (inv_user_index == -1 ) {
						printf("Cant find user xx!\n");
					}
					setPrivateChannel(inv_user_index, cur_index);
					sprintf(message, "User %s accept chat with you", users[cur_index].name);
					write(users[inv_user_index].sockfd, message, sizeof(message));
				}
				continue;
			}

			// neu user thong bao muon gui file , noi dung tin nhan co # o truoc
			if (strlen(message) > 0 && message[0] == '#') {
				//xoa di dau # o dau string
				char mess_send[256];
    			memmove(message, message+1, strlen(message));
    			message[strlen(message)] = '\0';
    			printf("User %s want to send file '%s'\n", users[cur_index].name, message);
    			sprintf(mess_send, "#%s,%s,%d", users[cur_index].name, message, users[cur_index].sockfd );
    			printf("Thong tin file gui den cac user la %s\n", mess_send);
				//gui thong bao den cac client khac
				sendMessagetoChannel(mess_send, cur_index);
				// nhan file tu file sender 
				printf("Nhan file tu client.. .\n");
				recvFile(users[cur_index].sockfd, message);
				printf("Ket thuc nhan file tu client\n");
				// gui file cho cac client khac
				printf("Start send file to channel\n");
				sendFiletoChannel(message, cur_index);
				printf("End send file to channel\n");
				continue;
			}		

			if (n < 256 ) message[n] = '\0';
			//in ra noi dung tin nhan cua client
			printf("User '%s' send message '%s' \n", users[cur_index].name, message );
			pthread_mutex_lock(&counter_mutex);
			// tao noi dung tin nhan gui den cac user
			char send[256];
			send[0] = '\0';
			strcat(send, users[cur_index].name);
			strcat(send, ":");
			strcat(send, message);
			printf("message win send = '%s'\n", send );
			// gui noi dung tin nhan cua client den cac client cung channel
			printf("sssss\n");
			sendMessagetoChannel(send, cur_index);
			pthread_mutex_unlock(&counter_mutex);
		}
    return 0;
}

void sendMessagetoChannel(char message[] , int cur_index) {
	printf("sendMEss(): %s\n", message);
	int k ;
	for (k = 0; k <= i - 1 ; k++){
		if (users[k].useFlag == 1 && k != cur_index){
			if (k != cur_index){
				if (strcmp (users[k].channel.name, users[cur_index].channel.name) == 0){
					write(users[k].sockfd , message, 256);
					printf("sendMEss() :Sent\n");
				}
			}
		}
	}
}
void sendFiletoChannel(char *filename, int cur_index) {
	int k ;
	for (k = 0; k <= i - 1 ; k++){
		if (users[k].useFlag == 1 && k != cur_index){
			if (k != cur_index){
				printf("Send file to user %d\n", k);
				sendFile(users[k].sockfd, filename);
				printf("Done\n");
			}
		}
	}
}
void sendFile(int sock, char *filename){
	FILE *rf = fopen(filename, "rb");
	char data[1024];
	while(1) {
		int j = fread(data, 1, 1024 , rf);
		if ( j == 0) {
			data[0] = '\0';
			j = 1;
		}
		int nw = write(sock, data, j);
		printf("nw = %d\n", nw);
		if (nw < 1024) break;
	}
    fclose(rf);
    printf("Send successful %s\n", filename);
}
void recvFile(int sock, char *filename){
	char data[1024];
	FILE *wf = fopen(filename, "wb+");
	if (!wf) {
		puts("Cant open file");
	}
	// recv filesize tu client 
	read(sock, data, sizeof(data));
	int filesize = atoi(data);
	printf("Fileszie = %d\n", filesize);
	while (1){
		data[0] = '\0';
		int n = read(sock, data, sizeof(data));
		int j = fwrite(data, 1, n, wf);
		if (j < 1024) break;
	}
	printf("Downloaded successful filename%s\n", filename );
	fclose(wf);
}
void sendUserList( int sock , int cur_index) {
	int j;
	char list_users[1000];
	pthread_mutex_lock(&counter_mutex);
	// ghi danh sach users vao message gui di
	int count = 0;
	strcpy(list_users, "");
	strcat(list_users, "List users: \n");
	for ( j = 0; j < i; ++j){
		if (users[j].useFlag == 1 && j != cur_index){
			count++;
			strcat(list_users , users[j].name);
			strcat(list_users, " in ");
			if ( strlen(users[j].channel.name) > 0 ){
				strcat(list_users , users[j].channel.name);
			}
			else {
				strcat(list_users, "None");
			}
			// them xuong dong trong tin nhan 
			strcat(list_users, "\n");
		}
	}
	if (count == 0 ) strcat(list_users, "Hien tai chua co user nao !");
	pthread_mutex_unlock(&counter_mutex);
	// in tin nhan se duoc gui den client
	// gui tin nhan cho client
	write(sock, list_users , sizeof(list_users));
}
int findIndex (){
	int t ;
	int cur_index = -1;
	for (t = 0; t < i; t++){
		if ( users[t].useFlag == 0){
			cur_index = t;
		}
	}
	if (cur_index == -1 ) {
		cur_index = i;
		i++;
	};
	return cur_index;
}
void setPrivateChannel(int index1, int index2 ){
	char pri_channel[256];
	sprintf(pri_channel, "%s%d%d", "private", index1, index2);
	strcpy(users[index1].channel.name, pri_channel);
	users[index1].channel.type = 1;
	strcpy(users[index2].channel.name, pri_channel);
	users[index2].channel.type = 1;
	printf("User '%s' in channel '%s'\n", users[index1].name, users[index1].channel.name );
	printf("User '%s' in channel '%s'\n", users[index2].name, users[index2].channel.name );
}
int findUserIndexbyName(char *name){
	pthread_mutex_lock(&counter_mutex);
	int j;
	for (j = 0; j < i; ++j)
	{
		if (strcmp(users[j].name, name) == 0){
			pthread_mutex_unlock(&counter_mutex);
			return j;
		}
	}
	pthread_mutex_unlock(&counter_mutex);
	return -1;
}