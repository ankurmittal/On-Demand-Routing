
#ifndef _ODR_H
#define _ODR_H

#include "common.h"

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include "hw_addrs.h"

struct interface_info {
    char if_haddr[IF_HADDR];
    unsigned long ip;
    int interfaceno;
    struct interface_info *next;
};

#endif
