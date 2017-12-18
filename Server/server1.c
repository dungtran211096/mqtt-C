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

#define MAX_USR 20
#define MAX_CHA 50
#define MESSLEN 256

typedef struct {
	char name[256];
	int sockfd;
	int useFlag;
} User;

User users[10];

typedef struct {
	int type ; 
	char name[256]; 
	User users[MAX_USR];
	int useFlag;
	int cur;
} Channel;

Channel channels[MAX_CHA];

int CH_TEMP = 0;

pthread_mutex_t	counter_mutex = PTHREAD_MUTEX_INITIALIZER;



void *connection_handler();


void publicMessagetoChannel(char message[] , int cur_index) {
	pthread_mutex_lock(&counter_mutex);
	int i ;
	for (i = 0; i < channels[cur_index].cur; ++i)
	{
		if (channels[cur_index].users[i].useFlag == 0) {
			write(channels[cur_index].users[i].sockfd , message, 256);
		}
	}
	pthread_mutex_unlock(&counter_mutex);
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

void publicFiletoChannel(char *filename, int cur_index) {
	pthread_mutex_lock(&counter_mutex);
	int i ;
	for (i = 0; i < channels[cur_index].cur; ++i)
	{
		if (channels[cur_index].users[i].useFlag == 0) {
			publicFile(channels[cur_index].users[i].sockfd, filename);	
		}
	}
	pthread_mutex_unlock(&counter_mutex);
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
		printf("Downloaded successful filename%s\n", filename );
		fclose(wf);
	}
	else{
		printf("Cannot open file\n");
	}
}
void publicList( int sock ) {
	pthread_mutex_lock(&counter_mutex);
	// for (int i = 0; i < CH_TEMP; ++i)
	// {

	// }
	char mess[MESSLEN];
	strcpy(mess ,"...");
	write(sock, mess ,sizeof(mess));
	pthread_mutex_unlock(&counter_mutex);
}

int findIndex (){
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
	return cur_index;
}

User getUserbyName(char *name){
	pthread_mutex_lock(&counter_mutex);
	int i, j;
	for ( i = 0; i < CH_TEMP; ++i)
	{
		for ( j = 0; j < MAX_USR; ++j)
		{
			if ( strcmp(name, channels[i].users[j].name) == 0 ) {
				return channels[i].users[j];
			}
		}		
	}
	pthread_mutex_unlock(&counter_mutex);
	exit(0);
}
void createChannel(int index , char *channel){
	strcpy(channels[index].name, channel);
	channels[index].useFlag = 1;
	channels[index].users[0].useFlag = 0;
	channels[index].cur = 0;
}

void addUserToChannel( User user , int chan_index ) {
	pthread_mutex_lock(&counter_mutex);
	int i ;
	for (i = 0; i < channels[chan_index].cur ; ++i)
	{
		if (channels[chan_index].users[i].useFlag == 0) {
			channels[chan_index].cur ++;
			channels[chan_index].users[i] = user;
			break;
		}
	}
	pthread_mutex_lock(&counter_mutex);
}
void sendInvite(int recv_sock, char *send_name){
	pthread_mutex_lock(&counter_mutex);
	char invite_mess[256];
	sprintf(invite_mess, "!%s", send_name);
	write( recv_sock, invite_mess, sizeof(invite_mess));
	pthread_mutex_unlock(&counter_mutex);
}
void rFCaTrim( char str[]) {
	memmove(str, str + 1, strlen(str));
	str[strlen(str)] = '\0';	
}

void makeMess(char send[], int cur_index, char message[]){
		send[0] = '\0';
		strcat(send, users[cur_index].name);
		strcat(send, ":");
		strcat(send, message);
}
int getChannelIndexbyName(char *channel_name){
	pthread_mutex_lock(&counter_mutex);
	int i ;
	for ( i = 0; i < CH_TEMP; ++i)
	{
		if (strcmp(channel_name, channels[i].name) == 0 ) {
			pthread_mutex_unlock(&counter_mutex);
			return i;
		}		
	}
	pthread_mutex_unlock(&counter_mutex);
	return -1;
}

int main(int argc, char const *argv[])
{
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

		char message[256];
		int cur_index = -1;
    	while(1)
		{	
			// doc tin nhan tu client
			int n = read(sock, message, sizeof(message));


			// NGAT KET NOI
			if (message[0] == '@') {
				printf("User %s end connect\n", cur_user.name);
				cur_user.useFlag = 0;
				close(sock);
				break;
			}

			// CHAT RIENG
			if (strlen(message) > 0 && message[0] == '$'){
				rFCaTrim(message);
				printf("find users '%s'\n", message);
				// find user with the name client want
				int recv_sock = getUserbyName(message).sockfd;
				if (recv_sock == -1) {
					char res[MESSLEN];
					strcpy(res, "User khong ton tai !");
					write(sock, res , sizeof(res));
					continue;
				}
				sendInvite( recv_sock, cur_user.name );
				continue;
			}

			// DANG KY KENH
			if (strlen(message) > 0 && message[0] == '%'){ 
				rFCaTrim(message);
				// khi nguoi dung muon join vao nhom 
				int index = getChannelIndexbyName(message);
				printf("index = %d\n", index );
				// channel da ton tai
				if (index != -1){ 
					if (channels[index].type == 0) {
						addUserToChannel(cur_user, index);
						cur_index = index;
						continue;
					}
					else {
						continue;
					}
				}
				//channel khong ton tai
				else {
					char mess[256];
					sprintf(mess, "You joined channel %s", message);
					write(sock, mess ,sizeof(mess));
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
				// if (inv_user) {
				// 	printf("Cant find user xx!\n");
				// 	continue;
				// }
				printf(" !!! mess = '%s'\n", message);
				if (message[0] == 'y') {
					int index = findIndex();
					char name[MESSLEN];
					sprintf(name, "%s%d%d", "private", cur_user.sockfd, inv_user.sockfd);
					createChannel(findIndex(), name);
					addUserToChannel(cur_user, index);
					addUserToChannel(inv_user, index);
					cur_index = index;
					channels[index].type = 1;
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

				publicMessagetoChannel(mess_send, cur_index);
				// nhan file tu file sender 
				subcribeFile(sock, message);
				// gui file cho cac client khac
				publicFiletoChannel(message, cur_index);
				continue;
			}		
			// END GUI FILE

			// GUI TIN NHAN BINH THUONG
			if (n < 256 ) message[n] = '\0';
			//in ra noi dung tin nhan cua client
			printf("User '%s' send message '%s' \n", cur_user.name, message );
			pthread_mutex_lock(&counter_mutex);
			// tao noi dung tin nhan gui den cac user
			char send[256];
			makeMess(send, cur_index,message);
			// gui noi dung tin nhan cua client den cac client cung channel
			publicMessagetoChannel(send, cur_index);
			pthread_mutex_unlock(&counter_mutex);

			// END GUI TIN NHAN BINH THUONG
		}
    return 0;
}

