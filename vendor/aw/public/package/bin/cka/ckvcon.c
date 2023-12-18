#ifndef VMS
      ERROR -- CKVCON.C is used only on the OpenVMS(tm) Operating System
#else  /* VMS */
#ifdef MULTINET
#define MULTINET_OLD_STYLE
#endif /* MULTINET */
#endif /* VMS */

char *connv = "CONNECT Command 8.0.063 10 March 2010";

/*  C K V C O N  --  Terminal session to remote system, for VMS  */
/*
  Author: Frank da Cruz <fdc@columbia.edu>,
  Columbia University Academic Information Systems, New York City.

  Copyright (C) 1985, 2010,
    Trustees of Columbia University in the City of New York.
    All rights reserved.  See the C-Kermit COPYING.TXT file or the
    copyright text in the ckcmai.c module for disclaimer and permissions.
*/
/*
 * Originally adapted from the UNIX C-Kermit CONNECT module by S. Rubenstein
 * for systems without fork(), circa 1985.  This version of conect() uses
 * contti(&c, &src) to return when a character is available from either the
 * console or the communications connection -- kind of like select().
 *
 * Edit Date       By   What
 *
 * 010 06-Mar-1989 mab	General cleanup
 * 011 23-Mar-1989 mab	Clean up doesc() code.  Add malloc() in place of
 *		 	static buffer space.  Also increase buffer space.
 * 012 27-Sep-1989 mab	Add XON sequence.
 * 013 30-Mar-1991 fdc  Add so/si, character set translation.
 * 014 06-Apr-1991 fdc  Adapted for TGV MultiNet TCP/IP connections.
 * 015 21-Jun-1991 tmk  Cleaned up typo in session logging display.
 * 016 28-Nov-1991 fdc  "Back at <hostname>", disallow CONNECT in background
 * 017 25-Dec-1991 fdc  Added support for NOPUSH, added "Hanging up" message
 * 018 11-Jan-1992 fdc  Added support for key mapping
 * 019 27-Jan-1992 fdc  Added support for ANSI escape sequence recognition
 * 020 00-May-1992 fdc  Rewrote conect() to used buffered i/o.
 * 021 10-Jun-1992 fdc  Added support for Wollongong WIN/TCP from Ray Hunter.
 *                      Fix messed-up help message.
 * 022 20-Jun-1992 fdc  Fixed handling of CR on TELNET sessions.
 * 023 28-Jun-1992 fdc  Cosmetic cleanup of "Connecting..." message.
 * 024 12-Jul-1992 fdc  Added mdmhup() feature (see ckudia.c).
 * 025 29-Jul-1992 fdc  Grouped TELNET IP and AYT into single writes.
 * 026 13-Aug-1992 fdc  Added support for SET TELNET NEWLINE-MODE.
 * 027 05-Sep-1992 lt   Added architecture ifdefs for OpenVMS, Alpha, at top.
 * 028 08-Sep-1992 fdc  Separated input and output SO/SO shift states.
 * 029 11-Oct-1992 fdc  Reduce modem-vs-net confusion in ttopen() calls.
 * 030 27-Oct-1993 fdc  Correct a typo in network protocol-checking code.
 * 031 23-Oct-1993 fdc  Reset i/o buffer pointers upon new connection.
 * 032 14-Feb-1994 fdc  Some minor cleanups.
 * 033  9-Mar-1994 fdc  Fixed failure to display quoted TELNET IAC (255).
 * 034  2-Jul-1994 fdc  Add initial APC support.
 * 035  9-Jul-1994 fdc  Fix APC support.
 * 036 15-Sep-1994 fdc  Add support for SET TELNET NEWLINE RAW.
 * 037 24-Mar-1996 fdc  Add support for nopush and autodownload.
 * 038 11-May-1996 fdc  Add support for SET TERM ESC { ENABLED, DISABLED }
 * 039  5-Jun-1996 fdc  Change H to mean Help rather than Hangup, U = hangUp.
 * 040  6-Jun-1996 fdc  Add Autoupload.
 * 041  6-Sep-1996 fdc  Try to handle parity better.
 * 042  1-Sep-1997 fdc  Add triggers.
 * 043 21-Sep-1997 fdc  Don't send NAK any more if autodownload detected.
 * 044  1-Jan-1998 fdc  Enforce SET CARRIER-WATCH.
 * 045  8-Feb-1998 fdc  In ckcgetc() allow contti() to return nothing.
 * 046 27-Dec-1998 fdc  Remove obsolete references to me_binary.
 * 047  8-Feb-1999 fdc  Allow for -2 and -3 returns from tn_doop().
 * 048 22-Jul-1999 fdc  Fix <escchar>S bug (print buffer sbuf[] too small)
 *                      and added more info to status display.
 * 049 24-Aug-1999 fdc  Add Unicode (UTF-8) support.
 * 050 24-Oct-1999 fdc  Correct #ifdefs for NOCSETS case.
 * 051 30-Oct-1999 fdc  Fix broken escape sequence recognizer.
 * 052  4-Nov-1999 fdc  Keep autodownload trigger to ourselves.
 * 053 15-Nov-1999 fdc  Fix broken escape sequence recognizer again.
 * 054 24-Nov-2000 fdc  Add learned scripts (search for CKLEARN).
 * 055  7-Dec-2000 mb   Fix Multinet/DECC header file conflict.
 * 056  5-May-2001 fdc  Use logchar() for session logging.
 * 057 27-Jun-2001 fdc  Set cx_status to indicate CONNECT status, fix ^\u.
 * 058 11-Sep-2002 fdc  Fix remote charset to local UTF-8 translation.
 * 059 24-Oct-2002 jea  Add SSL/TLS support (preliminary).
 * 060  5-Nov-2002 jea  More SSL work.
 * 061 29-Nov-2002 jea  Fix disconnection returns codes, which makes SSL
 *                      connections terminate correctly, plus fixes to Telnet
 *                      Com Port Control.
 * 062 05-Apr-2004 fdc  Allow for IA64 builds.
 * 063 15-Mar-2010 fdc  Make inesc[] and oldesc[] not static (for INPUT)
 */
#include "ckcdeb.h"
#include "ckcasc.h"
#include "ckcker.h"
#include "ckucmd.h"
#include "ckcnet.h"
#include "ckvvms.h"
#ifndef NOCSETS
#include "ckcxla.h"			/* Character set translation */
#endif /* NOCSETS */
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>

#ifdef CKLEARN
#include <time.h>
#endif /* CKLEARN */

static int src;				/* Where input character came from */

extern long speed;

extern int local, escape, duplex, parity, flow, seslog, mdmtyp, batch;
extern int cmask, cmdmsk, debses, sosi, ttyfd, what, quiet, tnlm, hints,
 tt_crd, tn_nlm, tt_escape, itsatty;
extern char ttname[], sesfil[], myhost[];
#ifdef TNCODE
extern int tn_b_nlm;
#endif /* TNCODE */

#ifdef CKLEARN
extern FILE * learnfp;
extern int learning;
static ULONG learnt1;
static char learnbuf[LEARNBUFSIZ] = { NUL, NUL };
static int  learnbc = 0;
static int  learnbp = 0;
static int  learnst = 0;
#endif /* CKLEARN */

#ifdef CK_TRIGGER
extern char * tt_trigger[], * triggerval;
#endif /* CK_TRIGGER */

extern int nopush;			/* Runtime NOPUSH */
extern int cx_status;			/* CONNECT status code */

#ifndef NOICP				/* Keyboard mapping */
#ifndef NOSETKEY
extern KEY *keymap;			/* Single-character key map */
extern MACRO *macrotab;			/* Key macro pointer table */
static MACRO kmptr = NULL;		/* Pointer to current key macro */
#endif /* NOSETKEY */
#endif /* NOICP */

extern int carrier;			/* CARRIER-WATCH selection */

/* Network support */
extern int ttnproto,			/* Virtual terminal protocol */
  network,				/* Network connection active */
  nettype;				/* Network type */

static int unicode = 0;

#ifndef NOCSETS
#ifdef CK_ANSIC
extern CHAR (*xls[MAXTCSETS+1][MAXFCSETS+1])(CHAR); /* Character set */
extern CHAR (*xlr[MAXTCSETS+1][MAXFCSETS+1])(CHAR); /* translation functions */
static CHAR (*sxo)(CHAR);	/* Local translation functions */
static CHAR (*rxo)(CHAR);	/* for output (sending) terminal chars */
static CHAR (*sxi)(CHAR);	/* and for input (receiving) terminal chars. */
static CHAR (*rxi)(CHAR);
#else
extern CHAR (*xls[MAXTCSETS+1][MAXFCSETS+1])();	/* Character set */
extern CHAR (*xlr[MAXTCSETS+1][MAXFCSETS+1])();	/* translation functions. */
static CHAR (*sxo)();		/* Local translation functions */
static CHAR (*rxo)();		/* for output (sending) terminal chars */
static CHAR (*sxi)();		/* and for input (receiving) terminal chars. */
static CHAR (*rxi)();
#endif /* CK_ANSIC */
extern int language;		/* Current language. */
static int langsv;		/* Remember language */
extern struct csinfo fcsinfo[]; /* File character set info */
extern int tcsr, tcsl;		/* Terminal character sets, remote & local. */
static int tcs;			/* Intermediate (xfer) char set */
static int tcssize = 0;		/* Size of tcs */
#ifdef UNICODE				/* UTF-8 support */
#ifdef CK_ANSIC
extern int (*xl_ufc[MAXFCSETS+1])(USHORT);  /* Unicode to FCS */
extern USHORT (*xl_fcu[MAXFCSETS+1])(CHAR); /* FCS to Unicode */
extern int (*xuf)(USHORT);		/* Translation function UCS to FCS */
extern USHORT (*xfu)(CHAR);		/* Translation function FCS to UCS */
#else
extern int (*xl_ufc[MAXFCSETS+1])();
extern USHORT (*xl_fcu[MAXFCSETS+1])();
extern int (*xuf)();
extern USHORT (*xfu)();
#endif /* CK_ANSIC */
#endif /* UNICODE */
#endif /* NOCSETS */

_PROTOTYP( VOID doesc, (unsigned char) );

#ifndef NOSHOW
_PROTOTYP( VOID shomdm, (void) );
#endif /* NOSHOW */

/* Internal prototypes */
_PROTOTYP( static int ckcputf, (void) );
_PROTOTYP( int conresne, (void) );
_PROTOTYP( int contti, (int *, int*) );
#ifdef CK_SSL
_PROTOTYP( int ssl_contti, (int *, int*) );
#endif /* CK_SSL */

#ifdef CK_TRIGGER
_PROTOTYP( int autoexitchk, (CHAR) );
#endif /* CK_TRIGGER */

#ifdef CK_APC
extern int apcactive;			/* Application Program Command (APC) */
extern int apcstatus;			/* items ... */
static int apclength = 0;
#ifdef DCMDBUF
extern char *apcbuf;
#else
extern char apcbuf[];
#endif /* DCMDBUF */
static int apcbuflen = APCBUFLEN - 2;
extern int autodl, protocol;		/* Autodownload */
#endif /* CK_APC */

int i, active;				/* Variables global to this module */
static char *p;

#define OUTXBUFSIZ 15
static CHAR inxbuf[OUTXBUFSIZ+1];	/* Host-to-screen expansion buffer */
static int inxcount = 0;		/* and count */
static CHAR outxbuf[OUTXBUFSIZ+1];	/* Keyboard-to-host expansion buf */
static int outxcount = 0;		/* and count */

static int escseq = 0;			/* 1 = Recognizer is active */
/* static */ int inesc[2] = { 0, 0 };	/* State of sequence recognizer */
/* static */ int oldesc[2] = { -1, -1 }; /* Previous state of recognizer */

#ifndef NOESCSEQ
/*
  As of edit 178, the CONNECT command skips past ANSI escape sequences to
  avoid translating the characters within them.  This allows the CONNECT
  command to work correctly with a host that uses a 7-bit ISO 646 national
  character set, in which characters like '[' would normally be translated
  into accented characters, ruining the terminal's interpretation (and
  generation) of escape sequences.

  As of edit 190, the CONNECT command responds to APC escape sequences
  (ESC _ text ESC \) if the user SETs TERMINAL APC ON or UNCHECKED, and the
  program was built with CK_APC defined.

  Non-ANSI/ISO-compliant escape sequences are not handled.
*/

/* States for the escape-sequence recognizer. */

#define ES_NORMAL 0			/* Normal, not in an escape sequence */
#define ES_GOTESC 1			/* Current character is ESC */
#define ES_ESCSEQ 2			/* Inside an escape sequence */
#define ES_GOTCSI 3			/* Inside a control sequence */
#define ES_STRING 4			/* Inside DCS,OSC,PM, or APC string */
#define ES_TERMIN 5			/* 1st char of string terminator */

/*
  ANSI escape sequence handling.  Only the 7-bit form is treated, because
  translation is not a problem in the 8-bit environment, in which all GL
  characters are ASCII and no translation takes place.  So we don't check
  for the 8-bit single-character versions of CSI, DCS, OSC, APC, or ST.
  Here is the ANSI sequence recognizer state table, followed by the code
  that implements it.

  Definitions:
    CAN = Cancel                       01/08         Ctrl-X
    SUB = Substitute                   01/10         Ctrl-Z
    DCS = Device Control Sequence      01/11 05/00   ESC P
    CSI = Control Sequence Introducer  01/11 05/11   ESC [
    ST  = String Terminator            01/11 05/12   ESC \
    OSC = Operating System Command     01/11 05/13   ESC ]
    PM  = Privacy Message              01/11 05/14   ESC ^
    APC = Application Program Command  01/11 05/15   ESC _

  ANSI escape sequence recognizer:

    State    Input  New State  ; Commentary

    NORMAL   (start)           ; Start in NORMAL state

    (any)    CAN    NORMAL     ; ^X cancels
    (any)    SUB    NORMAL     ; ^Z cancels

    NORMAL   ESC    GOTESC     ; Begin escape sequence
    NORMAL   other             ; NORMAL control or graphic character

    GOTESC   ESC               ; Start again
    GOTESC   [      GOTCSI     ; CSI
    GOTESC   P      STRING     ; DCS introducer, consume through ST
    GOTESC   ]      STRING     ; OSC introducer, consume through ST
    GOTESC   ^      STRING     ; PM  introducer, consume through ST
    GOTESC   _      STRING     ; APC introducer, consume through ST
    GOTESC   0..~   NORMAL     ; 03/00 through 17/14 = Final character
    GOTESC   other  ESCSEQ     ; Intermediate or ignored control character

    ESCSEQ   ESC    GOTESC     ; Start again
    ESCSEQ   0..~   NORMAL     ; 03/00 through 17/14 = Final character
    ESCSEQ   other             ; Intermediate or ignored control character

    GOTCSI   ESC    GOTESC     ; Start again
    GOTCSI   @..~   NORMAL     ; 04/00 through 17/14 = Final character
    GOTCSI   other             ; Intermediate char or ignored control char

    STRING   ESC    TERMIN     ; Maybe have ST
    STRING   other             ; Consume all else

    TERMIN   \      NORMAL     ; End of string
    TERMIN   other  STRING     ; Still in string
*/
/*
  C H K A E S  --  Check ANSI Escape Sequence.

  Call with EACH character in input stream.
  src = 0 means c is incoming from remote; 1 = char from keyboard.
  Sets global inesc[src] variable according to escape sequence state.
  Returns 0 normally, 1 if an APC sequence is to be executed.
  Handles transparent printing internally.
*/
int
#ifdef CK_ANSIC
chkaes(char c, int src)
#else
chkaes(c,src) char c; int src;
#endif /* CK_ANSIC */
/* chkaes */ {

    debug(F111,"chkaes entry inesc",ckitoa(src),inesc[src]);
    debug(F101,"chkaes c","",c);

    if (src < 0 || src > 1)		/* Don't allow bad args. */
      return(0);

    oldesc[src] = inesc[src];		/* Remember previous state */

#ifdef XPRINT
    if (inesc[src] && !src) {		/* Save up escape seq for printing  */
	if (!c) return(0);		/* Ignore NULs */
	if (escbufc < ESCBUFLEN) {
	    escbuf[escbufc++] = c;
	    escbuf[escbufc] = NUL;
	    debug(F111,"ESCBUF 1",escbuf,escbufc);
	} else {			/* Buffer overrun */
	    if (printing && escbufc)	/* Print what's there so far */
	      zsoutx(ZMFILE,escbuf,escbufc);
	    escbufc = 1;		/* clear it out */
	    escbuf[0] = c;		/* and start off fresh buffer */
	    escbuf[1] = NUL;		/* with this character. */
	}
    }
#endif /* XPRINT */

    if (c == CAN || c == SUB) {		/* CAN and SUB cancel any sequence */
#ifdef XPRINT
	if (!src) {
	    if (printing && escbufc > 1)
	      zsoutx(ZMFILE,escbuf,escbufc-1);
	    escbufc = 0;		/* Clear buffer */
	    escbuf[0] = NUL;
	}
#endif /* XPRINT */
	inesc[src] = ES_NORMAL;
    } else				/* Otherwise */

      switch (inesc[src]) {		/* enter state switcher */
	case ES_NORMAL:			/* NORMAL state */
	  if (c == ESC) {		/* Got an ESC */
	      inesc[src] = ES_GOTESC;	/* Change state to GOTESC */
#ifdef XPRINT
	      if (!src) {
		  escbufc = 1;		/* Clear escape sequence buffer */
		  escbuf[0] = c;	/* and deposit the ESC */
		  escbuf[1] = NUL;
		  debug(F111,"ESCBUF 2",escbuf,escbufc);
	      }
#endif /* XPRINT */
	  }
	  break;			/* Otherwise stay in NORMAL state */

	case ES_GOTESC:			/* GOTESC state - prev char was ESC*/
	  if (c == '[') {		/* Left bracket after ESC is CSI */
	      inesc[src] = ES_GOTCSI;	/* Change to GOTCSI state */
	  } else if (c == 'P' || (c > 0134 && c < 0140)) { /* P, ], ^, or _ */
	      inesc[src] = ES_STRING;	/* Switch to STRING-absorption state */
#ifdef XPRINT
	      debug(F111,"ESCBUF STRING",escbuf,escbufc);
#endif /* XPRINT */
#ifdef CK_APC
	      /* If APC not disabled */
	      if (!src && c == '_' && apcstatus != APC_OFF) {
		  debug(F100,"CONNECT APC begin","",0);
		  apcactive = APC_REMOTE; /* Set APC-Active flag */
		  apclength = 0;	/* and reset APC buffer pointer */
	      }
#endif /* CK_APC */
	  } else if (c > 057 && c < 0177) { /* Final character '0' thru '~' */
	      inesc[src] = ES_NORMAL;	/* Back to normal */
#ifdef XPRINT
	      if (!src) {
		  if (printing & escbufc > 1) {
		      /* Dump esc seq buf to printer */
		      zsoutx(ZMFILE,escbuf,escbufc-1);
		      debug(F111,"ESCBUF PRINT 1",escbuf,escbufc);
		  }
		  escbufc = 0;		/* Clear parameter buffer */
		  escbuf[0] = NUL;
	      }
#endif /* XPRINT */
	  } else if (c != ESC) {	/* ESC in an escape sequence... */
	      inesc[src] = ES_ESCSEQ;	/* starts a new escape sequence */
	  }
	  break;			/* Intermediate or ignored ctrl char */

	case ES_ESCSEQ:			/* ESCSEQ -- in an escape sequence */
	  if (c > 057 && c < 0177) {	/* Final character '0' thru '~' */
	      inesc[src] = ES_NORMAL;	/* Return to NORMAL state. */
#ifdef XPRINT
	      if (!src) {
		  if (printing && escbufc > 1) {
		      zsoutx(ZMFILE,escbuf,escbufc-1);
		      debug(F111,"ESCBUF PRINT 2",escbuf,escbufc);
		  }
		  escbufc = 0;		/* Clear escseq buffer */
		  escbuf[0] = NUL;
	      }
#endif /* XPRINT */
	  } else if (c == ESC) {	/* ESC ... */
	      inesc[src] = ES_GOTESC;	/* starts a new escape sequence */
	  }
	  break;			/* Intermediate or ignored ctrl char */

	case ES_GOTCSI:			/* GOTCSI -- In a control sequence */
	  if (c > 077 && c < 0177) {	/* Final character '@' thru '~' */
#ifdef XPRINT
	      if (!src && tt_print) {	/* Printer enabled? */
		  if (c == 'i') {	/* Final char is "i"? */
		      char * p = (char *) (escbuf + escbufc - 4);
		      if (!strncmp(p, "\033[5i", 4)) /* Yes, turn printer on */
			printon();
		      else if (!strncmp(p, "\033[4i", 4)) { /* Or off... */
			  int i;
			  printoff();	/* Turn off printer. */
			  for (i = 0; i < escbufc; i++)	/* And output the */
			    ckcputc(escbuf[i]);         /* sequence. */
		      } else if (printing && escbufc > 1) {
			  zsoutx(ZMFILE,escbuf,escbufc-1);
			  debug(F111,"ESCBUF PRINT 3",escbuf,escbufc);
		      }
		  } else if (printing && escbufc > 1) {
		      zsoutx(ZMFILE,escbuf,escbufc-1);
		      debug(F111,"ESCBUF PRINT 4",escbuf,escbufc);
		  }
	      }
	      if (!src) {
		  escbufc = 0;		/* Clear esc sequence buffer */
		  escbuf[0] = NUL;
	      }
#endif /* XPRINT */
	      inesc[src] = ES_NORMAL;	/* Return to NORMAL. */
	  } else if (c == ESC) {	/* ESC ... */
	      inesc[src] = ES_GOTESC;	/* starts over. */
	  }
	  break;

	case ES_STRING:			/* Inside a string */
	  if (c == ESC)			/* ESC may be 1st char of terminator */
	    inesc[src] = ES_TERMIN;	/* Go see. */
#ifdef CK_APC
	  else if (apcactive) {		/* If in APC */
	      if (apclength < apcbuflen) { /* and there is room... */
		  apcbuf[apclength++] = c; /* deposit this character. */
	      } else {			/* Buffer overrun */
		  apcactive = 0;	/* Discard what we got */
		  apclength = 0;	/* and go back to normal */
		  apcbuf[0] = 0;	/* Not pretty, but what else */
		  inesc[src] = ES_NORMAL; /* can we do?  (ST might not come) */
	      }
	  }
#endif /* CK_APC */
	  break;			/* Absorb all other characters. */

	case ES_TERMIN:			/* Maybe a string terminator */
	  if (c == '\\') {		/* which must be backslash */
	      inesc[src] = ES_NORMAL;	/* If so, back to NORMAL */
#ifdef XPRINT
	      if (!src) {
		  if (printing && escbufc > 1) { /* If printing... */
		      /* Print esc seq buffer */
		      zsoutx(ZMFILE,escbuf,escbufc-1);
		      debug(F111,"ESCBUF PRINT 5",escbuf,escbufc);
		  }
		  escbufc = 0;		/* Clear escseq buffer */
		  escbuf[0] = NUL;
	      }
#endif /* XPRINT */
#ifdef CK_APC
	      if (!src && apcactive) {	/* If it was an APC string, */
		  debug(F101,"CONNECT APC terminated","",c);
		  apcbuf[apclength] = NUL; /* terminate it and then ... */
		  return(1);
	      }
#endif /* CK_APC */
	  } else {			/* It's not a backslash so... */
	      inesc[src] = ES_STRING;	/* back to string absorption. */
#ifdef CK_APC
	      if (apcactive) {		/* In APC string */
		  if (apclength+1 < apcbuflen) { /* If enough room */
		      apcbuf[apclength++] = ESC; /* deposit the Esc */
		      apcbuf[apclength++] = c;   /* and this character too. */
		  } else {		/* Buffer overrun */
		      apcactive = 0;
		      apclength = 0;
		      apcbuf[0] = 0;
		      inesc[src] = ES_NORMAL;
		  }
	      }
#endif /* CK_APC */
	  }
      }	/* switch() */
    debug(F111,"chkaes exit inesc",ckitoa(src),inesc[src]);
    return(0);
}
#endif /* NOESCSEQ */

static char *ibp;			/* Input buffer pointer */
static int ibc;				/* Input buffer count */
#define IBUFL 1024			/* Input buffer length */

static char *obp;			/* Output buffer pointer */
static int obc;				/* Output buffer count */
#define OBUFL 1024			/* Output buffer length */

#ifdef DYNAMIC
static char *ibuf = NULL, *obuf = NULL;	/* Line and temp buffers */
#else
static char ibuf[IBUFL], obuf[OBUFL];
#endif /* DYNAMIC */

char kbuf[10], *kbp;			/* Keyboard buffer */

#ifdef CKLEARN
static VOID
learnchar(c) int c; {			/* Learned script keyboard character */
    int cc;
    char xbuf[8];

    if (!learning || !learnfp)
      return;

    switch (learnst) {			/* Learn state... */
      case 0:				/* Neutral */
      case 1:				/* Net */
	if (learnbc > 0) {		/* Have net characters? */
	    char buf[LEARNBUFSIZ];
	    int i, j, n;
	    ULONG t;

	    t = (ULONG) time(0);	/* Calculate INPUT timeout */
	    j = t - learnt1;
	    j += (j / 4) > 0 ? (j / 4) : 1; /* Add some slop */
	    if (j < 2) j = 2;		    /* 2 seconds minimum */

	    fputs("\nINPUT ",learnfp);	/* Give INPUT command for them */
	    fputs(ckitoa(j),learnfp);
	    fputs(" {",learnfp);
	    learnt1 = t;

	    n = LEARNBUFSIZ;
	    if (learnbc < LEARNBUFSIZ) {  /* Circular buffer */
		n = learnbc;		  /*  hasn't wrapped yet. */
		learnbp = 0;
	    }
	    j = 0;			/* Copy to linear buffer */
	    for (i = 0; i < n; i++) {	/* Number of chars in circular buf */

		cc = learnbuf[(learnbp + i) % LEARNBUFSIZ];

		/* Later account for prompts that end with a newline? */

		if (cc == CR && j > 0) {
		    if (buf[j-1] != LF)
		      j = 0;
		}
		buf[j++] = cc;
	    }
	    for (i = 0; i < j; i++) {	/* Now copy out the buffer */
		cc = buf[i];		/* interpreting control chars */
		if (cc == 0) {		/* We don't INPUT NULs */
		    continue;
		} else if (cc < SP ||	/* Controls need quoting */
			   cc > 126 && cc < 160) {
		    ckmakmsg(xbuf,8,"\\{",ckitoa((int)cc),"}",NULL);
		    fputs(xbuf,learnfp);
		} else {		/* Plain character */
		    putc(cc,learnfp);
		}
	    }
	    fputs("}\nIF FAIL STOP 1 INPUT timeout",learnfp);
	    learnbc = 0;
	}
	learnbp = 0;
	fputs("\nPAUSE 1\nOUTPUT ",learnfp); /* Emit OUTPUT and fall thru */

      case 2:				/* Already in Keyboard state */
	if (c == 0) {
	    fputs("\\N",learnfp);
	} else if (c == -7) {
	    fputs("\\B",learnfp);
	} else if (c == -8) {
	    fputs("\\L",learnfp);
	} else if (c < SP || c > 126 && c < 160) {
	    ckmakmsg(xbuf,8,"\\{",ckitoa((int)c),"}",NULL);
	    fputs(xbuf,learnfp);
	} else {
	    putc(c,learnfp);
	}
    }
}
#endif /* CKLEARN */

/*  C O N E C T  --  Perform terminal connection  */

int inshift, outshift;			/* SO/SI shift states */

/*  C K C P U T C  --  C-Kermit CONNECT Put Character to Screen  */
/*
  Output is buffered to avoid slow screen writes on fast connections.
*/
int
ckcputf() {				/* Dump the output buffer */
    int x;
    if (obc > 0)			/* If we have any characters, */
      x = conxo(obc,obuf);		/* dump them, */
    obp = obuf;				/* reset the pointer */
    obc = 0;				/* and the counter. */
    return(x);				/* Return conxo's return code */
}

int
ckcputc(c) int c; {
    int x;

    *obp++ = c & 0xff;			/* Deposit the character */
    obc++;				/* Count it */
    if (ibc == 0 ||			/* If input buffer empty */
	obc == OBUFL) {			/* or output buffer full */
	x = conxo(obc,obuf);		/* dump the buffer, */
	obp = obuf;			/* reset the pointer */
	obc = 0;			/* and the counter. */
	return(x);			/* Return conxo's return code */
    } else return(0);
}

#ifdef CK_SSL
    extern int ssl_active_flag, tls_active_flag;
#endif /* CK_SSL */

/*  C K C G E T C  --  C-Kermit CONNECT Get Character  */
/*
  Buffered read from communication device.
  Returns the next character, refilling the buffer if necessary.
  On error, returns ttinc's return code (see ttinc() description).
  Dummy argument for compatible calling conventions with ttinc().
  NOTE: We don't have a macro for this because we have to pass
  a pointer to this function as an argument to tn_doop().
*/
int
ckcgetc(dummy) int dummy; {
    int c, i, n;

    if (ibc > 0) {			/* Have buffered port characters */
	src = 1;			/* Say source is port */
	c = *ibp++ & 0xff;		/* Get next character */
	ibc--;				/* Reduce input buffer count */
#ifdef COMMENT
/* This makes the debug log really big. */
	debug(F101,"CKCGETC buffered port char","",c);
#endif /* COMMENT */
	return(c);			/* Return buffered port character */
    } else {				/* Need to refill buffer */
	for (i = 10; i >= 0; i--) {	/* Try 10 times... */
#ifdef CK_SSL
	    if ( ssl_active_flag || tls_active_flag )
		n = ssl_contti(&c, &src);
	    else
#endif /* CK_SSL */
	      n = contti(&c, &src);	/* to read one character */
	    if (src == -1) {		/* Connection dropped */
		return(n < 0 ? -2 : -1);/* Have to give up */
	    } else if (src == -2) {	/* HANGUP with CARRIER-WATCH OFF */
		msleep(i*10);		/* and no character arrived */
		debug(F101,"CKCGETC -2 countdown","",i);
		continue;		/* Try so many times */
	    } else
	      break;
	}
	if (src < 0) {			/* Give up after too many tries */
	    debug(F100,"CKCGETC -2 gives up","",0);
	    return(-1);
	}
	if (src == 0) {			/* Got a character from the keyboard */
	    debug(F101,"CKCGETC keyboard char","",c);
	    return(c);
	} else {			/* Got a port character */
	    ibp = ibuf;			/* Reset buffer pointer */
	    *ibp++ = c;			/* Deposit the character */
	    ibc++;			/* and count it */
/*
  Quickly read any more characters that might have arrived.  Ignore any errors
  here; we need to return all the characters that arrived.  When we run out,
  contti() will catch the error and return it.
*/
	    if ((n = ttchk()) > 0) {	/* Any more characters waiting? */
		if (n > (IBUFL - ibc))	/* Get them all at once. */
		  n = IBUFL - ibc;	/* Don't overflow the buffer */
		if ((n = ttxin(n,(CHAR *)ibp)) > 0) /* Read waiting chars */
		  ibc += n;		/* Advance counter */
	    }
	    ibp = ibuf;			/* Reset buffer pointer again */
	    c = *ibp++ & 0xff;		/* Get first character from buffer */
	    ibc--;			/* Reduce buffer count */
	    debug(F101,"CKCGETC port char","",c);
	    return(c);			/* Return the character */
	}
    }
}

int
conect() {
    int c;			/* c is a character, but must be signed
				   integer to pass thru -1, which is the
				   modem disconnection signal, and is
				   different from the character 0377 */
    int c2, csave;		/* Copies of c */
    char errmsg[50], *erp, *cp;
    int n, x;				/* Workers */
    int msgflg = 0;			/* Whether to print messages */
    int apcrc;
    int rc = 0;
#ifdef CKLEARN
    int crflag = 0;
#endif /* CKLEARN */

#ifdef CK_TRIGGER
    int ix;
#endif /* CK_TRIGGER */

    cx_status = CSX_INTERNAL;
    debok = 1;

    if (!local) {
#ifdef NETCONN
	printf("Sorry, you must SET LINE or SET HOST first\n");
#else
	printf("Sorry, you must SET LINE first\n");
#endif /* NETCONN */
	return(-2);
    }
    if (batch) {
	printf("\n\
Sorry, Kermit's CONNECT command requires a real terminal;\n");
	printf("It can't be used in a batch job.\n");
	printf("Use INPUT and OUTPUT commands instead.\n");
	return(0);
    } else if (!itsatty) {
	printf("\n\
Sorry, Kermit's CONNECT command can be used only on a real terminal.\n");
	printf(
"If you have started Kermit from a DCL command procedure, it needs:\n\n");
	printf("$ DEFINE /USER SYS$INPUT SYS$COMMAND\n\n");
	printf("before starting Kermit.\n\n");
	return(0);
    }
#ifdef TCPSOCKET
    if (network && (nettype != NET_TCPB)) {
	printf("Sorry, network type not supported yet\n");
	return(0);
    }
#endif /* TCPSOCKET */

    debug(F101,"ckvcon network","",network);
    debug(F101,"ckvcon speed","",speed);
    debug(F101,"ckvcon ttyfd","",ttyfd);
    debug(F110,"ckvcon host",ttname,0);
#ifdef CK_TRIGGER
    debug(F110,"ckvcon trigger",tt_trigger[0],0);
#endif /* CK_TRIGGER */

    if (speed < 0 && network == 0) {
	printf("Sorry, you must SET SPEED first\n");
	return(-2);
    }
    if ((escape < 0) || (escape > 0177)) {
	printf("Your escape character is not ASCII - %d\n",escape);
	return(-2);
    }
    if (ttyfd < 0) {			/* If communication device not open */
	debug(F111,"ckvcon opening",ttname,0); /* Open it now. */
	if (ttopen(ttname,
		   &local,
		   network ? -nettype : mdmtyp,
		   0
		   ) < 0) {
	    erp = errmsg;
	    sprintf(erp,"Sorry, can't open %s",ttname);
	    perror(errmsg);
	    debug(F110,"ckvcon open failure",errmsg,0);
	    return(-2);
	}
#ifdef IKS_OPTION
	/* If peer is in Kermit server mode, return now. */
	if (TELOPT_SB(TELOPT_KERMIT).kermit.u_start) {
	    cx_status = CSX_IKSD;
	    return(0);
	}
#endif /* IKS_OPTION */
    }
    msgflg = !quiet
#ifdef CK_APC
      && !apcactive
#endif /* CK_APC */
	;

    if (msgflg) {
#ifdef NETCONN
	if (network) {
	    printf("\nConnecting to host %s",ttname);
	} else {
#endif /* NETCONN */
	    printf("\nConnecting to %s",ttname);
	    if (speed > -1L) printf(", speed %ld",speed);
#ifdef NETCONN
	}
#endif /* NETCONN */
	if (tt_escape) {
	    printf("\r\n");
	    shoesc(escape);
	    printf("Type the escape character followed by C to get back,\r\n");
	    printf("or followed by ? to see other options.\r\n");
	} else {
	    printf(".\r\n\nESCAPE CHARACTER IS DISABLED\r\n\n");
	}
	if (seslog) {
	    extern int slogts, sessft;
	    char * s = "";
	    switch (sessft) {
	      case XYFT_D:
		s = "debug"; break;
	      case XYFT_T:
		s = slogts ? "timestamped-text" : "text"; break;
	      default:
		s = "binary";
	    }
	    printf("Session Log: %s, %s\r\n",sesfil,s);
	}
	if (debses) printf("Debugging Display...)\r\n");
        printf("----------------------------------------------------\r\n");
	printf("\r\n");
    }

/* Condition console terminal and communication line */

    rc = 0;
    if (conbin(escape) < 0) {
	printf("Sorry, can't condition console terminal\n");
        printf("----------------------------------------------------\r\n");
	goto conret0;
    }
    debug(F101,"ckvcon flow","",flow);
    if (ttvt(speed,flow) < 0) {
	/* tthang(); */ /* Closing it should be quite enough! */
	ttclos(0);
	if (ttopen(ttname,		/* Open it again... */
		   &local,
		   network ? -nettype : mdmtyp,
		   0
		   ) < 0) {
	    erp = errmsg;
	    sprintf(erp,"Sorry, can't reopen %s",ttname);
	    perror(errmsg);
	    goto conret0;
	}
#ifdef IKS_OPTION
	if (TELOPT_SB(TELOPT_KERMIT).kermit.u_start) {
	    cx_status = CSX_IKSD;
	    goto conret0;
	}
#endif /* IKS_OPTION */
	if (ttvt(speed,flow) < 0) {	/* Try virtual terminal mode again. */
	    conres();			/* Failure this time is fatal. */
	    printf("Sorry, Can't condition communication line\n");
	    goto conret0;
	}
    }
    debug(F101,"CONNECT ttvt ok, escape","",escape);

    debug(F101,"CONNECT carrier-watch","",carrier);
    if ((!network
#ifdef TN_COMPORT
	 || istncomport()
#endif /* TN_COMPORT */
	 )&& (carrier != CAR_OFF)) {
	int x;
	x = ttgmdm();
	debug(F100,"CONNECT ttgmdm","",x);
	if ((x > -1) && !(x & BM_DCD)) {
	    debug(F100,"CONNECT ttgmdm CD test fails","",x);
	    conres();
	    printf("?Carrier required but not detected.\n");
	    rc = 0;
	    if (hints) {
		printf("***********************************\n");
		printf(" Hint: To CONNECT to a serial device that\n");
		printf(" is not presenting the Carrier Detect signal,\n");
		printf(" first tell C-Kermit to:\n\n");
		printf("   SET CARRIER-WATCH OFF\n\n");
		printf("***********************************\n\n");
	    }
	    cx_status = CSX_CARRIER;
	    goto conret0;
	}
	debug(F100,"CONNECT ttgmdm ok","",0);
    }

#ifdef CK_APC
    apcactive = 0;
    apclength = 0;
#endif /* CK_APC */

#ifndef NOCSETS
/* Set up character set translations */

    unicode = 0;			/* Assume Unicode won't be involved */
    tcs = 0;				/* "Transfer" or "Other" charset */
    sxo = rxo = NULL;			/* Initialize byte-to-byte functions */
    sxi = rxi = NULL;
    if (tcsr != tcsl) {			/* Remote and local sets differ... */
#ifdef UNICODE
	if (tcsr == FC_UTF8 ||		/* Remote charset is UTF-8 */
	    tcsl == FC_UTF8) {		/* or local one is. */
	    xuf = xl_ufc[tcsl];		/* Incoming Unicode to local */
	    if (xuf || tcsl == FC_UTF8) {
		tcs = (tcsr == FC_UTF8) ? tcsl : tcsr; /* The "other" set */
		xfu = xl_fcu[tcs];	/* Local byte to remote Unicode */
		if (xfu)
		  unicode = (tcsr == FC_UTF8) ? 1 : 2;
	    }
	    tcssize = fcsinfo[tcs].size; /* Size of other character set. */
	} else {
#endif /* UNICODE */
	    tcs = gettcs(tcsr,tcsl);	/* Get intermediate set. */
	    sxo = xls[tcs][tcsl];	/* translation function */
	    rxo = xlr[tcs][tcsr];	/* pointers for output functions */
	    sxi = xls[tcs][tcsr];	/* and for input functions. */
	    rxi = xlr[tcs][tcsl];
#ifdef UNICODE
	}
#endif /* UNICODE */
    }
/*
  This is to prevent use of zmstuff() and zdstuff() by translation functions.
  They only work with disk i/o, not with communication i/o.  Luckily Russian
  translation functions don't do any stuffing...
*/
    langsv = language;
#ifndef NOCYRIL
    if (language != L_RUSSIAN)
#endif /* NOCYRIL */
      language = L_USASCII;

#ifndef NOESCSEQ
/*
  We need to activate escape-sequence recognition when:
  (a) translation is elected, and:
  (b) the local and/or remote set is 7-bit set other than US or UK ASCII, or:
  (c) TERMINAL APC is not OFF.
*/
    escseq = (tcs != TC_TRANSP) &&	/* Not transparent */
      (fcsinfo[tcsl].size == 128 || fcsinfo[tcsr].size == 128) && /* 7 bits */
	(fcsinfo[tcsl].code != FC_USASCII) && /* Not US ASCII */
	(fcsinfo[tcsl].code != FC_UKASCII);   /* Not UK ASCII */
#ifdef COMMENT
    debug(F101,"tcs","",tcs);
    debug(F101,"tcsl","",tcsl);
    debug(F101,"tcsr","",tcsr);
    debug(F101,"fcsinfo[tcsl].size","",fcsinfo[tcsl].size);
    debug(F101,"fcsinfo[tcsr].size","",fcsinfo[tcsr].size);
#endif /* COMMENT */
#endif /* NOESCSEQ */
#endif /* NOCSETS */

#ifndef NOESCSEQ
#ifdef CK_APC
    escseq = escseq || (apcstatus != APC_OFF);
    apcactive = 0;			/* An APC command is not active */
    apclength = 0;			/* ... */
#endif /* CK_APC */
    inesc[0] = ES_NORMAL;		/* Initial state of recognizer */
    inesc[1] = ES_NORMAL;
    debug(F101,"CONNECT escseq","",escseq);
#endif /* NOESCSEQ */

    rc = 0;
#ifdef DYNAMIC
    if (!ibuf) {
	if (!(ibuf = malloc(IBUFL+1))) { /* Allocate input line buffer */
	    printf("Sorry, CONNECT input buffer can't be allocated\n");
	    goto conret0;
	} else {
	    ibp = ibuf;
	    ibc = 0;
	}
    }
    if (!obuf) {
	if (!(obuf = malloc(OBUFL+1))) {    /* Allocate output line buffer */
	    printf("Sorry, CONNECT output buffer can't be allocated\n");
	    goto conret0;
	} else {
	    obp = obuf;
	    obc = 0;
	}
    }
#else
    ibp = ibuf;
    ibc = 0;
    obp = obuf;
    obc = 0;
#endif /* DYNAMIC */

#ifdef CKLEARN
    if (learning) {			/* Learned script active... */
	learnbp = 0;			/* INPUT buffer pointer */
	learnbc = 0;			/* INPUT buffer count */
	learnst = 0;			/* State (0 = neutral, none) */
	learnt1 = (ULONG) time(0);
    }
#endif /* CKLEARN */

    inshift = outshift = 0;		/* Initial SI/SO states */
    active = 1;

    debug(F100,"CONNECT starting contti loop","",0);
    do {				/* Top of big loop */
#ifndef NOSETKEY
/*
  Before reading anything from the keyboard, continue expanding the current
  active keyboard macro, if any.
*/
	if (kmptr) {			/* Have active macro */
	    src = 0;			/* Pretend char came from keyboard */
	    if ((c = (CHAR) *kmptr++) == NUL) { /* but get it from the macro */
		kmptr = NULL;		/* If no more chars in macro,  */
		continue;		/* reset pointer and continue. */
	    }
	} else 				/* OTHERWISE... */
#endif /* NOSETKEY */
/*
  contti() samples the keyboard and the communication device, in that order,
  in what amounts to a busy loop, and does not return until it has something
  from one or the other.  This drives the VAX crazy, and also gives poor
  performance: input is character-at a time, and the keyboard buffer is
  checked at least once for every character that comes in from the remote.
  Somebody who knows something about VMS, PLEASE FIND A BETTER WAY.

  We want a version of contti() that does what select() does in BSD, and we
  also want it to be buffered internally, so it doesn't have to call the
  operating system for every single input character.  And most of all we don't
  want it to run a busy loop all the time.  No doubt this involves ASTs,
  things of which I know nothing.    - fdc
*/
	c = ckcgetc(0);			/* Calls contti()... */
	debug(F000,"CKCGETC","c",c);
/*
  Get here with a character in c, and:

  src = -1 Communication error
         1 Character from comm line
         0 Character from console
*/
	if (c < 0 || src < 0) {		/* Comm line hangup or other error */
	    ckcputf();			/* flush screen-output buffer */
	    if (msgflg)
	      printf("\r\nCommunications disconnect ");
	    ttclos(0);			/* Close our end of the connection */
	    if (cx_status != CSX_USERDISC)
	      cx_status = CSX_HOSTDISC;
	    active = 0;

	} else if (!src) {		/* Character from console */
	    c &= cmdmsk;		/* Do requested masking */
#ifndef NOICP
#ifndef NOSETKEY
/*
  Note: kmptr is NULL if we got character c from the keyboard, and it is
  not NULL if it came from a macro.  In the latter case, we must avoid
  expanding it again.
*/
	    if (!kmptr && macrotab[c]) { /* If a macro is assigned to it */
		kmptr = macrotab[c];	/* set up the pointer */
		continue;		/* and do this again. */
	    } else c = keymap[c];	/* Else use single-char keymap */
#endif /* NOSETKEY */
#endif /* NOICP */
	    csave = c;
#ifdef CKLEARN
	    crflag = (c == CR);		/* Remember if it was CR. */
#endif /* CKLEARN */

	    if (
#ifndef NOICP
#ifndef NOSETKEY
		    !kmptr &&
#endif /* NOSETKEY */
#endif /* NOICP */
		(tt_escape && (c & 0xff) == escape)) { /* Escape character? */
		conresne();		/* Restore to normal attributes */
		c = coninc(0) & 0177;
		doesc(c);
		conbin(escape);
	    } else {			/* Ordinary character */
#ifndef NOCSETS
		if (inesc[1] == ES_NORMAL) { /* If not inside escape seq.. */
#ifdef UNICODE				/* Translate character sets */
		    int x;
		    CHAR ch;
		    ch = c;
		    if (unicode == 1) {	/* Remote is UTF-8 */
			outxcount = b_to_u(ch,outxbuf,OUTXBUFSIZ,tcssize);
			outxbuf[outxcount] = NUL;
		    } else if (unicode == 2) { /* Local is UTF-8 */
			x = u_to_b(ch);	/* So translate to remote byte */
			if (x < 0)
			  continue;
			outxbuf[0] = (unsigned)(x & 0xff);
			outxcount = 1;
			outxbuf[outxcount] = NUL;
		    } else {
#endif /* UNICODE */
			if (sxo) c = (*sxo)((char)c); /* Local-intermediate */
			if (rxo) c = (*rxo)((char)c); /* Intermediate-remote */
			outxbuf[0] = c;
			outxcount = 1;
			outxbuf[outxcount] = NUL;
#ifdef UNICODE
		    }
#endif /* UNICODE */
		} else {
		    outxbuf[0] = c;
		    outxcount = 1;
		    outxbuf[outxcount] = NUL;
		}
		if (escseq)
		  apcrc = chkaes((char)c,1);
#else  /* NOCSETS */
		outxbuf[0] = c;
		outxcount = 1;
		outxbuf[outxcount] = NUL;
#endif /* NOCSETS */

		for (i = 0; i < outxcount; i++) {
		    c = outxbuf[i];
/*
  If Shift-In/Shift-Out is selected and we have a 7-bit connection,
  handle shifting here.
*/
		    if (sosi) {		     /* Shift-In/Out selected? */
			if (cmask == 0177) { /* In 7-bit environment? */
			    if (c & 0200) { /* 8-bit character? */
				if (outshift == 0) { /* If not shifted, */
				    if (ttoc(dopar(SO)) < 0) { /* shift. */
					ckcputf();
					cx_status = CSX_INTERNAL;
					active = 0;
					continue;
				    }
				    outshift = 1;
				}
			    } else {
				if (outshift == 1) { /* 7-bit character */
				    if (ttoc(dopar(SI)) < 0) {
					ckcputf();
					cx_status = CSX_INTERNAL;
					active = 0;
					continue;
				    }
				    outshift = 0; /* unshift. */
				}
			    }
			}
			if (c == SO) outshift = 1; /* User typed SO */
			if (c == SI) outshift = 0; /* User typed SI */
		    }
		    c &= cmask;		/* Apply Kermit-to-host mask now. */
		    if (c == '\015') {	/* Carriage Return */
			int stuff = -1;
			if (tnlm) {	/* TERMINAL NEWLINE ON */
			    stuff = LF;	/* Stuff LF */
#ifdef TNCODE
			} else if (network &&	/* TELNET NEWLINE ON/OFF/RAW */
				   (ttnproto == NP_TELNET)) {
			    switch(!TELOPT_ME(TELOPT_BINARY) ?
				   tn_nlm :
				   tn_b_nlm
				   ) {
			      case TNL_CRLF:
				stuff = LF;
				break;
			      case TNL_CRNUL:
				stuff = NUL;
				break;
			    }
#endif /* TNCODE */
			}
			if (stuff > -1) {
			    ttoc(dopar('\015')); /* Send CR */
			    if (duplex) conoc('\015'); /* Maybe echo CR */
			    c = stuff;	/* Char to stuff */
			    csave = c;
			}
		    }
#ifdef TNCODE
/* If user types the 0xff character (TELNET IAC), it must be doubled. */
		    else
		      if (dopar(c) == IAC &&
			  network &&
			  ttnproto == NP_TELNET
			  ) {		/* Send one copy now */
			  ttoc(IAC);	/* and the other one just below. */
		      }
#endif /* TNCODE */
		    if (ttoc(dopar(c)) < 0) { /* Now send the character. */
			ckcputf();
			cx_status = CSX_INTERNAL;
			active = 0;
			continue;
		    }
#ifdef CKLEARN
		    if (learning) {	/* Learned script active */
			if (crflag) {
			    learnchar(CR);
			    learnst = 0;
			} else {
			    learnchar(c);
			    learnst = 2; /* Change state to keyboard */
			}
		    }
#endif /* CKLEARN */
		    if (duplex) {	/* Half duplex? */
			if (debses)	/* Yes, echo locally */
			  conol(dbchr(csave)); /* in appropriate mode */
			else
			  conoc(csave);
			if (seslog) logchar(c); /* And maybe log it. */
		    }
		} /* for... */
	    }
	} else {
/*
  Character from communications device or network connection...
*/
#ifdef TNCODE
	    /* Handle telnet options */
	    if (network && ttnproto == NP_TELNET && ((c & 0xff) == IAC)) {
		ckcputf();		/* Dump output buffer */
		if ((x = tn_doop(c & 0xff, duplex, ckcgetc)) == -1 && msgflg)
		  printf("\r\nCommunications disconnect ");
		if (x == -2 && msgflg)
		  printf("\r\nConnection closed by peer ");
		if (x == -3 && msgflg)
		  printf("\r\nConnection closed due to Telnet policy ");
		if (x == 1) duplex = 1;	/* Change duplex if necessary. */
		if (x == 2) duplex = 0;
		if (x == 3)		/* Quoted IAC */
		  c = parity ? 127 : 255;
#ifdef IKS_OPTION
                else if (x == 4) {	/* IKS State Change */
                    if (TELOPT_SB(TELOPT_KERMIT).kermit.u_start &&
			!tcp_incoming
			) {
			cx_status = CSX_IKSD;
			active = 0;
			break;
                    }
                }
#endif /* IKS_OPTION */
		else
		  continue;
	    } else if (parity)		/* NOW strip parity */
	      c &= 0x7f;

#ifdef CKLEARN
	    if (learning && learnst != 2) {
		learnbuf[learnbp++] = c;
		if (learnbp >= LEARNBUFSIZ)
		  learnbp = 0;
		learnbc++;
		learnst = 1;
	    }
#endif /* CKLEARN */

#endif /* TNCODE */
	    if (debses) {		/* Output character to screen */
		char *s;		/* Debugging display... */
		s = dbchr(c);
		while (*s)
		  ckcputc(*s++);
	    } else {			/* or regular... */
		int k;
		c &= cmask;		/* Do first masking */
#ifdef CK_APC
		if (autodl && (k = kstart((CHAR)c))) { /* Saw S/I packet? */
		    extern CHAR mystch, seol;
		    extern int justone;
		    CHAR buf[6];
		    int ksign = 0;
		    if (k < 0) {
			ksign = 1;
			k = 0 - k;
			justone = 1;
		    } else {
			justone = 0;
		    }
		    if (k == PROTO_K
#ifdef CK_XYZ
			|| k == PROTO_Z
#endif /* CK_XYZ */
			) {
			/* Damage the packet so that it doesn't trigger */
			/* autodownload detection downstream. */
			if (k == PROTO_K) {
			    extern CHAR ksbuf[];
			    int i, len = strlen((char*)ksbuf);
			    for (i = 0; i < len; i++)
			      ckcputc(BS);
			}
#ifdef CK_XYZ
			else {
			    int i;
			    for (i = 0; i < 3; i++)
			      ckcputc(CAN);
			}
#endif /* CK_XYZ */
		    }
		    switch (protocol) { /* Stuff RECEIVE into APC buffer */
		      case PROTO_K:
			strcpy(apcbuf, ksign ? "server" : "receive");
			break;
#ifdef CK_XYZ
		      case PROTO_G:
			strcpy(apcbuf,
			       "set proto kermit, receive, set proto g");
			break;
		      case PROTO_X:
			strcpy(apcbuf,
			       "set proto kermit, receive, set proto x");
			break;
		      case PROTO_Y:
			strcpy(apcbuf,
			       "set proto kermit, receive, set proto y");
			break;
		      case PROTO_Z:
			strcpy(apcbuf,
			       "set proto kermit, receive, set proto z");
			break;
#endif /* CK_XYZ */
		    }
		    apclength = strlen(apcbuf) ;
		    debug(F110,"autodownload",apcbuf,0);
		    apcactive = APC_LOCAL;
		    ckcputf();		/* flush screen-output buffer and... */
#ifdef COMMENT
		    printf("\r\n");	/* prevent CR-overstriking and...    */
#endif /* COMMENT */
		    cx_status = CSX_APC;
		    active = 0;		/* exit from the terminal emulator.. */
		    break;
		}
#endif /* CK_APC */
		if (sosi) {		/* Handle SI/SO */
		    if (c == SO) {	/* Shift Out */
			inshift = 1;
			continue;
		    } else if (c == SI) { /* Shift In */
			inshift = 0;
			continue;
		    }
		    if (inshift) c |= 0200;
		}
		inxbuf[0] = c;		/* In case there is no translation */
		inxcount = 1;
#ifndef NOCSETS
		if (inesc[0] == ES_NORMAL) { /* If not in an escape sequence */
#ifdef UNICODE
		    if (unicode == 1) {	/* Remote is UTF-8 */
			int x;
			x = u_to_b((CHAR)c);
			if (x == -1)
			  continue;
			else if (x == -2) { /* LS or PS */
			    inxbuf[0] = CR;
			    inxbuf[1] = LF;
			    inxcount = 2;
			} else {
			    inxbuf[0] = (unsigned)(x & 0xff);
			}
			c = inxbuf[0];
		    } else if (unicode == 2) { /* Local is UTF-8 */
			inxcount = b_to_u((CHAR)c,inxbuf,OUTXBUFSIZ,tcssize);
			c = inxbuf[0];
		    } else {
#endif /* UNICODE */
			if (sxi) c = (*sxi)((CHAR)c);
			if (rxi) c = (*rxi)((CHAR)c);
			inxbuf[0] = c;
#ifdef UNICODE
		    }
#endif /* UNICODE */
		}
#endif /* NOCSETS */

#ifndef NOESCSEQ
		if (escseq)		/* If handling escape sequences */
		  chkaes((char)c,0);	/* update our state */
#ifdef CK_APC
/*
  If we are handling APCs, we have several possibilities at this point:
   1. Ordinary character to be written to the screen.
   2. An Esc; we can't write it because it might be the beginning of an APC.
   3. The character following an Esc, in which case we write Esc, then char,
      but only if we have not just entered an APC sequence.
*/
		if (escseq && apcstatus != APC_OFF) {
		    if (inesc[0] == ES_GOTESC) { /* Don't write ESC yet */
			debug(F100,"XXX SKIPPING ESC","",0);
			continue;
		    } else if (oldesc[0] == ES_GOTESC && !apcactive) {
			debug(F100,"XXX WRITING ESC","",0);
			ckcputc(ESC);	/* Write saved ESC */
			if (seslog) logchar(ESC); /* And maybe log it. */
		    }
		}
#endif /* CK_APC */
#endif /* NOESCSEQ */

		for (i = 0; i < inxcount; i++) { /* Loop thru */
		    c = inxbuf[i];	/* input expansion buffer... */
		    if (
#ifdef CK_APC
			!apcactive
#else
			1
#endif /* CK_APC */
			) {
			c &= cmdmsk;	/* Apply mask */
			if (c == CR && tt_crd) { /* SET TERM CR-DISPLA CRLF? */
			    ckcputc(c);	/* Yes, output CR */
			    if (seslog) logchar(c);
			    c = LF;	/* and insert a linefeed */
			}
			ckcputc(c);	/* Put it on the screen. */
		    }
		    if (seslog) logchar(c); /* If logging, log it. */
#ifdef CK_TRIGGER
		    /* Check for trigger string */
		    if (tt_trigger[0]) if ((ix = autoexitchk((CHAR)c)) > -1) {
			int n;
			ckcputf();	/* Flush screen-output buffer */
			if (triggerval)	/* Make a copy of the trigger */
			  free(triggerval);
			triggerval = NULL;
			n = strlen(tt_trigger[ix]);
			if (triggerval = (char *)malloc(n+1))
			  strcpy(triggerval,tt_trigger[ix]);
			debug(F110,"CONNECT triggerval",triggerval,0);
			cx_status = CSX_TRIGGER;
			active = 0;
		    }
#endif /* CK_TRIGGER */
		} /* for... */
	    }
	}
    } while (active);
    rc = 1;

  conret0:
    ck_cancio();
    conres();

#ifdef CKLEARN
    if (learning && learnfp)
      fputs("\n",learnfp);
#endif /* CKLEARN */

    if (msgflg
#ifdef CK_APC
	&& !apcactive
#endif /* CK_APC */
	)
      printf("(Back at %s)", *myhost ? myhost : "local VMS system");
    what = W_NOTHING;
#ifdef CK_APC
    if (!apcactive)
#endif /* CK_APC */
      printf("\n");
#ifndef NOCSETS
    language = langsv;			/* Restore language */
#endif /* NOCSETS */
    if (msgflg) {
#ifdef CK_APC
	if (apcactive == APC_LOCAL)
	  printf("\n");
#endif /* CK_APC */
	printf("----------------------------------------------------\r\n");
    }
    return(rc);
}

/*  H C O N N E  --  Give help message for connect.  */

VOID
hconne() {
    int c;
    static char *hlpmsg[] = {
"\n",
"  C to return to C-Kermit prompt,   U to hangup and close the connection,\n",
"  B to send a BREAK,                L to send a Long BREAK,\n",
#ifdef TNCODE
"  A to send TELNET Are You There,   I to send TELNET Interrupt,\n",
#endif /* TNCODE */
"  0 (zero) to send a null,          X to send an XON,\n",
#ifdef NOPUSH
"  S for status of connection,       ? for this message, or:\n",
#else
"  @ to enter DCL,                   S for status of connection,\n",
"  ? for this message, or:\n",
#endif /* NOPUSH */
"  \\ to begin a backslash escape:\n",
"    \\nnn  (decimal character code)\n",
"    \\Onnn (octal character code)\n",
"    \\Xhh  (hexadecimal character code)\n",
"    Terminate with carriage return.\n\n",
" Type the escape character again to send the escape character, or\n",
" press the space-bar to resume the CONNECT command.\n",
"----------------------------------------------------\n\n",
"" };
/*
  Need to save term characteristics/ allow disable binary mode
  print message, get text and then restore previous state.
*/
    conoll("\r\n----------------------------------------------------");
    conol("\r\nPress C to return to ");
    conol(*myhost ? myhost : "the C-Kermit prompt");
    conoll(", or:");
    conola(hlpmsg);			/* Print the help message. */
    conol("Command>");			/* Prompt for command. */
    c = coninc(0) & 0x7f;
    conoc(c);				/* Echo it. */
    if (c != CMDQ)
      conoll("");
    conoll("----------------------------------------------------");
    doesc(c);
}

/*  D O E S C  --  Process an escape character argument  */

VOID
#ifdef CK_ANSIC
doesc(register unsigned char c)
#else
doesc(c) register unsigned char c;
#endif /* CK_ANSIC */
/* doesc() */ {
    int d;
    char temp[1024];

    c &= 0177;				/* Mask off 8th bit */

    if (c == escape) {			/* If it's the escape character, */
        d = dopar(c);			/* just send it. */
        ttoc(d);
	return;
    }
    if (isupper(c)) c = tolower(c);	/* Convert to lowercase letter. */
    if (iscntrl(c)) c += 'a' - '\001';

    switch (c) {			/* Take requested action. */
      case 'b':
#ifdef CKLEARN
	learnchar(-7);
#endif /* CKLEARN */
	ttsndb();			/* Send a BREAK signal */
	break;
#ifdef TNCODE
      case 'i':				/* Send TELNET interrupt */
#ifndef IP
#define IP 244
#endif /* IP */
	if (network && ttnproto == NP_TELNET) { /* TELNET */
	    CHAR temp[3];
	    temp[0] = IAC;		/* I Am a Command */
	    temp[1] = IP;		/* Interrupt Process */
	    temp[2] = NUL;
	    ttol((CHAR *)temp,2);
	} else conoc(BEL);
	break;
      case 'a':				/* "Are You There?" */
      case '\01':
#ifndef AYT
#define AYT 246
#endif /* AYT */
	if (network && ttnproto == NP_TELNET) {
	    CHAR temp[3];
	    temp[0] = IAC;		/* I Am a Command */
	    temp[1] = AYT;		/* Are You There? */
	    temp[2] = NUL;
	    ttol((CHAR *)temp,2);
	} else conoc(BEL);
#endif /* TNCODE */
	break;
      case 'c':				/* Return to prompt */
	cx_status = CSX_ESCAPE;
	active = 0;
	conol("\r\n");
	break;
      case 'u':				/* Hang up the connection */
	cx_status = CSX_USERDISC;
	if (network)
	  netclos();
#ifndef NODIAL
	else if (mdmhup() < 1)		/* Try via modem first, otherwise */
	  tthang();			/* the old-fashioned way. */
#endif /* NODIAL */
	conol("\r\nHanging up ");
	break;
      case 'l':				/* Send a Long BREAK signal */
#ifdef CKLEARN
	learnchar(-8);
#endif /* CKLEARN */
	ttsndlb();
	break;
      case 's':				/* Status */
	conoll("");
	conoll("----------------------------------------------------");
#ifdef NETCMD
	if (ttpipe)
	  sprintf(temp, " Pipe: \"%s\"", ttname);
	else
#endif /* NETCMD */
	  sprintf(temp, " %s: %s", (network ? "Host" : "Device"), ttname);
	conoll(temp);
	if (!network && speed >= 0L) {
	    sprintf(temp,"Speed %ld", speed);
	    conoll(temp);
	}
	sprintf(temp," Terminal echo: %s", duplex ? "local" : "remote");
	conoll(temp);
	sprintf(temp," Terminal bytesize: %d", (cmask  == 0177) ? 7 : 8);
	conoll(temp);
	sprintf(temp," Command bytesize: %d", (cmdmsk == 0177) ? 7 : 8 );
	conoll(temp);
	sprintf(temp," Parity: %s", parnam(parity));
	conoll(temp);
#ifdef CK_APC
	sprintf(temp," Autodownload: %s", autodl ? "on" : "off");
	conoll(temp);
#endif /* CK_APC */
	sprintf(temp," Session log: %s", *sesfil ? sesfil : "(none)");
	conoll(temp);
#ifndef NOSHOW
	    if (!network) shomdm();
#endif /* NOSHOW */
#ifdef CKLOGDIAL
	    {
		long z;
		z = dologshow(0);
		if (z > -1L) {
		    sprintf(temp," Elapsed time: %s",hhmmss(z));
		    conoll(temp);
		}
	    }
#endif /* CKLOGDIAL */
	    conoll("----------------------------------------------------");
	    return;
      case '!':
      case '@':
#ifndef NOPUSH
	if (!nopush) {
	    conres();			/* Put console back to normal */
	    zshcmd("");			/* Start DCL. */
	    if (conbin(escape) < 0) {
		printf("Error returning to remote session\n");
		active = 0;
	    }
	} else
	  conoc(BEL);
#else
	conoc(BEL);
#endif /* NOPUSH */
	return;

      case 'x':				/* XON */
/*
  NOTE: Here we should also issue QIO's to clear XOFF deadlock.
*/
	ttoc(dopar(XON));
	break;
      case 'h':				/* Help */
      case '?':				/* Give Help */
	hconne();
	break;
      case '0':				/* Send a NULL */
	c = '\0';
	d = dopar(c);
	ttoc(d);
	break;
      case SP:				/* Ignore space */
	break;
      default:
	if (c == CMDQ) {		/* Backslash escape */
	    int x;
	    kbp = kbuf;
	    *kbp++ = c;
	    while (((c = (coninc(0) & cmdmsk)) != '\r') && (c != '\n'))
	      *kbp++ = c;
	    *kbp = NUL; kbp = kbuf;
	    x = xxesc(&kbp);
	    if (x >= 0) {
		c = dopar(x);
		ttoc(c);
		return;
	    } else {
		conoc(BEL);
		return;
	    }
	}
	conoc(BEL); return;		/* Invalid esc arg, beep */
    }
}
