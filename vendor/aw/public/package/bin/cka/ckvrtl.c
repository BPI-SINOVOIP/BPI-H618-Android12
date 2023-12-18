/*
  C-Kermit C RTL replacement functions for VMS systems.
*/

/*----------------------------------------------------------------------
 *
 *       utime()
 *
 *    VMS C RTL before V7.3 lacks utime().  In V7.3, utime() sets only
 *    the modified (revised) date, not the created date of a file.
 *
 *    UNIX "ls -l" reports the modified time.  VMS "DIRECTORY /DATE"
 *    reports the creation time.  Reconciling these in FTP DIR reports
 *    is non-trivial.
 *
 *    UNIX utime() sets revision and access times.  VMS does not always
 *    maintain access times, so this utime() replacement sets the
 *    creation and revision times to the specified revision (or
 *    creation?) time.  Any access time is ignored.
 *
 *----------------------------------------------------------------------
 */

#if __CRTL_VER >= 70300000

/* Avoid "%CC-W-EMPTYFILE, Source file does not contain any declarations." */

int dummy_function(void);

#else /* __CRTL_VER < 70300000 */

#include <errno.h>
#include <string.h>
#include <time.h>
#include <unixlib.h>

#include <atrdef.h>
#include <descrip.h>
#include <fibdef.h>
#include <iodef.h>
#include <lib$routines.h>
#include <rms.h>
#include <starlet.h>
#include <stsdef.h>

#include "ckvrtl.h"
#include "ckvrms.h"

/* Use <iosbdef.h> if available.  Otherwise declare IOSB here. */

#if !defined( __VAX) && (__CRTL_VER >= 70000000)
#include <iosbdef.h>
#else /* __CRTL_VER >= 70000000 */
typedef struct _iosb {
        unsigned short int iosb$w_status; /* Final I/O status   */
        unsigned short int iosb$w_bcnt; /* 16-bit byte count    */
        unsigned int iosb$l_dev_depend; /* 32-bit dev dependent */
    } IOSB;
#endif /* !defined( __VAX) && (__CRTL_VER >= 70000000) */


/* Ugly work-around for bad type in VAX <atrdef.h>. */

#ifdef __VAX
#define UWA (unsigned int)
#else /* def __VAX */
#define UWA
#endif /* def __VAX */

/*--------------------------------------------------------------------*/

#ifdef __DECC

/* Private utime() code. */

/* Action routine for decc$to_vms(), in utime(). */

char vms_path[ NAMX_C_MAXRSS+ 1];

int set_vms_name( char *name, int type)
{
   strncpy( vms_path, name, NAMX_C_MAXRSS);
   vms_path[ NAMX_C_MAXRSS] = '\0';
   return 1;
}

#endif /* def __DECC */

/*--------------------------------------------------------------------*/

/* utime() replacement. */

int vms_utime( const char *path, const struct utimbuf *times)
{
time_t utc_unsigned;

int chan, i;
int sts, sts2;

unsigned short int vms_num_vec_time[ 7];
/* Static to avoid %CC-W-ADDRCONSTEXT from old (V4.0-000) DEC C. */
static unsigned int vms_abs_time[ 2];
struct tm *tms;
struct _iosb iosb_q;

/* QIOW item list used to set creation and revision dates. */

struct atrdef ut_atr[ 3] = {
 {sizeof( vms_abs_time), ATR$C_CREDATE, UWA vms_abs_time},
 {sizeof( vms_abs_time), ATR$C_REVDATE, UWA vms_abs_time},
 {0,0,0}};

/* Various RMS structures used for file access. */

struct FAB ut_fab = cc$rms_fab;
struct RAB ut_rab = cc$rms_rab;
struct NAMX ut_namx = CC_RMS_NAMX;
static struct fibdef ut_fib;

/* Device and file name buffers and their descriptors. */

static char dev_namx[ NAMX_C_MAXRSS+ 1];
char esa_namx[ NAMX_C_MAXRSS+ 1];
char rsa_namx[ NAMX_C_MAXRSS+ 1];

struct dsc$descriptor dev_dsc =
 {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, dev_namx};

struct dsc$descriptor fib_dsc =
 {sizeof( ut_fib), DSC$K_DTYPE_T, DSC$K_CLASS_S, (char *) &ut_fib};

#ifdef __DECC

/* We will accept either a UNIX-like path name or a VMS-like path name.
   If a slash is found in the name, assume that it's UNIX-like, and
   convert it to VMS form.  Otherwise, use it as-is.
*/

if (strchr( path, '/') != NULL)
   {
   sts = decc$to_vms( path, set_vms_name, 0, 0);
   path = vms_path;
   }

#endif /* def __DECC */

/* Install the VMS file specification into the FAB. */

ut_fab.fab$l_fna = (char *) path;
ut_fab.fab$b_fns = (unsigned char) strlen( path);

ut_fab.fab$l_dna = "";
ut_fab.fab$b_dns = 0;

/* Point the FAB to the NAMX. */

ut_fab.FAB_L_NAMX = &ut_namx;

/* Install the name buffers into the NAM. */

ut_namx.NAMX_L_ESA = esa_namx;
ut_namx.NAMX_B_ESL = 0;
ut_namx.NAMX_B_ESS = sizeof( esa_namx)- 1;

ut_namx.NAMX_L_RSA = rsa_namx;
ut_namx.NAMX_B_RSL = 0;
ut_namx.NAMX_B_RSS = sizeof( rsa_namx)- 1;

/* Convert the modification time (UTC time_t) to local "tm" time. */

tms = localtime( &(times-> modtime));

/* Move (translate) "tm" structure local time to VMS vector time. */

if (tms != NULL)
   {
   vms_num_vec_time[ 0] = tms-> tm_year+ 1900;
   vms_num_vec_time[ 1] = tms-> tm_mon+ 1;
   vms_num_vec_time[ 2] = tms-> tm_mday;
   vms_num_vec_time[ 3] = tms-> tm_hour;
   vms_num_vec_time[ 4] = tms-> tm_min;
   vms_num_vec_time[ 5] = tms-> tm_sec;
   vms_num_vec_time[ 6] = 0;  /* centiseconds */

/* Convert VMS vector time to VMS absolute time (quadword). */

   sts = lib$cvt_vectim( vms_num_vec_time, vms_abs_time);

   if ((sts& STS$M_SEVERITY) == STS$K_SUCCESS)
      {
/* Parse the file specification. */

      sts = sys$parse( &ut_fab, 0, 0);

      if ((sts& STS$M_SEVERITY) == STS$K_SUCCESS)
         {
/* Locate the file. (Gets the FID.) */

         sts = sys$search( &ut_fab, 0, 0);

         if ((sts& STS$M_SEVERITY) == STS$K_SUCCESS)
            {
/* Form the device name descriptor. */

            dev_dsc.dsc$w_length = ut_namx.NAMX_B_DEV;
            dev_dsc.dsc$a_pointer = (char *) ut_namx.NAMX_L_DEV;

/* Assign a channel to the disk device. */

            sts = sys$assign( &dev_dsc, &chan, 0, 0);

            if ((sts& STS$M_SEVERITY) == STS$K_SUCCESS)
               {
/* Move the FID (and not the DID) into the FIB. */

               memset( (void *) &ut_fib, 0, sizeof( ut_fib));

               for (i = 0; i < 3; i++)
                  {
                  ut_fib.FIB_W_FID[ i] = ut_namx.NAMX_W_FID[ i];
                  ut_fib.FIB_W_DID[ i] = 0;
                  }

/* Prevent this QIOW from setting the revision time to now. */

               ut_fib.FIB_L_ACCTL = FIB$M_NORECORD;

/* Set the file dates. */

               sts = sys$qiow( 0,
                               chan,
                               IO$_MODIFY,
                               &iosb_q,
                               0,
                               0,
                               &fib_dsc,
                               0,
                               0,
                               0,
                               ut_atr,
                               0);

               if ((sts& STS$M_SEVERITY) == STS$K_SUCCESS)
                  {
                   sts = iosb_q.iosb$w_status;
                  }
               sts2 = sys$dassgn( chan);

               if ((sts& STS$M_SEVERITY) == STS$K_SUCCESS)
                  {
                  sts = sts2;
                  }
               }
            }
         }
      }
   }

/* Convert successful VMS status to zero = success status.
   If failure, set errno and vaxc$errno, and return -1 = failure status.
*/

if ((sts& STS$M_SEVERITY) == STS$K_SUCCESS)
   {
   sts = 0;
   }
else
   {
   errno = EVMSERR;
   vaxc$errno = sts;
   sts = -1;
   }

return sts;
}

#endif /* __CRTL_VER >= 70300000 [else] */

/**********************************************************************/

#if !defined( __VAX) && (__CRTL_VER >= 70301000)

#include <stdio.h>
#include <unixlib.h>

/* Flag to sense if vms_init() was called.  (Handy for debug.) */

int vms_init_done = -1;


/* vms_init()

      Uses LIB$INITIALIZE to set a collection of C RTL features without
      requiring the user to define the corresponding logical names.
*/

/* Structure to hold a DECC$* feature name and its desired value. */

typedef struct
   {
   char *name;
   int value;
   } decc_feat_t;

/* Array of DECC$* feature names and their desired values. */

decc_feat_t decc_feat_array[] = {
   /* Preserve command-line case with SET PROCESS/PARSE_STYLE=EXTENDED */
 { "DECC$ARGV_PARSE_STYLE", 1 },
   /* Preserve case for file names on ODS5 disks. */
 { "DECC$EFS_CASE_PRESERVE", 1 },
   /* Enable multiple dots (and most characters) in ODS5 file names,
      while preserving VMS-ness of ";version". */
 { "DECC$EFS_CHARSET", 1 },
   /* List terminator. */
 { (char *)NULL, 0 } };

/* LIB$INITIALIZE initialization function. */

static void vms_init( void)
{
int feat_index;
int feat_value;
int feat_value_max;
int feat_value_min;
int i;
int sts;

/* Set the global flag to indicate that LIB$INITIALIZE worked. */

vms_init_done = 1;

/* Loop through all items in the decc_feat_array[]. */

for (i = 0; decc_feat_array[ i].name != NULL; i++)
   {
   /* Get the feature index. */
   feat_index = decc$feature_get_index( decc_feat_array[ i].name);
   if (feat_index >= 0)
      {
      /* Valid item.  Collect its properties. */
      feat_value = decc$feature_get_value( feat_index, 1);
      feat_value_min = decc$feature_get_value( feat_index, 2);
      feat_value_max = decc$feature_get_value( feat_index, 3);

      if ((decc_feat_array[ i].value >= feat_value_min) &&
       (decc_feat_array[ i].value <= feat_value_max))
         {
         /* Valid value.  Set it if necessary. */
         if (feat_value != decc_feat_array[ i].value)
            {
            sts = decc$feature_set_value( feat_index,
             1,
             decc_feat_array[ i].value);
            }
         }
      else
         {
         /* Invalid DECC feature value. */
         printf( " INVALID DECC FEATURE VALUE, %d: %d <= %s <= %d.\n",
          feat_value,
          feat_value_min, decc_feat_array[ i].name, feat_value_max);
         }
      }
   else
      {
      /* Invalid DECC feature name. */
      printf( " UNKNOWN DECC FEATURE: %s.\n", decc_feat_array[ i].name);
      }
   }
}

/* Get "vms_init()" into a valid, loaded LIB$INITIALIZE PSECT. */

#pragma nostandard

/* Establish the LIB$INITIALIZE PSECTs, with proper alignment and
   other attributes.  Note that "nopic" is significant only on VAX.
*/
#pragma extern_model save

#pragma extern_model strict_refdef "LIB$INITIALIZE" 2, nopic, nowrt
void (*const x_vms_init)() = vms_init;

#pragma extern_model strict_refdef "LIB$INITIALIZ" 2, nopic, nowrt
const int spare[ 8] = { 0 };

#pragma extern_model restore

/* Fake reference to ensure loading the LIB$INITIALIZE PSECT. */

#pragma extern_model save
int LIB$INITIALIZE(void);
#pragma extern_model strict_refdef
int dmy_lib$initialize = (int) LIB$INITIALIZE;
#pragma extern_model restore

#pragma standard

#endif /* !defined( __VAX) && (__CRTL_VER >= 70301000) */

