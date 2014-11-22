#include "destmap.h"

struct dest_map* insert_in_map(struct data_wrapper *data_wrapper, unsigned long destip) {
    struct dest_map *newNode = malloc(sizeof(struct dest_map));
    bzero(newNode, sizeof(struct dest_map));
    newNode->destip = destip;
    newNode->data = data_wrapper;
    newNode->hop = -1;
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
    newNode->timestamp = currentTimeInMillis();
    return newNode;

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
			free(tempData->data);
                        tempData->data = data_wrapper->data;
                        free(data_wrapper);
                        return;
                    }
                }
            }
            *i_data = data_wrapper;
            return;
        }
        temp = temp->next;
	i_data = &temp->data;
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
    staleness = staleness * 1000;
    while (temp != NULL) {
        if(temp->destip == destip) {
	    if(temp->resolved == 0)
		return NULL;
            diff = currentTimeInMillis() - temp->timestamp;
            if (diff < staleness)
                return temp;
	    printdebuginfo("Stale Entry for ip:%lu, staleness:%ld, diff:%ld, time:%ld, times:%ld\n", ntohl(destip), staleness, diff, currentTimeInMillis(), temp->timestamp);
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
struct dest_map *update_dest_map(unsigned long destip, char *nexthop_mac, char *src_mac, 
    int interface, uint32_t hop, unsigned long staleness) {
    int found = 0;
    staleness = staleness * 1000;
    struct dest_map *temp = hdestmap, *result = NULL;

    while (temp != NULL) {
        if(temp->destip == destip) {
            if (temp->resolved == 0 || temp->hop > hop || (currentTimeInMillis() - temp->timestamp) > staleness) {
		printdebuginfo(" Will update table for destip:%lu\n", ntohl(destip));
		break;
            } else if(temp->hop == hop)
	    {
		temp->timestamp = currentTimeInMillis();
	    }
            return NULL;
        } 
	temp = temp->next;
    }
    if(!temp)
	temp = insert_in_map(NULL, destip);
    memcpy(temp->nexthop_mac, nexthop_mac, 6);
    memcpy(temp->src_mac, src_mac, 6);
    temp->timestamp = currentTimeInMillis();
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
