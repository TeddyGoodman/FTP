#ifndef COMMAND_H
#define COMMAND_H
#include "utility.h"

int cmd_user(char* para, LoginStatus *login);
int cmd_pass(char* para, LoginStatus *login);
int cmd_quit(char* para, LoginStatus *login);
int cmd_type(char* para, LoginStatus *login);
int cmd_syst(char* para, LoginStatus *login);
int cmd_cwd(char* para, LoginStatus *login, char* name_prefix);
int cmd_pwd(char* para, LoginStatus *login);
int cmd_mkd(char* para, LoginStatus *login, char* name_prefix);
int cmd_rmd(char* para, LoginStatus *login, char* name_prefix);
int cmd_rnfr(char* para, LoginStatus *login, char* name_prefix, PreStore* Premsg);
int cmd_rnto(char* para, LoginStatus *login, char* name_prefix, PreStore* Premsg);

int cmd_pasv(char* para, LoginStatus *login, DataInfo* data_info);
int cmd_port(char* para, LoginStatus *login, DataInfo* data_info);


#endif