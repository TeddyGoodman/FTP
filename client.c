#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"

int data_lis_port;
int is_pasv;
struct sockaddr_in server_addr;
int data_fd;

int cmd_port(char* para){
	is_pasv = 0;
	int h1,h2,h3,h4,p1,p2;
	sscanf(para, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
	struct sockaddr_in addr;

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
	return 0;
}

int cmd_pasv(char* sentence) {
	is_pasv = 1;
	int h1,h2,h3,h4,p1,p2;
	int i = 0;
	while(sentence[i] != '=') i++;
	sscanf(sentence + i, "=%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
	//struct sockaddr_in addr;
	
	//创建socket
	if ((data_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//设置目标主机的ip和port
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(p1 * 256 + p2);
	
	//转换ip地址:点分十进制-->二进制
	if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	return 0;
}

int cmd_retr() {
	char* buff = (char*)malloc(4096);
	if (is_pasv == 0) {
		struct sockaddr_in client_fd;
		unsigned int size_sock = sizeof(struct sockaddr_in);
		if ((data_fd = accept(data_lis_port, (struct sockaddr *) &client_fd, &size_sock)) == -1) {
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			free(buff);
			return 1;
		}
		printf("FROM DATA Connect:\n");
		while (1) {
			int size = read(data_fd, buff, 4096);
			if (size != 0)
				printf("%s", buff);
			else break;
		}
		free(buff);
		return 0;
	}
	else {
		printf("FROM DATA Connect:\n");
		//连接上目标主机（将socket和目标主机连接）-- 阻塞函数
		if (connect(data_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
			printf("Error connect(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}
		//int total = 0;
		while (1) {
			int size = read(data_fd, buff, 4096);
			if (size != 0){
				//total += size;
				printf("%s", buff);
			}
			else break;
		}
		//printf("got %d\n", total);
		free(buff);
		return 0;
	}
}

int main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in addr;
	char sentence[8192];
	int len;

	//创建socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//设置目标主机的ip和port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(21);
	if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {			//转换ip地址:点分十进制-->二进制
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	//连接上目标主机（将socket和目标主机连接）-- 阻塞函数
	if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	int m = recv(sockfd, sentence, 8192, 0);
	if (!m) return 1;
	sentence[m] = '\0';
	printf("%s", sentence);

	int code;
	
	char* cmd = (char*)malloc(256);
	char* para = (char*)malloc(256);
	//char* buff = (char*)malloc(4096);
    //unsigned int size_sock = sizeof(struct sockaddr);
	//struct sockaddr_in client_fd;

	while (1) {
		printf("my_ftpclient > ");
		//获取键盘输入
		fgets(sentence, 4096, stdin);
		len = strlen(sentence);
		sentence[len - 1] = '\r';
		sentence[len] = '\n';
		sentence[len + 1] = '\0';
		len++;

		send(sockfd, sentence, len, MSG_WAITALL);
		// printf("msg sent to the server.\n");
			
		sscanf(sentence, "%s %s", cmd, para);
		
		if (strcmp(cmd, "PORT") == 0) {
			cmd_port(para);
		}
		else if (strcmp(cmd, "STOR") == 0) {
			break;
		}
		
		int m = recv(sockfd, sentence, 8192, 0);
		// printf("read from server: %d\n", m);
		sentence[m] = '\0';

		printf("FROM SERVER: %s", sentence);

		if (strcmp(cmd, "PASV") == 0) {
			cmd_pasv(sentence);
			//int m = recv(sockfd, sentence, 8192, 0);
			//sentence[m] = '\0';
			//printf("FROM SERVER: %s", sentence);
		}
		else if (strcmp(cmd, "LIST") == 0) {
			cmd_retr();
			int m = recv(sockfd, sentence, 8192, 0);
			sentence[m] = '\0';
			printf("FROM SERVER: %s", sentence);
		}
		else if (strcmp(cmd, "RETR") == 0) {
			cmd_retr();
			int m = recv(sockfd, sentence, 8192, 0);
			sentence[m] = '\0';
			printf("FROM SERVER: %s", sentence);
		}
		if (data_fd > 0) close(data_fd);

		if (sscanf(sentence, "%d", &code) != 0)
			if (code == 221) break;
	}

	close(sockfd);

	return 0;
}
