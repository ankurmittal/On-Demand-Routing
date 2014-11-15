#ifndef _COMMON_H
#define _COMMON_H

#include "unp.h"

struct msg_rec {
    char msg[10];
    char ip[16];
    int port;
};

int msg_send(int sockfd, char *ip, int port, char *msg, int flag);
struct msg_rec* msg_recv(int sockfd);

#endif
