#include "stdio.h"
#include "stdlib.h"
#include "string.h"


void rFCaTrim( char str[]) {
	// memmove(str, str + 1, strlen(str));
	str[strlen(str) - 1 ] = '\0';	
	strcat(str, "hahaha");
}
int main(int argc, char const *argv[])
{
	char mess[256];
	fgets(mess, sizeof(mess), stdin);
	rFCaTrim(mess);
	printf("'%s'\n", mess);
	return 0;
}