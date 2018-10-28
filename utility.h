#ifndef UTILITY_H
#define UTILITY_H

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

//保存上一条消息（只有在返回正确code时会改变）
typedef enum PreviousMsg
{
	//USER,
	RNFR,
	OTHER
}PreviousMsg;

//保存上一条消息的内容
typedef struct PreStore
{
	PreviousMsg premsg;
	char content[256];
}PreStore;

//数据传输部分的信息
typedef struct DataInfo
{
	int data_fd;
	struct sockaddr_in client_addr;

}DataInfo;

void remove_enter(char* str);

void send_const_msg(int fd, const char* str);

int remove_dir(char* dir);

//通过prefix和输入参数获取绝对路径，成功返回路径，否则返回NULL，第三个参数为是文件还是目录
char* get_absolute_dir(char* name_prefix, char* parameter, int is_file);

#endif