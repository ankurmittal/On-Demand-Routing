#include "common.h"

static char* sun_path;

int clean_input() {
	char c;
	while ((c = getchar()) != '\n' && c != EOF);
	return 1;
}

void getInput(char *result) {
	int num =0;
	char count[3];
	char v,m,c;
	do {
		printf("\nEnter server (vm1 - vm10): ");
	} while (((scanf("%c%c%d%c", &v, &m, &num, &c)!=4 || v!='v' || m!='m' || c!='\n') && clean_input()) || num<1 || num>10);
	sprintf(count, "%d", num);
	sprintf(result, "vm");
	strcat(result + strlen(result), count);
}

void intHandler(int sig) {
    unlink(sun_path);
}

int main()
{
    int sockfd, n = 0, tfd;
    struct sockaddr_un cliaddr;
    struct msg_rec *msg;
    char *c = "c";
    char vm[5], hostname[10], buffer[16], data[50];
    struct hostent *ent;
    char buff[22];


    gethostname(hostname, sizeof(hostname));
    printdebuginfo("My hostname: %s\n", hostname);

    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sun_family = AF_LOCAL;
    strcpy(cliaddr.sun_path,"/tmp/myTmpFile-XXXXXX");
    tfd = mkstemp(cliaddr.sun_path);
    unlink(cliaddr.sun_path);
    //sun_path = cliaddr.sun_path;
    Bind(sockfd, (SA *) &cliaddr, sizeof(cliaddr));

    printdebuginfo("\n%s\n", cliaddr.sun_path);
    //signal(SIGINT, intHandler);

    while (1) {
	getInput(vm);
	if((ent = gethostbyname(vm)) == NULL) {
	    perror("gethostbyname returned NULL");
	    exit(1);
	}
	inet_ntop(PF_INET, ent->h_addr_list[0], buffer, sizeof(buffer));
	printdebuginfo("%s\n", buffer);
	printf("\nclient at node %s sending request to server at %s\n", hostname, vm);
	n = msg_send(sockfd, buffer, SERVER_PORT, c, 0);
	msg = msg_recv(sockfd, 1);
	if(msg == NULL) {
	    printf("client at node %s: timeout on response from %s\n", hostname, vm);
	    n = msg_send(sockfd, buffer, SERVER_PORT, c, 1);
	    msg = msg_recv(sockfd, 1);
	    if(msg == NULL) {
		printf("client at node %s: timeout on response from %s\n", hostname, vm);
		continue;
	    }
	}
	printf("client at node %s: received from %s <%s>\n", hostname, vm,  msg->msg);
	free(msg);
    }

    close(sockfd);
    exit(0);
}
