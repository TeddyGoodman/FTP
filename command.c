#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include "utility.h"
#include "command.h"
#include "session.h"
#include "conf.h"

// cmd函数返回code，在dispatch中返回相应信息
// cmd函数中，先判断参数是否正确，以返回504或501
// 之后再判断登录状态

// TODO: need to implement user table
int cmd_user(char* para, session* sess) {
	if (para == NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}

	if (sess->login_status == logged) {
		reply_custom_msg(sess, 202, "permission already granted.");
		return 202;
	}
	else if (sess->login_status == need_pass){
		reply_form_msg(sess, 530);
		return 530;	
	}
	//only support anonymous
	// 331 or 332 for permission might be granted after a PASS request
	if (strcmp(para, "anonymous") == 0) {
		sess->login_status = need_pass;
		reply_custom_msg(sess, 331, "require password, please use PASS");
		return 331;
	}
	else {
		// 530 for the username is unacceptable
		reply_form_msg(sess, 530);
		return 530;
	}
	// 230 for client has permission to access files under that username
	// 或许是一个通用的用户名？
	// 230 情况下:
	// *login = logged;

}

// TODO: check the password
int cmd_pass(char* para, session* sess) {
	if (para == NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}

	if (sess->login_status == logged) {
		reply_form_msg(sess, 503);
		return 503; //503: 上一个不是USER.
	}
	else if (sess->login_status == unlogged) {
		reply_form_msg(sess, 530);
		return 530;
	}

	//202: USER时已经有权限。目前不存在USER直接230的情况

	//匹配密码
	int match = 1;
	if (match){
		sess->login_status = logged;
		reply_custom_msg(sess, 230, "permission granted. welcome!");
		return 230;
	}
	else {
		reply_form_msg(sess, 530);
		return 530;
	}
}

int cmd_quit(char* para, session* sess) {
	if (para != NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}

	if (sess->login_status == unlogged || 
		sess->login_status == need_pass) {
		reply_form_msg(sess, 530);
		return 530;
	}

	sess->login_status = unlogged;
	reply_custom_msg(sess, 221, "Bye.");
	return 221;
}

int cmd_type(char* para, session* sess) {
	if (para == NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}

	if (sess->login_status == unlogged ||
		sess->login_status == need_pass) {
		reply_form_msg(sess, 530);
		return 530;
	}

	//可能需要定制消息
	if (strcmp(para, "I") == 0) {
		reply_custom_msg(sess, 200, "Type set to I.");
		return 200;
	}
	else {
		reply_form_msg(sess, 501);
		return 501;
	}
}

int cmd_syst(char* para, session* sess) {
	if (para != NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}

	if (sess->login_status == unlogged ||
		sess->login_status == need_pass) {
		reply_form_msg(sess, 530);
		return 530;
	}

	reply_custom_msg(sess, 215, "UNIX Type: L8");
	return 215;
}

int cmd_pwd(char* para, session* sess) {
	if (para != NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}
	if (sess->login_status == unlogged ||
		sess->login_status == need_pass) {
		reply_form_msg(sess, 530);
		return 530;
	}
	// ?:
	// a double quote;
	// the name prefix, with each double quote replaced by a pair of double quotes, and with \012 encoded as \000;
	// another double quote;
	// a space;
	// useless human-readable information.
	char* temp_msg = (char*)malloc(512);
	sprintf(temp_msg, "%s is current directory.", sess->working_root);
	reply_custom_msg(sess, 257, temp_msg);
	free(temp_msg);
	return 257;
}

//路径方面的问题：encoded需不需要想法处理
int cmd_cwd(char* para, session* sess) {
	if (para == NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}
	if (sess->login_status == unlogged ||
		sess->login_status == need_pass) {
		reply_form_msg(sess, 530);
		return 530;
	}

	if (strstr(para, "..") != NULL) {
		//不能有..
		reply_form_msg(sess, 530);
		return 530;
	}

	int code;
	char* dir = get_absolute_dir(sess->working_root, para, 0);
	if (dir != NULL) {
		char* temp_msg = (char*)malloc(512);
		sprintf(sess->working_root, "%s", dir);
		sprintf(temp_msg, "%s is current directory.", sess->working_root);
		reply_custom_msg(sess, 250, temp_msg);
		code = 250;
		free(dir);
		free(temp_msg);
	}
	else{
		reply_custom_msg(sess, 550, "failed, the directory doesn't exist.");
		code = 550;
	}
	return code;
}

int cmd_mkd(char* para, session* sess) {
	if (para == NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}
	if (sess->login_status == unlogged ||
		sess->login_status == need_pass) {
		reply_form_msg(sess, 530);
		return 530;
	}

	if (strstr(para, "..") != NULL) {
		//不能有..
		reply_form_msg(sess, 530);
		return 530;
	}

	int len = strlen(sess->working_root);
	char* temp_dir = (char*)malloc(256);
	int code = 550;
	if (para[0] == '/'){
		//定义绝对路径且存在
		if (mkdir(para, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == 0) {
			reply_custom_msg(sess, 250, "creation successful!");
			code = 250;
		}
		else {
			reply_custom_msg(sess, code, "creation failed!");
		}
	}
	else{
		//定义当前路径下的相对路径
		if (sess->working_root[len-1] == '/') sprintf(temp_dir, "%s%s", sess->working_root, para);
		else sprintf(temp_dir, "%s/%s", sess->working_root, para);

		if (mkdir(temp_dir, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == 0) {
			reply_custom_msg(sess, 250, "creation successful!");
			code = 250;
		}
		else {
			reply_custom_msg(sess, code, "creation failed!");
		}
	}
	free(temp_dir);
	return code;
}

int cmd_rmd(char* para, session* sess) {
	if (para == NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}
	if (sess->login_status == unlogged ||
		sess->login_status == need_pass) {
		reply_form_msg(sess, 530);
		return 530;
	}

	if (strstr(para, "..") != NULL) {
		//不能有..
		reply_form_msg(sess, 530);
		return 530;
	}

	int code = 550;
	char* dir = get_absolute_dir(sess->working_root, para, 0);
	if (dir == NULL) {
		reply_custom_msg(sess, 550, "deletion failed!");
		return code;
	}

	if (remove_dir(dir) == 0) {
		reply_custom_msg(sess, 250, "deletion successful!");
		code = 250;
	}
	else {
		reply_custom_msg(sess, 550, "deletion failed!");
	}
	return code;
}

int cmd_rnfr(char* para, session* sess) {

	if (para == NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}
	if (sess->login_status == unlogged ||
		sess->login_status == need_pass) {
		reply_form_msg(sess, 530);
		return 530;
	}

	if (strstr(para, "..") != NULL) {
		//不能有..
		reply_form_msg(sess, 530);
		return 530;
	}

	int code;
	char* dir = get_absolute_dir(sess->working_root, para, 1);
	if (dir != NULL) {
		sess->is_RNFR = 1;
		sprintf(sess->pre_msg_content, "%s", dir);
		code = 350;
		reply_custom_msg(sess, 350, "the file exists.");
		free(dir);
	}
	else {
		code = 550;
		reply_custom_msg(sess, 350, "the file doesn't exist.");
	}
	return code;
}

int cmd_rnto(char* para, session* sess) {

	if (para == NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}
	if (sess->login_status == unlogged ||
		sess->login_status == need_pass) {
		reply_form_msg(sess, 530);
		return 530;
	}
	if (sess->is_RNFR != 1) {
		reply_form_msg(sess, 503);
		return 503;
	}

	if (strstr(para, "..") != NULL) {
		//不能有..
		reply_form_msg(sess, 530);
		return 530;
	}
	
	int code;
	sess->is_RNFR = 0;
	char* dir = get_absolute_dir(sess->working_root, para, -1);
	if (dir != NULL) {
		//原文件位置为sess->pre_msg_content
		char* shell_cmd = (char*)malloc(512);
		sprintf(shell_cmd, "mv %s %s", sess->pre_msg_content, dir);
		if (system(shell_cmd) == -1) {
			printf("Error running shell: %s(%d)\n", strerror(errno), errno);
			reply_custom_msg(sess, 550, "mv command failed.");
			code = 550;
		}
		else {
			reply_custom_msg(sess, 550, "mv command successful.");
			code = 250;
		}
		free(shell_cmd);
		free(dir);
	}
	else {
		reply_custom_msg(sess, 550, "failed, maybe directory wrong?");
		code = 550;
	}
	return code;
}

//data connection relative
int cmd_pasv(char* para, session* sess) {
	if (para != NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}
	if (sess->login_status == unlogged ||
		sess->login_status == need_pass) {
		reply_form_msg(sess, 530);
		return 530;
	}

	// 如果已经有，则直接关闭
	if (sess->pasv_lis_fd > 0) close(sess->pasv_lis_fd);
	if (sess->data_fd > 0) close(sess->data_fd);

	//获取本机IP
	int h1,h2,h3,h4,p1,p2;
	if (sscanf(server_ip, "%d.%d.%d.%d", &h1, &h2, &h3, &h4) != 4) {
		reply_form_msg(sess, 500);
		return 500;
	}
	int temp_port = rand() % 45535 + 20000;
	p1 = temp_port / 256;
	p2 = temp_port % 256;
	char reply_msg[128];
	sprintf(reply_msg, "=%d,%d,%d,%d,%d,%d", h1,h2,h3,h4,p1,p2);

    struct sockaddr_in addr;
    //创建socket
    if ((sess->pasv_lis_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		reply_form_msg(sess, 500);
        return 500;
    }

    //设置本机的ip和port
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(temp_port);
	//printf("temp port: %d\n", temp_port);
    //监听任何来源
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //将本机的ip和port与socket绑定
    if (bind(sess->pasv_lis_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		reply_form_msg(sess, 500);
        return 500;
    }

    //开始监听socket
    if (listen(sess->pasv_lis_fd, 1) == -1) {
        printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		reply_form_msg(sess, 500);
        return 500;
    }

    sess->current_pasv = 1;
    reply_custom_msg(sess, 227, reply_msg);
	return 227;
}

int cmd_port(char* para, session* sess) {
	if (para == NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}
	if (sess->login_status == unlogged ||
		sess->login_status == need_pass) {
		reply_form_msg(sess, 530);
		return 530;
	}

	int h1,h2,h3,h4,p1,p2;
	char check_s;
	int count = 0;

	count = sscanf(para, "%d,%d,%d,%d,%d,%d %c", &h1, &h2, &h3, &h4, &p1, &p2, &check_s);
	if (count != 6) {
		reply_form_msg(sess, 501);
		return 501;
	}

	//检验格式问题
	if (h1 < 0 || h1 > 255 || h2 < 0 || h2 > 255 || h3 < 0 || h3 > 255 ||
		h4 < 0 || h4 > 255 || p1 < 0 || p1 > 255 || p2 < 0 || p2 > 255){
		reply_form_msg(sess, 501);
		return 501;
	}

	char ip_str[128];
	//已经有socket，则断开连接
	if (sess->data_fd > 0) close(sess->data_fd);
	//创建socket
	if ((sess->data_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		reply_form_msg(sess, 500);
		return 500;
	}

	//设置客户端发送的IP
	memset(&(sess->client_addr), 0, sizeof(sess->client_addr));
	sess->client_addr.sin_family = AF_INET;
	sess->client_addr.sin_port = htons(p1*256 + p2);

	sprintf(ip_str, "%d.%d.%d.%d", h1, h2, h3, h4);
	//转换ip地址:点分十进制-->二进制
	if (inet_pton(AF_INET, ip_str, &(sess->client_addr.sin_addr)) <= 0) {
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		reply_form_msg(sess, 500);
		return 500;
	}
	
	//成功，修改目前的数据传输模式
	sess->current_pasv = 0;
	reply_custom_msg(sess, 200, "Okay.");
	return 200;
}

int cmd_list(char* para, session* sess) {
	
	if (sess->login_status == unlogged ||
		sess->login_status == need_pass) {
		reply_form_msg(sess, 530);
		return 530;
	}
	
	if (para != NULL) {
		char* abs_dir = get_absolute_dir(sess->working_root, para, 1);
		if (abs_dir == NULL) {
			reply_custom_msg(sess, 451, "failed, uncorrect pathname.");
			return 451;
		}
		else {
			reply_custom_msg(sess, 150, "Begin listing.");
			int temp_code = reply_list(sess, abs_dir);
			reply_form_msg(sess, temp_code);
			free(abs_dir);
			return temp_code;
		}
	}
	else {
		reply_custom_msg(sess, 150, "Begin listing.");
		int temp_code = reply_list(sess, sess->working_root);
		reply_form_msg(sess, temp_code);
		return temp_code;
	}
}

int cmd_retr(char* para, session* sess) {
	if (para == NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}
	if (sess->login_status == unlogged ||
		sess->login_status == need_pass) {
		reply_form_msg(sess, 530);
		return 530;
	}

	reply_custom_msg(sess, 150, "Begin retrieve the given file.");

	char* abs_dir = get_absolute_dir(sess->working_root, para, 1);
	if (abs_dir == NULL) {
		reply_custom_msg(sess, 451, "failed, file not exist.");
		return 451;
	}

	int file = open(abs_dir, O_RDONLY);
	if (file == -1) {
		// 没有权限
		reply_custom_msg(sess, 451, "failed, no permission.");
		return 451;
	}

	int temp_code = retrieve_file(sess, file);
	reply_form_msg(sess, temp_code);
	return temp_code;
	// accepts 226 if the entire file was successfully written to the server's TCP buffers;
	// rejects 425 if no TCP connection was established;
	// rejects 426 if the TCP connection was established but then broken by the client or by network failure; or
	// rejects 451 or 551 if the server had trouble reading the file from disk.
}

int cmd_stor(char* para, session* sess) {
	if (para == NULL) {
		reply_form_msg(sess, 504);
		return 504;
	}
	if (sess->login_status == unlogged ||
		sess->login_status == need_pass) {
		reply_form_msg(sess, 530);
		return 530;
	}

	char* abs_dir = get_absolute_dir(sess->working_root, para, -1);
	if (abs_dir == NULL) {
		reply_custom_msg(sess, 451, "failed, uncorrect pathname.");
		return 451;
	}

	int file = open(abs_dir, O_WRONLY | O_CREAT | O_TRUNC);
	if (file == -1) {
		// 没有权限
		reply_custom_msg(sess, 451, "failed, no permission.");
		return 451;
	}

	reply_custom_msg(sess, 150, "Begin store the given file.");

	int temp_code = store_file(sess, file);
	reply_form_msg(sess, temp_code);
	return temp_code;
}

int reply_list(session* sess, char* dir) {
	//未建立任何连接或没有建立data套接字
	if (sess->current_pasv == -1) return 425;
	else if (sess->current_pasv == 0) {
		//port,服务器发起连接
		if (sess->data_fd <= 0) return 425;
		if (connect(sess->data_fd, (struct sockaddr*)&sess->client_addr,
				sizeof(sess->client_addr)) < 0) {
			printf("Error connect(): %s(%d)\n", strerror(errno), errno);
			return 426;
		}
	}
	else{
		//pasv模式,服务器等待连接
		if (sess->pasv_lis_fd <= 0) return 425;
		//printf("开始pasv");
		unsigned int size_sock = sizeof(struct sockaddr);
		if ((sess->data_fd = accept(sess->pasv_lis_fd,
				(struct sockaddr *)&(sess->client_addr),&size_sock)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            return 426;
        }
        //printf("Receive data connection from:%s:%hu\n", 
			//   inet_ntoa(sess->client_addr.sin_addr), ntohs(sess->client_addr.sin_port));
    }

    //开始处理
    char* list_res = (char*)malloc(4096);
    struct stat dir_stat;
    DIR *dir_p;
    struct dirent *dp;
    int offset = 0;

    offset += sprintf(list_res, "%s", "   name   |   type   |   size   |   modified time\r\n");
	
	//获取目录属性失败
    if (stat(dir, &dir_stat) < 0) return 451;

    if (S_ISREG(dir_stat.st_mode)) {
    	//是一个普通文件
    	sprintf(list_res + offset, "%s", file_info(&dir_stat, dir));
    }
    //是一个目录
    else if (S_ISDIR(dir_stat.st_mode)) {
        dir_p = opendir(dir);
        while ((dp = readdir(dir_p)) != NULL) {
            // 忽略 . 和 ..
            if (strcmp(".", dp->d_name) == 0 ) continue;
            if (strcmp("..", dp->d_name) == 0 ) continue;
			
			char* temp_info;
			temp_info = file_info(&dir_stat, dp->d_name);
    		offset += sprintf(list_res + offset, "%s", temp_info);
			free(temp_info);
        }
        closedir(dir_p);
    }
    else return 451;

	send(sess->data_fd, list_res, strlen(list_res), MSG_WAITALL);
	free(list_res);
	//传输完成
	//sleep(0.01);
	close(sess->data_fd);
	if (sess->current_pasv == 1) {
		//pasv模式
		close(sess->pasv_lis_fd);
	}
	sess->current_pasv = -1;
	return 226;
}

int retrieve_file(session* sess, int file){
	//未建立任何连接或没有建立data套接字
	if (sess->current_pasv == -1) return 425;
	else if (sess->current_pasv == 0) {
		//port,模式,服务器发起连接,此时已经存在data_fd
		if (sess->data_fd <= 0) return 425;
		if (connect(sess->data_fd, (struct sockaddr*)&sess->client_addr,
				sizeof(sess->client_addr)) < 0) {
			printf("Error connect(): %s(%d)\n", strerror(errno), errno);
			return 426;
		}
	}
	else{
		//pasv模式,此时已经存在lis_fd
		if (sess->pasv_lis_fd <= 0) return 425;
		unsigned int size_sock = sizeof(struct sockaddr);
		if ((sess->data_fd = accept(sess->pasv_lis_fd,
				(struct sockaddr *)&(sess->client_addr),&size_sock)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            return 426;
        }
    }
	struct stat file_stat;
	fstat(file, &file_stat);

	long long bytes_to_send = file_stat.st_size;
	// printf("total: %lld", bytes_to_send);
	while(bytes_to_send) {
		int temp_size = bytes_to_send > 4096 ? 4096 : bytes_to_send;
		int sent_size = sendfile(sess->data_fd, file, NULL, temp_size);
		// printf("sent: %d", sent_size);
		if (sent_size == -1) return 426;
		bytes_to_send -= sent_size;
	}

	//传输完成,关闭连接
	close(sess->data_fd);
	if (sess->current_pasv == 1) {
		//pasv模式
		close(sess->pasv_lis_fd);
	}
	close(file);
	sess->current_pasv = -1;
	return 226;
}

int store_file(session* sess, int file) {

	//未建立任何连接或没有建立data套接字
	if (sess->current_pasv == -1) return 425;

	else if (sess->current_pasv == 0) {
		//port,服务器发起连接
		if (sess->data_fd <= 0) return 425;
		if (connect(sess->data_fd, (struct sockaddr*)&sess->client_addr,
				sizeof(sess->client_addr)) < 0) {
			printf("Error connect(): %s(%d)\n", strerror(errno), errno);
			return 426;
		}
	}
	else{
		//pasv模式,服务器等待连接
		if (sess->pasv_lis_fd <= 0) return 425;
		unsigned int size_sock = sizeof(struct sockaddr);
		if ((sess->data_fd = accept(sess->pasv_lis_fd,
				(struct sockaddr *)&(sess->client_addr),&size_sock)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            return 426;
        }
    }

    //开始接收
    int temp_size;
    while (1) {
    	temp_size = sendfile(file, sess->data_fd, NULL, 4096);
    	if (!temp_size) break;
    }
	//传输完成
	close(sess->data_fd);
	if (sess->current_pasv == 1) {
		//pasv模式
		close(sess->pasv_lis_fd);
	}
	close(file);
	sess->current_pasv = -1;
	return 226;
}