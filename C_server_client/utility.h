#ifndef UTILITY_H
#define UTILITY_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>

void remove_enter(char* str);

// void send_const_msg(int fd, const char* str);

int remove_dir(char* dir);

//通过prefix和输入参数获取绝对路径，成功返回路径，否则返回NULL，第三个参数为是文件还是目录
char* get_absolute_dir(char* name_prefix, char* parameter, int is_file);

char* file_info(struct stat *dir_stat, char* file_name);

#endif