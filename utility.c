/*
* this file is utilities for the ftp server
*
*/
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include "utility.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

void remove_enter(char* str) {
	int i = 0;
	while (1){
		if (str[i] == '\0') return;
		else if (str[i] == '\r' || str[i] == '\n') {
			str[i] = '\0';
			return;
		}
		i++;
	}
}

void send_const_msg(int fd, const char* str) {
	int len = strlen(str);
	send(fd, (char *)str, len, MSG_WAITALL);
	return;
}

//删除某个目录，直接返回code
int remove_dir(char* dir) {

    char current_dir[] = ".";
    char parent_dir[] = "..";
    char dir_name[128];
    DIR *dir_p;
    struct dirent *dp;
    struct stat dir_stat;

    //目录不存在，返回-1
	if (access(dir, 0) != 0) return -1;
	
	//获取目录属性失败
    if (stat(dir, &dir_stat) < 0) return -1;

    //是一个普通文件，直接删除
    if (S_ISREG(dir_stat.st_mode)) {
    	if (remove(dir) != 0) return -1;
    	return 0;
    }

    //是一个目录，需要进行递归删除
    else if (S_ISDIR(dir_stat.st_mode)) {
        dir_p = opendir(dir);
        while ((dp=readdir(dir_p)) != NULL) {
            // 忽略 . 和 ..
            if (strcmp(current_dir, dp->d_name) == 0 ) continue;
            if (strcmp(parent_dir, dp->d_name) == 0 ) continue;

            sprintf(dir_name, "%s/%s", dir, dp->d_name);
            // 递归调用
            if (remove_dir(dir_name) != 0) return -1;
        }
        closedir(dir_p);

        //删除空目录
        if (rmdir(dir) != 0) return -1;
        return 0;
    }
    else return -1;
}

//通过prefix和输入参数获取绝对路径，成功返回路径，否则返回NULL，
//第三个参数: 0->目录，1->文件，-1->只做组合路径
char* get_absolute_dir(char* name_prefix, char* parameter, int is_file) {
	int len = strlen(name_prefix);
	char* temp_dir = (char*)malloc(256);

	if (parameter[0] == '/'){
		//定义绝对路径且存在
		if (is_file > 0 && access(parameter, R_OK) == 0){
			sprintf(temp_dir, "%s", parameter);
			return temp_dir;
		}
		else if (is_file == 0 && access(parameter, 0) == 0){
			sprintf(temp_dir, "%s", parameter);
			return temp_dir;
		}
		else if (is_file < 0) {
			sprintf(temp_dir, "%s", parameter);
			return temp_dir;
		}
	}
	else{
		//定义当前路径下的相对路径
		if (name_prefix[len-1] == '/')
			sprintf(temp_dir, "%s%s", name_prefix, parameter);
		else sprintf(temp_dir, "%s/%s", name_prefix, parameter);

		if (is_file > 0 && access(temp_dir, R_OK) == 0) {
			return temp_dir;
		}
		else if (is_file == 0 && access(temp_dir, 0) == 0) {
			return temp_dir;
		}
		else if (is_file < 0) {
			return temp_dir;
		}
	}
	free(temp_dir);
	return NULL;
}