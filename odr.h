
#ifndef _ODR_H
#define _ODR_H

#include "common.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#include "porttable.h"

#include "hw_addrs.h"

#define TYPE_RREQ 0
#define TYPE_RREP 1
#define TYPE_PAYLOAD 2

struct interface_info 
{
    char if_haddr[IF_HADDR];
    unsigned long ip;
    int interfaceno;
    struct interface_info *next;
};

struct payload_hdr
{
    unsigned int port_src, port_dest;	
    uint32_t hop, message_len;
};

struct rrep_hdr
{
    uint32_t hop, broadcastid;
    short forceflag;
};


struct rreq_hdr
{
    uint32_t hop, broadcastid;
    short forceflag;
    short rrep_sent;
};

struct odr_hdr
{
    int type; 
    long int sourceip, destip;
    union {
	struct rreq_hdr rreq_hdr;
	struct rrep_hdr rrep_hdr;
	struct payload_hdr payload_hdr;
    } hdr;
};


#endif
