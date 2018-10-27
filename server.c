#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <getopt.h>
#include <ctype.h>
#include <stdlib.h>
#include "server.h"
#include "utility.h"
#include "command.h"

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

    send_const_msg(client_fd, "220 FTP server ready.\r\n");

    char* sentence = (char*)malloc(8192);
    int count = 0;
    int len;
    char* command = (char*)malloc(1024);
    char* parameter = (char*)malloc(1024);
    char check_s;
    LoginStatus login = unlogged;
    int code;

    //客户端发送的消息
    while (1) {
        len = recv(client_fd, sentence, 8192, 0);
        //长度小于2代表断开连接
        if (len < 2) break;
        sentence[len] = '\0';
        remove_enter(sentence);
        count = sscanf(sentence, "%s %s %c", command, parameter, &check_s);

        if (count < 0) {
            printf("None string read.\n");
            send_const_msg(client_fd, "500 No Verb found.\r\n");
            continue;
        }
        else if (count == 1) {
            printf("read 1, command: %s\n", command);
            //处理无参数指令
            code = dispatch_cmd(command, NULL, &login);
            reply_msg(client_fd, code, sentence);
        }
        else if (count == 2){
            printf("read 2, command: %s, para: %s\n", command, parameter);
            //处理有参数指令
            code = dispatch_cmd(command, parameter, &login);
            reply_msg(client_fd, code, sentence);
        }
        else {
            printf("read more than 2.\n");
            send_const_msg(client_fd, "501 too many parameters.\r\n");
            continue;
        }
        if (code == 221) break;
        // send(client_fd, sentence, len, MSG_WAITALL);
    }

    printf("connection closed.\n");
    free(sentence);
    free(command);
    free(parameter);
    close(client_fd);
    return;
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

int dispatch_cmd(char* cmd, char* para, LoginStatus* login) {
    // 530 for permission is denied.
    int code;
    if (strcmp(cmd, "USER") == 0) {
        code = cmd_user(para, login);
    }
    else if (strcmp(cmd, "PASS") == 0) {
        code = cmd_pass(para, login);
    }
    else if (strcmp(cmd, "QUIT") == 0) {
        code = cmd_quit(para, login);
    }
    else {
        code = 502;
    }
    // if (*login == unlogged) {
    //     //未登录，只期望USER，其他的均为530
    //     code = 502;
    //     if (strcmp(cmd, "USER") == 0)
    //         
    // }
    // else if (*login == need_pass){
    //     //已经使用USER返回了331，期望使用PASS,pass只有在user之后会发起
    //     code = 530;
    //     if (strcmp(cmd, "PASS") == 0)
    //         code = cmd_pass(para, login);
    // }
    // else if (*login == logged) {
    //     //已经登录,默认502
    //     code = 502;
    //     if (strcmp(cmd, "PASS") == 0)
    //         code = cmd_pass(para, login);
    //     // else if (strcmp(cmd, ))
    // }
    // else {
    //     code = 500;
    // }
    return code;
}

void reply_msg(int client_fd, int code, char* sentence) {
    switch (code) {
        case 500:
            sprintf(sentence, "%d %s\r\n", code, "syntax error, please check your input.");
            break;
        case 502:
            sprintf(sentence, "%d %s\r\n", code, "unsupported verb.");
            break;
        case 503:
            sprintf(sentence, "%d %s\r\n", code, "wrong usage of PASS, should be after USER.");
            break;
        case 504:
            sprintf(sentence, "%d %s\r\n", code, "parameter error, please check your parameter.");
            break;
        case 501:
            sprintf(sentence, "%d %s\r\n", code, "wrong format of the parameter.");
            break;
        case 530:
            sprintf(sentence, "%d %s\r\n", code, "permission denied.");
            break;
        case 331:
        case 332:
            sprintf(sentence, "%d %s\r\n", code, "need password, please use PASS.");
            break;
        case 230:
            sprintf(sentence, "%d %s\r\n", code, "permission granted. welcome!");
            break;
        case 202:
            sprintf(sentence, "%d %s\r\n", code, "permission already granted. no need for this.");
            break;
        case 221:
            sprintf(sentence, "%d %s\r\n", code, "Bye.");
            break;
        default:
            sprintf(sentence, "%d %s\r\n", code, "this code didn't has a custom message.");
            break;
    }
    int len = strlen(sentence);
    send(client_fd, sentence, len, MSG_WAITALL);
    return;
}
