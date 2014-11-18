#include "odr.h"
static unsigned long cononicalip;
static struct interface_info *hinterface = NULL, *tempinterface;



void free_interface_info() {
    while(hinterface != NULL) {
	tempinterface = hinterface;
	hinterface = hinterface->next;
	free(tempinterface);
    }
}

void recieveframe(int sockfd)
{
    struct sockaddr_ll socket_address;
    struct ethhdr *ethh;
    socklen_t addrlen = sizeof(socket_address);
    char *ptr;
    void* buffer = (void*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/
    int length = 0; /*length of the received frame*/ 
    memset(&socket_address, 0, addrlen);
    length = recvfrom(sockfd, buffer, ETH_FRAME_LEN, 0, (SA *)&socket_address, &addrlen);
    if (length < 0) { perror("Error while recieving packet"); return;}
    ethh = (struct ether_header *)buffer;

    printf("message recieved at interface %d with mac address: ", socket_address.sll_ifindex);


    ptr = buffer + ETH_ALEN;
    length = IF_HADDR;
    do {
	printf("%.2x%s", *ptr++ & 0xff, (length == 1) ? " " : ":");
    } while (--length > 0);
    /*printf("data: ");
    do {
    printf("%c", *ptr++);
    length++;
    } while (length < 1500);
    printf("\n");*/
}

int build_interface_info() 
{
    struct hwa_info *hwa, *hwahead;
    int tinterfaces = 0;
    for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
	struct sockaddr_in  *sin = (struct sockaddr_in *) (hwa->ip_addr);
	printf("%ld\n", sin->sin_addr.s_addr);
	if(strcmp(hwa->if_name, "lo") == 0)
	    continue;
	if(strcmp(hwa->if_name, "eth0") == 0) {
	    cononicalip = sin->sin_addr.s_addr;
	} else {
	    struct interface_info *iinfo = (struct interface_info*)malloc(sizeof(struct interface_info));
	    int i;
	    memset(iinfo, 0, sizeof(struct interface_info));
	    tinterfaces++;
	    if(hinterface == NULL)
		hinterface = iinfo;
	    else
		tempinterface->next = iinfo;
	    
	    iinfo->ip=sin->sin_addr.s_addr;
	    iinfo->interfaceno = hwa->if_index;
	    for(i = 0; i < IF_HADDR; i++)
		iinfo->if_haddr[i] = hwa->if_haddr[i];
	    iinfo->next = NULL;
	    
	    tempinterface = iinfo;
	}
    }

    printf("Cononical IP: %ld\n", cononicalip);

    free_hwa_info(hwahead);
    return tinterfaces;
    
}

int main(int argc, char **argv)
{
    int localfd, connfd;
    struct sockaddr_un cliaddr, odraddr;
    socklen_t clilen;
    int n, tinterfaces = 0, sockfd;
    struct msg_send msg_content;
    localfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    unlink(ODR_PATH);
    bzero(&odraddr, sizeof(odraddr));
    odraddr.sun_family = AF_LOCAL;
    strcpy(odraddr.sun_path, ODR_PATH);
    printf("ODR path:%s\n", ODR_PATH);
    Bind(localfd, (SA *) &odraddr, sizeof(odraddr));
    
    tinterfaces = build_interface_info();
    tempinterface = hinterface;
    n = 0;

    printf("PROTO:%d", htons(PROTO));
    sockfd = Socket(AF_PACKET, SOCK_RAW, htons(PROTO));
    printf("Sock created\n");

    while(tempinterface != NULL)
    {
	struct sockaddr_ll socket_address;
	int j;
	/*buffer for ethernet frame*/
	void* buffer = (void*)malloc(ETH_FRAME_LEN);

	/*pointer to ethenet header*/
	unsigned char* etherhead = buffer;

	/*userdata in ethernet frame*/
	unsigned char* data = buffer + 14;

	/*another pointer to ethernet header*/
	struct ethhdr *eh = (struct ethhdr *)etherhead;

	int send_result = 0;

	/*our MAC address*/
	unsigned char *src_mac = tempinterface->if_haddr;

	/*other host MAC address*/
	unsigned char dest_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	printf("Sending msg to %d\n", tempinterface->interfaceno);
	/*prepare sockaddr_ll*/

	/*RAW communication*/
	socket_address.sll_family   = PF_PACKET;   
	/*we don't use a protocoll above ethernet layer
	  ->just use anything here*/
	socket_address.sll_protocol = htons(0);   

	/*index of the network device
	  see full code later how to retrieve it*/
	socket_address.sll_ifindex  = tempinterface->interfaceno;

	/*ARP hardware identifier is ethernet*/
	socket_address.sll_hatype   = ARPHRD_ETHER;

	/*target is another host*/
	socket_address.sll_pkttype  = PACKET_OTHERHOST;

	/*address length*/
	socket_address.sll_halen    = ETH_ALEN;	
	/*MAC - begin*/
	socket_address.sll_addr[0]  = 0xff;	    
	socket_address.sll_addr[1]  = 0xff;	    
	socket_address.sll_addr[2]  = 0xff;
	socket_address.sll_addr[3]  = 0xff;
	socket_address.sll_addr[4]  = 0xff;
	socket_address.sll_addr[5]  = 0xff;
	/*MAC - end*/
	socket_address.sll_addr[6]  = 0x00;/*not used*/
	socket_address.sll_addr[7]  = 0x00;/*not used*/


	/*set the frame header*/
	memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
	eh->h_proto = htons(PROTO);
	/*fill the frame with some data*/
	for (j = 0; j < 1500; j++) {
	    data[j] = 'a';
	}

	/*send the packet*/
	n = sendto(sockfd, buffer, ETH_FRAME_LEN, 0, 
		(struct sockaddr*)&socket_address, sizeof(socket_address));
	if (n < 0) 
	{ 
	    perror("Error while sending packet.");
	    goto exit;
	}
	tempinterface = tempinterface->next;
    }

    recieveframe(sockfd);


    //Listen(localfd, LISTENQ);
    /*
       clilen = sizeof(cliaddr);
       n = recvfrom(localfd, &msg_content, sizeof(msg_content), 0, (SA*)&cliaddr, &clilen);
       printf("%d, %d, %s, %s\n", msg_content.port, msg_content.flag, msg_content.msg, msg_content.ip);
       printf("Cli sun_name:%s\n", cliaddr.sun_path);
     */
exit:
    free_interface_info();

}


