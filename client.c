#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
//#include <sys/sendfile.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <memory.h>
#include <pthread.h>
#include <time.h>
#include "client.h"
#include "utility.h"

/*
* 在port模式下接受连接
*/
int port_connect_data() {
	//port模式
	struct sockaddr_in client_fd;
	unsigned int size_sock = sizeof(struct sockaddr_in);
	if ((data_fd = accept(data_lis_port, (struct sockaddr *) &client_fd, &size_sock)) == -1) {
		printf("Error accept(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	return 0;
}

/*
* 在pasv模式下开始连接
*/
int pasv_connect_data() {
	if (connect(data_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	return 0;
}

/*
* 初始化客户端
*/
int client_init() {
	struct sockaddr_in addr;

	//创建socket
	if ((control_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//设置目标主机的ip和port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(control_port);
	//转换ip地址:点分十进制-->二进制
	if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//连接上目标主机（将socket和目标主机连接）-- 阻塞函数
	if (connect(control_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	return 0;
}

/*
* port模式，客户端创建套接字data_lis_port，监听所选的端口
* 这里的addr是临时的，（监听来源严格一点应该是服务器端IP）
*/
int cmd_port(char* para){
	int h1,h2,h3,h4,p1,p2;
	if (sscanf(para, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
		printf("input error\n");
		return 1;
	}
	struct sockaddr_in addr;//临时设置的地址，为bind准备

	if (data_lis_port > 0){
		close(data_lis_port);
		data_lis_port = 0;
	}

	//创建socket
	if ((data_lis_port = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	//设置本机的ip和port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(p1*256 + p2);
	//监听任何来源
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//将本机的ip和port与socket绑定
	if (bind(data_lis_port, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//开始监听socket
	if (listen(data_lis_port, 1) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}
	is_pasv = 0;
	return 0;
}

/*
* Passive 模式，服务器发过来其地址，将其保存在server_addr
* 创建data_fd作为数据端口的套接字
* 服务器测试结果：PASV结束后要去连接服务器
*/
int cmd_pasv(char* sentence) {
	int h1,h2,h3,h4,p1,p2;
	int i = 0;
	int len = strlen(sentence);
	int ret_code;
	if (sscanf(sentence, "%d", &ret_code) != 1) return 1;
	if (ret_code != 227) {
		printf("ret code not 227\n");
		return 1;
	}

	//第一个空格
	while (i < len) {
		if (sentence[i] == ' ') break;
		else i++;
	}
	if (i == len) {
		printf("Error with server's reply: %s\n", sentence);
		return 1;
	}
	//之后的第一个数字
	while (i < len) {
		if (sentence[i] >= '0' && sentence[i] <= '9') break;
		else i++;
	}
	if (i == len) {
		printf("Error with server's reply: %s\n", sentence);
		return 1;
	}

	sscanf(sentence + i, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
	
	//创建socket
	if (data_fd > 0) {
		close(data_fd);
		data_fd = 0;
	}

	if ((data_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//设置目标主机的ip和port
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(p1 * 256 + p2);

	//转换ip地址:点分十进制-->二进制
	if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	if (pasv_connect_data()) return 1;
	is_pasv = 1;
	return 0;
}

/*
* 用于多线程接口
*/
void* download_file_pthread(void* ptr) {
	download_file(*(int*)ptr);
	return NULL;
}

/*
* 用于多线程
*/
void* upload_file_pthread(void *ptr) {
	upload_file(*(int*)ptr);
	return NULL;
}

/*
* 下载文件
*/
void download_file(int file) {
    //开始接收
    int temp_size;
    char* buff = (char*)malloc(4096);
    while (1) {
    	temp_size = read(data_fd, buff, 4096);
    	write(file, buff, temp_size);
    	if (temp_size == 0) break;
    	
    	// if (temp_size < 4096) {
    	// 	break;
    	// }
    }
	//传输完成
	close(data_fd);
	data_fd = 0;
	if (is_pasv == 0) {
		//port模式
		close(data_lis_port);
		data_lis_port = 0;
	}
	close(file);
	free(buff);
	return;
}

/*
* 上传文件函数
*/
void upload_file(int file) {

	struct stat file_stat;
	fstat(file, &file_stat);

	long long bytes_to_send = file_stat.st_size;

    char* buff = (char*)malloc(4096);

    //for linux
	// while(bytes_to_send) {
	// 	int temp_size = bytes_to_send > 4096 ? 4096 : bytes_to_send;
	// 	int sent_size = sendfile(data_fd, file, NULL, temp_size);
	// 	if (sent_size == -1) return;
	// 	bytes_to_send -= sent_size;
	// }

	//for macos
	while (bytes_to_send) {
		int temp_size = bytes_to_send > 4096 ? 4096 : bytes_to_send;
    	int read_size = read(file, buff, temp_size);
    	if (read_size) {
    		write(data_fd, buff, read_size);
    	}
    	bytes_to_send -= read_size;
    }
    free(buff);

	//传输完成,关闭连接
	close(data_fd);
	data_fd = 0;
	if (is_pasv == 0) {
		//pasv模式
		close(data_lis_port);
		data_lis_port = 0;
	}
	close(file);
	return;
}

/*
* 显示list函数，多线程，传输给端口
*/
void* show_list() {
	char* buff = (char*)malloc(4096);
	while (1) {
		int size = read(data_fd, buff, 4096);
		if (size < 4096){
			buff[size] = '\0';
			printf("%s", buff);
			break;
		}
		else printf("%s\n", buff);
	}
	free(buff);

	close(data_fd);
	data_fd = 0;
	if (is_pasv == 0) {
		//pasv模式
		close(data_lis_port);
		data_lis_port = 0;
	}

	return NULL;
}

/*
* retr指令，此时要开始从data端口接收东西
* 但是需要使用多线程形式
*/
int cmd_retr(char* sentence, char* para) {
	int ret_code;
	if (sscanf(sentence, "%d", &ret_code) != 1) return 1;
	if (ret_code != 150) {
		printf("ret code not 150\n");
		return 1;
	}

	if (is_pasv == -1) {
		printf("no connection made.\n");
		return 1;
	}

	if (is_pasv == 0) {
		//port 模式，需要此时去连接
		if (port_connect_data()) return 1;
	}

	//打开文件
	char* file_root = get_absolute_dir(root_directory, para, -1);
	int file = open(file_root, O_WRONLY | O_CREAT | O_TRUNC,
		S_IRWXU | S_IXGRP | S_IROTH | S_IXOTH | S_IRGRP);
	if (file == -1) {
		printf("No such file\n");
		return 1;
	}
	free(file_root);

	//开始下载
	pthread_t thid;
	pthread_create(&thid, NULL, (void*)download_file_pthread, (void*)&file);

	int m = recv(control_fd, sentence, 8192, 0);
	sentence[m] = '\0';
	printf("FROM SERVER: %s", sentence);

	sscanf(sentence, "%d", &ret_code);

	if (ret_code == 226)
		pthread_join(thid, NULL);
	else {
		pthread_cancel(thid);
		close(data_fd);
		data_fd = 0;
		if (is_pasv == 0) {
			//pasv模式
			close(data_lis_port);
			data_lis_port = 0;
		}
		close(file);
	}
	is_pasv = -1;
	return 0;
}

/*
* stor指令，和retr相似
*/
int cmd_stor(char* sentence, char* para) {

	int ret_code;
	if (sscanf(sentence, "%d", &ret_code) != 1) return 1;
	if (ret_code != 150) {
		printf("ret code not 150\n");
		return 1;
	}

	if (is_pasv == -1) {
		printf("no connection made.\n");
		return 1;
	}

	if (is_pasv == 0) {
		//port 模式，需要此时去连接
		if (port_connect_data()) return 1;
	}

	//打开文件
	char* file_root = get_absolute_dir(root_directory, para, -1);
	int file = open(file_root, O_RDONLY);
	free(file_root);

	if (file <= 0) {
		close(data_fd);
		data_fd = 0;
		if (is_pasv == 0) {
			//pasv模式
			close(data_lis_port);
			data_lis_port = 0;
		}
		close(file);
	}

	//开始上传
	pthread_t thid;
	pthread_create(&thid, NULL, (void*)upload_file_pthread, (void*)&file);

	int m = recv(control_fd, sentence, 8192, 0);
	sentence[m] = '\0';
	printf("FROM SERVER: %s", sentence);

	sscanf(sentence, "%d", &ret_code);
	
	if (ret_code == 226)
		pthread_join(thid, NULL);
	else {
		pthread_cancel(thid);
		close(data_fd);
		data_fd = 0;
		if (is_pasv == 0) {
			//pasv模式
			close(data_lis_port);
			data_lis_port = 0;
		}
		close(file);
	}
	is_pasv = -1;
	return 0;
}

/*
* list指令的主控制
*/
int cmd_list(char* sentence) {
	int ret_code;
	if (sscanf(sentence, "%d", &ret_code) != 1) return 1;
	if (ret_code != 150) {
		printf("ret code not 150\n");
		return 1;
	}
	if (is_pasv == -1) {
		printf("no connection made.\n");
		return 1;
	}
	if (is_pasv == 0) {
		//port 模式，需要此时去连接
		if (port_connect_data()) return 1;
	}

	pthread_t thid;
	pthread_create(&thid, NULL, (void*)show_list, NULL);

	int m = recv(control_fd, sentence, 8192, 0);
	sentence[m] = '\0';
	printf("FROM SERVER: %s", sentence);

	sscanf(sentence, "%d", &ret_code);
	
	if (ret_code == 226)
		pthread_join(thid, NULL);
	else {
		pthread_cancel(thid);
		close(data_fd);
		data_fd = 0;
		if (is_pasv == 0) {
			//pasv模式
			close(data_lis_port);
			data_lis_port = 0;
		}
	}
	is_pasv = -1;
	return 0;
}

int main(int argc, char **argv) {
	if (client_init()) return 1;

	char sentence[8192];
	int len;

	//初始连接成功的提示
	int m = recv(control_fd, sentence, 8192, 0);
	if (!m){
		printf("Not getting message from server\n");
		return 1;
	}
	sentence[m] = '\0';
	printf("%s", sentence);

	char* cmd = (char*)malloc(256);
	char* para = (char*)malloc(256);

	while (1) {
		printf("my_ftpclient-> ");
		//获取键盘输入
		fgets(sentence, 4096, stdin);
		len = strlen(sentence);
		if (len < 2) {
			printf("no input given.\n");
			continue;
		}
		sentence[len - 1] = '\r';
		sentence[len] = '\n';
		sentence[len + 1] = '\0';
		len++;

		sscanf(sentence, "%s %s", cmd, para);
		
		if (strcmp(cmd, "PORT") == 0) {
			//首先根据输入开始监听，如果错误则不会发消息
			if (cmd_port(para)) continue;
			//send msg to server
			send(control_fd, sentence, len, MSG_WAITALL);

			int m = recv(control_fd, sentence, 8192, 0);
			sentence[m] = '\0';
			printf("FROM SERVER: %s", sentence);
		}
		else if (strcmp(cmd, "PASV") == 0) {
			//pasv模式，首先发给服务器
			send(control_fd, sentence, len, MSG_WAITALL);

			int m = recv(control_fd, sentence, 8192, 0);
			sentence[m] = '\0';
			printf("FROM SERVER: %s", sentence);

			cmd_pasv(sentence);
		}
		else if (strcmp(cmd, "RETR") == 0) {
			//下载文件，首先直接将消息发给服务器
			send(control_fd, sentence, len, MSG_WAITALL);

			int m = recv(control_fd, sentence, 8192, 0);
			sentence[m] = '\0';
			printf("FROM SERVER: %s", sentence);

			if (cmd_retr(sentence, para)) {
				int m = recv(control_fd, sentence, 8192, 0);
				sentence[m] = '\0';
				printf("FROM SERVER: %s", sentence);
			}
		}
		else if (strcmp(cmd, "STOR") == 0) {
			//上传文件，首先直接将消息发给服务器
			send(control_fd, sentence, len, MSG_WAITALL);

			int m = recv(control_fd, sentence, 8192, 0);
			sentence[m] = '\0';
			printf("FROM SERVER: %s", sentence);

			if (cmd_stor(sentence, para))  {
				int m = recv(control_fd, sentence, 8192, 0);
				sentence[m] = '\0';
				printf("FROM SERVER: %s", sentence);
			}
		}
		else if (strcmp(cmd, "LIST") == 0) {
			//首先直接将消息发给服务器
			send(control_fd, sentence, len, MSG_WAITALL);

			int m = recv(control_fd, sentence, 8192, 0);
			sentence[m] = '\0';
			printf("FROM SERVER: %s", sentence);

			if (cmd_list(sentence)) {
				int m = recv(control_fd, sentence, 8192, 0);
				sentence[m] = '\0';
				printf("FROM SERVER: %s", sentence);
			}
		}
		else {
			//正常情况，直接显示服务器返回的消息
			//首先直接将消息发给服务器
			send(control_fd, sentence, len, MSG_WAITALL);

			int ret_code;
			int m = recv(control_fd, sentence, 8192, 0);
			sentence[m] = '\0';
			printf("FROM SERVER: %s", sentence);
			if (sscanf(sentence, "%d", &ret_code) != 0)
				if (ret_code == 221) break;
		}
	}

	free(cmd);
	free(para);
	close(control_fd);

	return 0;
}
