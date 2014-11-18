#include "common.h"


int msg_send(int sockfd, char *ip, int port, char *msg, int flag)
{
    struct sockaddr_un odraddr;
    struct msg_send msg_content;
    int n;
    bzero(&odraddr, sizeof(odraddr));
    bzero(&msg_content, sizeof(msg_content));

    strcpy(msg_content.ip, ip);
    strcpy(msg_content.msg, msg);
    msg_content.port = port;
    msg_content.flag = flag;

    odraddr.sun_family = AF_LOCAL;
    printf("ODR_PATH:%s\n", ODR_PATH);
    strcpy(odraddr.sun_path, ODR_PATH);
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

struct msg_rec* msg_recv(int sockfd, int timeout)
{
    struct msg_rec *recv;
    static struct timeval selectTime;
    int n = 0;
    fd_set allset;
    
    recv = malloc(sizeof(struct msg_rec));
    selectTime.tv_sec = 3;
    selectTime.tv_usec = 0;
    
    bzero(recv,sizeof(recv));
    
    FD_ZERO(&allset);
    FD_SET(sockfd, &allset);
    select(sockfd+1, &allset, NULL, NULL, ((timeout) ? (&selectTime) : NULL));
    if(FD_ISSET(sockfd, &allset)) {
        n = read(sockfd,recv,sizeof(recv));
        if (n < 0) {
            perror("ERROR reading from socket");
            exit(1);
        }
        printf("Here is the message: %s\n",recv->msg);
        return recv;
    } else {
        printdebuginfo("Timeout occured in receive message..!!\n");
        return NULL;
    }
}
