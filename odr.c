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
    Bind(listenfd, (SA *) &odraddr, sizeof(odraddr));
    Listen(listenfd, LISTENQ);
    for ( ; ; ) {
	clilen = sizeof(cliaddr);
	if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {

	    if (errno == EINTR)
		continue; /* back to for() */
	    else
		err_sys("accept error");
	}
	break;
    }
    n = read(listenfd, &msg_content, sizeof(msg_content));
    printf("%d, %d, %s, %s\n", msg_content.port, msg_content.flag, msg_content.msg, msg_content.ip);
    printf("Cli sun_name:%s\n", cliaddr.sun_path);

}
