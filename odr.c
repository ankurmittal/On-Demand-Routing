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
    socklen_t addrlen = sizeof(socket_address);
    char *ptr;
    void* buffer = (void*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/
    struct odr_hdr *odr_hdr;
    int length = 0; /*length of the received frame*/ 
    memset(&socket_address, 0, addrlen);
    length = recvfrom(sockfd, buffer, ETH_FRAME_LEN, 0, (SA *)&socket_address, &addrlen);
    if (length < 0) { perror("Error while recieving packet"); return;}

    printdebuginfo("message recieved at interface %d with mac address: ", socket_address.sll_ifindex);


    ptr = buffer + ETH_ALEN;
    length = IF_HADDR;
    do {
	printdebuginfo("%.2x%s", *ptr++ & 0xff, (length == 1) ? " " : ":");
    } while (--length > 0);
    odr_hdr = buffer + 14;
    printf("\n\n%lu, %lu\n", odr_hdr->sourceip, odr_hdr->destip);

    /*printdebuginfo("data: ");
      do {
      printdebuginfo("%c", *ptr++);
      length++;
      } while (length < 1500);
      printdebuginfo("\n");*/
}

int build_interface_info() 
{
    struct hwa_info *hwa, *hwahead;
    int tinterfaces = 0;
    for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
	struct sockaddr_in  *sin = (struct sockaddr_in *) (hwa->ip_addr);
	printdebuginfo("%ld\n", sin->sin_addr.s_addr);
	if(strcmp(hwa->if_name, "lo") == 0)
	    continue;
	if(strcmp(hwa->if_name, "eth0") == 0) {
	    cononicalip = htons(sin->sin_addr.s_addr);
    printf("Cononical IP: %lu", cononicalip);
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

    printdebuginfo("Cononical IP: %lu\n", cononicalip);

    free_hwa_info(hwahead);
    return tinterfaces;

}

int sendframe(int sockfd, char *destmac, int interface, char *srcmac, 
	void *payload, int data_lenght)
{

    struct sockaddr_ll socket_address;
    int n;
    /*buffer for ethernet frame*/
    void* buffer = (void*)malloc(ETH_FRAME_LEN);

    /*pointer to ethenet header*/
    unsigned char* etherhead = buffer;

    /*userdata in ethernet frame*/
    void* data = buffer + 14;

    /*another pointer to ethernet header*/
    struct ethhdr *eh = (struct ethhdr *)etherhead;

    int send_result = 0;

    printdebuginfo("Sending msg to %d\n", interface);
    /*prepare sockaddr_ll*/

    /*RAW communication*/
    socket_address.sll_family   = PF_PACKET;   
    /*we don't use a protocoll above ethernet layer
      ->just use anything here*/
    socket_address.sll_protocol = htons(0);   

    /*index of the network device*/
    socket_address.sll_ifindex  = interface;

    /*ARP hardware identifier is ethernet*/
    socket_address.sll_hatype   = ARPHRD_ETHER;

    /*target is another host*/
    socket_address.sll_pkttype  = PACKET_OTHERHOST;

    /*address length*/
    socket_address.sll_halen    = ETH_ALEN;	
    /*MAC - begin*/
    socket_address.sll_addr[0]  = destmac[0];	    
    socket_address.sll_addr[1]  = destmac[1];	    
    socket_address.sll_addr[2]  = destmac[2];	    
    socket_address.sll_addr[3]  = destmac[3];	    
    socket_address.sll_addr[4]  = destmac[4];	    
    socket_address.sll_addr[5]  = destmac[5];	    
    socket_address.sll_addr[6]  = destmac[6];	    
    /*MAC - end*/
    socket_address.sll_addr[6]  = 0x00;/*not used*/
    socket_address.sll_addr[7]  = 0x00;/*not used*/


    /*set the frame header*/
    memcpy((void*)buffer, (void*)destmac, ETH_ALEN);
    memcpy((void*)(buffer+ETH_ALEN), (void*)srcmac, ETH_ALEN);
    eh->h_proto = htons(PROTO);
    memcpy(data, payload, data_lenght);
    /*fill the frame with some data*/

    /*send the packet*/
    n = sendto(sockfd, buffer, ETH_FRAME_LEN, 0, 
	    (struct sockaddr*)&socket_address, sizeof(socket_address));
    if (n < 0) 
    { 
	perror("Error while sending packet.");
    }
    return n;
}

int process_msg_cs(int sockfd, struct msg_send *msg_content, unsigned port)
{
    struct odr_hdr odr_hdr;
    struct rreq_hdr *rreq_hdr = &(odr_hdr.hdr.rreq_hdr);
    odr_hdr.sourceip = cononicalip;
    inet_pton(AF_INET, msg_content->ip, &(odr_hdr.destip));
    tempinterface = hinterface;
    while(tempinterface != NULL) 
    {
	unsigned char dest[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	sendframe(sockfd, dest,tempinterface->interfaceno,tempinterface->if_haddr, &odr_hdr, sizeof(odr_hdr)); 
	tempinterface = tempinterface->next;
    }
    return 1;

}

int main(int argc, char **argv)
{
    int localfd, connfd;
    struct sockaddr_un cliaddr, odraddr;
    socklen_t clilen;
    fd_set allset;
    int n, tinterfaces = 0, sockfd;
    struct msg_send msg_content;

    init_port_table();

    localfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    unlink(ODR_PATH);
    bzero(&odraddr, sizeof(odraddr));
    odraddr.sun_family = AF_LOCAL;
    strcpy(odraddr.sun_path, ODR_PATH);
    Bind(localfd, (SA *) &odraddr, sizeof(odraddr));

    tinterfaces = build_interface_info();

    sockfd = Socket(AF_PACKET, SOCK_RAW, htons(PROTO));

    while(1)
    {
	FD_ZERO(&allset);
	FD_SET(localfd, &allset);
	FD_SET(sockfd, &allset);
	n = select(max(localfd, sockfd) + 1, &allset, NULL, NULL, NULL);
	if(n < 0) {
	    perror("Error during select, exiting.");
	    goto exit;
	}
	if(FD_ISSET(sockfd, &allset)) {
	    //Recieved message from other vm
	    recieveframe(sockfd);
	}
	if(FD_ISSET(localfd, &allset)) {
	    int port;
	    //Recieved message from client/server
	    clilen = sizeof(cliaddr);
	    memset(&cliaddr, 0, sizeof(cliaddr));
	    n = recvfrom(localfd, &msg_content, sizeof(msg_content), 0, (SA*)&cliaddr, &clilen);
	    printdebuginfo("Message from client/server %d, %d, %s, %s\n", msg_content.port, msg_content.flag, msg_content.msg, msg_content.ip);
	    printdebuginfo("Cli sun_name:%s\n", cliaddr.sun_path);
	    port = update_ttl_ptable(cliaddr.sun_path);
	    process_msg_cs(sockfd, &msg_content, port);
	}
    }
    //Listen(localfd, LISTENQ);
    /*
     */
exit:
    free_interface_info();

}


