#ifndef FTPSERVER_H
#define FTPSERVER_H

#define MAX_Client 10

#include <stdio.h>

//全局变量
short unsigned lis_port = 21;
int listenfd;
char file_root[128] = {0};

int server_init();

void serve_client(int client_fd);

#endif