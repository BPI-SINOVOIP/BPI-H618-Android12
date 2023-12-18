/*
 * ckvcvt - assorted outboard processing for C-Kermit/VMS labeled files
 */

/* Revision History:
 * T1.0-00 - 08-Apr-91 - tmk - Initial coding, sync w/ 5A(167) and ckvfio.c
 *			       2.0-066.
 * T1.0-01 - 14-Apr-91 - tmk - Fix errors in help output, show actual system
 *			       message on RMS error.
 * T1.0-02 - 15-Apr-91 - tmk - Redefine fab$b_journal as fab$b_rfm+1.
 * T1.0-03 - 15-Apr-91 - tmk - ACL support, sync w/ 5A(169) and ckvfio.c 2.0-
 *			       069.
 * T1.0-04 - 16-Apr-91 - tmk - Fix _another_ journaling bug (whimper), handle
 *			       missing semicolon in filespec gracefully.
 * T1.0-05 - 04-Sep-92 - tmk - Implement ckvfio.c 087 fix.
 * T1.0-06 - 08-Apr-93 - tmk - Implement ckvfio.c 097 fix.
 * T1.0-07 - 24-Feb-95 - mpjz- Fix for DEC C on VAX.
 * T1.0-08 - 06-Sep-95 - fdc - Get rid of nonportable memmove().
 */

#ifdef WINTCP
#include stdio
#include stdlib
#include ctype
#include string
#include rms
#include ssdef
#else
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <rms.h>
#include <ssdef.h>
#endif /* WINTCP */
#define	VERSION	"T1.0-06"

#ifndef OLD_VMS
#include <starlet.h>
#endif /* OLD_VMS */

#define	R_MODE	"rb"
#define	IO_ERROR	0
#define IO_SUCCESS	1

/*
 * Definitions for output file
 */

static struct FAB fab_ofile;
static struct RAB rab_ofile;
static struct XABDAT xabdat_ofile;
static struct XABFHC xabfhc_ofile;
static struct XABPRO xabpro_ofile;
static struct XABALL xaball_ofile;
static struct XABRDT xabrdt_ofile;
static short ofile_ffb;
static int ofile_rec;

/*
 * Common RMS items
 */

static unsigned long int rms_sts;

/*
 * Global varables
 */

int	keepacl = 0;			/* Preserve ACL data? */
int	backup	= 0;			/* Preserve file backup date? */
int	debug	= 0;			/* Debug output? */
int	notrim	= 0;			/* Don't trim names? */
int	inquire = 0;			/* Testing if labeled? */
int	owner	= 0;			/* Preserve file ownership? */
int	strip	= 0;			/* Just stripping? */
FILE	*infd	= NULL;			/* Input file descriptor */
char	*infn	= NULL;			/* Input file name */
char	*outfn	= NULL;			/* Special output file name */
char	buffer[512];			/* Work buffer */
char	label[99];			/* Label name */
int	lblen	= 0;			/* Length of label */
char	vmsname[255];			/* Stored name */
char	vmsfile[70];			/* Stored file info */
char	vmsacl[512];			/* Stored ACL data */
int	acllen = 0;			/* Size of same */
char	*filptr = vmsfile;		/* Attribute pointer */
int	gotname	= 0;			/* Found name? */
int	gotfile = 0;			/* Found file info? */
int	gotacl = 0;			/* Found ACL info? */
int	bail	= 0;			/* If we should bail out */
char	revdat[8];			/* Revision date */
unsigned short revnum;			/* Revision number */
unsigned short jnlflg;			/* Journaling flags */

/*
 * Function prototypes
 */

extern int main(int, char **);
void do_help();
void barf(char *);
void strip_file();

#if !defined(__DECC) && !defined(VAX)
#define XABP char
#else
#define XABP void
#endif

/* memmove() replacement */

void
mymove(to, from, len) char * to; char * from; int len; {
    char tmp[16384];
    strncpy((char *)tmp,from,len);
    strncpy(to,(char *)tmp,len);
}

/*
 * Ok, let's do it
 */

int main(argc, argv)
int argc;
char *argv[];
{
    register char *ap;
    int i, j;					/* How original */

    if (argc < 2)				/* User say anything? */
	do_help();				/* If not... */

    if (*argv[1] != '-') {
	infn = argv[1];
	if ((infd = fopen(argv[1], R_MODE)) == NULL) {
	    perror(argv[1]);
	    exit(IO_ERROR);
	}
	argc--;
	argv++;
    }
    else
	bail++;

    while (argc > 1) {
	ap = argv[1];
	if (*ap != '-') {
	    fprintf(stderr, "Unknown option '%s',", ap);
	    fprintf(stderr, " do CKVCVT -? for help.\n");
	}
	else for (ap++; *ap; ap++) {
	    switch (tolower(*ap)) {

	    case 'a':				/* Preserve ACL's */
		keepacl++;
		break;

	    case 'b':				/* Preserve backup date */
		backup++;
		break;

	    case 'd':				/* Debug mode? */
		debug++;
		break;

	    case 'f':				/* Name output file */
		if (isgraph(ap[1]) != 0)
		    outfn = &ap[1];
		else if (argc > 2) {
		    outfn = argv[2];
		    argc--;
		    argv++;
		}
		else {
		    break;
		}
		goto next_arg;

	    case 'i':				/* Inquire if labeled */
		inquire++;
		strip++;
		break;

	    case 'o':				/* Preserve file ownership */
		owner++;
		break;

	    case 's':				/* Just strip it */
		strip++;
		break;

	    case 't':				/* Don't trim filenames */
		notrim++;
		break;

	    case '?':				/* Emit help text */
	    case 'h':
		do_help();
		break;

	    default:
		fprintf(stderr, "?Unknown option '%c',", *ap);
		fprintf(stderr, " do CKVCVT -? for help\n");
	    }
	}
	next_arg:
	argc--;
	argv++;
    }

    if (bail != 0)
	exit(IO_ERROR);

    fread(buffer, 20, 1, infd);
    if (strncmp(buffer, "KERMIT LABELED FILE:", 20) != 0)
	barf("not a Kermit labeled file");

    fread(buffer, 2, 1, infd);
    buffer[2] = '\0';
    lblen = atoi(buffer);
    if (lblen != 2)
	barf("");

    fread(buffer, lblen, 1, infd);

    if (strip != 0)				/* Stripping headers? */
	strip_file();

    if (strncmp(buffer, "D7", 2) != 0)
	barf("not from a VAX/VMS system");

    fread(buffer, 6, 1, infd);
    if (strncmp(buffer, "04VERS", 6) != 0)
	barf("");

    fread(buffer, 8, 1, infd);
    buffer[8] = '\0';
    lblen = atoi(buffer);

    fread(buffer, lblen, 1, infd);
    buffer[lblen] = '\0';
    if (debug)
	fprintf(stderr, "File created under VAX/VMS %s\n", buffer);

    fread(buffer, 7, 1, infd);
    if (strncmp(buffer, "05KVERS", 7) != 0)
	barf("");

    fread(buffer, 8, 1, infd);
    buffer[8] = '\0';
    lblen = atoi(buffer);

    fread(buffer, lblen, 1, infd);
    buffer[lblen] = '\0';
    if (debug)
	fprintf(stderr, "File created with C-Kermit/VMS %s\n", buffer);

    next_label:
    fread(buffer, 2, 1, infd);
    buffer[2] = '\0';
    lblen = atoi(buffer);
    if (lblen == 0)
	barf("lost sync");

    fread(buffer, lblen, 1, infd);
    buffer[lblen] = '\0';
    if (strcmp(buffer, "VMSNAME") == 0) {
	fread(buffer, 8, 1, infd);
	buffer[8] = '\0';
	lblen = atoi(buffer);
	fread(vmsname, lblen, 1, infd);
	vmsname[lblen] = '\0';
	gotname++;
	if (debug)
	    fprintf(stderr, "Loaded file name block as %s\n", vmsname);
	i = (int)strstr(vmsname, "::");
	if (i != (int)NULL) {
	    char temp[255];
	    i += 2;
	    mymove(vmsname, (char *)i, (int)strlen(vmsname));
	}
        if (!notrim) {
	    char temp[255];
	    i = (int)strrchr(vmsname, ':');
	    j = (int)strrchr(vmsname, ']');
	    if (j == (int)NULL)
		j = (int)strrchr(vmsname, '>');
	    if (j > i)
		i = j;
	    i++;
	    mymove(vmsname, (char *)i, (int) strlen(vmsname));
	}
	if (strchr(vmsname, ';') != NULL) {
	    for (j = strlen(vmsname); vmsname[j] != ';'; j--)
	    ;
	    vmsname[j] = '\0';
	}
	if (debug)
	    fprintf(stderr, "Resultant filespec: %s\n", vmsname);
	goto next_label;
    }
    else if (strcmp(buffer, "VMSFILE") == 0) {
	fread(buffer, 8, 1, infd);
	buffer[8] = '\0';
	lblen = atoi(buffer);
	fread(vmsfile, lblen, 1, infd);
	vmsfile[lblen] = '\0';
	gotfile++;
	if (debug)
	    fprintf(stderr, "Loaded file attribute block\n");
	goto next_label;
    }
    else if (strcmp(buffer, "VMSACL") == 0) {
	fread(buffer, 8, 1, infd);
	buffer[8] = '\0';
	acllen = atoi(buffer);
	fread(vmsacl, acllen, 1, infd);
	vmsacl[acllen] = '\0';
	gotacl++;
	if (debug)
	    fprintf(stderr, "Loaded file ACL block\n");
	goto next_label;
    }
    else if (strcmp(buffer, "DATA") == 0) {
	fread(buffer, 8, 1, infd);
	buffer[8] = '\0';
	lblen = atoi(buffer);
	if (lblen != 0)
	    barf("");
	if (debug)
	    fprintf(stderr, "Positioned at start of file data\n");
	goto all_set;
    }
    else {
	fprintf(stderr, "%s: unrecognized label '%s'\n", infn, buffer);
	fread(buffer, 8, 1, infd);
	buffer[8] = '\0';
	lblen = atoi(buffer);
	if (lblen > 512)
	    barf("unrecognized label too long to skip");
	fread(vmsfile, lblen, 1, infd);
	goto next_label;
    }

    all_set:
    if (gotfile != 1 || gotname != 1)
	barf("missing required labels");

/*
 * Prep the characteristics
 */

    fab_ofile = cc$rms_fab;
    fab_ofile.fab$b_fac = FAB$M_BIO | FAB$M_PUT;
    fab_ofile.fab$l_fop = FAB$M_MXV;
    if (outfn == NULL) {
	fab_ofile.fab$l_fna = vmsname;
	fab_ofile.fab$b_fns = strlen(vmsname);
    }
    else {
	fab_ofile.fab$l_fna = outfn;
	fab_ofile.fab$b_fns = strlen(outfn);
    }
    fab_ofile.fab$l_xab = (XABP *)&xabdat_ofile;
    rab_ofile = cc$rms_rab;
    rab_ofile.rab$l_fab = &fab_ofile;
    xabdat_ofile = cc$rms_xabdat;
    xabdat_ofile.xab$l_nxt = (XABP *)&xabrdt_ofile;
    xabrdt_ofile = cc$rms_xabrdt;
    xabrdt_ofile.xab$l_nxt = (XABP *)&xabfhc_ofile;
    xabfhc_ofile = cc$rms_xabfhc;
    xabfhc_ofile.xab$l_nxt = (XABP *)&xabpro_ofile;
    xabpro_ofile = cc$rms_xabpro;
    xabpro_ofile.xab$l_nxt = (XABP *)&xaball_ofile;
    xaball_ofile = cc$rms_xaball;

/*
 * Load 'em up
 */

    mymove(&xabpro_ofile.xab$w_pro, filptr, 2);
    filptr += 2;
    if (owner != 0)
	mymove(&xabpro_ofile.xab$l_uic, filptr, 4);
    filptr += 4;
    mymove(&fab_ofile.fab$b_rfm, filptr, 1);
    filptr += 1;
    mymove(&fab_ofile.fab$b_org, filptr, 1);
    filptr += 1;
    mymove(&fab_ofile.fab$b_rat, filptr, 1);
    filptr += 5;				/* 4 bytes reserved for char */
    mymove(&fab_ofile.fab$b_fsz, filptr, 1);
    filptr += 1;
    mymove(&xabfhc_ofile.xab$w_lrl, filptr, 2);
    filptr += 2;
    mymove(&fab_ofile.fab$w_mrs, filptr, 2);
    filptr += 2;
    mymove(&xabfhc_ofile.xab$l_ebk, filptr, 4);
    filptr += 4;
/* preserve this as RMS won't remember it for us */
    mymove(&ofile_ffb, filptr, 2);
    filptr += 2;
    mymove(&xaball_ofile.xab$l_alq, filptr, 4);
    filptr += 4;
    mymove(&xaball_ofile.xab$w_deq, filptr, 2);
    filptr += 2;
#ifdef COMMENT /* was: defined(VAX) && defined(__DECC) */
/*
   This is really annoying. The people from DEC changed xaball, but only on
   VAX not AXP!  - mpjz
*/
    mymove(&xaball_ofile.xaballdef$$_fill_7, filptr, 1);
#else
    mymove(&xaball_ofile.xab$b_bkz, filptr, 1);
#endif /* COMMENT */
    filptr += 1;
    mymove(&fab_ofile.fab$w_gbc, filptr, 2);
    filptr += 2;
    mymove(&xabfhc_ofile.xab$w_verlimit, filptr, 2);
    filptr += 2;
    mymove(&jnlflg, filptr, 1);
    if (jnlflg !=0)
      printf(
	     "Original file was marked for RMS Journaling, this copy is not.\n"
	     );
    filptr += 1;
    mymove(&xabdat_ofile.xab$q_cdt, filptr, 8);
    filptr += 8;
    mymove(&revdat, filptr, 8);
    filptr += 8;
    mymove(&revnum, filptr, 2);
    filptr += 2;
    mymove(&xabdat_ofile.xab$q_edt, filptr, 8);
    filptr += 8;
    if (backup != 0)
	mymove(&xabdat_ofile.xab$q_bdt, filptr, 8);
    filptr += 8;

/*
 * ACL's?
 */

    if(keepacl != 0 && gotacl != 0) {
	xabpro_ofile.xab$l_aclbuf = (XABP *)&vmsacl;
	xabpro_ofile.xab$w_aclsiz = acllen;
    }

/*
 * Give it a quick whirl around the dance floor
 */

    printf("Creating %s...\n", fab_ofile.fab$l_fna);
    rms_sts = sys$create(&fab_ofile);
    if (!(rms_sts & 1))
	exit(rms_sts);

    if(keepacl != 0 && gotacl != 0) {
	if (!(xabpro_ofile.xab$l_aclsts & 1))
	    exit(xabpro_ofile.xab$l_aclsts);
    }

    rms_sts = sys$connect(&rab_ofile);
    if (!(rms_sts & 1))
	exit(rms_sts);

    ofile_rec = 1;
    fread(buffer, 512, 1, infd);
    while (!feof(infd)) {
	rab_ofile.rab$l_rbf = buffer;
	rab_ofile.rab$w_rsz = 512;
	if (ofile_rec == xabfhc_ofile.xab$l_ebk) {
	    xabfhc_ofile.xab$w_ffb = ofile_ffb;
	    if (ofile_ffb)
		rab_ofile.rab$w_rsz -= (512 - ofile_ffb);
	    if (debug) {
		fprintf(stderr,"FFB (first free byte) = %d\n", ofile_ffb);
		fprintf(stderr,"Last record size = %d\n", rab_ofile.rab$w_rsz);
	    }
	}
	rms_sts = sys$write(&rab_ofile);
	if (!(rms_sts & 1))
	    exit(rms_sts);
	fread(buffer, 512, 1, infd);
	ofile_rec++;
    }

/*
 * Update the revision information
 */

    mymove(&xabrdt_ofile.xab$q_rdt, revdat, 8);
    mymove(&xabrdt_ofile.xab$w_rvn, &revnum, 2);

    rms_sts = sys$close(&fab_ofile);
    if (!(rms_sts & 1))
	exit(rms_sts);
    printf("...done.\n");
    exit(IO_SUCCESS);
}

void do_help()
{
    printf("This is CKVCVT %s\n\n", VERSION);
    printf("Usage: CKVCVT infile options\n\n");
    printf("Options:\n");
    printf("         -a     Preserve file ACL data (may require privs)\n");
    printf("         -b     Preserve file backup date\n");
    printf("         -d     Print debugging information\n");
    printf("         -f fn  Name output file fn instead of default\n");
    printf("         -i     Inquire if a file is labeled\n");
    printf("         -o     Preserve file ownership (requires privs)\n");
    printf("         -s     Strip one level of label information\n");
    printf("         -t     Don't trim paths from output file name\n");
    printf("         -?     Display this message\n\n");
    exit(IO_SUCCESS);
}

void barf(text)
char *text;
{
    if (text == "")
	fprintf(stderr, "%s: corrupted Kermit labeled file\n", infn);
    else
	fprintf(stderr, "%s: %s\n", infn, text);
    exit(IO_ERROR);
}

void strip_file()
{
    next_label:
    fread(buffer, 2, 1, infd);			/* Get label length */
    buffer[2] = '\0';
    lblen = atoi(buffer);
    if (lblen == 0)				/* Better not be zero! */
	barf("lost sync");

    fread(label, lblen, 1, infd);		/* Get the label name */
    label[lblen] = '\0';
    fread(buffer, 8, 1, infd);			/* Get the contents length */
    buffer[8] = '\0';
    lblen = atoi(buffer);
    if (strcmp(label, "DATA") == 0) {		/* If done... */
	if (lblen != 0)
	    barf("");
	if (debug)
	    fprintf(stderr, "Positioned at start of file data\n");
	goto all_set;
    }
    fread(buffer, lblen, 1, infd);		/* Else skip contents */
    goto next_label;				/* And loop */

/*
 * We've skipped the first header, now do whatever task is needed
 */

    all_set:
    if (inquire != 0) {
	printf("%s is a properly formatted Kermit labeled file\n", infn);
	exit(IO_SUCCESS);
    }

    fab_ofile = cc$rms_fab;
    fab_ofile.fab$b_fac = FAB$M_BIO | FAB$M_PUT;
    fab_ofile.fab$l_fop = FAB$M_MXV;
    fab_ofile.fab$l_fna = infn;
    fab_ofile.fab$b_fns = strlen(infn);
    fab_ofile.fab$b_rfm = FAB$C_FIX;
    fab_ofile.fab$w_mrs = 512;
    rab_ofile = cc$rms_rab;
    rab_ofile.rab$l_fab = &fab_ofile;

    printf("Stripping %s...\n", fab_ofile.fab$l_fna);
    rms_sts = sys$create(&fab_ofile);
    if (!(rms_sts & 1))
	exit(rms_sts);

    rms_sts = sys$connect(&rab_ofile);
    if (!(rms_sts & 1))
	exit(rms_sts);

    fread(buffer, 512, 1, infd);
    while (!feof(infd)) {
	rab_ofile.rab$l_rbf = buffer;
	rab_ofile.rab$w_rsz = 512;
	rms_sts = sys$write(&rab_ofile);
	if (!(rms_sts & 1))
	    exit(rms_sts);
	fread(buffer, 512, 1, infd);
    }

    rms_sts = sys$close(&fab_ofile);
    if (!(rms_sts & 1))
	exit(rms_sts);
    printf("...done.\n");
    exit(IO_SUCCESS);
}
