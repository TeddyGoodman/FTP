#ifndef CLIENT_H
#define CLIENT_H

/*
* 全局变量在这里定义s
*/
int is_pasv;
//port模式下,本机需要监听，故需要监听socket
int data_lis_port = 0;
int control_fd;
//保存服务器地址
struct sockaddr_in server_addr;
//data端链接
int data_fd = 0;
//连接服务器所在的IP
char server_ip[] = "127.0.0.1";
//连接服务器的控制端口
int control_port = 21;

char root_directory[] = "/root/CSAPP/FTP";

int client_init();

int cmd_port(char* para);
int cmd_pasv(char* sentence);
void* download_file_pthread(void* ptr);
void* upload_file_pthread(void *ptr);
void download_file(int file);
void upload_file(int file);
int cmd_retr(char* sentence, char* para);
int cmd_stor(char* sentence, char* para);
int cmd_list(char* sentence);


#endif