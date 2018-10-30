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
#include <time.h>
#include <pthread.h>
#include "server.h"
#include "utility.h"
#include "command.h"
#include "session.h"

int server_init() {
    srand((unsigned int)time(NULL));
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
        pthread_t thid;
        pthread_create(&thid, NULL, (void*)serve_client_pthread, (void*)&connfd);
        pthread_detach(thid);
    }

    close(listenfd);

    return 0;
}

void* serve_client_pthread(void* ptr) {
    serve_client(*(int*)ptr);
    return NULL;
}

void serve_client(int client_fd) {
    session client_sess;
    init_session(&client_sess, client_fd, file_root);

    int count = 0; //计算sscnaf取到的数量
    int len; //接受消息的长度
    char check_s; //检测输入长度
    int code; //返回的code

    char* command = (char*)malloc(256);
    char* parameter = (char*)malloc(256);

    //向客户端发送服务器已经准备好的消息
    reply_custom_msg(&client_sess, 220, "FTP server ready.");

    //客户端发送的消息
    while (1) {
        count = 0;

        len = recv(client_fd, client_sess.sentence, 8192, 0);
        //长度小于2代表断开连接
        if (len < 2) break;

        //对消息进行预处理
        client_sess.sentence[len] = '\0';
        remove_enter(client_sess.sentence);

        //对内容进行匹配
        count = sscanf(client_sess.sentence, "%s %s %c", command, parameter, &check_s);

        if (count < 0) {
            printf("None string read.\n");
            reply_custom_msg(&client_sess, 500, "No Verb found.");
            continue;
        }
        else if (count == 1) {
            //处理无参数指令
            code = dispatch_cmd(command, NULL, &client_sess);
        }
        else if (count == 2){
            //处理有参数指令
            code = dispatch_cmd(command, parameter, &client_sess);
        }
        else {
            //此处可能需要处理文件名带有空格的指令
            printf("read more than 2.\n");
            reply_custom_msg(&client_sess, 501, "too many parameters.");
            continue;
        }

        //code为221则断开连接
        if (code == 221) break;
    }

    printf("connection closed.\n");

    //释放空间
    free(command);
    free(parameter);
    close_session(&client_sess);

    close(client_fd);
    return;
}


int dispatch_cmd(char* cmd, char* para, session* sess) {
    int code;

    //总体判断,修改is_RNFR
    if (sess->is_RNFR && strcmp(cmd, "RNTO") != 0) {
        sess->is_RNFR = 0;
    }
    if (sess->login_status == need_pass && strcmp(cmd, "PASS") != 0) {
        sess->login_status = unlogged;
    }

    //注意判断支持的指令
    if (strcmp(cmd, "USER") == 0) {
        code = cmd_user(para, sess);
    }
    else if (strcmp(cmd, "PASS") == 0) {
        code = cmd_pass(para, sess);
    }
    else if (strcmp(cmd, "QUIT") == 0) {
        code = cmd_quit(para, sess);
    }
    else if (strcmp(cmd, "TYPE") == 0) {
        code = cmd_type(para, sess);
    }
    else if (strcmp(cmd, "SYST") == 0) {
        code = cmd_syst(para, sess);
    }
    else if (strcmp(cmd, "CWD") == 0) {
        code = cmd_cwd(para, sess);
    }
    else if (strcmp(cmd, "PWD") == 0) {
        code = cmd_pwd(para, sess);
    }
    else if (strcmp(cmd, "MKD") == 0) {
        code = cmd_mkd(para, sess);
    }
    else if (strcmp(cmd, "RMD") == 0) {
        code = cmd_rmd(para, sess);
    }
    else if (strcmp(cmd, "RNFR") == 0) {
        code = cmd_rnfr(para, sess);
    }
    else if (strcmp(cmd, "RNTO") == 0) {
        code = cmd_rnto(para, sess);
    }
    else if (strcmp(cmd, "PASV") == 0) {
        code = cmd_pasv(para, sess);
    }
    else if (strcmp(cmd, "PORT") == 0) {
        code = cmd_port(para, sess);
    }
    else if (strcmp(cmd, "RETR") == 0) {
        code = cmd_retr(para, sess);
    }
	else if (strcmp(cmd, "STOR") == 0) {
		code = cmd_stor(para, sess);
	}
	else if (strcmp(cmd, "LIST") == 0) {
		code = cmd_list(para, sess);
	}
    else {
        code = 502;
        reply_form_msg(sess, code);
    }
    return code;
}
