#ifndef FTPSERVER_H
#define FTPSERVER_H

#define MAX_Client 10

#include <stdio.h>
#include "utility.h"
#include "session.h"

char server_ip[] = "127.0.0.1";
//全局变量
short unsigned lis_port = 21;
int listenfd;
char file_root[256] = {0};

int server_init();

void serve_client(int client_fd);

int dispatch_cmd(char* cmd, char* para, session* sess);

void* serve_client_pthread(void* ptr);

#endif