#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <getopt.h>
#include "server.h"
#include <ctype.h>
#include <stdlib.h>

int server_init() {
    struct sockaddr_in addr;

    //创建socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    //设置本机的ip和port
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(lis_port);
    //监听任何来源
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //将本机的ip和port与socket绑定
    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        printf("Error bind(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    //开始监听socket
    if (listen(listenfd, MAX_Client) == -1) {
        printf("Error listen(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    return 0;
}

void serve_client(int client_fd) {
    int connfd = client_fd;
    char sentence[8192];
    int p;
    int len;
    //持续监听连接请求
    while (1) {
        //榨干socket传来的内容
        p = 0;
        while (1) {
            int n = read(connfd, sentence + p, 8191 - p);
            if (n < 0) {
                printf("Error read(): %s(%d)\n", strerror(errno), errno);
                close(connfd);
                continue;
            } else if (n == 0) {
                break;
            } else {
                p += n;
                if (sentence[p - 1] == '\n') {
                    break;
                }
            }
        }
        //socket接收到的字符串并不会添加'\0'
        sentence[p - 1] = '\0';
        len = p - 1;
        
        //字符串处理
        for (p = 0; p < len; p++) {
            sentence[p] = toupper(sentence[p]);
        }

        //发送字符串到socket
        p = 0;
        while (p < len) {
            int n = write(connfd, sentence + p, len + 1 - p);
            if (n < 0) {
                printf("Error write(): %s(%d)\n", strerror(errno), errno);
                return;
            } else {
                p += n;
            }           
        }

        printf("write done\n");
    }
    close(connfd);
}

int main(int argc, char const *argv[])
{
    //解析命令行参数
    int opt;
    char check_s;
    struct sockaddr_in client_fd;
    int connfd;
    unsigned int size_sock = sizeof(struct sockaddr);

    const struct option argu_options[] = {

        {"port", required_argument, NULL, 'p'},

        {"root", required_argument, NULL, 'r'},

        {NULL, 0 ,NULL, 0},
    };

    //逐一读取参数
    while((opt = getopt_long_only(argc, (char *const *)argv,    
            "p:r:", argu_options, NULL)) != -1)
    {
        switch (opt) {
            case 'r':
                if (access(optarg, 0)) {
                    printf("wrong path given: %s.", optarg);
                    return 1;
                }
                strcpy(file_root, optarg);
                break;
            case 'p':
                if (sscanf(optarg, "%hd%c", &lis_port, &check_s) != 1) {
                    printf("wrong port given: %s.\n", optarg);
                    return 1;
                }
                break;
            case '?':
                printf("wrong argument.\n");
                return 1;
        }
    }

    if (!(*file_root))
        sprintf(file_root, "/tmp");
    printf("The root directory set to: %s.\n", file_root);
    printf("The listening port set to: %d.\n", lis_port);

    // Ftp init
    if (server_init()) return 1;


    //waiting for connection
    while (1) {
        //等待client的连接 -- 阻塞函数
        if ((connfd = accept(listenfd, (struct sockaddr *) &client_fd, &size_sock)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            continue;
        }
        printf("Receive connection from:%s:%hu\n", inet_ntoa(client_fd.sin_addr), ntohs(client_fd.sin_port));
        serve_client(connfd);
    }

    close(listenfd);

    return 0;
}