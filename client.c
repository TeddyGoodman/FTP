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
	int listenfd;
	
	char* cmd = (char*)malloc(256);
	char* para = (char*)malloc(256);
	char* buff = (char*)malloc(4096);
    unsigned int size_sock = sizeof(struct sockaddr);
	struct sockaddr_in client_fd;
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
			int h1,h2,h3,h4,p1,p2;
			sscanf(para, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
			struct sockaddr_in addr;

			//创建socket
			if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
				printf("Error socket(): %s(%d)\n", strerror(errno), errno);
				break;
			}

			//设置本机的ip和port
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(p1*256 + p2);
			//监听任何来源
			addr.sin_addr.s_addr = htonl(INADDR_ANY);

			//将本机的ip和port与socket绑定
			if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
				printf("Error bind(): %s(%d)\n", strerror(errno), errno);
				break;
			}

			//开始监听socket
			if (listen(listenfd, 1) == -1) {
				printf("Error listen(): %s(%d)\n", strerror(errno), errno);
				break;
			}
		}
		else if (strcmp(cmd, "RETR") == 0) {
			int connfd;
			if ((connfd = accept(listenfd, (struct sockaddr *) &client_fd, &size_sock)) == -1) {
				printf("Error accept(): %s(%d)\n", strerror(errno), errno);
				continue;
			}
			while (1) {
				int size = read(connfd, buff, sizeof(buff));
				if (size != 0)
					printf("%s\n", buff);
				else break;
			}
			continue;
		}
		
		int m = recv(sockfd, sentence, 8192, 0);
		// printf("read from server: %d\n", m);
		sentence[m] = '\0';

		printf("FROM SERVER: %s", sentence);

		if (sscanf(sentence, "%d", &code) != 0)
			if (code == 221) break;
	}

	close(sockfd);

	return 0;
}
