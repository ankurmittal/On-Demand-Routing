#include "common.h"

int main()
{
    int sockfd, n;
    struct sockaddr_un servaddr;
    struct msg_rec *msg;
    char vm[4], hostname[10], buffer[16], ts[30];
    struct hostent *ent;
    int client_port = 0;
    time_t ticks;
    struct hostent *he;
    struct in_addr ipv4addr;
    
    gethostname(hostname, sizeof(hostname));
    printf("My hostname: %s\n", hostname);
    
    unlink(SERVER_PATH);
    
    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, SERVER_PATH);
    Bind(sockfd, (SA *) &servaddr, sizeof(servaddr));
    
    //while (1) {
        //msg = msg_recv(sockfd);
        //printf("Received Msg: %s\n")msg->msg);
        //printf("Received IP: %s\n", msg->ip);
        //printf("Received Port: %d\n", msg->port);
        client_port = msg->port;
        
        inet_pton(AF_INET, msg->ip, &ipv4addr);
        he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
        client_hostname = he->h_name;
        printf("Client hostname: %s\n", client_hostname);
        
        ticks = time(NULL);
        snprintf(ts, sizeof(ts), "%.24s\r\n", ctime(&ticks));
        
        printf("\nserver at %s responding to request from %s\n", hostname, client_hostname);
        
        printf("Sending timestamp: %s\n", ts);
        n = msg_send(sockfd, msg->ip, client_port, ts, 0);
    //}
}
