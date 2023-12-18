#ifndef __IF_NAMEINDEX_H_ /* include guard */
#define __IF_NAMEINDEX_H_

#include <netinet/in.h>
#include <netinet/in6.h>
//#include <linux/in.h>
//#include <linux/in6.h>
#include <linux/rtnetlink.h>

#define   AF_LINK         18              /* Link layer interface */

#if 0
struct if_nameindex {
    unsigned int   if_index;  /* 1, 2, ... */
    char          *if_name;   /* null terminated line name */
};
#endif

struct ifaddrs
{
    struct ifaddrs *ifa_next;
    char            ifa_name[16];
    int             ifa_ifindex;
    struct sockaddr *ifa_addr;
    struct sockaddr_storage ifa_addrbuf;
};

struct if_nameindex *if_nameindex(void);


#endif /* __IFTOP_H_ */
