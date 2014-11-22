#include "odr.h"
#include "destmap.h"

static unsigned long cononicalip;
static struct interface_info *hinterface = NULL, *tempinterface;
static unsigned long broadcastid = 0;
static int framefd = 0, staleness, localfd = 0;
static char b_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, hostname[10];

struct rreq_map
{
    unsigned long sourceip;
    unsigned long broadcastid;
    int srchop, desthop;
    int resolved;
    unsigned long timestamp;
    struct rreq_map *next, *prev;
};

#define TTL_RREQ_MAP 5

static struct rreq_map *hrreqmap = NULL, *trreqmap = NULL, *temprreq_map;

static int forwardrreq(struct odr_hdr *odr_hdr, int interfacetoexclude);
static int sendremainingpayload(struct dest_map *dest_entry);
static int send_rreq(unsigned long destip, short forceflag, int excludeinterface);

int writeToServerClient(unsigned int port_dest, unsigned int port_src, char *buf, int msglen, unsigned long destip) {
	char *filename = NULL;
    	int n = 0;
	struct msg_rec *recvStruct = NULL;
	struct sockaddr_un destaddr;
	unsigned long dip = ntohl(destip);	
	
	filename = get_sun_path(port_dest);
	if(filename == NULL) {
		printdebuginfo("sun_path not found..!!\n");
		return 0;
	}
    	bzero(&destaddr, sizeof(destaddr));
    	destaddr.sun_family = AF_LOCAL;
    	strcpy(destaddr.sun_path, filename);
	printdebuginfo("sun_path_name:%s\n", filename);

	recvStruct = malloc(sizeof(struct msg_rec));
	recvStruct->port = port_src;
	memcpy(recvStruct->msg, buf, msglen);
	inet_ntop(AF_INET, &dip, recvStruct->ip, sizeof(recvStruct->ip));

	n = sendto(localfd, recvStruct, sizeof(struct msg_rec), 0, (SA *) &destaddr, sizeof(destaddr));
	if(n < 0) {
		perror("error writing..!!");
	}
	free(recvStruct);
	return n;
}

void insert_rreq_map(unsigned long sourceip, unsigned long broadcastid, int hop)
{
    struct rreq_map *obj = (struct rreq_map *)malloc(sizeof(struct rreq_map));
    memset(obj,0,sizeof(struct rreq_map));
    obj->next=obj->prev=NULL;
    if(hrreqmap == NULL)
	trreqmap = hrreqmap = obj;
    else
    {
	trreqmap->next = obj;
	obj->prev = trreqmap;
	trreqmap = obj;
    }
    obj->sourceip = sourceip;
    obj->broadcastid = broadcastid;
    obj->resolved = 0;
    obj->srchop = hop;
    obj->desthop = 1 << 14;
    obj->timestamp = time(NULL);
}

struct rreq_map* find_rreq(unsigned long sourceip, unsigned long broadcastid)
{
    unsigned long t = time(NULL);
    temprreq_map = hrreqmap;
    while(temprreq_map != NULL)
    {
	if(t - temprreq_map->timestamp > TTL_RREQ_MAP)
	{
	    struct rreq_map *temp = temprreq_map;
	    if(temprreq_map->next)
		temprreq_map->next->prev = temprreq_map->prev;
	    if(temprreq_map->prev)
		temprreq_map->prev->next = temprreq_map->next;
	    if(hrreqmap == temprreq_map) {
		hrreqmap = temprreq_map->next;
	    }
	    if(trreqmap == temprreq_map) {
		trreqmap = temprreq_map->prev;
	    }
	    temprreq_map = temprreq_map->next;
	    free(temp);
	    continue;
	}
	if(temprreq_map->sourceip == sourceip && temprreq_map->broadcastid == broadcastid) 
	{
	    return temprreq_map;
	}
	temprreq_map = temprreq_map->next;
    }
    return NULL;
}

int update_rreq_map(unsigned long sourceip, unsigned long broadcastid, int hop, short issrc)
{
    temprreq_map = find_rreq(sourceip, broadcastid);
    if(temprreq_map)
    {
	int *oldhop = issrc ? &temprreq_map->srchop : &temprreq_map->desthop; 
	printdebuginfo("old hop:%d\n", *oldhop);
	if(*oldhop > hop) {
	    *oldhop = hop;
	    temprreq_map->timestamp = time(NULL);
	    return 1;
	}
    } else {
	insert_rreq_map(sourceip, broadcastid, hop);
	return 1;
    }
    return 0;
}

void mark_rreq_resolved(unsigned long sourceip, unsigned long broadcastid)
{
    temprreq_map = find_rreq(sourceip, broadcastid);
    if(temprreq_map) {
	temprreq_map->resolved = 1;
	temprreq_map->timestamp = time(NULL);
    }
}

void free_interface_info() {
    while(hinterface != NULL) {
	tempinterface = hinterface;
	hinterface = hinterface->next;
	free(tempinterface);
    }
}

unsigned char *getmacbyinterface(int interfaceno)
{
    tempinterface = hinterface;
    while(tempinterface != NULL)
    {
	if(tempinterface->interfaceno == interfaceno)
	{
	    return tempinterface->if_haddr;
	}
	tempinterface = tempinterface->next;
    }
    return "";
}

void recieveframe()
{
    struct sockaddr_ll socket_address;
    socklen_t addrlen = sizeof(socket_address);
    char *ptr;
    char *my_mac;
    unsigned char src_mac[IF_HADDR];
    void* buffer = (void*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/
    struct odr_hdr *odr_hdr;
    int length = 0, n; /*length of the received frame*/ 
    memset(&socket_address, 0, addrlen);
    length = recvfrom(framefd, buffer, ETH_FRAME_LEN, 0, (SA *)&socket_address, &addrlen);
    if (length < 0) { perror("Error while recieving packet"); return;}


    printdebuginfo("\n message recieved, len:%d, at interface %d with mac address: ", length, socket_address.sll_ifindex);


    ptr = buffer + ETH_ALEN;
    length = IF_HADDR;
    memcpy(src_mac, ptr, IF_HADDR);
    do {
	printdebuginfo(" %.2x%s", *(src_mac - length + IF_HADDR), (length == 1) ? " " : ":");
    } while (--length > 0);
    odr_hdr = buffer + sizeof(struct ethhdr);
    printdebuginfo(" \n%lu, %lu\n", ntohl(odr_hdr->sourceip), ntohl(odr_hdr->destip));
    my_mac = getmacbyinterface(socket_address.sll_ifindex);

    switch(odr_hdr->type)
    {
	case TYPE_RREQ:
	{
	    struct rreq_hdr *rreq_hdr = &odr_hdr->hdr.rreq_hdr;
	    struct dest_map *dest_entry;
	    printdebuginfo(" rreq recieved, hop:%d, rrep_sent:%d\n", rreq_hdr->hop, rreq_hdr->rrep_sent);
	    if(cononicalip == odr_hdr->sourceip || update_rreq_map(odr_hdr->sourceip, rreq_hdr->broadcastid, rreq_hdr->hop, 1) == 0)
	    {
		printdebuginfo(" Duplicate rreq\n");
		break;
	    }
	    update_dest_map(odr_hdr->sourceip, src_mac, my_mac, socket_address.sll_ifindex, ++rreq_hdr->hop, rreq_hdr->forceflag?0:staleness);
	    dest_entry = get_dest_entry(odr_hdr->destip, rreq_hdr->forceflag?0:staleness);
	    if((dest_entry || odr_hdr->destip == cononicalip) && rreq_hdr->rrep_sent == 0)
	    {
		struct odr_hdr rrep_odr_hdr;
		struct dest_map *src_dest_entry =  NULL;
		char rrep_d_mac[IF_HADDR];
		int interfaceno;
		memset(&rrep_odr_hdr, 0, sizeof(rrep_odr_hdr));
		struct rrep_hdr *rrep_hdr = &rrep_odr_hdr.hdr.rrep_hdr;
		rreq_hdr->rrep_sent = 1;
		rrep_odr_hdr.sourceip = odr_hdr->sourceip;
		rrep_odr_hdr.destip = odr_hdr->destip;
		rrep_hdr->forceflag = rreq_hdr->forceflag;
		rrep_hdr->broadcastid = rreq_hdr->broadcastid;
		rrep_odr_hdr.type = TYPE_RREP;
		printdebuginfo(" Found entry in dest_map, or same host,  need to send rrep\n");
		if(odr_hdr->destip == cononicalip)
		{
		    rrep_hdr->hop = 0;
		    memcpy(rrep_d_mac, src_mac, IF_HADDR);
		    interfaceno = socket_address.sll_ifindex;
		}
		else
    		{
		    src_dest_entry =  get_dest_entry(odr_hdr->sourceip, 1 << 15);

		    if(!src_dest_entry)
		    {
			printdebuginfo(" Something wrong, src_dest_map not found\n");
			break;
		    }
		    rrep_hdr->hop = dest_entry->hop;
		    memcpy(rrep_d_mac, src_dest_entry->nexthop_mac, IF_HADDR);
		    my_mac = src_dest_entry->src_mac;
		    interfaceno = src_dest_entry->interface;
		}
		printdebuginfo(" Sending rrep\n");
		n = sendframe(rrep_d_mac, interfaceno, 
		    my_mac, &rrep_odr_hdr, sizeof(struct odr_hdr), NULL, 0);
		if(n < 0)
		    goto exit;
		if(odr_hdr->destip != cononicalip)
		{
		    n = forwardrreq(odr_hdr, socket_address.sll_ifindex);
		    if(n < 0)
			goto exit;
		}

	    }
	    else 
	    {
		n = forwardrreq(odr_hdr, socket_address.sll_ifindex);
		if(n < 0)
		    goto exit;
	    }
	    break;
	}
	case TYPE_RREP:
	{
	    struct rrep_hdr *rrep_hdr = &odr_hdr->hdr.rrep_hdr;
	    struct dest_map *src_dest_entry, *dest_e;
	    printdebuginfo(" rrep recieved, hop:%d, bid:%ld\n", rrep_hdr->hop, rrep_hdr->broadcastid);
	    if(update_rreq_map(odr_hdr->sourceip, rrep_hdr->broadcastid, rrep_hdr->hop, 0) == 0)
	    {
		printdebuginfo(" Duplicate rrep, bid\n");
		break;
	    }
	    dest_e = update_dest_map(odr_hdr->destip, src_mac, my_mac, 
		    socket_address.sll_ifindex, ++rrep_hdr->hop, rrep_hdr->forceflag? 0 : staleness);

	    if(!dest_e)
	    {
		printdebuginfo(" Duplicate rrep\n");
		break;
	    }

	    if(dest_e->data != NULL)
	    {
		printdebuginfo(" Forwarding all data\n");
		n = sendremainingpayload(dest_e);
		if(n < 0)
		    goto exit;
	    }

	    if(odr_hdr->sourceip == cononicalip)
	    {
		printdebuginfo(" Got rrep for dest\n");
		break;
	    }

	    src_dest_entry =  get_dest_entry(odr_hdr->sourceip, 1 << 15);

	    if(!src_dest_entry)
	    {
		printdebuginfo(" Something wrong, src_dest_map not found, sourceip: %lu\n", ntohl(odr_hdr->sourceip));
		break;
	    }

	    printdebuginfo(" Forwarding rrep\n");
	    n = sendframe(src_dest_entry->nexthop_mac, src_dest_entry->interface, 
		    src_dest_entry->src_mac, odr_hdr, sizeof(struct odr_hdr), NULL, 0);
	    if(n < 0)
		goto exit;

	    break;
	}
	case TYPE_PAYLOAD:
	{
	    //What to do with hop?
	    struct payload_hdr *payload_hdr = &odr_hdr->hdr.payload_hdr;
	    struct dest_map *dest_entry;
	    int hdr_len = sizeof(struct odr_hdr);
	    struct msg_rec *recvStruct = NULL;

		printdebuginfo(" payload recieved, hop:%d, data:%s\n", payload_hdr->hop, ((char *)odr_hdr)+ hdr_len);
	    if(odr_hdr->destip == cononicalip)
	    {
		printdebuginfo("Recieved msg on odr, forward to server, %u\n", payload_hdr->port_dest);
		
		writeToServerClient(ntohs(payload_hdr->port_dest), ntohs(payload_hdr->port_src), ((char *)odr_hdr)+hdr_len, payload_hdr->message_len, odr_hdr->sourceip);
		break;
	    }
	    update_dest_map(odr_hdr->sourceip, src_mac, my_mac, socket_address.sll_ifindex, ++payload_hdr->hop, staleness);
	    dest_entry = get_dest_entry(odr_hdr->destip, staleness);
	    if(!dest_entry)
	    {
		char *data = malloc(payload_hdr->message_len);
		struct data_wrapper *data_wrapper = malloc(sizeof(struct data_wrapper));
		data_wrapper->next = NULL;
		printdebuginfo("Dest routing not available\n");
		memcpy(data, (char *)odr_hdr + hdr_len, payload_hdr->message_len);
		memcpy(&data_wrapper->odr_hdr, odr_hdr, sizeof(struct odr_hdr));
		data_wrapper->data = data;
		insert_data_dest_table(data_wrapper, odr_hdr->destip);
		send_rreq(odr_hdr->destip, 0, socket_address.sll_ifindex);
	    }
	    else
	    {
		n = sendframe(dest_entry->nexthop_mac, dest_entry->interface, 
			dest_entry->src_mac, odr_hdr, sizeof(struct odr_hdr), (char *)odr_hdr + hdr_len, payload_hdr->message_len);
		if(n < 0)
		    goto exit;
	    }

	    break;
	}
    }

exit:
    free(buffer);
}

int build_interface_info() 
{
    struct hwa_info *hwa, *hwahead;
    int tinterfaces = 0;
    for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
	struct sockaddr_in  *sin = (struct sockaddr_in *) (hwa->ip_addr);
	if(strcmp(hwa->if_name, "lo") == 0)
	    continue;
	if(strcmp(hwa->if_name, "eth0") == 0) {
	    cononicalip = htonl(sin->sin_addr.s_addr);
	    printdebuginfo(" Cononical IP: %lu\n", ntohl(cononicalip));
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

    free_hwa_info(hwahead);
    return tinterfaces;

}

int sendframe(char *destmac, int interface, char *srcmac, 
	struct odr_hdr *odr_hdr, int hdr_len, void *data, int data_length)
{

    struct sockaddr_ll socket_address;
    int n;
    /*buffer for ethernet frame*/
    void* buffer;
    char src_name[10], dest_name[10];

    /*userdata in ethernet frame*/
    void* data_p;

    /*another pointer to ethernet header*/
    struct ethhdr *eh;

    int len = sizeof(struct ethhdr) + hdr_len + data_length;

    int send_result = 0;

    buffer = (void*)malloc(len);
    eh = buffer;

    data_p = buffer + sizeof(struct ethhdr);
    gethostnamebyaddr(ntohl(odr_hdr->sourceip), src_name);
    gethostnamebyaddr(ntohl(odr_hdr->destip), dest_name);

    printdebuginfo(" Sending msg to %d, data:%s\n", interface, data);

    printf("ODR at %s: sending frame hdr src %s dest ", hostname, hostname);
    for(n = 0; n < IF_HADDR; n++)
	printf("%.2x%s", *(destmac + n) & 0xff, (n == IF_HADDR - 1) ? " " : ":");
    printf("\n\tODR msg type %d src %s  dest %s\n", odr_hdr->type, src_name, dest_name);
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
    memcpy(data_p, odr_hdr, hdr_len);
    if(data)
	memcpy(data_p + hdr_len, data, data_length);
    /*fill the frame with some data*/

    /*send the packet*/
    n = sendto(framefd, buffer, len, 0, 
	    (struct sockaddr*)&socket_address, sizeof(socket_address));
    printdebuginfo(" Frame length sent:%d, hdr_len:%d\n", n, hdr_len);
    if (n < 0) 
    { 
	perror("Error while sending packet.");
    }
    free(buffer);
    return n;
}

static int sendremainingpayload(struct dest_map *dest_entry)
{
    struct data_wrapper *data_wrapper = dest_entry->data, *t;
    dest_entry->data = NULL;
    int n;
    while(data_wrapper != NULL) {
	n = sendframe(dest_entry->nexthop_mac, dest_entry->interface, 
		dest_entry->src_mac, &data_wrapper->odr_hdr, sizeof(struct odr_hdr), data_wrapper->data, data_wrapper->odr_hdr.hdr.payload_hdr.message_len);
	if(n < 0)
	    return n;
	t = data_wrapper;
	data_wrapper = data_wrapper->next;
	printdebuginfo(" debug 3: %s\n", t->data);
	free(t->data);
	free(t);
    }
    return 0;
}

static int forwardrreq(struct odr_hdr *odr_hdr, int interfacetoexclude)
{
    int n;
    tempinterface = hinterface;
    while(tempinterface != NULL)
    {
	if(tempinterface->interfaceno != interfacetoexclude)
	{	
	    n = sendframe(b_mac, tempinterface->interfaceno, 
		    tempinterface->if_haddr, odr_hdr, sizeof(struct odr_hdr), NULL, 0);
	    if(n < 0)
		return n;
	}
	tempinterface = tempinterface->next;
    }
    return 0;
}

static int send_rreq(unsigned long destip, short forceflag, int excludeinterface)
{
    struct odr_hdr odr_hdr;
    struct rreq_hdr *rreq_hdr = &odr_hdr.hdr.rreq_hdr;
    int n;
    memset(&odr_hdr, 0, sizeof(odr_hdr));

    odr_hdr.type = TYPE_RREQ;
    odr_hdr.sourceip = cononicalip;
    odr_hdr.destip = destip;

    rreq_hdr->hop = 0;
    rreq_hdr->broadcastid = broadcastid++;
    rreq_hdr->forceflag = forceflag;
    rreq_hdr->rrep_sent = 0;

    tempinterface = hinterface;
    while(tempinterface != NULL)
    {
	if(tempinterface->interfaceno != excludeinterface)
	{
	    n = sendframe(b_mac, tempinterface->interfaceno, 
		    tempinterface->if_haddr, &odr_hdr, sizeof(struct odr_hdr), NULL, 0);
	    if(n < 0)
		return n;
	}
	tempinterface = tempinterface->next;
    }
    return 0;
}


int process_msg_cs(struct msg_send *msg_content, unsigned port)
{
    struct data_wrapper *data_wrapper = (struct data_wrapper *)malloc(sizeof(struct data_wrapper));
    struct odr_hdr *odr_hdr = &data_wrapper->odr_hdr;
    int data_len, n;
    struct payload_hdr *payload_hdr = &(odr_hdr->hdr.payload_hdr);
    struct dest_map *dest_map;

    odr_hdr->sourceip = cononicalip;
    odr_hdr->type = TYPE_PAYLOAD;
    inet_pton(AF_INET, msg_content->ip, &(odr_hdr->destip));
    odr_hdr->destip = htonl(odr_hdr->destip);
    data_len = strlen(msg_content->msg);
    data_wrapper->data = malloc(data_len + 1);
    memcpy(data_wrapper->data, msg_content->msg, data_len + 1);
    data_wrapper->next = NULL;
    payload_hdr->port_src = htons(port);
    payload_hdr->port_dest = htons(msg_content->port);
    payload_hdr->hop = 0;
    payload_hdr->message_len = data_len + 1;
    dest_map = get_dest_entry(odr_hdr->destip, staleness);
    //Handle when src == dest
	if(cononicalip == odr_hdr->destip) {	
		writeToServerClient(ntohs(payload_hdr->port_dest), ntohs(payload_hdr->port_src), data_wrapper->data, data_len+1, odr_hdr->destip);
		free(data_wrapper->data);
		free(data_wrapper);
		return 0;
	}
    if(dest_map && !msg_content->flag)
    {
	printdebuginfo(" Sending frame to next hop");
	n = sendframe(dest_map->nexthop_mac, dest_map->interface, 
		dest_map->src_mac, odr_hdr, sizeof(struct odr_hdr), data_wrapper->data, data_len + 1);
	if(n < 0)
	    return n;
    }
    else //Store msg in quene 
    {
	insert_data_dest_table(data_wrapper, odr_hdr->destip);	
	n = send_rreq(odr_hdr->destip, msg_content->flag, -1);
	if(n < 0)
	    return n;
    }

    return 0;

}

int main(int argc, char **argv)
{
    struct sockaddr_un cliaddr, odraddr;
    socklen_t clilen;
    fd_set allset;
    int n, tinterfaces = 0;
    struct msg_send msg_content;

    if(argc != 2)
    {
	printf("Usage ODR_anmittal <staleness>\n");
	exit(1);
    }

    staleness = strtol(argv[1], NULL, 10);
    if(errno)
    {
	perror("Error while converting staleness");
	exit(1);
    }
    if(staleness < 0)
    {
	printf("Staleness should be less than 0");
	exit(1);
    }

    gethostname(hostname, sizeof(hostname));

    init_port_table();

    localfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    unlink(ODR_PATH);
    bzero(&odraddr, sizeof(odraddr));
    odraddr.sun_family = AF_LOCAL;
    strcpy(odraddr.sun_path, ODR_PATH);
    Bind(localfd, (SA *) &odraddr, sizeof(odraddr));

    tinterfaces = build_interface_info();

    framefd = Socket(AF_PACKET, SOCK_RAW, htons(PROTO));

    while(1)
    {
	FD_ZERO(&allset);
	FD_SET(localfd, &allset);
	FD_SET(framefd, &allset);
	n = select(max(localfd, framefd) + 1, &allset, NULL, NULL, NULL);
	if(n < 0) {
	    perror("Error during select, exiting.");
	    goto exit;
	}
	if(FD_ISSET(framefd, &allset)) {
	    //Recieved message from other vm
	    recieveframe();
	}
	if(FD_ISSET(localfd, &allset)) {
	    int port;
	    //Recieved message from client/server
	    clilen = sizeof(cliaddr);
	    memset(&cliaddr, 0, sizeof(cliaddr));
	    n = recvfrom(localfd, &msg_content, sizeof(msg_content), 0, (SA*)&cliaddr, &clilen);
	    printdebuginfo(" Message from client/server %d, %d, %s, %s\n", msg_content.port, msg_content.flag, msg_content.msg, msg_content.ip);
	    printdebuginfo(" Cli sun_name:%s\n", cliaddr.sun_path);
	    port = update_ttl_ptable(cliaddr.sun_path);
	    printf("Assigned port %u\n", htons(port));
	    n = process_msg_cs(&msg_content, port);
	    if(n < 0)
		goto exit;
	}
    }
    //Listen(localfd, LISTENQ);
    /*
     */
exit:
    free_interface_info();

}


