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
#include <time.h>

#define MAX_USR 20
#define MAX_CHA 50
#define MESSLEN 256

typedef struct {
	char name[MESSLEN];
	int sockfd;
	int useFlag;
} User;

typedef struct {
	int type ; 
	char name[MESSLEN]; 
	User users[MAX_USR];
	int useFlag;
	int cur;
} Channel;

Channel channels[MAX_CHA];

int CH_TEMP = 0;

pthread_mutex_t	mutex1 = PTHREAD_MUTEX_INITIALIZER;

void *connection_handler();

void listUser ( int index ) {
	int i ;
	printf("List user in channels '%d' name =  '%s' \n", index , channels[index].name  );
	for (i = 0; i < MAX_USR; ++i)
	{
		if ( channels[index].users[i].useFlag == 1) {
			printf("user[%d].name = '%s'\n" , i, channels[index].users[i].name );
		}
	}
}
void publicMessagetoChannel(int sock ,char message[] , int index) {
	printf("SendMESS() : send message = '%s' to channel %d (name = '%s') \n", message , index, channels[index].name   );
	int i ;
	for (i = 0; i < channels[index].cur; ++i)
	{
		if (channels[index].users[i].useFlag == 1 && channels[index].users[i].sockfd != sock ) {
			printf("SendMESS() : Sent messs to '%s'\n" , channels[index].users[i].name);
			write(channels[index].users[i].sockfd , message, MESSLEN);
		}
	}
	printf("SendMESS(): End Func.\n");
}
void publicFile(int sock, char *filename){
	char data[1024];
	FILE *rf = fopen(filename, "rb");
	if( rf ){
		while(1) {
			int j = fread(data, 1, 1024 , rf);
			if ( j == 0) {
				data[0] = '\0';
				j = 1;
			}
			int nw = write(sock, data, j);
			if (nw < 1024) break;
		}
	    fclose(rf);
	    printf("Send successful %s\n", filename);
	}
	else{
		printf("File does not exist \n" );
	}
}

void publicFiletoChannel(int sock , char *filename, int cur_index) {
	printf("publicFile(): file %s\n",  filename);
	int i ;
	for (i = 0; i < channels[cur_index].cur; ++i)
	{
		if (channels[cur_index].users[i].useFlag == 1 && channels[cur_index].users[i].sockfd != sock ) {
			publicFile(channels[cur_index].users[i].sockfd, filename);	
		}
	}
	printf("publicFile(): done \n");
}


void subcribeFile(int sock, char *filename){
	char data[1024];
	FILE *wf = fopen(filename, "wb+");
	if (wf) {
		while (1){
			data[0] = '\0';
			int n = read(sock, data, sizeof(data));
			int j = fwrite(data, 1, n, wf);
			if (j < 1024) break;
		}
		printf("Downloaded successful filename '%s'\n", filename );
		fclose(wf);
	}
	else{
		printf("Cannot open file\n");
	}
}
void publicList( int sock ) {
	int i,j;
	char list[MESSLEN];
	strcpy(list, "List User :\n");
	for (i = 0; i < CH_TEMP; ++i)
	{	
		if( channels[i].useFlag == 1) {
			strcat(list, channels[i].name);
			strcat(list, ": ");
			for (j = 0; j < MAX_USR; ++j)
			{
				if ( channels[i].users[j].useFlag == 1) {
					strcat(list, channels[i].users[j].name);
					strcat(list, " ");
				}
			}
			strcat(list, "\n");
		}
	}
	write(sock, list , sizeof(list));
}

int findIndex (){
	printf("------------------In find Index()\n");
	pthread_mutex_lock(&mutex1);
	int t ;
	int cur_index = -1;
	for (t = 0; t < CH_TEMP; t++){
		if ( channels[t].useFlag == 0){
			cur_index = t;
		}
	}
	if (cur_index == -1 ) {
		cur_index = CH_TEMP;
		CH_TEMP++;
	};
	pthread_mutex_unlock(&mutex1);
	printf("--------------End find Index()\n");
	return cur_index;
}

User getUserbyName(char *name){
	pthread_mutex_lock(&mutex1);
	printf("findUser() : find user name '%s'\n", name);
	int i, j;
	User res_user;
	res_user.useFlag = 0;
	for ( i = 0; i < CH_TEMP; ++i)
	{
		printf("findUser() : Finding in channel[%d] . .. \n", i );
		for ( j = 0; j < MAX_USR; ++j)
		{
			if ( strcmp(name, channels[i].users[j].name) == 0 ) {
				printf("findUser(): found user '%s'\n", channels[i].users[j].name);
				res_user =  channels[i].users[j];
				pthread_mutex_unlock(&mutex1);
				return res_user;
			}
		}		
	}
	printf("findUser(): User not found '%s'\n", name);
	pthread_mutex_unlock(&mutex1);
	return res_user;
}
void createChannel(int index , char *channel){
	printf("------------------In createChannel Index()\n");
	pthread_mutex_lock(&mutex1);
	strcpy(channels[index].name, channel);
	channels[index].useFlag = 1;
	channels[index].users[0].useFlag = 0;
	channels[index].cur = 0;
	printf("Create channel '%s' successful , channel index = %d\n", channels[index].name , index);
	printf("CH_TEMP now = %d\n", CH_TEMP );
	pthread_mutex_unlock(&mutex1);
	printf("----------------- unlock createChannel()\n");
}

void addUserToChannel( User user , int chan_index ) {
	printf("------------------In addUserToChannel Index()\n");
	pthread_mutex_lock(&mutex1);
	int i ;
	for (i = 0; i < MAX_USR ; ++i)
	{
		if (channels[chan_index].users[i].useFlag == 0) {
			channels[chan_index].users[i] = user ;
			channels[chan_index].cur ++;
			int temp = channels[chan_index].cur ;
			channels[chan_index].users[temp].useFlag = 0 ;
			break;
		}
	}
	pthread_mutex_unlock(&mutex1);
	printf("ADD user '%s' to channel '%s' with index = %d \n", user.name, channels[chan_index].name, i );
	printf("------------------End addUserToChannel Index()\n");
}
void sendInvite(int recv_sock, char *send_name){
	char invite_mess[256];
	sprintf(invite_mess, "!%s", send_name);
	write( recv_sock, invite_mess, sizeof(invite_mess));
}
void rFCaTrim( char str[]) {
	memmove(str, str + 1, strlen(str));
	str[strlen(str)] = '\0';	
}

void makeMess(char send[], char *name, char message[]){
	strcpy(send, "");
	strcat(send, name);
	strcat(send, ":");
	strcat(send, message);
}
int getChannelIndexbyName(char *channel_name){
	pthread_mutex_lock(&mutex1);
	int i ;
	for ( i = 0; i < CH_TEMP; ++i)
	{
		if (channels[i].useFlag == 1 && strcmp(channel_name, channels[i].name) == 0 ) {
			pthread_mutex_unlock(&mutex1);
			return i;
		}		
	}
	pthread_mutex_unlock(&mutex1);
	return -1;
}
int getUserChannelIndex(char *username ){
	int i , j ;
	for ( i = 0; i < CH_TEMP; ++i)
	{
		for( j = 0 ; j < channels[i].cur ; j++ )
		{
			if ( strcmp(channels[i].users[j].name, username) == 0 )
			{
				return i;
			}
		}
	}
	return -1;
}
void clearUser(int channel_index, char *username){
	printf("------------------In clearUser ()\n");
	pthread_mutex_lock(&mutex1);
	int i ;
	printf("clearUser() '%s' : channel[%d]\n",username, channel_index );
	for (i = 0; i < channels[channel_index].cur; ++i)
	{
		printf("user[%d].name = '%s'\n", i, channels[channel_index].users[i].name  );
		if (strcmp(channels[channel_index].users[i].name, username) == 0) {
			channels[channel_index].users[i].useFlag = 0;
			strcpy(channels[channel_index].users[i].name, "");
			printf("clear: '%s' useFlag = %d\n", channels[channel_index].users[i].name, channels[channel_index].users[i].useFlag );
		}
	}
	pthread_mutex_unlock(&mutex1);
	printf("------------------End Clear Index()\n");
}
void clearEmptyChannel (){
	printf("------------------In clearChannel ()\n");
	pthread_mutex_lock(&mutex1);
	printf("Clear():\n");
	int i ,j;
	for (i = 1; i < CH_TEMP; ++i)
	{
		int isEmpty = 1;
		for ( j = 0; j < channels[i].cur; ++j)
		{
			if ( channels[i].users[j].useFlag == 1 ) {
				printf("A active user (name =  '%s') in channel '%d'\n" , channels[i].users[j].name , i );
				isEmpty = 0;
				break;
			}
		}
		if (isEmpty == 1) {
			channels[i].useFlag = 0;
		}
	}
	pthread_mutex_unlock(&mutex1);
	printf("------------------Out clearChannel ()\n");
}
int main(int argc, char const *argv[])
{
	channels[0].users[0].useFlag = 0;
	channels[0].useFlag = 1;
	strcpy(channels[0].name , "PUBLIC");
	CH_TEMP++;

	int socket_desc = 0 ; 
	struct sockaddr_in cliaddr, servaddr; 
	socklen_t clilen; 	
	if ( (socket_desc = socket(AF_INET, SOCK_STREAM, 0) ) < 0){
		perror("Error in create socket");
		exit(1);
	}
	else {
		puts("Socket created");
	}
	memset(&servaddr, '0', sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(8888);
	if (bind(socket_desc, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
		perror("Error in bind()");
		exit(2);
	}
	else {
		puts("Bind successful");
	}
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
		pthread_detach(pthread_self()); 
		int sock = *(int *) connfd;
		char username[256];  
		read(sock, username, sizeof(username));
		username[strlen(username) - 1] = '\0';
		printf("Client username : %s\n", username);

		User cur_user;

		strcpy(cur_user.name, username);
		cur_user.useFlag = 1;
		cur_user.sockfd = sock;

		publicList(sock);

		addUserToChannel( cur_user , 0);
		char message[256];
		int cur_index = -1;

    	while(1)
		{	
			printf("wfmess\n");
			// doc tin nhan tu client
			read(sock, message, sizeof(message));

			// NGAT KET NOI
			if (message[0] == '@') {
				printf("User %s end connect\n", cur_user.name);
				int check_index = getUserChannelIndex(cur_user.name);
				printf("User '%s' is in channel '%d'\n", cur_user.name , check_index);
				if (check_index != -1 ) {
					clearUser(check_index, cur_user.name);
				}
				clearEmptyChannel();
				cur_user.useFlag = 0;
				close(sock);
				break;
			}

			// CHAT RIENG
			if (strlen(message) > 0 && message[0] == '$'){
				rFCaTrim(message);
				printf("find users '%s'\n", message);
				// find user with the name client want
				printf("User '%s' invite user '%s' to chat\n", cur_user.name , message);
				printf("Find user '%s' in all list users\n", message );
				User invite_user = getUserbyName(message);
				if (invite_user.useFlag == 0 ) {
					char res[MESSLEN];
					strcpy(res, "User khong ton tai !");
					write(sock, res , sizeof(res));
					printf("Khong tim thay user '%s' \n", message );
					continue;
				}
				printf("Tim duoc user '%s'\n", message );
				int index = findIndex();
				char name[MESSLEN];
				srand(time(NULL));
				int r = rand() % 20;
				sprintf(name, "%s%d%d%d", "private", cur_user.sockfd, invite_user.sockfd,r);
				createChannel(index, name);
				int check_index = getUserChannelIndex(cur_user.name);
				printf("User '%s' is in channel '%d'\n", cur_user.name , check_index);
				if (check_index != -1 ) {
					clearUser(check_index, cur_user.name);
				}
				addUserToChannel(cur_user, index );
				clearEmptyChannel();
				sendInvite( invite_user.sockfd, cur_user.name );
				pthread_mutex_lock(&mutex1);
				channels[index].type = 1;
				pthread_mutex_unlock(&mutex1);
				cur_index = index;
				printf("SEnd invite successful to users '%s'\n" , message);
				continue;
			}

			// DANG KY KENH
			if (strlen(message) > 0 && message[0] == '%'){ 
				rFCaTrim(message);
				// khi nguoi dung muon join vao nhom 
				printf("user %s register chan = '%s'\n", cur_user.name , message );
				int index = getChannelIndexbyName(message);
				printf("tim index channel cho user = '%d'\n", index );
				// channel da ton tai
				if (index != -1){ 
					if (channels[index].type == 0) {
						int check_index = getUserChannelIndex(cur_user.name);
						printf("User '%s' is in channel '%d'\n", cur_user.name , check_index);
						if (check_index != -1 ) {
							clearUser(check_index, cur_user.name);
						}
						clearEmptyChannel();
						addUserToChannel(cur_user, index);
						listUser(index);
						char mess[MESSLEN];
						sprintf(mess, "You joined channel '%s'", channels[index].name );
						write(cur_user.sockfd , mess , sizeof(mess));
						strcpy(mess, "");
						sprintf(mess, "User '%s' join channel, lets talk ...", cur_user.name);
						publicMessagetoChannel(cur_user.sockfd , mess , index);
						cur_index = index;
						continue;
					}
					else {
						char mess[MESSLEN];
						sprintf(mess, "You can joined this channel %s ", message);
						write(cur_user.sockfd , mess , sizeof(mess));
						continue;
					}
				}
				//channel khong ton tai
				else {
					char mess[256];
					sprintf(mess, "You joined channel '%s'", message);
					write(sock, mess ,sizeof(mess));
					int check_index = getUserChannelIndex(cur_user.name);
					printf("User '%s' is in channel '%d'\n", cur_user.name , check_index);
					if (check_index != -1 ) {
						clearUser(check_index, cur_user.name);
					}
					index = findIndex();
					createChannel(index , message);
					addUserToChannel(cur_user, index);
					cur_index = index;
					continue;				
				}
			}

			// DONG Y CHAT RIENG VA LIST USER
			if ( strlen(message) > 0 && message[0] == '!' ){
				rFCaTrim(message);

				if (strcmp(message , "list") == 0) {
					printf("User requires user list\n");
					publicList(cur_user.sockfd);
					continue;
				}
				char temp[256];
				strcpy(temp, message);
				strtok(temp, ",");
				char *invite_user = strtok(NULL, ",");
				if (invite_user == NULL) {
					continue;
				}
				User inv_user = getUserbyName( invite_user );
				if (inv_user.useFlag == 0 ) {
					printf("Cant find user '%s'!\n" , invite_user);
					continue;
				}
				printf("Tim duoc user '%s'\n", invite_user );

				printf(" !!! mess = '%s'\n", message);

				if (message[0] == 'y') {
					printf("User '%s' accept\n", cur_user.name );
					int check_index = getUserChannelIndex(cur_user.name);
					printf("check_index = %d\n", check_index);
					if (check_index != -1 ) {
						printf("xxxxx\n");
						clearUser(check_index, cur_user.name);
					}
					int send_invite_ci = getUserChannelIndex(invite_user);
					addUserToChannel(cur_user, send_invite_ci);
					clearEmptyChannel();
					cur_index = send_invite_ci;

					sprintf(message, "User %s accept chat with you", cur_user.name);
					write(inv_user.sockfd, message, sizeof(message));
					sprintf(message, "Two you in channel %s", channels[cur_index].name);
					write(inv_user.sockfd, message, sizeof(message));
					write(cur_user.sockfd, message, sizeof(message));
				}
				else {
					sprintf(message, "User %s does not accept chat with you", cur_user.name);
					write(inv_user.sockfd, message, sizeof(message));
				}
				continue;
			}
			// END DONG Y CHAT

			// GUI FILE
			if (strlen(message) > 0 && message[0] == '#') {
				//xoa di dau # o dau string
				char mess_send[256];
    			memmove(message, message +1 , strlen(message));
    			message[strlen(message)] = '\0';

    			printf("User %s want to send file '%s'\n", cur_user.name, message);
    			sprintf(mess_send, "#%s,%s,%d", cur_user.name, message, cur_user.sockfd );
    			printf("Thong tin file gui den cac user la %s\n", mess_send);
				//gui thong bao den cac client khac
    			if ( cur_index == -1 ){
    				continue;
    			}
				publicMessagetoChannel(sock ,mess_send, cur_index);
				// nhan file tu file sender 
				subcribeFile(sock, message);
				// gui file cho cac client khac
				publicFiletoChannel(sock, message, cur_index);
				continue;
			}		
			// END GUI FILE

			// GUI TIN NHAN BINH THUONG
			message[strlen(message)] = '\0';
			//in ra noi dung tin nhan cua client
			printf("User '%s' send message '%s' \n", cur_user.name, message );

			if ( cur_index == -1 ) {
				char warning[MESSLEN];
				strcpy(warning, "You dont have a channel");
				write(cur_user.sockfd , warning , sizeof(warning));
				printf("Nhan duoc message '%s' tu user '%s', nhung k gui den channel nao !\n", message , cur_user.name );
			}
			else {
				char send[256];
				makeMess(send, cur_user.name, message);
				publicMessagetoChannel(sock ,send, cur_index);
			}
		}
    return 0;
}

