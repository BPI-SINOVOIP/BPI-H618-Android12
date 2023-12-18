/*	$OpenBSD: if_nameindex.c,v 1.10 2010/09/24 13:29:29 claudio Exp $	*/
/*	$KAME: if_nameindex.c,v 1.7 2000/11/24 08:17:20 itojun Exp $	*/

/*-
 * Copyright (c) 1997, 2000
 *	Berkeley Software Design, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY Berkeley Software Design, Inc. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Berkeley Software Design, Inc. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	BSDI Id: if_nameindex.c,v 2.3 2000/04/17 22:38:05 dab Exp
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if_dl.h>
#include <net/if.h>
//#include <ifaddrs.h>
#include <stdlib.h>
#include <string.h>
#include "if_nameindex.h"

/*
 * From RFC 2553:
 *
 * 4.3 Return All Interface Names and Indexes
 *
 *    The if_nameindex structure holds the information about a single
 *    interface and is defined as a result of including the <net/if.h>
 *    header.
 *
 *       struct if_nameindex {
 *         unsigned int   if_index;
 *         char          *if_name;
 *       };
 *
 *    The final function returns an array of if_nameindex structures, one
 *    structure per interface.
 *
 *       struct if_nameindex  *if_nameindex(void);
 *
 *    The end of the array of structures is indicated by a structure with
 *    an if_index of 0 and an if_name of NULL.  The function returns a NULL
 *    pointer upon an error, and would set errno to the appropriate value.
 *
 *    The memory used for this array of structures along with the interface
 *    names pointed to by the if_name members is obtained dynamically.
 *    This memory is freed by the next function.
 *
 * 4.4.  Free Memory
 *
 *    The following function frees the dynamic memory that was allocated by
 *    if_nameindex().
 *
 *        #include <net/if.h>
 *
 *        void  if_freenameindex(struct if_nameindex *ptr);
 *
 *    The argument to this function must be a pointer that was returned by
 *    if_nameindex().
 */

int parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
        while (RTA_OK(rta, len)) {
                if (rta->rta_type <= max)
                        tb[rta->rta_type] = rta;
                rta = RTA_NEXT(rta,len);
        }
        return 0;
}

static void recvaddrs(int fd, struct ifaddrs **ifa, __u32 seq)
{
        char    buf[8192];
        struct sockaddr_nl nladdr;
        struct iovec iov = { buf, sizeof(buf) };
        struct ifaddrmsg *m;
        struct rtattr * rta_tb[IFA_MAX+1];
        struct ifaddrs *I;

        while (1) {
                int status;
                struct nlmsghdr *h;

                struct msghdr msg = {
                        (void*)&nladdr, sizeof(nladdr),
                        &iov,   1,
                        NULL,   0,
                        0
                };

                status = recvmsg(fd, &msg, 0);

                if (status < 0)
                        continue;

                if (status == 0)
                        return;

                if (nladdr.nl_pid) /* Message not from kernel */
                        continue;

                h = (struct nlmsghdr*)buf;
                while (NLMSG_OK(h, status)) {
                        if (h->nlmsg_seq != seq)
                                goto skip_it;

                        if (h->nlmsg_type == NLMSG_DONE)
                                return;

                        if (h->nlmsg_type == NLMSG_ERROR)
                                return;

                        if (h->nlmsg_type != RTM_NEWADDR)
                                goto skip_it;

                        m = NLMSG_DATA(h);

                        if (m->ifa_family != AF_INET &&
                            m->ifa_family != AF_INET6)
                                goto skip_it;

                        if (m->ifa_flags&IFA_F_TENTATIVE)
                                goto skip_it;

                        memset(rta_tb, 0, sizeof(rta_tb));
                        parse_rtattr(rta_tb, IFA_MAX, IFA_RTA(m), h->nlmsg_len - NLMSG_LENGTH(sizeof(*m)));

                        if (rta_tb[IFA_LOCAL] == NULL)
                                rta_tb[IFA_LOCAL] = rta_tb[IFA_ADDRESS];
                        if (rta_tb[IFA_LOCAL] == NULL)
                                goto skip_it;

                        I = malloc(sizeof(struct ifaddrs));
                        if (!I)
                                return;
                        memset(I, 0, sizeof(*I));

                        I->ifa_ifindex = m->ifa_index;
                        I->ifa_addr = (struct sockaddr*)&I->ifa_addrbuf;
                        I->ifa_addr->sa_family = m->ifa_family;
                        if (m->ifa_family == AF_INET) {
                                struct sockaddr_in *sin = (void*)I->ifa_addr;
                                memcpy(&sin->sin_addr, RTA_DATA(rta_tb[IFA_LOCAL]), 4);
                        } else {
                                struct sockaddr_in6 *sin = (void*)I->ifa_addr;
                                memcpy(&sin->sin6_addr, RTA_DATA(rta_tb[IFA_LOCAL]), 16);
                                if (IN6_IS_ADDR_LINKLOCAL(&sin->sin6_addr))
                                        sin->sin6_scope_id = I->ifa_ifindex;
                        }
                        I->ifa_next = *ifa;
                        *ifa = I;

skip_it:
                        h = NLMSG_NEXT(h, status);
                }
                if (msg.msg_flags & MSG_TRUNC)
                        continue;
        }
        return;
}

static int getifaddrs(struct ifaddrs **ifa0)
{
        struct {
                struct nlmsghdr nlh;
                struct rtgenmsg g;
        } req;
        struct sockaddr_nl nladdr;
        static __u32 seq;
        struct ifaddrs *i;
        int fd;

        fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
        if (fd < 0)
                return -1;

        memset(&nladdr, 0, sizeof(nladdr));
        nladdr.nl_family = AF_NETLINK;

        req.nlh.nlmsg_len = sizeof(req);
        req.nlh.nlmsg_type = RTM_GETADDR;
        req.nlh.nlmsg_flags = NLM_F_ROOT|NLM_F_MATCH|NLM_F_REQUEST;
        req.nlh.nlmsg_pid = 0;
        req.nlh.nlmsg_seq = ++seq;
        req.g.rtgen_family = AF_UNSPEC;

        if (sendto(fd, (void*)&req, sizeof(req), 0, (struct sockaddr*)&nladdr, sizeof(nladdr)) < 0) {
                close(fd);
                return -1;
        }

        *ifa0 = NULL;

        recvaddrs(fd, ifa0, seq);

        close(fd);

        fd = socket(AF_INET, SOCK_DGRAM, 0);

        for (i=*ifa0; i; i = i->ifa_next) {
                struct ifreq ifr;
                ifr.ifr_ifindex = i->ifa_ifindex;
                ioctl(fd, SIOCGIFNAME, (void*)&ifr);
                memcpy(i->ifa_name, ifr.ifr_name, 16);
        }
        close(fd);

        return 0;
}

static void freeifaddrs(struct ifaddrs *ifa0)
{
        struct ifaddrs *i;

        while (ifa0) {
                i = ifa0;
                ifa0 = i->ifa_next;
                free(i);
        }
}


struct if_nameindex *if_nameindex(void)
{
	struct ifaddrs *ifaddrs, *ifa;
	unsigned int ni;
	size_t nbytes;
	struct if_nameindex *ifni, *ifni2;
	char *cp;

	if (getifaddrs(&ifaddrs) < 0)
		return(NULL);

	/*
	 * First, find out how many interfaces there are, and how
	 * much space we need for the string names.
	 */
	ni = 0;
	nbytes = 0;
	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr &&
		    ifa->ifa_addr->sa_family == AF_LINK) {
			nbytes += strlen(ifa->ifa_name) + 1;
			ni++;
		}
	}

	/*
	 * Next, allocate a chunk of memory, use the first part
	 * for the array of structures, and the last part for
	 * the strings.
	 */
	cp = malloc((ni + 1) * sizeof(struct if_nameindex) + nbytes);
	ifni = (struct if_nameindex *)cp;
	if (ifni == NULL)
		goto out;
	cp += (ni + 1) * sizeof(struct if_nameindex);

	/*
	 * Now just loop through the list of interfaces again,
	 * filling in the if_nameindex array and making copies
	 * of all the strings.
	 */
	ifni2 = ifni;
	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr &&
		    ifa->ifa_addr->sa_family == AF_LINK) {
			ifni2->if_index =
			    ((struct sockaddr_dl*)ifa->ifa_addr)->sdl_index;
			ifni2->if_name = cp;
			nbytes = strlen(ifa->ifa_name) + 1;
			memcpy(cp, ifa->ifa_name, nbytes);
			ifni2++;
			cp += nbytes;
		}
	}
	/*
	 * Finally, don't forget to terminate the array.
	 */
	ifni2->if_index = 0;
	ifni2->if_name = NULL;
out:
	freeifaddrs(ifaddrs);
	return(ifni);
}

void
if_freenameindex(struct if_nameindex *ptr)
{
	free(ptr);
}
