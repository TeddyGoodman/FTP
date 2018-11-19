#ifndef COMMAND_H
#define COMMAND_H
#include "utility.h"
#include "session.h"

//user relative
int cmd_user(char* para, session* sess);
int cmd_pass(char* para, session* sess);

int cmd_quit(char* para, session* sess);

// reply a message
int cmd_type(char* para, session* sess);
int cmd_syst(char* para, session* sess);

// dir relative
int cmd_cwd(char* para, session* sess);
int cmd_pwd(char* para, session* sess);
int cmd_mkd(char* para, session* sess);
int cmd_rmd(char* para, session* sess);
int cmd_rnfr(char* para, session* sess);
int cmd_rnto(char* para, session* sess);

int cmd_list(char* para, session* sess);
int cmd_rest(char* para, session* sess);

// begin make data connection
int cmd_pasv(char* para, session* sess);
int cmd_port(char* para, session* sess);
void* pasv_accept_pthread(void* ptr);

// download or upload a file
int cmd_retr(char* para, session* sess);
int cmd_stor(char* para, session* sess);

typedef struct transmit_input
{
    session* sess;
    int file;
}transmit_input;

void* retrieve_file_pthread(void* transmit_in);
void* store_file_pthread(void* transmit_in);

int reply_list(session* sess, char* dir);

#endif