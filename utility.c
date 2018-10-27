/*
* this file is utilities for the ftp server
*
*/
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include "utility.h"

void remove_enter(char* str) {
	int i = 0;
	while (1){
		if (str[i] == '\0') return;
		else if (str[i] == '\r' || str[i] == '\n') {
			str[i] = '\0';
			return;
		}
		i++;
	}
}

void send_const_msg(int fd, const char* str) {
	int len = strlen(str);
	send(fd, (char *)str, len, MSG_WAITALL);
	return;
}