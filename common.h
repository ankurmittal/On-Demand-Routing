#ifndef _COMMON_H
#define _COMMON_H

#include "unp.h"
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define ODR_PATH "/tmp/odr_fd_" STR(PROTO)

struct msg_rec {
    char msg[10];
    char ip[16];
    int port;
};

struct msg_send {
    char ip[16];
    int port;
    char msg[10];
    int flag;
};

int msg_send(int sockfd, char *ip, int port, char *msg, int flag);
struct msg_rec* msg_recv(int sockfd);

#endif
