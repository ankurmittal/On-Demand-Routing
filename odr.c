#include "common.h"

int main(int argc, char **argv)
{
    int listenfd, connfd;
    struct sockaddr_un cliaddr, odraddr;
    socklen_t clilen;
    int n;
    struct msg_send msg_content;
    listenfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    unlink(ODR_PATH);
    bzero(&odraddr, sizeof(odraddr));
    odraddr.sun_family = AF_LOCAL;
    strcpy(odraddr.sun_path, ODR_PATH);
    printf("ODR path:%s\n", ODR_PATH);
    Bind(listenfd, (SA *) &odraddr, sizeof(odraddr));
    //Listen(listenfd, LISTENQ);
    clilen = sizeof(cliaddr);
    n = recvfrom(listenfd, &msg_content, sizeof(msg_content), 0, (SA*)&cliaddr, &clilen);
    printf("%d, %d, %s, %s\n", msg_content.port, msg_content.flag, msg_content.msg, msg_content.ip);
    printf("Cli sun_name:%s\n", cliaddr.sun_path);

}
