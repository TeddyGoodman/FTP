#ifndef COMMAND_H
#define COMMAND_H

int cmd_user(char* para, LoginStatus *login);
int cmd_pass(char* para, LoginStatus *login);
int cmd_quit(char* para, LoginStatus *login);

#endif