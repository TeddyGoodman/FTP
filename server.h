#ifndef FTPSERVER_H
#define FTPSERVER_H

#define MAX_Client 10

#include <stdio.h>

//全局变量
int lis_port = 21;
int listenfd;
char file_root[128] = {0};

int ftp_init();

#endif