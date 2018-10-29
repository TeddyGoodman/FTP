#ifndef SESSION_H
#define SESSION_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

//此用户当前的状态
typedef enum LoginStatus
{
	unlogged,
	need_pass,
	logged
}LoginStatus;

//定义一个回话，保存所有client端的信息
typedef struct session
{
	LoginStatus login_status;
	//保留目前的模式，1表示pasv模式，0表示port模式，-1表示没有
	int current_pasv;
	int client_fd; //和用户连接的套接字描述符

	//pasv独有
	int pasv_lis_fd; //pasv模式监听套接字
	//均有
	struct sockaddr_in client_addr; //port链接时client端的地址
	int data_fd; //数据传输的套接字描述符
	
	int is_RNFR; //上一条消息是否是RNFR
	char* pre_msg_content; //保存上一条消息的内容
	char* sentence; //保存发送的和接受的消息
    // char* command; //保存命令
    // char* parameter; //保存参数
    char* working_root; //工作目录
}session;

int init_session(session* sess, int client_fd, char* global_file_root);

int close_session(session* sess);

//服务器向客户端返回自定义消息，用于单个消息
void reply_custom_msg(session* sess, int code, char* str);

//一部分code返回同样的消息，此函数用于返回这些消息
void reply_form_msg(session* sess, int code);

#endif