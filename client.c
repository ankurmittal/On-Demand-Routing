#include "common.h"

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
		printf("\nEnter client(vm1 - vm10): ");
	} while (((scanf("%c%c%d%c", &v, &m, &num, &c)!=4 || v!='v' || m!='m' || c!='\n') && clean_input()) || num<1 || num>10);
	sprintf(count, "%d", num);
	sprintf(result, "vm");
	strcat(result + strlen(result), count);
}

int main()
{
	int sockfd, n = 0;
	struct sockaddr_un cliaddr;
	struct msg_rec *msg;
	char *c = "c";
	char vm[5], hostname[10], buffer[16], data[50];
	struct hostent *ent;
	int timeoutflag = 0;

	gethostname(hostname, sizeof(hostname));
	printf("My hostname: %s\n", hostname);

	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	bzero(&cliaddr, sizeof(cliaddr));
	cliaddr.sun_family = AF_LOCAL;
	strcpy(cliaddr.sun_path, tmpnam(NULL));
	Bind(sockfd, (SA *) &cliaddr, sizeof(cliaddr));

	printf("\n%s\n", cliaddr.sun_path);

	//while (1) {
	getInput(vm);
	if((ent = gethostbyname(vm)) == NULL) {
		perror("gethostbyname returned NULL");
		exit(1);
	}
	inet_ntop(PF_INET, ent->h_addr_list[0], buffer, sizeof(buffer));
	printf("%s\n", buffer);
	printf("\nclient at node %s sending request to server at %s\n", hostname, vm);
sendmsg:
	n = msg_send(sockfd, buffer, SERVER_PORT, c, timeoutflag);
	msg = msg_recv(sockfd, 1);
	if(msg == NULL) {
		timeoutflag = 1;
		if(timeoutflag) {
			n = msg_send(sockfd, buffer, SERVER_PORT, c, timeoutflag);
			msg = msg_recv(sockfd, 1);
			if(msg == NULL) {
			    printf("timeout..!!\n");
			    unlink(cliaddr.sun_path);
			    exit(0);
			}
		}
		goto sendmsg;
	} else {
	    timeoutflag = 0;
	}
	printf("Received Msg: %s\n", msg->msg);
	printf("Received IP: %s\n", msg->ip);
	printf("Received Port: %d\n", msg->port);
	//}

	unlink(cliaddr.sun_path);
	exit(0);
}
