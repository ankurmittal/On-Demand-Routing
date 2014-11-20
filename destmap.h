
#ifndef _DEST_MAP_H
#define _DEST_MAP_H

#include "odr.h"
struct data_wrapper
{
  struct odr_hdr odr_hdr;
  char *data;
  struct data_wrapper *next;
};

struct dest_map
{
    unsigned long destip;
    char next_hop_mac[6];
    int interface;
    uint32_t hop;
    long timestamp; //in millis
    struct data_wrapper *data; //NULL if no data
    int resolved;
    struct dest_map *next;
}
static struct dest_map *hdestmap = NULL, *tdestmap = NULL;

//Will check sourceip and port no from odr_hdr, if they are same, just replace data, else insert new node
void insert_data_dest_table(struct data_wrapper data_wrapper, unsigned long destip);

// if entry is stale, mark resolved as 0 and return NULL, else return node
// Also if map doesn't contain this destip, crate new node and add it and return NULL
struct dest_map get_dest_entry(unsigned long destip, long staleness);

//if stale or new hop less than this hop, update entry, if destip not found, add the entry. if entry is updated or added, return this entry else return null
struct dest_mac update_dest_map(unsigned long destip, char *next_hop_mac, int interface, uint32_t hop, unsigned long staleness);

//Return first data_wrapper node
struct data_wrapper get_data_from_queue(unsigned long destip);

#endif
