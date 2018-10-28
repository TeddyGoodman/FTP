#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utility.h"
#include "command.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

// cmd函数返回code，在dispatch中返回相应信息
// cmd函数中，先判断参数是否正确，以返回504或501
// 之后再判断登录状态

// TODO: need to implement user table
int cmd_user(char* para, LoginStatus* login) {
	if (para == NULL) return 504;

	if (*login == logged) return 202;
	else if (*login == need_pass) return 530;
	//only support anonymous
	// 331 or 332 for permission might be granted after a PASS request
	if (strcmp(para, "anonymous") == 0) {
		*login = need_pass;
		return 331;
	}
	
	// 230 for client has permission to access files under that username
	// 或许是一个通用的用户名？
	// 230 情况下:
	// *login = logged;

	// 530 for the username is unacceptable
	return 530;
}

// TODO: check the password
int cmd_pass(char* para, LoginStatus* login) {
	if (para == NULL) return 504;

	if (*login == logged) return 503; //503: 上一个不是USER.
	else if (*login == unlogged) return 530;

	//202: USER时已经有权限。目前不存在USER直接230的情况

	//匹配密码
	int match = 1;
	if (match){
		*login = logged;
		return 230;
	}
	else return 530;
}

int cmd_quit(char* para, LoginStatus *login) {
	if (para != NULL) return 504;

	if (*login == unlogged || *login == need_pass) return 530;

	*login = unlogged;
	return 221;
}

int cmd_type(char* para, LoginStatus *login) {
	if (para == NULL) return 504;

	if (*login == unlogged || *login == need_pass) return 530;

	//可能需要定制消息
	if (strcmp(para, "I") == 0) return 200;
	else return 501;
}

int cmd_syst(char* para, LoginStatus *login) {
	if (para != NULL) return 504;

	if (*login == unlogged || *login == need_pass) return 530;

	return 215;
}

int cmd_pwd(char* para, LoginStatus *login) {
	if (para != NULL) return 504;
	if (*login == unlogged || *login == need_pass) return 530;
	// ?:
	// a double quote;
	// the name prefix, with each double quote replaced by a pair of double quotes, and with \012 encoded as \000;
	// another double quote;
	// a space;
	// useless human-readable information.
	return 257;
}

//路径方面的问题：encoded需不需要想法处理
int cmd_cwd(char* para, LoginStatus *login, char* name_prefix) {
	if (para == NULL) return 504;
	if (*login == unlogged || *login == need_pass) return 530;

	if (strstr(para, "..") != NULL) return 530;//不能有..

	int code;
	char *dir = get_absolute_dir(name_prefix, para, 0);
	if (dir != NULL) {
		sprintf(name_prefix, "%s", dir);
		code = 250;
		free(dir);
	}
	else code = 550;
	return code;
}

int cmd_mkd(char* para, LoginStatus *login, char* name_prefix) {
	if (para == NULL) return 504;
	if (*login == unlogged || *login == need_pass) return 530;

	if (strstr(para, "..") != NULL) return 530;//不能有..

	int len = strlen(name_prefix);
	char* temp_dir = (char*)malloc(256);
	int code = 550;
	if (para[0] == '/'){
		//定义绝对路径且存在
		if (mkdir(para, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == 0) {
			code = 250;
		}
	}
	else{
		//定义当前路径下的相对路径
		if (name_prefix[len-1] == '/') sprintf(temp_dir, "%s%s", name_prefix, para);
		else sprintf(temp_dir, "%s/%s", name_prefix, para);

		if (mkdir(temp_dir, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == 0) {
			code = 250;
		}
	}
	free(temp_dir);
	return code;
}

int cmd_rmd(char* para, LoginStatus *login, char* name_prefix) {
	if (para == NULL) return 504;
	if (*login == unlogged || *login == need_pass) return 530;

	if (strstr(para, "..") != NULL) return 530;//不能有..

	int len = strlen(name_prefix);
	char* temp_dir = (char*)malloc(256);
	int code = 550;
	if (para[0] == '/'){
		//定义绝对路径
		if (remove_dir(para) == 0)
			code = 250;
	}
	else{
		//定义当前路径下的相对路径
		if (name_prefix[len-1] == '/') sprintf(temp_dir, "%s%s", name_prefix, para);
		else sprintf(temp_dir, "%s/%s", name_prefix, para);

		if (remove_dir(temp_dir) == 0) code = 250;
	}
	return code;
}

int cmd_rnfr(char* para, LoginStatus *login, char* name_prefix, PreStore* Premsg) {

	if (para == NULL) return 504;
	if (*login == unlogged || *login == need_pass) return 530;

	if (strstr(para, "..") != NULL) return 530;//不能有..

	int code;
	char* dir = get_absolute_dir(name_prefix, para, 1);
	if (dir != NULL) {
		Premsg->premsg = RNFR;
		sprintf(Premsg->content, "%s", dir);
		code = 350;
		free(dir);
	}
	else code = 550;
	return code;
}

int cmd_rnto(char* para, LoginStatus *login, char* name_prefix, PreStore* Premsg) {

	if (para == NULL) return 504;
	if (*login == unlogged || *login == need_pass) return 530;
	if (Premsg->premsg != RNFR) return 503;

	if (strstr(para, "..") != NULL) return 530;//不能有..
	
	int code;
	Premsg->premsg = OTHER;
	char* dir = get_absolute_dir(name_prefix, para, -1);
	if (dir != NULL) {
		//原文件位置为Premsg->content，现在为dir
		char* shell_cmd = (char*)malloc(512);
		sprintf(shell_cmd, "mv %s %s", Premsg->content, dir);
		if (system(shell_cmd) == -1) {
			printf("Error running shell: %s(%d)\n", strerror(errno), errno);
			code = 553;
		}
		else code = 250;
		free(shell_cmd);
		free(dir);
	}
	else code = 553;
	return code;
}

int cmd_pasv(char* para, LoginStatus *login, DataInfo* data_info) {
	if (para != NULL) return 504;
	if (*login == unlogged || *login == need_pass) return 530;
	return 0;
}

int cmd_port(char* para, LoginStatus *login, DataInfo* data_info) {
	if (para == NULL) return 504;
	if (*login == unlogged || *login == need_pass) return 530;

	int h1,h2,h3,h4,p1,p2;
	char check_s;
	int count = 0;

	count = sscanf(para, "%d,%d,%d,%d,%d,%d %c", &h1, &h2, &h3, &h4, &p1, &p2, &check_s);
	if (count != 6)
		return 501;

	//检验格式问题
	if (h1 < 0 || h1 > 255)
		return 501;
	if (h2 < 0 || h2 > 255)
		return 501;
	if (h3 < 0 || h3 > 255)
		return 501;
	if (h4 < 0 || h4 > 255)
		return 501;
	if (p1 < 0 || p1 > 255)
		return 501;
	if (p2 < 0 || p2 > 255)
		return 501;

	char ip_str[128];
	//已经有socket，则断开连接
	if (data_info->data_fd != 0) close(data_info->data_fd);
	//创建socket
	if ((data_info->data_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 500;
	}

	//设置客户端发送的IP
	memset(&(data_info->client_addr), 0, sizeof(data_info->client_addr));
	data_info->client_addr.sin_family = AF_INET;
	data_info->client_addr.sin_port = htons(p1*256 + p2);

	sprintf(ip_str, "%d.%d.%d.%d", h1, h2, h3, h4);
	//转换ip地址:点分十进制-->二进制
	if (inet_pton(AF_INET, ip_str, &(data_info->client_addr.sin_addr)) <= 0) {
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return 500;
	}

	return 200;
}