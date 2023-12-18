/*
  NOTE: DEC C on OpenVMS AXP does not like an empty header file,
  so we include the following header file unconditionally.
*/
#include "ckcdeb.h"			/* Kermit universals */

#ifdef DEC_TCPIP
#ifdef VMS
/*
  ioctl() similation for DEC TCP/IP, based on DEC example.
  Used only for DEC TCP/IP (nee UCX).
*/
#include "ckvioc.h"			/* IOCTL-specific definitions */

#define ISOK(s) (s & 01)		/* For checking $QIOW return value */
/*
  Select proper library function for getting socket device channel.
*/
#if defined (__DECC)
# define GET_SDC decc$get_sdc
#elif (defined (VAXC) || defined (__VAXC) || defined(__GNUC__))
# define GET_SDC vaxc$get_sdc
#else
# error unknown compiler, not DECC and not VAXC
#endif /* __DECC */

#ifdef __DECC
#include <starlet.h>
#include <lib$routines.h>
#include <socket.h>
#endif /* __DECC */

#ifndef UCX$C_IOCTL
#define UCX$C_IOCTL 2
#endif /* UCX$C_IOCTL */

int
ioctl(d, request, argp)
    int d, request;
#ifdef __DECC
    void
#else
    char
#endif /* __DECC */
    *argp;
{

    int eflagnum;			/* Event Flag Number */
    int sdc;				/* Socket Device Channel */
    int status;				/* QIOW return code */
    unsigned short fn;			/* QIOW function code  */
    unsigned short iosb[4];		/* IO Status Block */

    struct comm {
	int command;
	char *addr;
    } ioctl_comm;			/* QIOW ioctl commands. */

    struct it2 {
	unsigned short len;
	unsigned short opt;
	struct comm *addr;
    } ioctl_desc;			/* QIOW IOCTL commands descriptor */

#ifdef CK_GETEFN
/*
  It should not be necessary to ask the system for an EFN because:

    (a) the $QIOW will do a $SYNC
    (b) there is an explicit IOSB (needed for correct multiprocessor operation)
    (c) we are not threaded
    (d) both the $QIOW return status and the IOSB status are checked
*/
    status = lib$get_ef(&eflagnum);	/* Get an event flag number. */
    if (!ISOK(status))			/* Did we? */
      eflagnum = 0;			/* No event flag available, use 0. */
#else
    eflagnum = 0;			/* Use event flag number 0 */
#endif /* CK_GETEFN */

    sdc = GET_SDC(d);			/* Get socket device channel number. */
    if (sdc == 0) {
	errno = EBADF;			/* Not an open socket descriptor. */
	return -1;
    }
    ioctl_desc.opt = UCX$C_IOCTL;	/*  Fill in ioctl descriptor. */
    ioctl_desc.len = sizeof(struct comm);
    ioctl_desc.addr = &ioctl_comm;

/* Decide QIOW function code and In / Out parameter. */

    ioctl_comm.command = request;
    ioctl_comm.addr = argp;
    if (request & IOC_OUT) {
	fn = IO$_SENSEMODE;
	status = sys$qiow(eflagnum,
			  sdc, fn, iosb, 0, 0, 0, 0, 0, 0, 0, &ioctl_desc);
    } else {
	fn = IO$_SETMODE;
	status = sys$qiow(eflagnum,
			  sdc, fn, iosb, 0, 0, 0, 0, 0, 0, &ioctl_desc, 0);
    }
    if (!ISOK(status)) {
	debug(F101,"ioctl failed: status","",status);
	errno = status;
	return -1;
    }
    if (!ISOK(iosb[0])) {
#ifdef DEBUG
	char tmpbuf[80];
	sprintf(tmpbuf,"ioctl failed: status = %x, %x, %x%x\n",
		iosb[0], iosb[1], iosb[3], iosb[2]);
	debug(F100,(char *)tmpbuf,"",0);
#endif /* DEBUG */
	errno = (long int) iosb[0];
	return -1;
    }
#ifdef CK_GETEFN
    status = lib$free_ef(&eflagnum);
#endif /* CK_GETEFN */
    return 0;
}
#endif /* VMS */
#endif /* DEC_TCPIP */
