#ifndef _COMMON_H
#define _COMMON_H

#include "unp.h"
#include <stdarg.h> 
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define ODR_PATH "/tmp/odr_fd_" STR(PROTO)
#define SERVER_PATH "/tmp/server_path_" STR(PROTO)
#define SERVER_PORT 7005

struct msg_rec {
    char msg[30];
    char ip[16];
    int port;
};

struct msg_send {
    char ip[16];
    int port;
    char msg[30];
    int flag;
};

int msg_send(int sockfd, char *ip, int port, char *msg, int flag);
struct msg_rec* msg_recv(int sockfd, int timeout);

static void printdebuginfo(const char *format, ...)
{
#ifdef NDEBUGINFO
    return;
#endif
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

#endif
