#include "session.h"
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

/*
* 初始化会话
*/
int init_session(session* sess, int client_fd, char* global_file_root) {
	sess->sentence = (char*)malloc(8192);
	sess->working_root = (char*)malloc(512);
    sess->pre_msg_content = (char*)malloc(256);

	sess->login_status = unlogged;
	sess->is_RNFR = 0;
	
	sess->data_fd = 0;
	sess->pasv_lis_fd = 0;
	sess->client_fd = client_fd;
	sess->current_pasv = -1;

    sess->rest_ptr = 0;
    sess->is_transmitting = 0;
	
	sprintf(sess->working_root, "%s", global_file_root);
	return 0;
}

/*
* 关闭会话
*/
int close_session(session* sess) {
	free(sess->sentence);
	free(sess->working_root);
    free(sess->pre_msg_content);
	return 0;
}

//向客户端发送确定的消息
void reply_custom_msg(session* sess, int code, char* str) {
	sprintf(sess->sentence, "%d %s\r\n", code, str);
	int len = strlen(sess->sentence);
	send(sess->client_fd, sess->sentence, len, MSG_WAITALL);
	return;
}

/*
* 回复固定式的消息，有些code的消息类似，返回一样的即可
*/
void reply_form_msg(session* sess, int code) {
    switch (code) {
        case 500:
            sprintf(sess->sentence, "%d %s\r\n", code, "server meet with a error, please check your input.");
            break;
        case 502:
            sprintf(sess->sentence, "%d %s\r\n", code, "unsupported verb.");
            break;
        case 503:
            sprintf(sess->sentence, "%d %s\r\n", code, "this message doesn't match the previous one.");
            break;
        case 504:
            sprintf(sess->sentence, "%d %s\r\n", code, "parameter error, please check your parameter.");
            break;
        case 501:
            sprintf(sess->sentence, "%d %s\r\n", code, "wrong format of the parameter.");
            break;
        case 530:
            sprintf(sess->sentence, "%d %s\r\n", code, "permission denied.");
            break;
        case 226:
            sprintf(sess->sentence, "%d %s\r\n", code, "data connection finished.");
            break;
        case 425:
            sprintf(sess->sentence, "%d %s\r\n", code, "no TCP connection was established");
            break;
        case 426:
            sprintf(sess->sentence, "%d %s\r\n", code, "network failure, try again?");
            break;
        case 451:
            sprintf(sess->sentence, "%d %s\r\n", code, "read the file failed.");
            break;
        default:
            sprintf(sess->sentence, "%d %s\r\n", code, "this code didn't has a custom message.");
            break;
    }
    int len = strlen(sess->sentence);
    send(sess->client_fd, sess->sentence, len, MSG_WAITALL);
    return;
}
