/*
  C-Kermit C RTL replacement functions for VMS systems.
*/

#ifndef __CKVVMS_H
#define __CKVVMS_H


#ifdef OLDFIB
# define FIB_L_ACCTL fib$r_acctl_overlay.fib$l_acctl
# define FIB_W_DID fib$r_did_overlay.fib$w_did
# define FIB_W_FID fib$r_fid_overlay.fib$w_fid
#else /* def OLDFIB */
# define FIB_L_ACCTL fib$l_acctl
# define FIB_W_DID fib$w_did
# define FIB_W_FID fib$w_fid
#endif /* def OLDFIB [else] */


/* CKCFTP.C uses unlink(). */

#if __CRTL_VER < 70000000
#  define unlink delete
#endif /* __CRTL_VER < 70000000 */

/* CKCFTP.C and others use utime(). */

#if __CRTL_VER >= 70300000
#  include <utime.h>
#else /* __CRTL_VER >= 70300000 */
#  include <time.h>
#  define utime vms_utime

struct utimbuf {
    time_t actime;	/* access time */
    time_t modtime;	/* modification time */
};

int utime(const char *path, const struct utimbuf *times);

#endif /* __CRTL_VER >= 70300000 [else] */

#endif /* ndef __CKVVMS_H */
