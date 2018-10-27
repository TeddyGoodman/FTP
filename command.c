#include <stdio.h>
#include <string.h>
#include "utility.h"
#include "command.h"

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

	return 221;
}