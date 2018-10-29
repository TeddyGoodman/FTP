#include "session.h"
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

int init_session(session* sess, int client_fd, char* global_file_root) {
	sess->sentence = (char*)malloc(8192);
	sess->working_root = (char*)malloc(512);

	sess->login_status = unlogged;
	sess->is_RNFR = 0;
	
	sess->data_fd = 0;
	sess->pasv_lis_fd = 0;
	sess->client_fd = client_fd;
	sess->current_pasv = -1;
	
	sprintf(sess->working_root, "%s", global_file_root);
	return 0;
}

int close_session(session* sess) {
	free(sess->sentence);
	free(sess->working_root);
	return 0;
}

//向客户端发送确定的消息
void reply_custom_msg(session* sess, int code, char* str) {
	sprintf(sess->sentence, "%d %s\r\n", code, str);
	int len = strlen(sess->sentence);
	send(sess->client_fd, sess->sentence, len, MSG_WAITALL);
	return;
}


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
        default:
            sprintf(sess->sentence, "%d %s\r\n", code, "this code didn't has a custom message.");
            break;
    }
    int len = strlen(sess->sentence);
    send(sess->client_fd, sess->sentence, len, MSG_WAITALL);
    return;
}

// case 331:
        // case 332:
        //     sprintf(sess->sentence, "%d %s\r\n", code, "need password, please use PASS.");
        //     break;
        // case 230:
        //     sprintf(sess->sentence, "%d %s\r\n", code, "");
        //     break;
        // case 202:
        //     sprintf(sess->sentence, "%d %s\r\n", code, "permission already granted. no need for this.");
        //     break;
        // case 221:
        //     sprintf(sess->sentence, "%d %s\r\n", code, "Bye.");
        //     break;
        // case 215:
        //     sprintf(sess->sentence, "%d %s\r\n", code, "");
        //     break;
        // case 257:
        //     sprintf(sess->sentence, "%d is current directory.\r\n", code);
        //     break;
        // case 250:
        //     sprintf(sess->sentence, "%d Okay, is current directory.\r\n", code);
        //     break;
        // case 550:
        //     sprintf(sess->sentence, "%d %s\r\n", code, "No such file or directory or creation failed.");
        //     break;
        // case 350:
        //     sprintf(sess->sentence, "%d %s\r\n", code, "The file exists.");
        //     break;
        // case 227:
        //     return;