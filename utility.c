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
#include <time.h>

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

// void send_const_msg(int fd, const char* str) {
// 	int len = strlen(str);
// 	send(fd, (char *)str, len, MSG_WAITALL);
// 	return;
// }

//删除某个目录，0代表成功，-1代表失败
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

//通过prefix和输入参数获取绝对路径，文件或目录存在返回绝对路径，否则返回NULL
//第三个参数: 0->目录，1->文件，-1->只做组合路径
char* get_absolute_dir(char* name_prefix, char* parameter, int is_file) {
	int len = strlen(name_prefix);
	char* temp_dir = (char*)malloc(256);
    int flag = 1;
    
    if (parameter[0] == '/')
        //定义绝对路径
        sprintf(temp_dir, "%s", parameter);
    else {
        //定义当前路径下的相对路径
        if (name_prefix[len-1] == '/')
            sprintf(temp_dir, "%s%s", name_prefix, parameter);
        else sprintf(temp_dir, "%s/%s", name_prefix, parameter);
    }

    //希望的路径保存在temp_dir中
    struct stat file_stat;
    if (is_file > 0) {
        if (access(parameter, 0) != 0) flag = 0;
        else if (stat(parameter, &file_stat) < 0) flag = 0;
        else if (!S_ISREG(file_stat.st_mode)) flag = 0;
    }
    else if (is_file == 0) {
        if (access(parameter, 0) != 0) flag = 0;
    }

    //否则只做合并
    if (flag == 0) {
        free(temp_dir);
        return NULL;
    }
    else return temp_dir;
}

char* file_info(struct stat *dir_stat, char* file_name){
    char* info = (char*)malloc(256);
    int offset = 0;
    offset = sprintf(info, "%s | ", file_name);

    switch (dir_stat->st_mode & S_IFMT) {
       case S_IFBLK:  offset = sprintf(info + offset, "%s | ", "block device");            break;
       case S_IFCHR:  offset = sprintf(info + offset, "%s | ", "character device");        break;
       case S_IFDIR:  offset = sprintf(info + offset, "%s | ", "directory");               break;
       case S_IFIFO:  offset = sprintf(info + offset, "%s | ", "FIFO/pipe");               break;
       case S_IFLNK:  offset = sprintf(info + offset, "%s | ", "symlink");                 break;
       case S_IFREG:  offset = sprintf(info + offset, "%s | ", "regular file");            break;
       case S_IFSOCK: offset = sprintf(info + offset, "%s | ", "socket");                  break;
       default:       offset = sprintf(info + offset, "%s | ", "unknown?");                break;
    }

    offset = sprintf(info + offset, "%ld bytes | ", (long) dir_stat->st_blksize);
    sprintf(info + offset, "%s\r\n", ctime(&(dir_stat->st_mtime)));
    return info;
}