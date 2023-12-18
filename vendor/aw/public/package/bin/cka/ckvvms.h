/* DEC/CMS REPLACEMENT HISTORY, Element CKVVMS.H */
/* *3    29-AUG-1989 00:32:39 BUDA "Add ^Y/^C from server mode" */
/* *2    16-APR-1989 17:54:51 BUDA "Add definition of SUB_PROC buffer size" */
/* *1    11-APR-1989 22:57:00 BUDA "Initial creation" */
/* DEC/CMS REPLACEMENT HISTORY, Element CKVVMS.H */
/*
 * 006 22-Aug-1993 fdc     Add prototype for print_msg().
 * 005 26-Aug-1992 tmk	   Remove unused CKVRMS32 stuff
 * 004 01-Dec-1989 mab	   Add RMS support
 * 002 01-Aug-1989 mab	   Add partial buffer definition
 * 002 08-Jul-1989 mab	   Add ^C/^Y code to server mode
 * 001 05-Mar-1989 mab     Put some common pieces in one place
 */
/*
  C-Kermit structures and symbols for interrupt, terminal control, and i/o
  for VMS systems

  Mark A. Buda, Digital Equipment Corporation
*/
struct iosb_struct {
    unsigned short status;
    unsigned short size;
    unsigned short terminator;
    unsigned short termsize;
};

struct tt_mode {
    unsigned char class;
    unsigned char type;
    unsigned short width;
    unsigned int basic : 24;
    unsigned char length;
    unsigned long extended;
};

struct itmlst {
    unsigned short len;
    unsigned short code;
    char *adr;
    char *retlen;
};

struct sysatr {
    unsigned long int retval;	/* Address or value returned */
    unsigned long int len;	/* length of item */
    unsigned long int type;
    char *bpnt;			/* Buffer pointer */
    unsigned long int bcnt;	/* Buffer count */
    char *tbuf_adr;		/* Starting address of tmp buffer */
    char *tbuf_pnt;		/* Address within buffer */
    unsigned long int tbuf_len; /* Length of buffer */
    unsigned long int tbuf_cnt;
};

struct trmmbx_message {			/* jah - for edit 031 in ckvtio.c */
    unsigned short type;		/* Mailbox message type */
    unsigned short unit;		/* TTY unit number */
    unsigned char ctlname_size;		/* TTY controller name length */
    char ctlname[15];			/* TTY controller name */
    /* Add broadcast message stuff here if/when needed */
};

struct trmmbx_struct {			/* jah - for edit 031 in ckvtio.c */
    unsigned short channel;		/* Mailbox channel */
    unsigned short efn;			/* EFN we set for unsolicited input */
    unsigned long status;		/* Completion status set by AST code */
    struct iosb_struct iosb;		/* I/O status block for mailbox read */
    struct trmmbx_message message;	/* A short mailbox message buffer */
};

/*  Event flags used for I/O completion testing  */
#define CON_EFN 1
#define TTY_EFN 2
#define TIM_EFN 3
#define QIOW_EFN 4

/* Error checking and reporting macros */

int print_msg();

#define SUCCESS(x) ((vms_status = (x)) & 1)
#define CHECK_ERR(s,x) (SUCCESS(x) ? 1 : print_msg(s))
/*
 * Module CKVTIO
 * Routine print_msg(sts) 	! Print VMS error message.
 * define buffer sizes that are malloced at routine first call.
 */
#define PMSG_BUF_SIZE 255
#define PMSG_MSG_SIZE PMSG_BUF_SIZE+5

/*
 * Size of subprocess buffer
 * This will have a direct affect on the size of a message that comes
 * from the subprocess mailbox.
 */
#define SUB_BUF_SIZE 300

#ifndef DEVNAMLEN
#define DEVNAMLEN 64
#endif

/*
 * The following macro was borrowed from a posting on USENET - Thank you.
 */

#define IDENT(arg) arg	/* Handy macro expands to its arguments */

#define VSTRING(name, maxlen, init)	\
    struct IDENT(name)_vs {		\
	struct dsc$descriptor_s dsc;	\
	unsigned short int curlen;	\
	char body[maxlen];		\
    } name = {{maxlen, DSC$K_DTYPE_VT, DSC$K_CLASS_VS, &name.curlen}, \
	sizeof(init)-1, init}

/*
 * Example of using VSTRING:
 *
 * VSTRING(user,12,"Humpty Dumpty");
 * VSTRING(line,256,"");
 *
 */

/*
 * UCB$M_* symbols are not in a 'C' usable format.  Define this
 * symbol for use in CKVTIO, when doing a getdviw()
 */

#ifndef UCB$M_ONLINE
#define UCB$M_ONLINE 16
#endif

/*
 * Used by parsedir as the flags argument
 */

#define PARSE_NODE	0x001
#define PARSE_DEVICE	0x002
#define PARSE_DIRECTORY	0x004
#define PARSE_NAME	0x008
#define PARSE_TYPE	0x010
#define PARSE_VERSION	0x020

/*
 *
 */

#define CKV_M_CTRLY (1 << ('Y'-64))
#define CKV_M_CTRLC (1 << ('C'-64))
#define CKV_K_CTRLC ('C'-64)
#define CKV_K_CTRLY ('Y'-64)

/*
 * Partial Buffer size.  This is used by zoutdump() when it gets
 * an incomplete record.
 */

#define PARTIAL_BUF_SIZE 1024

