
#ifndef _PORT_TABLE_H
#define _PORT_TABLE_H
#define PORT_TABLE_TTL 10

struct portentry
{
    unsigned int port;
    char sun_path[108];
    long timestamp;
    struct portentry *next, *prev;
};

static unsigned int emperalport = 32768;
static struct portentry *hportentry = NULL, *tportentry = NULL;

static void init_port_table()
{   
    hportentry = tportentry = (struct portentry *) malloc(sizeof(struct portentry));
    hportentry->port = SERVER_PORT;
    strcpy(hportentry->sun_path, SERVER_PATH);
    hportentry->timestamp = 0;
    hportentry->next = hportentry->prev = NULL;
}

static void insert_into_port_table(unsigned int port, char *sun_path)
{
    struct portentry *temp = (struct portentry *) malloc(sizeof(struct portentry));
    temp->port = port;
    strcpy(temp->sun_path, sun_path);
    temp->timestamp = time(NULL);
    temp->next = NULL;
    temp->prev = tportentry;
    tportentry->next = temp;
    tportentry = temp;
}

static void delete_port_entry(struct portentry *nportentry)
{
    if(tportentry == nportentry)
	tportentry = nportentry->prev;
    nportentry->prev->next = nportentry->next;
    if(nportentry->next)
	nportentry->next->prev = nportentry->prev;
    free(nportentry);
}

static int update_ttl_ptable(char *sun_path)
{
    struct portentry *temp = hportentry;
    while(temp != NULL)
    {
	if(strcmp(temp->sun_path, sun_path) == 0)
	{
	    if(temp->timestamp != 0)
		temp->timestamp = time(NULL);
	    return temp->port;
	}
	temp = temp->next;
    }
    insert_into_port_table(++emperalport, sun_path);
    return emperalport;
}

static char *get_sun_path(unsigned int port)
{
    struct portentry *temp = hportentry;
    while(temp != NULL)
    {
	printdebuginfo("Port %u, %u\n", port, temp->port);
	if(temp->port == port)
	{ 
	    if(temp->timestamp !=0 && time(NULL) - temp->timestamp > PORT_TABLE_TTL)
	    {
		printdebuginfo("Deleting port entry, %ld, %ld, %ld\n", temp->timestamp, time(NULL), time(NULL) - temp->timestamp);
		delete_port_entry(temp);
		return NULL;
	    }
	    return temp->sun_path;
	} else if(temp->timestamp !=0 && time(NULL) - temp->timestamp > PORT_TABLE_TTL)
	{
	    struct portentry *t = temp;
	    temp = temp->next;
	    delete_port_entry(t);
	    continue;
	}
	temp = temp->next;
    }
    return NULL;
}

#endif
