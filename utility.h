#ifndef UTILITY_H
#define UTILITY_H

typedef enum LoginStatus
{
	unlogged,
	need_pass,
	logged
}LoginStatus;

void remove_enter(char* str);

void send_const_msg(int fd, const char* str);

#endif