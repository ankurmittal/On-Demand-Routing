#include "common.h"

int main()
{
    int sockfd;
    struct sockaddr_un cliaddr, servaddr;
    char input[5];
    char *c = "c";
    int valid = 0;
    char vm[4];
    char hostname[10];
    struct hostent *ent;
    int i = 0;
    char buffer[16];
    
    // How to figure out own vm
    gethostname(hostname, sizeof(hostname));
    printf("My hostname: %s\n", hostname);
    
    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sun_family = AF_LOCAL;
    strcpy(cliaddr.sun_path, tmpnam(NULL));
    Bind(sockfd, (SA *) &cliaddr, sizeof(cliaddr));
   
    printf("\n%s\n", cliaddr.sun_path);
    //while (1) {
        printf("\nEnter vm number (1-10): ");
        scanf("%s", vm);
        if((ent = gethostbyname("vm2")) == NULL) {
            perror("gethostbyname returned NULL");
            exit(1);
        }
        inet_ntop(PF_INET, ent->h_addr_list[0], buffer, sizeof(buffer));
        printf("%s\n",buffer);
        printf("\nclient at node %s sending request to server at %s\n", hostname, vm);
        msg_send(sockfd, buffer, 7001, c, 0);
   // }
    unlink(cliaddr.sun_path);
    exit(0);
}
