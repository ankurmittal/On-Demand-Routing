#include "destmap.h"

struct dest_map* insert_in_map(struct data_wrapper *data_wrapper, unsigned long destip) {
    struct dest_map *newNode = malloc(sizeof(struct dest_map));
    bzero(newNode, sizeof(struct dest_map));
    newNode->destip = destip;
    newNode->data = data_wrapper;
    newNode->resolved = 0;
    newNode->next = NULL;
    if(hdestmap == NULL)
    {
	hdestmap = tdestmap = newNode;
    }  else
    {
	tdestmap->next = newNode;
	tdestmap = newNode;
    }
    return newNode;

}

long currentTimeInMillis() {
    long ms = 0;
    struct timeval now;
    gettimeofday(&now, NULL);
    ms = (now.tv_sec*1000) + (now.tv_usec/1000);
    return ms;
}

/*
 * Will check sourceip and port no from odr_hdr,
 * if they are same, just replace data, else insert new node
 */
void insert_data_dest_table(struct data_wrapper *data_wrapper, unsigned long destip) {
    int insertFlag = 0;
    struct dest_map *temp = hdestmap;
    struct data_wrapper *tempData, **i_data = &temp->data;
    
    while (temp != NULL) {
        if(temp->destip == destip) {
            tempData = temp->data;
            while (tempData != NULL) {
		i_data = &tempData->next;
                if (tempData->odr_hdr.sourceip == data_wrapper->odr_hdr.sourceip) {
                    if ((tempData->odr_hdr.hdr.payload_hdr.port_src == data_wrapper->odr_hdr.hdr.payload_hdr.port_src)
                        && (tempData->odr_hdr.hdr.payload_hdr.port_dest== data_wrapper->odr_hdr.hdr.payload_hdr.port_dest)) {
                        tempData->data = data_wrapper->data;
                        free(data_wrapper);
                        return;
                    }
                }
            }
            *i_data = data_wrapper;
            return;
        } else
            temp = temp->next;
    }
    insert_in_map(data_wrapper, destip);
}

/*
 * if entry is stale, mark resolved as 0 and return NULL,
 * else return node. Also if map doesn't contain this destip,
 * create new node and add it and return NULL
 */
struct dest_map *get_dest_entry(unsigned long destip, long staleness) {
    long diff = 0;
    struct dest_map *temp = hdestmap;
    while (temp != NULL) {
        if(temp->destip == destip) {
            diff = currentTimeInMillis() - temp->timestamp;
            if (diff < staleness)
                return temp;
            temp->resolved = 0;
	    return NULL;
        } 
        temp = temp->next;
    }
    insert_in_map(NULL, destip);
    return NULL;
}

/*
 * if stale or new hop less than this hop, update entry,
 * if destip not found, add the entry. if entry is updated or added,
 * return this entry else return null
 */
struct dest_map *update_dest_map(unsigned long destip, char *dest_mac, char *src_mac, 
    int interface, uint32_t hop, unsigned long staleness) {
    int found = 0;
    struct dest_map *temp = hdestmap, *result = NULL;

    while (temp != NULL) {
        if(temp->destip == destip) {
            if (temp->hop > hop || (temp->timestamp - currentTimeInMillis()) > staleness) {
		break;
            }
            return NULL;
        } 
	temp = temp->next;
    }
    if(!temp)
	temp = insert_in_map(NULL, destip);
    memcpy(temp->dest_mac, dest_mac, 6);
    memcpy(temp->src_mac, src_mac, 6);
    temp->interface = interface;
    temp->hop = hop;
    temp->resolved = 1;
    return temp;
}

/*
 * Return first data_wrapper node
 */
struct data_wrapper *get_data_from_queue(unsigned long destip) {
    struct dest_map *temp = hdestmap;
    while (temp != NULL) {
	if(temp->destip == destip) {
	    return temp->data;
	} else
	    temp = temp->next;
    }
    return NULL;
}
