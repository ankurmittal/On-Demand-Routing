#include "common.h"


int msg_send(int sockfd, char *ip, int port, char *msg, int flag)
{
    struct sockaddr_un odraddr;
    struct msg_send msg_content;
    int n;
    bzero(&odraddr, sizeof(odraddr));
    bzero(&msg_content, sizeof(msg_content));

    strcpy(msg_content.ip, ip);
    strcpy(msg_content.msg, ip);
    msg_content.port = port;
    msg_content.flag = flag;

    odraddr.sun_family = AF_LOCAL;
    strcpy(odraddr.sun_path, ODR_PATH);
    printf("ODR_PATH:%s\n", ODR_PATH);
    n = connect(sockfd, (SA *) &odraddr, sizeof(odraddr));
    if(n < 0)
    {
	perror("Error connecting to ODR");
	return n;
    }
    n = write(sockfd, &msg_content, sizeof(msg_content));
    if(n < 0)
    {
	perror("Error writing to odr socket");
    }
    return n;
}

struct msg_rec* msg_recv(int sockfd)
{
}
