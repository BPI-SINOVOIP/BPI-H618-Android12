#ifndef VMS
      ERROR -- CKVFIO.C is used only on the OpenVMS(tm) Operating System
#endif /* VMS */

/* On VAX, define Goofy VAX Type-Cast to obviate /standard = vaxc.
   Otherwise, lame system headers (<atrdef.h>) on VAX cause compiler
   warnings.  (GNU C may define vax but not __VAX.)
*/
#ifdef vax
# define __VAX 1
#endif /* def vax */

#ifdef __VAX
# define GVTC (unsigned int)
#else /* def __VAX */
# define GVTC
#endif /* def __VAX */

#if defined(__ia64) || defined(__ia64__)
# define CKVFIO_OS_ARCH_STRING " OpenVMS(tm) IA64"

#else
# if defined(__ALPHA) || defined(__alpha)
#  define CKVFIO_OS_ARCH_STRING " OpenVMS(tm) Alpha(tm)"
         /* do nothing */
#else
# ifdef VAX
#  define CKVFIO_OS_ARCH_STRING " OpenVMS(tm) VAX(tm)"
# else
#  ifdef __GNUC__
#     define CKVFIO_OS_ARCH_STRING " OpenVMS(tm) VAX(tm) (GCC)"
#  else
#     ERROR -- CKVFIO.C unknown architecture - Not VAX ALPHA or IA64
#  endif /* __GNUC__ */
# endif /* VAX */
#endif /* __ALPHA */
#endif /* __IA64 */

char *ckzv = "File support, 9.0.183,  16 Mar 2010";
char *ckzsys = CKVFIO_OS_ARCH_STRING;

/* C K V F I O  --  Kermit file system support for VAX/VMS.  */

/*
  Author: Frank da Cruz <fdc@columbia.edu>
  Columbia University Academic Information Systems, New York City.

  Copyright (C) 1985, 2010,
    Trustees of Columbia University in the City of New York.
    All rights reserved.  See the C-Kermit COPYING.TXT file or the
    copyright text in the ckcmai.c module for disclaimer and permissions.
*/

/*
  Originally adapted to VMS by:
  Stew Rubenstein, Harvard University Chemical Labs, 1985,
  Contributors:
  Frank da Cruz (fdc), Columbia University, New York City (1985-present)
  Terry Kennedy (TMK), St. Peter's College, Jersey City, NJ (1990-present)
  William Bader (WB), Lehigh University, Bethlehem, PA (1990-present)
  Mark Berryman (mb), SAIC (1994-present)
  Lucas Hart (LH), Oregon State U, (1998-present)
  Stew Rubenstein, Harvard University Chemical Labs, Cambridge, MA (1985)
  Martin Minow (MM), Digital Equipment Corporation, Maynard MA (1985) (d.2000)
  Dan Schullman (DS), Digital Equipment Corporation, Maynard MA (1985)
  Mark Buda (MAB), Digital Equipment Corporation, Nashua, NH (1989-90)
  Gary Mussar (GM), Bell-Northern Research, Ottawa, Canada (1991)
  James Sturdevant (JS) (1992)
  Tarjei T. Jensen (ttj), Norwegian Hydrographic Service (1993-94)
  Martin PJ Zinzer (mpjz), Gesellschaft fuer Schwerionenforschung GSI Darmstadt
*/
/* Edit history
 * 003 20-Mar-85 MM  fixed fprintf bug in zsout.c
 * 004 21-Mar-84 MM  create text files in variable-stream.
 * 005  8-May-85 MM  filled in zkself (not tested), fixed other minor bugs
 * 006  5-Jul-85 DS  handle version number in zltor, zrtol
 * 007 11-Jul-85 fdc fix zclose() to give return codes
 * 008 19-Mar-86 fdc Fix system() for "!", zopeni() for REMOTE commands.
 * 008 17-Sep-87 fdc Define PWDCMD.
 * 009 (???)
 * 010 24-Jan-88 fdc Add zgtdir() function, even tho it doesn't work...
 * 011 14-Feb-89 mab Make zgtdir() work in V2/V3 C envirements,
 *		     Make zkself work using delprc() using Will Wood's changes.
 * 012 26-Feb-89 mab Add function that searches for kermit.ini file in various
 *                   ways
 * 013 05-Mar-89 mab Add Barry Archers enhancements/fixes.
 * 014 15-Mar-89 mab Check for non-null data, not array of pointers in
 *                   zkermini
 * 015 04-Apr-89 mab Add latent support for attribute packet.  Clean up
 *		     file name translation code.
 * 016 05-Apr-89 mab Add PWP code to optimize packetizing.
 * 017 16-Apr-89 mab PWP changes broke REMOTE command.  Fixed.
 * 018 18-Apr-89 mab #ifdef chkfn.  This removes a lot of overhead.
 *		     Add code to gtdir() for V4.x.
 * 019 12-Jun-89 mab Add PWP's encode logic
 * 020 09-Jul-89 mab Add logic to check for system() availability
 * 021 10-Jul-89 mab Fix SHOW USER USERNAME.  Added space after 'SHOW USER'.
 * 022 27-Sep-89 mab Added zmail/zprint, plus added changes from CKUFIO.C
 * 023 01-Dec-89 mab Add RMS file support
 * 024 20-Jul-90 wb  Add support for old VAX C & VMS versions + zstrip & rename
 * 025 29-Jul-90 tmk Change space command to show avail, not used (match spec)
 * 026 29-Jul-90 tmk Hack out the RMS stuff - it can come back when it works
 * 027 29-Jul-90 tmk Likewise the VMS V3 stuff - ancient history
 * 028 29-Jul-90 tmk Replace the attribute stuff. It now works.
 * 029 31-Jul-90 tmk Fix CWD command (via hack)
 * 030 31-Jul-90 tmk Fix assorted bugs preventing remote commands from working
 * 031 31-Jul-90 tmk Correctly handle interrupted remote commands
 * 032 04-Aug-90 tmk Start work on full RMS support for input files
 * 033 04-Aug-90 tmk Tack LF on end of subprocess output lines
 * 034 04-Aug-90 tmk Complete work on full RMS support for input files
 * 035 04-Aug-90 tmk Add support for Fortran CC, fill in recfm data
 * 036 05-Aug-90 tmk Add trailing CRLF on print format files
 * 037 12-Aug-90 tmk Start work on full RMS support for output files
 * 038 12-Aug-90 tmk Honor first free byte (FFB) on SENDs
 * 039 13-Aug-90 tmk Finished first cut of full RMS support for output files
 * 040 29-Sep-90 tmk Add iswild() from FDC for C-Kermit 5A edit 157
 * 041 06-Oct-90 tmk Add filetype IMAGE support for outbound transfers. Note
 *		     that this doesn't currently work as the receiver overrides
 *		     it (must talk to fdc).
 * 042 06-Oct-90 tmk Make logfiles MRS=80. Being able to edit them outweighs
 *		     any use for un-split lines.
 * 043 17-Oct-90 wb  Make zclosf() remove delete mailboxes & deassign channels
 *                   used to talk to the subprocess, so quotas are not used
 *                   up after repeated mailbox use (installed by fdc).
 * 044 19-Oct-90 fdc Changed zxcmd() to use the fp[] arrays in the normal way,
 *                   and zsyscmd to call zxcmd(ZIFILE) rather than
 *                   zxcmd(ZSYSFN).  Got rid of all calls to system(), used
 *                   zsyscmd() instead, so commands like DIR could be
 *                   interrupted.  Made zoutdump() return(-1) rather than
 *                   exit() when "line too long for buffer", and increased
 *                   line output buffer from 1K to 4K.
 * 045 01-Nov-90 tmk Corrected behavior of error check on $create call so a
 *		     file supersede would work properly.
 * 046 01-Nov-90 tmk Clone binary flag to ofile_bmode so we have a consistent
 *		     view of this flag during file operations - the binary flag
 *		     tends to toggle when we don't want/expect it to.
 * 047 01-Nov-90 tmk Make IMAGE mode work. Note that image mode is only used
 *		     when VMS is sending a file, and includes all record
 *		     control characters not normally sent. Only useful in
 *		     unusual circumstances.
 * 048 01-Nov-90 tmk Remove spurious \n from zsoutl() which caused debug logs
 *		     to have spurious <CR>'s when viewed with editors.
 * 049 02-Nov-90 fdc Adapt to dynamic allocation of file i/o buffers.  Changes
 *                   are within #ifdef DYNAMIC..#else..#endif brackets.
 * 050 02-Nov-90 fdc Make zsyscmd() close inferior process.
 * 051 ??-???-?? ??? Add ckermit_init logical, return 0 on wildcard operations.
 * 052 24-Dec-90 tmk Fix performance problems after 32Kb w/ ASCII receives, fix
		     2-nulls-per-32Kb in binary mode bug (actually in ckcker.h,
		     this is a placeholder).
 * 053 13-Jan-91 tmk Add support for SET FILE RECORD-LENGTH.
 * 054 14-Jan-91 tmk Fix cases of /x/CR/LF/y/ and /x/CR/LF/y/CR/LF/ in ASCII
 *		     file receives.
 * 055 16-Jan-91 tmk Log requested file type to debug log when receiving.
 * 056 16-Jan-91 tmk Add support for all zstime() functions.
 * 057 17-Jan-91 tmk Add support for zchkspa() function.
 * 058 17-Jan-91 tmk Move debug() call into if clause in zxpand, per fdc.
 * 059 18-Jan-91 tmk Support remote (DECnet) file accesses.
 * 060 18-Jan-91 tmk Fix READ command.
 * 061 30-Jan-91 tmk Support creation of UNDEFINED file types for brain-dead
 *		     BASIC implementation.
 * 062 30-Jan-91 tmk Fix REMOTE commands when VERIFY is set.
 * 063 29-Mar-91 tmk Add padding factor for received text files to accomodate
 *		     space taken up by record delimiters (per fdc).
 * 064 29-Mar-91 gm  Remove unnecessary mem-mem moves during ASCII receives.
 *		     (Installed by tmk. To back out, #define OLD_WAY).
 * 065 30-Mar-91 tmk First pass at implementing LABELED. Send only, dummy
 *		     data records.
 * 066 02-Apr-91 tmk Finish first pass at LABELED. Send VMS filename, attri-
 *		     butes. Still need ACL's, "hidden" char. longword, recep-
 *		     tion.
 * 067 09-Apr-91 tmk LABELED bugfixes - VMSFILE is 70 bytes, not 74, use the
 *		     xab$w_lrl field instead of rab$w_rsz, fab$w_deq instead
 *		     of xab$w_rsz, fab$b_bks instead of xab$b_bkz, always pro-
 *		     cess an even multiple of 512 bytes when LABELED.
 * 068 14-Apr-91 tmk Don't use C definition of fab$b_journal as it doesn't ex-
 *		     ist before C V3.1. Compute it ourselves instead.
 * 069 15-Apr-91 tmk Initial work on retrieving ACL information for LABELED.
 * 070 16-Apr-91 tmk Make edits 066-069 compatible with DECnet.
 * 071 21-May-91 tmk Address R. Weiner QAR item 2 (filesize).
 * 072 21-Jun-91 tmk Check (and prohibit) spawns from captive accounts.
 * 073 21-Jun-91 tmk Fix session logging (for Charlie Luce/DECUServe).
 * 074 21-Jun-91 tmk Rework 071 to only apply to SPAWN/PUSH and not to the
 *		     pseudo-builtins like DEL, SPACE, WHO, PWD, etc.
 * 075 21-Jun-91 tmk Fix possible endless loop when flushing output file in
 *		     zclosf() after zoutdump() error.
 * 076 21-Jun-91 tmk First pass on handling inbound LABELED files.
 * 077 14-Nov-91 tmk Fix zprint(), zmail() (need to use system() for these).
 *                   This is a partial backout of 044.
 * 078 14-Nov-91 tmk Various cleanups.  Delete files after successful mailing
 *		     or printing, remove dead code inside #ifdef COMMENT and
 *                   #ifdef OLD_WAY, fix typo in spawning message, make sure
 *                   all source lines < 80 chars.
 * 079 22-Nov-91 fdc Change zmail(), zprint() error return values to improve
 *		     error reporting.
 * 080 18-Jan-92 tmk Fix REMOTE so output from a remote command correctly dis-
 *		     plays on terminal. This has been broken since 040 or so.
 * 081 10-Jun-92 tmk Add William Bader's fix for fixed-length files which have
 *                   record attributes.
 * 082 03-Jul-92 tmk Fix really bad bug introduced in 081 (which made *all*
 *                   fixed-format files be sent as text).
 * 083 15-Jul-92 jah Fix fencepost error in zoutdump when line breaks at 32K.
 * 084 03-Aug-92 fdc Remove current directory from init file search.
 * 085 26-Aug-92 tmk Add Bernd Onasch's fix for fgen().
 * 086 28-Aug-92 tmk Fix bug reported by Bill Hoelzer where C-K would execute
 *		     a file named "." as a C-K initialization file.
 * 087 04-Sep-92 tmk Fix bug reported by Chuck McMichael where C-K would not
 *		     set the FFB properly when receiving a labeled file which
 *		     did not have the FFB on a record boundary.
 * 088 09-Sep-92 tmk Fix Hunter Goatley's problem with SPAWN command ignoring
 *		     Ctrl-C.
 * 089 11-Sep-92  js Fixed malloc() in zmail().
 * 090 28-Oct-92 tmk Fix null-byte error introduced by 087. Gee, this looked
 *		     so simple when I designed it.
 * 091 02-Nov-92 tmk Start work on fixing spawn/push/remote commands, due to
 *		     popular whining.
 * 092 03-Nov-92 tmk Finish up initial 091 work. Vote for Kermit!
 * 093 03-Nov-92 fdc Change zkermini() to work with "-y" command-line option.
 * 094 04-Nov-92 tmk Make zxpand() not return all files if given null string.
 * 095 05-Nov-92 fdc Make zxcmd(), zclose(), etc, handle ZRFILE (OPEN !READ).
 * 096 17-Feb-93 fdc prevent zopeno from calling zstime if date struct is NULL,
 *                   and add support for ZMFILE (misc output file).
 * 097 08-Apr-93 tmk Correctly handle "international VMS" which uses <> instead
 *		     of [] for directory delimiters.
 * 098 16-May-93 fdc ANSIfication for GNU CC, from James Sturdevant, plus
 *                   add FAB$M_PRN to list of text-file types, for VMS batch
 *                   logs.
 * 099 07-Jun-93 fdc Fix calculation of file size in zchki(), fix declaration
 *                   of mbxnam[] (add one to size) to prevent overflow, which
 *                   would result in failure of server to respond to REMOTE
 *                   directory, etc.  Both fixes from Bill Glass.
 * 100 21-Jun-93 fdc file_date[] and attr_date[] declarations in zstime()
 *                   changed from long to unsigned long to prevent signed date
 *                   comparisons, which could prevent SET FILE COLLISION
 *                   UPDATE from working.  From James Sturdevant.
 * 101  8-Aug-93 fdc Add types to all function declarations.
 * 102 18-Aug-93 ttj Minor updates in zsattr() and do_label_recv() mainly to
 *                   quieten the compiler (which had every reason to complain).
 * 103  5-Nov-93 wb   Add isdir() function.
 * 104  8-Nov-93 wb   Add zfcdate() function.
 * 105 25-Nov-93 fdc  Correct record format for session log; change name of
 *                    password structure member of zattr struct.
 * 106 22-Dec-93 tmk  Correct fd "leakage" in OPEN READ/CLOSE READ pairs.
 * 107 26-Feb-94 mb   Addition of zmkdir() routine.
 * 108 27-Mar-94 tmk  Increase max record size for logs from 80 to 254.
 *                    Add support for file append operations.
 *                    Make zkself() retry a few times to avoid zombies on BYE.
 * 109  5-Apr-94 tmk  Fix xx->lengthk not being set in zsattr().
 * 110  8-Jun-94 tmk  Use private fab in zchki (fixes OPEN READ bug).
 * 111 17-Jun-94 tmk  Let zsattr() work even if there is no rights database.
 * 112  7-Jul-94 js   A couple small ANSIfications for gcc.
 * 113  7-Aug-94 fdc  Make zshcmd()/zsyscmd() return proper return code, with
 *                    help from Larry Henry at TWG.  Still not quite right...
 * 114 29-Sep-94 fdc  Increase max wildcard matches from 500 to 4096.
 * 115  4-Oct-94 mb   Add support for RESEND.
 * 116 26-Oct-94 mb   Minor fix to 115.
 * 117  1-Nov-94 wb   A couple #ifdefs added to allow compilation in VMS v4.
 * 118 25-Feb-95 mpjz Fix for DECC on VAX.
 * 119 11-May-96 fdc  Change znewn() to change version to x+1, or add ;0.
 * 120 31-May-96 fdc  Fix zfnqfp() to remove trailing .; if directory name.
 * 121 12-Jun-96 fdc  Prevent dereferencing null pointer in do_label_recv().
 * 122 23-Jun-96 fdc  Fix a bug in do_label_recv that I introduced in edit 121.
 * 123 21-Aug-96 fdc  Separate ZRFILE from ZIFILE so now we can have both.
 * 124 21-Aug-96 fdc  Fix includes for getenv & strcpy for DECC vs VAXC.
 * 125 25-Aug-96 mpjz More DECC/VAX fixes.
 * 126 05-Sep-96 fdc  Remove #module, remove refs to xaballdef$$_fill_7.
 * 127 06-Sep-96 fdc  Recover from interrupted labeled-mode receives.
 * 128 06-Sep-96 fdc  Recover from the previous edit.
 * 129 06-Sep-96 fdc  Fix zchki() to return -2 if blah.DIR;n is a directory.
 * 130 06-Sep-96 fdc  Fixes to isdir(), zstrip(), zchdir().
 * 131 17-Feb-97 fdc  Another fix to isdir() (free() was called too early).
 * 132 14-Jul-97 fdc  fgen() malloc'd but never freed filenames; isdir()
 *                    now handles directory files too (like [FOO]BAR.DIR;1).
 * 133 14-Jul-97 mb   New isdir() function uses sys$parse().
 * 134 15-Jul-97 fdc  Added cvtdir(); rewrote zchdir() to use it & new isdir().
 * 135 14-Aug-97 fdc  zopeni: Change mainline binary variable when switching
 *                    automatically from text to binary mode (cosmetic only).
 * 136 24-Aug-97 fdc  zhome: Return SYS$LOGIN value rather than HOME value.
 * 137 04-Sep-97 fdc  zchdir: use sys$setddir() rather than chdir() because
 *                    VMS 6.2 CRTL chdir() has bugs.
 * 138 06-Sep-97 fdc  zchdir again -- try one, then the other.  Add startupdir.
 * 139 04-Nov-97 fdc  Handle absolute/relative/standard/literal dir names.
 * 140 12-Dec-97 fdc  Fix fgen() to report directories if not told not to.
 * 141  1-Jan-98 fdc  Send and set file protections: zsattr(), zstime().
 * 142 15-Jan-98 fdc  Raised max number of files (MAXWLD) from 4K to 1024K.
 *                    and made the mtchs[] array dynamic.
 * 143 29-Jan-98 fdc  Make zshcmd() (RUN command) return proper status,
 *                    ditto for zxcmd(), and make both set pexitstat.
 *                    Also, added ckvmserrstr().
 * 144  8-Feb-98 lh   Add diagnostic message when SPAWN fails; handle files
 *                    with odd fixed record length; isdir() improvements.
 * 145  8-Feb-98 fdc  Slightly better handling of failures in zrmdir().
 * 146  3-May-98 fdc  Fix out-of-bounds cdate[] array reference.
 * 147  7-May-98 lh   another isdir() update; expanded mb (Apr-98) version.
 *               lh   Add REL_DIR item for do_label transfers.
 *               lh   FTN CC to handle zero-length records and overprinting.
 *               lh   Fix zltor for fspath and no match.
 *               fdc  Fix zmkdir to return 1 if >0 directories were created.
 * 148 28-Jun-98 fdc  Fix to odd-record-length code from edit 144, which broke
 *                    RESEND/REGET.  Added zgetfs().
 * 149  5-Sep-98 fdc  cdate[] array declaration needed to be 24, not 23.
 * 150 21-Dec-98 fdc  Added zrelname(), zgperm(), and ziperm().
 * 151 26-Jan-99 fdc  nzltor(): convert path format if other Kermit not VMS.
 * 152  8-Feb-99 lh   Fix constructions like "cd sys$manager" in isdir().
 * 153  8-Feb-99 fdc  Add zxrewind(), add support for recursion in fgen(),
 *                    make zgetfs call stat() to get size.
 *                    Fix chkfn() for connection log.
 * 154 17-Mar-99 lh   Add support for sending PRN files.
 * 155 15-Apr-99 fdc  Fix zmkdir() to tolerate long filenames.
 * 156 16-Apr-99 fdc  Add (nonworking) timeout support to get_subprc_line().
 *                    Somebody please look at this routine & fix it.  It's
 *                    supposed to time out and return after 1 second if nothing
 *                    comes from the subprocess, but instead it seems wait
 *                    forever.  To test, put VMS C-K in server mode and then
 *                    tell the client "remote host wait 00:00:30".  VMS C-K
 *                    is supposed to send back an empty data packet every
 *                    second but it doesn't.
 * 157 16-Apr-99 lh   Fix 156.
 * 158 20-Jun-99 fdc  Fix rinfill() to also work with mailboxes.  I don't see
 *                    how OPEN !READ/ READ/ CLOSE READ could ever have worked.
 * 159 22-Jul-99 lh   Add zcopy().
 * 160 23-Jul-99 fdc  Add lots of signed-vs-unsigned char casts.
 * 161 27-Jul-99 lh   Fix sys$synch() call in get_subprc_line().
 * 162 29-Jul-99 lh   Fixed get_subprc_line() keepalive performance.
 * 163 24-Aug-99 fdc  Fix do_label_send() broken by edit 160 (diagnosis by lh).
 * 164 02-Sep-99 fdc  Add nopush checks to zxcmd(), zshcmd(), and zsyscmd().
 * 165 19-Sep-99 lh   Fix zfnqfp() error return (NULL, not -1).
 *                    Subtly alter fgen()'s expansion of wildcards (*.* -> .*).
 * 166 06-Oct-99 lh   Fill in zchko.  Change log file error handling, zopen
 *                    and zclose.  Use parse result to handle <> and rel names
 *                    in zmkdir; cleanup.  Extend nzrtol to handle device spec
 *                    in absolute to relative; more <> dir syntax; no longer
 *                    need absolute for zmkdir.  Convert non-file oriented
 *                    device name to upper case in zfnqfp instead of appending
 *                    cwd; replace multiple strncat calls.  Fix fgen edit 165.
 *                    Fix typo in zrmdir.
 * 167 11-Oct-99 fdc  Replace all strncpy() and most strcpy() by ckstrncpy()
 *                    from ckclib.c.
 * 168 19-Nov-99 fdc  Fix a couple from the previous edit that were wrong.
 * 169 29-Aug-A0 fdc  Fix a couple signed/unsigned conflicts.
 * 170 01-Feb-A2 fdc  Make zkermini() return 1 on success and 0 on failure.
 * 171 27-Apr-A2 fdc  Fix zgetfs() to fail when file doesn't exist.
 * 172 30-Jul-A2 mb   Create a proper zchkpath().
 * 173 24-Oct-A2 fdc  Don't #include <conv$routines.h> ifdef NOCONVROUTINES,
 *                    needed in VMS 6.1 with UCX 4.2.
 * 174 24-Oct-A2 lh   define nam$w_did_num/_seq for VAX C 2.x.
 * 175 28-Oct-A2 fdc  Separate text and binary mode open for session log.
 * 176  3-May-A2 jea  ON_CD support.
 * 177  2-Jul-A2 fdc  #include <ckucmd.h> for prototypes needed by ON_CD.
 * 178 05-Apr-A2 fdc  Add __ia64 arch_string.
 * 179 23-Aug-A5 fdc  Fix nzrtol() for filenames like DEV:FOO.BAR.
 * 180 29-Dec-A5 fdc  Change file-size/offset variables from long to CK_OFF_T.
 * 181 15-Feb-A7 sms  Add zxin() routine.
 * 182 15-Mar-B0 sms  Fix include of stat.h for old VMS's
 *                    Added conditionality on ZCOPY for zcopy().
 *
 *  Note: If it makes sense to look for <conv$routines.h> only
 *  "#ifndef BUGFILL7", then it might make as much sense to require
 *  "#ifndef OLDFIB", too, because OLDFIB seems to apply to even older
 *  compiler environments than BUGFILL7.  But I didn't do that.
 *
 * 183 15-Mar-B0 sms  Added local prototypes for conv$*() functions, to be
 *                    used if <conv$routines.h> is missing (NOCONVROUTINES).
 *                    Changed to use NAML where available, to allow longer
 *                    file names.  New NAMX macros are defined in new
 *                    "ckvrms.h" header file.  Still no serious attempt to
 *                    handle exotic characters in ODS5 extended file names,
 *                    but caret escapes are handled in some places.
 *                    Moved some "#ifdef COMMENT" code for isdir() out of the
 *                    way.  Fixed a case-sensitivity problem in nzltor().
 *                    Changed zrmdir() to use QIO to set directory file
 *                    protection on local files, and delete() to delete them.
 *                    Changed cvtdir() to handle some (any?) non-trivial
 *                    cases, and to stop upcasing the path.
 *                    Removed some type-cast clutter.  Added a crude unit test
 *                    main program ("#ifdef UNIT_TEST").
 */

/* Definitions of some VMS system commands */

char *DIRCMD = "directory ";		/* For directory listing */
char *DIRCM2 = "directory ";		/* For directory listing, no args */
char *DELCMD = "delete ";		/* For file deletion */
char *TYPCMD = "type ";			/* For typing a file */
char *SPACMD = "show quota "; 		/* Space/quota of current directory */
char *SPACM2 = "show quota "; 		/* Space/quota of specified dir */
char *WHOCMD = "show users ";		/* For seeing who's logged in */
char *PWDCMD = "show default ";		/* For seeing current directory */

/*
  Functions (n is one of the predefined file numbers from ckermi.h):

   zopeni(n,name)   -- Opens an existing file for input.
   zopeno(n,name)   -- Opens a new file for output.
   zclose(n)        -- Closes a file.
   zchin(n)         -- Gets the next character from an input file.
   zsout(n,s)       -- Write a null-terminated string to output file, buffered.
   zsoutl(n,s)      -- Like zsout, but appends a line terminator.
   zsoutx(n,s,x)    -- Write x characters to output file, unbuffered.
   zchout(n,c)      -- Add a character to an output file, unbuffered.
   zchki(name)      -- Check if named file exists and is readable, return size.
   zchko(name)      -- Check if named file can be created.
   zchkspa(name,n)  -- Check if n bytes available to create new file, name.
   znewn(name,s)    -- Make a new unique file name based on the given name.
   zdelet(name)     -- Delete the named file.
   zxpand(string)   -- Expands the given wildcard string into a list of files.
   znext(string)    -- Returns the next file from the list in "string".
   zxcmd(n,cmd)     -- Execute the command in a lower fork on file number n.
   zclosf()         -- Close input file associated with zxcmd()'s lower fork.
   zrtol(n1,n2)     -- Convert remote filename into local form.
   zltor(n1,n2)     -- Convert local filename into remote form.
   zchdir(dirnam)   -- Change working directory.
   zhome()          -- Return pointer to home directory name string.
   zkself()         -- Log self out
   zsattr(struc zattr *) -- Return attributes for file which is being sent.
   zkermini(n1,n2)  -- Find kermit.ini using default scanning process
 */

/* Includes */

#define CKVFIO_C
#include "ckcdeb.h"
#include "ckcasc.h"
#include "ckcker.h"
#ifndef NOSPL
#include "ckuusr.h"                     /* ON_CD: mlook, parser, dodo */
#endif /* NOSPL */
#include "ckvvms.h"
#include "ckvrtl.h"
#include <stdio.h>
#include <types.h>
#include <stat.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <rms.h>
#include <ssdef.h>
#include <atrdef.h>
#include <descrip.h>
#include <dvidef.h>
#include <dcdef.h>
#include <fibdef.h>
#include <iodef.h>
#include <jpidef.h>
#include <signal.h>
#include <string.h>
#include <stsdef.h>
#include <syidef.h>
#ifndef OLD_VMS
#include <uaidef.h>
#endif  /* OLD_VMS */

/* rms.h above includes nam, fab, xab and rmsdef */
/* vms v4 headers do not check against multiple inclusion */
/* ifndefs below prevent multiple declaration of FAB and NAM structs */

#ifndef FAB$C_BID
#include <fab.h>			/* These are needed for isdir() */
#endif /* FAB$C_BID */

#ifndef NAM$C_BID
#include <nam.h>
#endif /* NAM$C_BID */

#ifdef VAXCV2         /* VAX C v2.x nam.h does not define the unions */
#define nam$w_did_num  nam$w_did[0]
#define nam$w_did_seq  nam$w_did[1]
#endif

#include <lnmdef.h>
#include <rmsdef.h>

#ifndef MAXWLD
#define MAXWLD 102400			/* Maximum wildcard filenames */
#endif /* MAXWLD */

/* external def. of things used in buffered file input and output */

#ifdef DYNAMIC
extern CHAR *zinbuffer, *zoutbuffer;
#else
extern CHAR zinbuffer[], zoutbuffer[];
#endif /* DYNAMIC */

static CHAR rinbuffer[INBUFSIZE], *rinptr;
static int rincnt;
static int lastcount = 0;
static int mtchsinit = 0;

extern CHAR *zinptr, *zoutptr;
extern int zincnt, zoutcnt, binary, nopush, rcflag, frecl;
extern long vernum;

/* Declarations */

FILE *fp[ZNFILS] = { 			/* File pointers */
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL
};

#define VMSVERSIONS

int vmssversions = 0;			/* Include version number w/filename */
int vmsrversions = 0;			/* Send and Receive.... */

/* Flags for each file indicating whether it is a mailbox */
int ispipe[ZNFILS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static CK_OFF_T iflen = (CK_OFF_T)-1;	/* Input file length */
static long rflen = -1;
static long oflen = -1;			/* Output file length */
static int fcount = 0;			/* Number of files in wild group */
static int nxpand = 0;			/* Copy of fcount */
static char nambuf[ CKMAXPATH];		/* maximum size of a file spec */
static char cwdbuf[ NAMX_C_MAXRSS];
static struct iosb_struct tmpiosb;	/* For QIOW */

extern unsigned int vms_status;		/* Used by CHECK_ERR */
extern unsigned int vms_lasterr;
static int cflag;			/* Flag indicating console in use */

int check_spawn(void);
int do_label_recv(void);
int do_label_send(char *name);
int fgen(char *pat, char *resarry[], int len);
static int rinfill(void);

#ifdef COMMENT
#ifdef __DECC
#include <stdlib.h>
#else /* def __DECC */
char *getenv(), *strcpy();
#endif /* __DEC [else] */
#else /* def COMMENT */
/* Recommended by Lee Tibbert */
#ifdef __DECC
#include <stdlib.h>
#include <string.h>
#else /* def __DECC */
char *getenv(), *strcpy();
#endif /* def __DECC [else] */
#endif /* def COMMENT [else] */

#ifdef __DECC
#include <lib$routines.h>
#include <starlet.h>
#endif /* __DECC */

int pexitstat = -2;			/* Process exit status */
unsigned long pexitlong = -2L;		/* ULONG version of same */

#ifdef COMMENT
/* static */				/* Not static any more! */
char *mtchs[MAXWLD],			/* Matches found for filename */
  **mtchptr;				/* Pointer to current match */
#else /* def COMMENT */
char **mtchs = NULL, **mtchptr = NULL;
#endif /* def COMMENT [else] */
static unsigned short mbx_chan;		/* Mailbox chan for REMOTE commands */
static int subprocess_input = 0, sub_count;
static char *sub_ptr, sub_buf[SUB_BUF_SIZE];

#define PIPETIMEOUT			/* For timing out subprocess reads */
#define	SUPERSAFE			/* For safe subprocesses */

static unsigned long sub_pid;

/*
 * Structures for input (SEND) file
 */

static	struct FAB fab_ifile;		/* For SEND file */
static	struct NAMX nam_ifile;
static	struct RAB rab_ifile;
static	struct XABDAT xabdat_ifile;
static	struct XABFHC xabfhc_ifile;
static	struct XABPRO xabpro_ifile;
static	struct XABALL xaball_ifile;

static	int ifile_bmode;		/* For SEND file */
static	int ifile_bcount;
static	char aclbuf[512];
static	unsigned long xuchar = 0;

static	struct FAB fab_rfile;		/* For OPEN READ file */
static	struct NAMX nam_rfile;
static	struct RAB rab_rfile;

static	struct XABDAT xabdat_rfile;
static	struct XABFHC xabfhc_rfile;
static	struct XABPRO xabpro_rfile;
static	struct XABALL xaball_rfile;

static	int rfile_bmode;		/* For READ file */
static	int rfile_bcount;
static	char raclbuf[512];

/*
 * Structures for output (RECEIVE) file
 */

static	struct FAB fab_ofile;
static	struct NAMX nam_ofile;
static	struct RAB rab_ofile;
static	struct XABDAT xabdat_ofile;
static	struct XABFHC xabfhc_ofile;
static	struct XABPRO xabpro_ofile;
static	struct XABALL xaball_ofile;
static	struct XABRDT xabrdt_ofile;
static	int ofile_dump;
static	int ofile_bmode;
static	int ofile_lblopts;
static	int ofile_lblproc = 0;
static	char revdat[8];
static	unsigned short revnum;

static	char ofile_vmsname[CKMAXPATH+1];
static	char ofile_vmsacl[512];
static	int ofile_acllen;
static	short ofile_ffb;

char startupdir[NAMX_C_MAXRSS+1];

/*
 * Common RMS items
 */
static unsigned long int rms_sts;

/*  I S W I L D  --  Tells whether filespec "str" is wild  */
/*  Returns 0 if not wild, 1 if wild */

int
iswild(str) char *str; {
    char c;
    if (!str) return(0);
    /* Should also check for [dir...] */
    while ((c = *str++) != '\0')
      if (c == '*' || c == '%') return(1);
    return(0);
}

#ifdef CK_TMPDIR

/*  I S D I R  --  Tells if string pointer s is the name of a directory. */
/*
   Returns:
    0 if s is not a directory or doesn't exist
    Nonzero if it is a directory:
      1 if the string is a directory specification, e.g., [FOO.BAR]
      2 if it is the name of a directory file, like [FOO]BAR.DIR;1
*/

/*  Searching for a file ".;" in the specified directory handles both local
 *  and DECNET references and determines whether a ".DIR;1" file is indeed
 *  a directory. No test is performed for multiple directories matching a
 *  wildcard expansion; cmifi2 does that after a isdir call.
 */

/* Mark Berryman's version.  See (far) below for (old) FDC version. */

int
isdir(path) char *path; {
    unsigned int retval, status, srch_lst;
    struct FAB path_fab;
    struct NAMX path_nam;
    const char *directory_type_l = ".dir";      /* Use case-insensitive */
    const char *directory_type_u = ".DIR";      /* compare below, instead? */
    char ch;

    char expanded_name[ NAMX_C_MAXRSS];
    char resultant_name[ NAMX_C_MAXRSS];

    path_fab = cc$rms_fab;                      /* Initialize FAB. */
    path_nam = CC_RMS_NAMX;                     /* Initialize NAM[L]. */
    path_fab.FAB_L_NAMX = &path_nam;            /* Point FAB to NAM[L]. */

    /* Install the path argument in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
    path_fab.fab$l_dna = (char *) -1;   /* Using NAML for default name. */
    path_fab.fab$l_fna = (char *) -1;   /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
    FAB_OR_NAML( path_fab, path_nam).FAB_OR_NAML_FNA = path;
    FAB_OR_NAML( path_fab, path_nam).FAB_OR_NAML_FNS = strlen( path);

    path_nam.NAMX_L_ESA = (char *) &expanded_name;
    path_nam.NAMX_B_ESS = sizeof( expanded_name);
    path_nam.NAMX_L_RSA = (char *) &resultant_name;
    path_nam.NAMX_B_RSS = sizeof( resultant_name);

    path_nam.NAMX_B_NOP = NAMX_M_SRCHXABS | NAMX_M_NOCONCEAL;
                                         /* remote node DISPLAY w/o OPEN */
                                          /* show physical device & root */
    retval = 1;                             /* default type return value */

/* PARSE the string */
    status = sys$parse(&path_fab,0,0);
    if (!(status & 1)) {		  /* any $PARSE errors are fatal */
        debug(F111,"isdir $PARSE", path, status);
        vms_lasterr = status;
        return (0);
    }

/* a directory may be specified as "lnm:foo.dir". Do a $SEARCH to traverse
 * a search list logical and locate the file;  pass on the resultant name
 * to determine whether the file is a directory file.
 */
    srch_lst = path_nam.NAMX_L_FNB & NAMX_M_SEARCH_LIST;
    if (srch_lst && ((path_nam.NAMX_B_NAME + path_nam.NAMX_B_TYPE) != 1)) {
        status = sys$search(&path_fab,0,0);
        if (!(status & 1)) {
            vms_lasterr = status;
            debug(F101,"isdir srch_lst status", "", status);
            return(0);
        }
        *(path_nam.NAMX_L_VER + path_nam.NAMX_B_VER) = '\0';
        debug(F110, "isdir srch_lst result", path_nam.NAMX_L_NODE,0);
    }

/* Was a file name given ? */

    if (path_nam.NAMX_B_NAME == 0) {
        if (path_nam.NAMX_B_TYPE != 1)
	  return(0);

/* Parse returns a file name of ".;" if no filename is given.  However,
 * that is a valid filename.  Testing gets complicated; the following
 * will not catch the case of such a file name passed as a logical,
 * but will otherwise return(0) when given a file name as "." or ";"
 */
        if (*path_nam.NAMX_L_DIR == '[' )
	  ch = ']';
        else
	  ch = '>';
        if (strchr(path,ch)) {		/* directory in path */
            if (*(strchr(path,ch)+1) == '.' ||
                *(strchr(path,ch)+1) == ';'    ) {
                debug(F111,"isdir file w/ zero length name" ,path, 0);
                return(0);
            }
        } else {                        /* no directory specification */
            if (strchr(path, '.') || strchr(path, ';') )
              return(0);
        }
        if (srch_lst) return(retval);  /* already did the SEARCH for ".;" */
    } else {

/* If a filename was in the path, a directory will only be something.dir */
/* Check to see if a directory was specified as a filename.
   This is done as follows:
	a) The result of the parse has been returned in a string pointed
	   to by path_nam.nam$l_esa in the form of device:[dir.dir]name.type;
	   nam$l_dev ==> start of device
	   nam$l_dir ==> first [
	   nam$l_name ==> first char after closing ]
	   nam$l_type ==> period between name and type
	   nam$l_ver ==> the semicolon
	b) NUL-terminate the TYPE by replacing the ; with a NUL
	c) Make sure TYPE is .DIR
	d) replace the closing ] with a .
	e) replace the . that starts the file TYPE with a ]
	f) reparse
*/
        *path_nam.NAMX_L_VER = '\0';	/* zero terminate file TYPE */
	if ((!strcmp(path_nam.NAMX_L_TYPE, directory_type_u)) ||
	 (!strcmp(path_nam.NAMX_L_TYPE, directory_type_l))) {
	    *path_nam.NAMX_L_TYPE = *(path_nam.NAMX_L_NAME-1); /* copy the ] */
	    *(path_nam.NAMX_L_NAME-1) = '.';		/* then change to . */
            /* Point FAB_OR_NAML to result. */
            FAB_OR_NAML( path_fab, path_nam).FAB_OR_NAML_FNA =
             path_nam.NAMX_L_NODE;
            FAB_OR_NAML( path_fab, path_nam).FAB_OR_NAML_FNS =
             path_nam.NAMX_L_TYPE+ 1- path_nam.NAMX_L_NODE;

	    status = sys$parse(&path_fab,0,0);		/* parse new string */
            if (!(status & 1)) {
                debug(F111,"isdir second $PARSE", path, status);
                vms_lasterr = status;
                return (0);
            }
	    retval = 2;
        } else
	  return(0);			/* non-directory filename specified */
    }

/* Access the directory */

    status = sys$search(&path_fab,0,0);
    if (!((status == RMS$_FNF) || (status & 1))) {
	switch (status) {
	  case RMS$_DNF:
	    debug(F100,"isdir SEARCH RMS$_DNF", "", 0);
	    break;
	  case RMS$_FND:
	    if (path_fab.fab$l_stv == SS$_BADIRECTORY) {
		debug(F100,"isdir .DIR file not a directory", "" ,0);
		break;
	    } else {
		debug(F101,"isdir directory access RMS$_FND stv","",
		      path_fab.fab$l_stv);
		vms_lasterr = status;
		break;
	    }
	  default:
	    retval = 0;
	    debug(F101,"isdir $SEARCH status","", status);
	    vms_lasterr = status;
        }
        return(0);
    }
    *(path_nam.NAMX_L_VER + path_nam.NAMX_B_VER) = '\0';
    debug(F110,"isdir final string", path_nam.NAMX_L_NODE, 0);
    debug(F101,"isdir directory code","",retval);
    return(retval);
}
#endif /* CK_TMPDIR */

/*  Z K S E L F  --  Log self out.  */

int
zkself() {
    int i;
    unsigned long int rms_s;
/*
  We need a better way.  If C-Kermit was spawned, this does not log out the
  whole job.    It also does not hang up LAT terminal sessions.
*/
    for (i = 0; i < 10; i++) {
	rms_s = sys$delprc(0,0);	/* Maybe some output is still */
	if (!(rms_s & 1)) vms_lasterr = rms_s;
	debug(F101,"zkself rms_s","",rms_s);
	if (rms_s == SS$_NORMAL)	/* queued; try a few times... */
	  exit(1);
	sleep(1);
    }
    exit(rms_s == SS$_NORMAL);
    return(0); /* dummy - required as this is called in a non-void context */
}

/*  Z O P E N I  --  Open an existing file for input. */

int
zopeni(n,name) int n; char *name; {
    debug(F111,"zopeni",name,n);
    debug(F101,"zopeni fp","",(int) fp[n]);
    if (chkfn(n)) return(0);
    ispipe[n] = 0;
    if (n == ZSYSFN) {			/* Input from a system function? */
	debug(F110,"zopeni called with ZSYSFN, failing!",name,0);
	*nambuf = '\0';			/* No filename. */
	return(0);			/* fail. */
    }
    zincnt = 0;				/* Initializing these couldn't hurt */
    zinptr = zinbuffer;
    if (n == ZSTDIO) {			/* Standard input? */
	if (isatty(0)) {
	    ermsg("?Terminal input not allowed\n");
	    debug(F110,"zopeni attempted input from unredirected stdin","",0);
	    return(0);
	}
	fp[ZIFILE] = stdin;
	return(1);
    }
/*
 * We open the file but waffle on the access mode we're going to use. We then
 * inspect the file characteristics to see if the organization is fixed or un-
 * defined. If it is, we convert to block mode operation. This is needed since
 * VMS maintains a "first free byte" field to tell us how much of the last rec-
 * ord really contains data, but won't terminate reads at that point. Thus, if
 * we want to SEND the exact same file we RECEIVEd, we have to honor the FFB
 * internally.
 */
    if (n == ZIFILE) {
	ifile_bmode = 0;		/* 0 = not binary */
	ifile_bcount = 0;
	fab_ifile = cc$rms_fab;                 /* Initialize FAB. */
	nam_ifile = CC_RMS_NAMX;                /* Initialize NAM[L]. */
	fab_ifile.FAB_L_NAMX = &nam_ifile;      /* Point FAB to NAM[L]. */
	fab_ifile.fab$b_fac = FAB$M_BRO | FAB$M_GET;
/*
 * Some non-VMS DECnet implementations don't allow switching modes, so set
 * block-I/O only mode if the user said SET FILE TYPE IMAGE or LABELED.
 */
	if (binary == XYFT_I || binary == XYFT_L)
	  fab_ifile.fab$b_fac = FAB$M_BIO | FAB$M_GET;

	/* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
	fab_ifile.fab$l_dna = (char *) -1;  /* Using NAML for default name. */
	fab_ifile.fab$l_fna = (char *) -1;  /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
	FAB_OR_NAML( fab_ifile, nam_ifile).FAB_OR_NAML_FNA = name;
	FAB_OR_NAML( fab_ifile, nam_ifile).FAB_OR_NAML_FNS = strlen( name);

	fab_ifile.fab$l_xab = (char *)&xabdat_ifile;
	rab_ifile = cc$rms_rab;
	rab_ifile.rab$l_fab = &fab_ifile;
	xabdat_ifile = cc$rms_xabdat;
	xabdat_ifile.xab$l_nxt = (char *)&xabfhc_ifile;
	xabfhc_ifile = cc$rms_xabfhc;
	xabfhc_ifile.xab$l_nxt = (char *)&xabpro_ifile;
	xabpro_ifile = cc$rms_xabpro;
	memset(&aclbuf, 0, sizeof(aclbuf));
	xabpro_ifile.xab$l_aclsts = SS$_NORMAL;		/* Oh Joy! DECnet */
	xabpro_ifile.xab$l_aclbuf = (char *)&aclbuf;
	xabpro_ifile.xab$w_aclsiz = sizeof(aclbuf);
	xabpro_ifile.xab$l_aclctx = 0;
	xabpro_ifile.xab$l_nxt = (char *)&xaball_ifile;
	xaball_ifile = cc$rms_xaball;

	rms_sts = sys$open(&fab_ifile);
	if (!(rms_sts & 1)) vms_lasterr = rms_sts;
	if (rms_sts != RMS$_NORMAL) {
	    debug(F101,"zopeni $open failed, status","",rms_sts);
	    debug(F101,"zopeni $open failed, stv","",fab_ifile.fab$l_stv);
	    return(0);
	}
	if (!(xabpro_ifile.xab$l_aclsts & 1)) {
	    if (xabpro_ifile.xab$l_aclsts != SS$_ACLEMPTY) {
		debug(F101,"zopeni $open ACL failed, status","",
		  xabpro_ifile.xab$l_aclsts);
		return(0);
	    }
	}

/* We have the file opened. See if it's fixed or undefined format... */

	iflen = ((xabfhc_ifile.xab$l_ebk-1)*512)+xabfhc_ifile.xab$w_ffb;
	if (fab_ifile.fab$b_rfm == FAB$C_UDF) {
	    debug(F100,"zopeni undefined file format - using blk I/O","",0);
	    ifile_bmode = 1;
	}
	if (fab_ifile.fab$b_rfm == FAB$C_FIX) {
	    if ((fab_ifile.fab$b_rat & (FAB$M_FTN | FAB$M_CR | FAB$M_PRN))
		== 0) {
/* But it may be changed later, for BINARY mode and odd record length */
		debug(F100,"zopeni fixed file format - using blk I/O","",0);
		ifile_bmode = 1;
	    }
  	}
	debug(F101,"zopeni binary flag at open","",binary);
	if (binary == XYFT_I) {
	    debug(F100,"zopeni using IMAGE mode by user request","",0);
	    ifile_bmode = 1;
	}
	if (binary == XYFT_L) {
	    debug(F100,"zopeni using LABELED mode by user request","",0);
	    ifile_bmode = 2;
	}
	debug(F101,"zopeni ifile_bmode","",ifile_bmode);
	debug(F101,"zopeni binary","",binary);
	if (ifile_bmode == 1 && binary == XYFT_T) {
	    debug(F100,"zopeni autoswitch from TEXT to BINARY","",0);
	    binary = XYFT_B;
	} else if (ifile_bmode == 0 && binary == XYFT_B) {
	    debug(F100,"zopeni autoswitch from BINARY to TEXT","",0);
	    binary = XYFT_T;
	}
/*
 * In BINARY mode, for the case of fixed record length format with odd
 * length records, use record i/o to suppress the RMS NUL pad ala Kermit-32
 */
	if ((fab_ifile.fab$b_rfm == FAB$C_FIX) &&
	    (fab_ifile.fab$w_mrs & 1) && /* was "(...mrs | 1)" oops! */
	    (binary == XYFT_B)) {
	    ifile_bmode = 0;

	    /* WARNING: setting ifile_bmode = 0 here can break RESEND */
	    /* (but only for odd-record length fixed-block files) */

	    debug(F100,"zopeni record i/o for BINARY, Odd Fixed RL","",0);
      	}
	rab_ifile.rab$l_rop = 0;
	rms_sts = sys$connect(&rab_ifile);
	if (!(rms_sts & 1)) vms_lasterr = rms_sts;
	if (rms_sts != RMS$_NORMAL) {
	    debug(F101,"zopeni $connect failed, status","",rms_sts);
	    return(0);
	}
	debug(F100,"zopeni RMS operations completed ok","",0);
	fp[n] = fopen("NLA0:","r");	/* it wants a fp, give it one */
	zincnt = 0;			/* reset input buffer */
	if (binary == XYFT_L)
	    do_label_send(name);	/* make a file label */
	return(1);
    } else if (n == ZRFILE) {		/* READ file */
	rfile_bmode = 0;
	rfile_bcount = 0;
	fab_rfile = cc$rms_fab;                 /* Initialize FAB. */
	nam_rfile = CC_RMS_NAMX;                /* Initialize NAM[L]. */
	fab_rfile.FAB_L_NAMX = &nam_rfile;      /* Point FAB to NAM[L]. */

        /* Always READ in text mode */
	fab_rfile.fab$b_fac = FAB$M_BRO | FAB$M_GET;

	/* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
	fab_rfile.fab$l_dna = (char *) -1;  /* Using NAML for default name. */
	fab_rfile.fab$l_fna = (char *) -1;  /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
	FAB_OR_NAML( fab_rfile, nam_rfile).FAB_OR_NAML_FNA = name;
	FAB_OR_NAML( fab_rfile, nam_rfile).FAB_OR_NAML_FNS = strlen( name);

	fab_rfile.fab$l_xab = (char *)&xabdat_rfile;
	rab_rfile = cc$rms_rab;
	rab_rfile.rab$l_fab = &fab_rfile;
	xabdat_rfile = cc$rms_xabdat;
	xabdat_rfile.xab$l_nxt = (char *)&xabfhc_rfile;
	xabfhc_rfile = cc$rms_xabfhc;
	xabfhc_rfile.xab$l_nxt = (char *)&xabpro_rfile;
	xabpro_rfile = cc$rms_xabpro;
	memset(&raclbuf, 0, sizeof(raclbuf)); /* Do we need this for READ? */
	xabpro_rfile.xab$l_aclsts = SS$_NORMAL; /* Oh Joy! DECnet */
	xabpro_rfile.xab$l_aclbuf = (char *)&raclbuf;
	xabpro_rfile.xab$w_aclsiz = sizeof(raclbuf);
	xabpro_rfile.xab$l_aclctx = 0;
	xabpro_rfile.xab$l_nxt = (char *)&xaball_rfile;
	xaball_rfile = cc$rms_xaball;

	rms_sts = sys$open(&fab_rfile);
	if (!(rms_sts & 1)) vms_lasterr = rms_sts;
	if (rms_sts != RMS$_NORMAL) {
	    debug(F101,"zopeni $open ZRFILE failed, status","",rms_sts);
	    debug(F101,"zopeni $open ZRFILE failed, stv","",
		  fab_rfile.fab$l_stv);
	    return(0);
	}
#ifdef COMMENT				/* who needs it */
	if (!(xabpro_rfile.xab$l_aclsts & 1)) {
	    if (xabpro_rfile.xab$l_aclsts != SS$_ACLEMPTY) {
		debug(F101,"zopeni $open ACL failed, status","",
		  xabpro_rfile.xab$l_aclsts);
		return(0);
	    }
	}
#endif /* COMMENT */

/* We have the file opened. See if it's fixed or undefined format... */

	rflen = ((xabfhc_rfile.xab$l_ebk-1)*512)+xabfhc_rfile.xab$w_ffb;
	if (fab_rfile.fab$b_rfm == FAB$C_UDF) {
	    debug(F100,"zopeni ZRFILE undefined file format - fail","",0);
	    return(0);
	}
	if (fab_rfile.fab$b_rfm == FAB$C_FIX) {
	    if ((fab_rfile.fab$b_rat & (FAB$M_FTN | FAB$M_CR | FAB$M_PRN))
		== 0) {
		debug(F100,"zopeni ZRFILE fixed file format - fail","",0);
		return(0);
	    }
  	}
	rab_rfile.rab$l_rop = 0;
	rms_sts = sys$connect(&rab_rfile);
	if (!(rms_sts & 1)) vms_lasterr = rms_sts;
	if (rms_sts != RMS$_NORMAL) {
	    debug(F101,"zopeni $connect failed, status","",rms_sts);
	    return(0);
	}
	debug(F100,"zopeni RFILE RMS operations completed ok","",0);
	fp[n] = fopen("NLA0:","r");	/* it wants a fp, give it one */
	rincnt = 0;			/* reset input buffer */
	return(1);
    }
    zincnt = 0;				/* Reset input buffer */
    fp[n] = fopen(name,"r");		/* Real file. */
    debug(F111,"zopeni", name, (int) fp[n]);
    if (fp[n] == NULL) perror("zopeni");
    return((fp[n] != NULL) ? 1 : 0);
}

/*  Z O P E N O  --  Open a new file for output.  */

char * ckvmserrstr(unsigned long);

int
zopeno(n,name,zz,fcb)
    int n; char *name; struct zattr *zz; struct filinfo *fcb; {
/*
  As of Version 5A, the attribute structure and the file information
  structure are included in the arglist.
*/
    int x;
    int fildes;
    int writeover = 0;
    extern int rs_len;
    char p[4];

    debug(F111,"zopeno",name,n);
    x = chkfn(n);
    debug(F101,"zopeno chkfn","",x);
    if (x != 0)
      return(0);
    ispipe[n] = 0;
    zoutcnt = 0;			/* Reset output buffer */
    zoutptr = zoutbuffer;
    cflag = 0;				/* Default to not using console */
    ofile_lblproc = 0;

    /* Open terminal or STDIO */

    if ((n == ZCTERM) || (n == ZSTDIO)) {
	fp[ZOFILE] = stdout;
	cflag = 1;			/* Say using console */
	debug(F101,"zopeno fp[]=stdout", "", (int) fp[n]);
	return(1);
    }
/*
  Open Debug, Transaction, Packet, Session logfile, or a Write file.
  The only other possibility at this point is the output file, so we test that.
*/
#ifdef DEBUG
    if (deblog) {
	if (fcb)
	  debug(F101,"zopeno fcb disposition", "", fcb->dsp);
	if (zz)
	  debug(F111,"zopeno zz disposition",zz->disp.val,zz->disp.len);
    }
#endif /* DEBUG */
    if (n != ZOFILE) {
	strcpy(p,"w");				/* Assume write/create mode */
	if (fcb) {				/* If called with an FCB... */
	    if (fcb->dsp == XYFZ_A)		/* Does it say Append? */
	    strcpy(p,"a");			/* Yes. */
	}

/* Note: don't add "ctx=rec", "shr=get" here - it slows writes to a crawl */

	if (n != ZSFILE) {
	    /* was mrs = 80; 254 is max record size for EDT */
	    fp[n] = fopen(name, p, "rat=cr", "rfm=var", "mrs=254");
	} else {			/* Session Log */
	    extern int sessft;		/* Type */
	    if (sessft == XYFT_T) {	/* Text */
		fp[n] = fopen(name, p, "ctx=stm", "rat=cr", "rfm=stmlf");
	    } else {			/* Binary */
		fp[n] = fopen(name, p, "ctx=bin", "rat=none",
			               "rfm=fix", "mrs=512");
	    }
	}
	if (fp[n] == NULL) {		/* Failed */
            if (errno == EVMSERR) {
	        debug(F111,"zopeno fopen failed vaxc$errno",name,vaxc$errno);
                if (vaxc$errno == RMS$_SYN)
                  printf("?fopen file name syntax error : %s\n", name);
                else
                  printf("?fopen failed %s : %s\n",name,
                         ckvmserrstr(vaxc$errno));
            } else {
	        debug(F111,"zopeno fopen failed errno",name,errno);
                perror(name);
            }
	} else {			/* Didn't fail */
	    debug(F100,"zopeno fopen ok", "", 0);
	}
	return((fp[n] != NULL) ? 1 : 0);
    }

/* Open a file to store data being RECEIVEd */

    if (n == ZOFILE) {
#ifdef DEBUG
	if (deblog)
	  switch (binary) {
	    case XYFT_T:
		debug(F100,"zopeno receiving TEXT file","",0);
		break;
	    case XYFT_B:
		debug(F100,"zopeno receiving BINARY file","",0);
		break;
	    case XYFT_I:
		debug(F100,"zopeno receiving IMAGE file-program bug!","",0);
		break;
	    case XYFT_L:
		debug(F100,"zopeno receiving LABELED file","",0);
		break;
	    case XYFT_U:
		debug(F100,"zopeno receiving UNDEFINED file","",0);
		break;
	    default:
		debug(F101,"zopeno unknown file type","",binary);
	}
#endif /* DEBUG */
	ofile_bmode = binary;
	ofile_dump = 0;
	ofile_ffb = -1;
	fab_ofile = cc$rms_fab;                 /* Initialize FAB. */
	nam_ofile = CC_RMS_NAMX;                /* Initialize NAM[L]. */
	fab_ofile.FAB_L_NAMX = &nam_ofile;      /* Point FAB to NAM[L]. */

	/* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
	fab_ofile.fab$l_dna = (char *) -1;  /* Using NAML for default name. */
	fab_ofile.fab$l_fna = (char *) -1;  /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
	FAB_OR_NAML( fab_ofile, nam_ofile).FAB_OR_NAML_FNA = name;
	FAB_OR_NAML( fab_ofile, nam_ofile).FAB_OR_NAML_FNS = strlen( name);

	fab_ofile.fab$l_fop = FAB$M_MXV;
	fab_ofile.fab$b_fac = FAB$M_PUT;
/*
  Note that we could actually implement a true overwrite (step on existing
  file version) operation here by testing for a new XYFZ_* type as well in
  the line below, and then simply not doing a RAB$M_EOF about 25 lines down
  from here. We might also have to do something to reset the file allocation
  so we aren't left with a large number of "leftover" blocks if the new file
  is smaller than the old one.
*/
	if (fcb) {
	    if (fcb->dsp == XYFZ_A) {
		fab_ofile.fab$l_fop = FAB$M_CIF;
/*
  This is for RESEND.  If the output mode is APPEND and the incoming
  attributes structure (zz) says "Resend", then we know this file is
  being resent.
*/
		if (*(zz->disp.val) == 'R' && ofile_bmode)
		  writeover = 1;
		debug(F101,"zopeno APPENDing, writeover","",writeover);
		if (writeover)
		  debug(F101,"zopeno RESEND, rs_len","",rs_len);
/*
  So if writeover != 0, we can take rs_len to be the length of the existing
  file to keep, and to write over the rest, which normally should be the final
  block.
*/
	    }
	}
	if (ofile_bmode) {
	    fab_ofile.fab$b_fac = FAB$M_BIO;
	    debug(F101,"zopeno using record size","",frecl);
	    fab_ofile.fab$w_mrs = frecl;
	    if (ofile_bmode == XYFT_U)
	      fab_ofile.fab$b_rfm = FAB$C_UDF;
	    else
	      fab_ofile.fab$b_rfm = FAB$C_FIX;
	} else {
	    fab_ofile.fab$b_rat = FAB$M_CR;
	    fab_ofile.fab$b_rfm = FAB$C_VAR;
	}
	fab_ofile.fab$b_shr = FAB$M_NIL;
	fab_ofile.fab$l_xab = (char *)&xabdat_ofile;
	rab_ofile = cc$rms_rab;
	rab_ofile.rab$l_fab = &fab_ofile;
	if (fcb)
	    if ((fcb->dsp == XYFZ_A) && (writeover == 0))
		rab_ofile.rab$l_rop = RAB$M_EOF;
	xabdat_ofile = cc$rms_xabdat;
	xabdat_ofile.xab$l_nxt = (char *)&xabfhc_ofile;
	xabfhc_ofile = cc$rms_xabfhc;
#ifndef THIS_IS_WRONG
	xabfhc_ofile.xab$l_nxt = (char *)&xabpro_ofile;
	xabpro_ofile = cc$rms_xabpro;
#endif /* THIS_IS_WRONG */
	if (zz)
	  zstime(name, zz, 2);		/* Set creation date from A packet */
	if (ofile_bmode == XYFT_L) {	/* DEFER OPEN IF LABELED <-- NOTE  */
	    ofile_lblproc = 0;		/* (Haven't processed labels yet.) */
	    ofile_lblopts = fcb->lblopts;
	    debug(F101,"zopeno lblopts","",ofile_lblopts);
	    debug(F100,"zopeno RMS operations deferred","",0);
	} else {
	    rms_sts = sys$create(&fab_ofile);
	    if (!(rms_sts & 1)) {
		vms_lasterr = rms_sts;
		debug(F101,"zopeno $create failed, status","",rms_sts);
		return(0);
	    }
	    rms_sts = sys$connect(&rab_ofile);
	    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
	    if (rms_sts != RMS$_NORMAL) {
		debug(F101,"zopeno $connect failed, status","",rms_sts);
		return(0);
	    }
	    if (writeover == 1) {		/* if resend ... */
		rab_ofile.rab$l_bkt = (unsigned long) rs_len >> 9;
		rms_sts = sys$space(&rab_ofile);/* space forward to last */
		if (!(rms_sts & 1)) vms_lasterr = rms_sts;
		rab_ofile.rab$l_bkt = 0;	/* complete block */
		if (rms_sts != RMS$_NORMAL) {
		    debug(F101,"zopeno $space failed, status","",rms_sts);
		    return(0);
		}
	    }
	    debug(F100,"zopeno RMS operations completed ok","",0);
	}
	fp[n] = fopen("NLA0:","r");	/* CK wants a fp, give it one */
	return(1);
    }
}

/*  Z C L O S E  --  Close the given file.  */

/*  Returns 0 if arg out of range, 1 if successful, -1 if close failed.  */

int
zclose(n) int n; {
    int x = 0;

    debug(F101,"zclose n","",n);
    if (chkfn(n) < 1)
      return(0);

/* If this is the subprocess file, close it to flush output */

    if ((n == ZIFILE || (n == ZRFILE)) && (subprocess_input != 0)) {
        debug(F100, "zclose calling zclosf", "", 0);
	return (zclosf(n));
    }
    if (n == ZIFILE) {			/* Input (e.g. SEND) file */
	rms_sts = sys$close(&fab_ifile);
	if (!(rms_sts & 1)) vms_lasterr = rms_sts;
	if (rms_sts != RMS$_NORMAL) {
	    debug(F101,"zclose ZIFILE $close failed, status","",rms_sts);
	    return(-1);
	}
	debug(F100,"zclose ZIFILE RMS operations completed ok","",0);
	x = fclose(fp[n]);		/* Close the dummy C library file */
	fp[n] = NULL;			/* Mark it closed */
	iflen = -1;			/* Invalidate length */
	return(1);
    } else if (n == ZRFILE) {		/* READ file */
	rms_sts = sys$close(&fab_rfile); /* Close it */
	if (!(rms_sts & 1)) vms_lasterr = rms_sts;
	if (rms_sts != RMS$_NORMAL) {	/* Check status */
	    debug(F101,"zclose ZRFILE $close failed, status","",rms_sts);
	    return(-1);
	}
	debug(F100,"zclose ZRFILE RMS operations completed ok","",0);
	x = fclose(fp[n]);		/* Close the dummy C-Library file */
	fp[n] = NULL;			/* and mark it closed */
	rflen = -1;			/* invalidate its length */
	return(1);
    }
    if (n == ZOFILE) {			/* Output (e.g. RECEIVE) file */
	ofile_dump = 1;			/* Force complete dump */
	while (zoutcnt != 0) {
	    rms_sts = zoutdump();	/* Flush buffers to disk */
	    if (rms_sts != 0) {
		x = fclose(fp[n]);	/* Close the associated dummy file */
		fp[n] = NULL;		/* Mark it closed */
		iflen = -1;		/* Invalidate length */
		return(-1);
	    }
	}
	if (ofile_bmode == XYFT_L) {	/* Update revisions if labeled */
#ifdef BUGFILL7				/* We should use a separate symbol.. */
	    int i;
	    char * s1, * s2;
	    s1 = (char *)&xabrdt_ofile.xab$q_rdt;
	    s2 = (char *)revdat;
	    for (i = 0; i < 8; i++)
	      *s1++ = *s2++;
	    s1 = (char *)&xabrdt_ofile.xab$w_rvn;
	    s2 = (char *)revnum;
	    for (i = 0; i < 2; i++)
	      *s1++ = *s2++;
#else
	    memmove(&xabrdt_ofile.xab$q_rdt, revdat, 8);
	    memmove(&xabrdt_ofile.xab$w_rvn, &revnum, 2);
#endif /* BUGFILL7 */
	    debug(F100,"zclose ZOFILE updated labeled revision count","",0);
	}
	if (cflag != 1) {		/* Not console */
	    rms_sts = sys$close(&fab_ofile); /* So close */
	    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
	    if (rms_sts != RMS$_NORMAL) {
		debug(F101,"zclose ZOFILE $close failed, status","",rms_sts);
		return(-1);
	    }
	    x = fclose(fp[n]);		/* Close the associated dummy file */
	} else				/* Console */
	  cflag = 0;			/* So not console any more */

	ofile_lblproc = 0;		/* Done with this file's labels */
	debug(F100,"zclose ZOFILE RMS operations completed ok","",0);
	fp[n] = NULL;			/* Mark it closed */
	iflen = -1;			/* Invalidate length */
	return(1);
    }
    if ((fp[n] != stdout) && (fp[n] != stdin)) { /* Other kind of file */
        x = fclose(fp[n]);		/* C-Library close */
        if (x == EOF) {			/* If we got a close error */
            debug(F101,"zclose failed errno","",errno);
            perror("zclose");
        }
    }
    debug(F101,"zclose OTHER ","",fp[n]);
    fp[n] = NULL;			/* Mark it closed */
    iflen = -1; 			/* Invalidate file length */
    if (x == EOF) 			/* If we got a close error */
      return (-1);			/* fail */
    else 				/* otherwise */
      return (1);			/* succeed */
}

int
get_subprc_line() {
    struct iosb_struct subiosb;
    int timedout = 0;
#ifdef PIPETIMEOUT
    extern int kactive, srvping;
#endif /* PIPETIMEOUT */

    unsigned int sts;
/*
 * Someone complained that subprocess deletion would hang the Kermit server.
 * This can be triggered by sending something silly like REMOTE HOST STOP/ID=0.
 * If SUPERSAFE is defined we will check to make sure the subprocess still
 * exists before every read from the mailbox. This will slow things down a bit,
 * but should stop the "C-Kermit just dies" reports.
 */
#ifdef	SUPERSAFE
    unsigned short pid;

    struct itmlstdef {
	short int buflen;
	short int itmcod;
	char *bufaddr;
	long int *retlen;
    };
    struct itmlstdef itmlst[] = {
	4, JPI$_PID, NULL, 0,
	0, 0, 0, 0
    };
    itmlst[0].bufaddr = (char *)&pid;

    sts = sys$getjpiw(0, &sub_pid, 0, &itmlst, 0, 0, 0);
    if (!(sts & 1)) vms_lasterr = sts;

    debug(F101,"get_subprc_line sys$getjpiw status", "", sts);
    if (sts == SS$_NONEXPR)
      return(-1);
#endif	/* SUPERSAFE */

#ifndef PIPETIMEOUT
    sts = sys$qiow(QIOW_EFN, mbx_chan, IO$_READVBLK, &subiosb, 0, 0, sub_buf,
		   sizeof(sub_buf), 0, 0, 0, 0);
#else
    if (kactive && srvping)  {
	unsigned long wait_sts, reqidt = 0L;
	unsigned long int mask = (1 << QIOW_EFN) | (1 << TIM_EFN);
	struct { int hi, lo; } qtime;
	qtime.hi = -10*1000*1000;	/* One second delta-time */
	qtime.lo = -1;

	sts = sys$setimr(TIM_EFN, &qtime, 0, reqidt, 0);
	if (sts != SS$_NORMAL) {
	    debug(F101,"get_subprc_line sys$setimr", "", sts);
	    vms_lasterr = sts;
	    return(-1);
	}
	sts = sys$qio(QIOW_EFN, mbx_chan, IO$_READVBLK,
		      &subiosb, 0, 0, sub_buf, sizeof(sub_buf), 0, 0, 0, 0);

	wait_sts = sys$wflor(TIM_EFN,mask);
	if (wait_sts != SS$_NORMAL) {
	    debug(F101,"get_subprc_line sys$wflor status", "", wait_sts);
	    vms_lasterr = wait_sts;
	    return(-1);
	}
	wait_sts = sys$readef(TIM_EFN, &mask);
	if (wait_sts != SS$_WASCLR && wait_sts != SS$_WASSET) {
	    vms_lasterr = wait_sts;
	    return(-1);
	}
	if (wait_sts == SS$_WASSET) {
	    sys$clref(TIM_EFN);
	    debug(F100,"get_subprc_line TIM_EFN timedout", "",0);
	    timedout = 1;
	} else {
	    wait_sts = sys$cantim(reqidt,0);
	    debug(F100,"get_subprc_line TIM_EFN was clear", "",0);
	    sys$synch(QIOW_EFN,&subiosb); /* Check QIO completion */
	    debug(F100,"get_subprc_line $synch(QIO) complete","",0);
	}
    } else
      sts = sys$qiow(QIOW_EFN, mbx_chan, IO$_READVBLK,
		     &subiosb, 0, 0, sub_buf, sizeof(sub_buf), 0, 0, 0, 0);
#endif /* PIPETIMEOUT */

    debug(F101,"get_subprc_line sys$qio{w} status", "", sts);
    if (sts != SS$_NORMAL) {
        vms_lasterr = sts;
        return(-1);
    }
    debug(F101,"get_subprc_line sys$qiow subiosb.status", "", subiosb.status);
    if (subiosb.status != SS$_NORMAL && !timedout) {
        vms_lasterr = subiosb.status;
        return(-1);
    }
    sub_count = subiosb.size;
    debug(F111,"get_subprc_line size", sub_buf, sub_count);

    if (timedout && (sub_count == 0))
      return(-3);

    sub_buf[sub_count] = '\r';
    sub_buf[sub_count + 1] = '\n';
    sub_buf[sub_count + 2] = '\0';
    sub_count += 2;
    debug(F111,"get_subprc_line size 2", sub_buf, sub_count);
    sub_ptr = sub_buf;

    return(0);
}

/*  Z C H I N  --  Get a character from the input file.  */

/*  Returns -1 if EOF, 0 otherwise with character returned in argument  */

int
zchin(n,c) int n, *c; {
    int a = -1;

#ifdef DEBUG
    if (chkfn(n) < 1) return(-1);
#endif /* DEBUG */

    if (n == ZIFILE) {
	if (subprocess_input) {
	    if (--sub_count < 0)
	      if (get_subprc_line()) return(-1);
	    a = *sub_ptr++;
	} else {
	    a = zminchar();
	}
    } else if (n == ZRFILE) {
	a = ((--rincnt)>=0) ? ((int)(*rinptr++) & 0377) : rinfill();
	/* debug(F101,"zchin a","",a); */
    }
    if (a == -1)
      return(-1);
    *c = (unsigned char)a;
    return(0);
}

/*  Z S I N L  --  Read a line from a file.  */

/*
  Writes the line into the address provided by the caller.
  n is the Kermit "channel number".
  Writing terminates when newline is encountered, newline is not copied.
  Writing also terminates upon EOF or if length x is exhausted.
  Returns 0 on success, -1 on EOF or error.
*/

int
zsinl(n,s,x) int n, x; char *s; {
    int a, z = 0, count = 0;
    int old;

    if (!s || chkfn(n) < 1)		/* Make sure file is open etc */
      return(-1);
    s[0] = '\0';			/* Clear buffer */
    a = -1;
    while (x--) {
	old = a;			/* Previous character */
	z = zchin(n,&a);
	/* debug(F101,"zsinl z","",z); */
	if (z < 0) {			/* Read a character from the file */
	    if (count == 0)
	      return(-1);		/* Signal EOF if problem */
	    else			/* Or if we have something */
	      return(0);		/* save failure till next time */
	} else
	  count++;
	a = a & 0377;
#ifdef NLCHAR
	if (a == (char) NLCHAR) break;	/* Single-character line terminator */
#else
	if (a == '\r') {
	    continue;
	}
	if (old == '\r') {
	    if (a == '\n') break;
	    else *s++ = '\r';
	}
#endif /* NLCHAR */
	*s = a;
	s++;
    }
    *s = '\0';
    return(z);
}

/*  Z X I N  --  Read x bytes from a file  */

/*
  Reads x bytes (or less) from channel n and writes them
  to the address provided by the caller.
  Returns number of bytes read on success, 0 on EOF or error.
*/
int
zxin(n,s,x) int n, x; char *s; {
#ifdef IKSD
    if (inserver && !local && (n == ZCTERM || n == ZSTDIO)) {
        int a, i;
        a = ttchk();
        if (a < 1) return(0);
        for (i = 0; i < a && i < x; i++)
          s[i] = coninc(0);
        return(i);
    }
#endif /* IKSD */
    return(fread(s, sizeof (char), x, fp[n]));
}


/*  Z I N F I L L  --  Read a line from a file.  */

/*
 * (re)fill the buffered file input buffer with data.  All file input
 * should go through this routine, usually by calling the zminchar()
 * macro (defined in ckcker.h).
 */

int
zinfill() {
    char cchar;
    int x, linelen;
    unsigned char vfcbuf[255];	/* 2 is default for PRN, but 255 is max */

    debug(F101,"zinfill rab_ifile.rab$l_bkt 1","",rab_ifile.rab$l_bkt);
    debug(F101,"zinfill ifile_bmode","",ifile_bmode);

    if (subprocess_input) {
	x = get_subprc_line();
	debug(F101,"zinfill get_subprc_line","",x);
	if (x == -3)
	  return(-3);
	if (x != 0)
	  return(-1);
/*
 * The size problem should never happen.  sub_buf of a size greater than
 * 1k is highly unlikely to be needed.
 */
	if (INBUFSIZE < SUB_BUF_SIZE) {
	    fprintf(stderr,"zinfill: sub_buf too large for zinbuffer");
	    exit(BAD_EXIT);
	}
	zinptr = (CHAR *)sub_buf;
	zincnt = sub_count;
    } else {
	if (ifile_bmode != 0) {
	    rab_ifile.rab$l_rop = RAB$M_BIO;	/* block mode I/O */
#ifdef DYNAMIC
	    rab_ifile.rab$l_ubf = (char *)zinbuffer;
#else
/* NOTE: Might need "(char *)&zinbuffer" here */
	    rab_ifile.rab$l_ubf = &zinbuffer;
#endif /* DYNAMIC */
/*
  There is a serious flaw here, namely that reading blocks rather than
  records *includes* the record pad byte (NUL) when the record has an
  odd length.  All RMS records are stored on even-numbered byte boundaries.
  Kermit-32 did it right, sigh.  So we need a total rewrite to allow for
  odd-length records.  Hmmm.. I wonder what the impact on RESEND is...
*/
	    rab_ifile.rab$w_usz = 512;
	    rms_sts = sys$read(&rab_ifile);
	    if (!(rms_sts & 1)) {
		debug(F101,"zinfill sys$read failed, status","",rms_sts);
		vms_lasterr = rms_sts;
	    }
	    if (rms_sts == RMS$_EOF)
	      return(-1);		/* End of file */
	    if (rms_sts != RMS$_NORMAL)
	      return(-1);		/* Fatal */

	    ifile_bcount++;		/* No error, say another block read */
	    debug(F101,"zinfill sys$read ok, ifile_bcount","",ifile_bcount);
	    zincnt = 512;
	    zinptr = zinbuffer;
	    if (rab_ifile.rab$l_bkt != 0) { /* If just a file position... */
		debug(F101,"zinfill updating block counter","",0);
		ifile_bcount = rab_ifile.rab$l_bkt; /* update block counter */
	    }
	    if (ifile_bcount == xabfhc_ifile.xab$l_ebk) {
		if (ifile_bmode == 1)	/* BINARY but not LABELED */
		    zincnt = xabfhc_ifile.xab$w_ffb;
	    }
	    debug(F101,"zinfill rab_ifile.rab$l_bkt 2","",rab_ifile.rab$l_bkt);
	    if (rab_ifile.rab$l_bkt != 0) { /* If just a file position... */
		return(0);		    /*...then done                */
	    }
	    zincnt--;			/* one less char in buffer */
	    return((int)(*zinptr++) & 0377); /* because we return the first */
	}

/* if we reached this point, we want to do record IO; first initialize RAB */

	if (fab_ifile.fab$b_rat & FAB$M_FTN) {
#ifdef DYNAMIC
	    rab_ifile.rab$l_ubf = (char *)zinbuffer+2;
#else
/* NOTE: Might need "(char *)&zinbuffer+2" here */
	    rab_ifile.rab$l_ubf = &zinbuffer+2;
#endif /* DYNAMIC */
	    rab_ifile.rab$w_usz = INBUFSIZE-4; /* Space for carriage control */
	} else {
#ifdef DYNAMIC
	    rab_ifile.rab$l_ubf = (char *)zinbuffer;
#else
/* NOTE: Might need "(char *)&zinbuffer" here */
	    rab_ifile.rab$l_ubf = &zinbuffer;
#endif /* DYNAMIC */
	    rab_ifile.rab$w_usz = INBUFSIZE-2; /* Space for possible CR/LF */
	}
	if (fab_ifile.fab$b_rat & FAB$M_PRN) { /* A fixed control area */
	    rab_ifile.rab$l_rhb = (char *)&vfcbuf;
	    rab_ifile.rab$w_usz = INBUFSIZE-255; /* Max fixed size*/
        }
	rab_ifile.rab$l_rop = 0;	/* Now do the record I/O */
	rms_sts = sys$get(&rab_ifile);
	if (!(rms_sts & 1)) {
	    debug(F101,"zinfill sys$get failed, status","",rms_sts);
	    vms_lasterr = rms_sts;
	}
	if (rms_sts == RMS$_EOF)
	  return(-1);			/* End of file */
	if (rms_sts != RMS$_NORMAL)
	  return(-1);			/* Fatal */
/*
 * Do assorted contortions with Fortran carriage control to make it formatted
 * ASCII instead, since many systems don't know about Fortran format in files.
 */
	if (fab_ifile.fab$b_rat & FAB$M_FTN) {
            linelen = rab_ifile.rab$w_rsz;
            if (linelen > 0) {
	        linelen --;		/* sans control code */
	        cchar = zinbuffer[2];	/* control code */
            } else {			/* zero length record */
                cchar = ' ';		/* space to give <LF><CR> */
            }
	    switch (cchar) {
	      case '\0':		/* data */
		zinptr = zinbuffer+3;
		zincnt = linelen;
		break;
	      case '+':				/* data<CR> */
		zinbuffer[linelen+3] = '\r';	/* insert return */
		zinptr = zinbuffer+3;
		zincnt = linelen+1;		/* count it */
		break;
	      case '$':				/* <LF>data */
		zinbuffer[2] = '\n';		/*  insert newline */
		zinptr = zinbuffer+2;
		zincnt = linelen+1;		/* count it */
		break;
	      case ' ':				/* <LF>data<CR> */
		zinbuffer[2] = '\n';		/*  insert newline */
		zinbuffer[linelen+3] = '\r';	/*  insert return */
		zinptr = zinbuffer+2;
		zincnt = linelen+2;		/*  count 'em */
		break;
	      case '0':				/* <LF><CR><LF>data<CR> */
		zinbuffer[0] = '\n';		/*  insert 1st newline */
		zinbuffer[1] = '\r';		/*  insert 1st return */
		zinbuffer[2] = '\n';		/*  insert 2nd newline */
		zinbuffer[linelen+3] = '\r';	/*  insert 2nd return */
		zinptr = zinbuffer;
		zincnt = linelen+4;		/*  count 'em */
		break;
	      case '1':				/* <FF>data<CR> */
		zinbuffer[2] = '\f';		/*  insert formfeed */
		zinbuffer[linelen+3] = '\r';	/*  insert return */
		zinptr = zinbuffer+2;
		zincnt = linelen+2;		/*  count 'em */
		break;
	      default:				/* <LF>data<CR> */
		zinbuffer[2] = '\n';		/*  insert newline */
		zinbuffer[linelen+3] = '\r';	/*  insert return */
		zinptr = zinbuffer+2;
		zincnt = linelen+2;		/*  count 'em */
		break;
	    }
	} else {
	    zincnt = rab_ifile.rab$w_rsz;
	    zinptr = zinbuffer;			/* reset pointer */
	}
/*
 * Here we see if we need to insert CR/LF pairs at the record boundary. For
 * the moment, we will add them if the file has "carriage return carriage
 * control" when looked at by a DIRECTORY command. As of edit 036 we also do
 * this for "print file carriage control" files. I'm open to comments de-
 * scribing cases where this doesn't work...
 */
	if (fab_ifile.fab$b_rat & FAB$M_CR) {
	    zinbuffer[zincnt] = '\r';
	    zinbuffer[zincnt + 1] = '\n';
	    zincnt += 2;
	}

/* that worked for the default PRN format of batch log files etc., but
 * ignored any user format specification (v. RMS FAB$B_RAT docs).
 */
	if (fab_ifile.fab$b_rat & FAB$M_PRN) {

            int i, ifcv, ipcnt, iscnt;
            unsigned char ipchr, ischr;	/* Eight bit ASCII control code */

	    if (vfcbuf[0] == 0x01 && vfcbuf[1] == 0x8D) {
	        zinbuffer[zincnt] = '\r'; /* Do batch log files the old way */
	        zinbuffer[zincnt + 1] = '\n';
	        zincnt += 2;
	    } else {			/* Decode the control area */
		ipcnt = 0;		/* First, the prefix byte */
		ipchr = '\0';
                if (vfcbuf[0] != '\0') {
                    ifcv =  (vfcbuf[0] & 0xe0) >> 5; /* The control bits */
                    switch (ifcv) {
		      case 0: ipcnt = (int) vfcbuf[0];
			break;
		      case 4: ipchr = vfcbuf[0] & 0x1f;
			break;
		      case 6: ipchr = (vfcbuf[0] & 0x1f) + 128;
			break;
		      case 7:
			debug(F101,
			      "zinfill illegal PRN code, byte 0 :",
			      "",
			      vfcbuf[0]
			      );
			ipcnt = 1;	/* Ignore code, apply CRLF */
			break;
		      default:
			debug(F101,
			      "zinfill unknown PRN code, byte 0 :",
			      "",
			      vfcbuf[0]
			      );
			ipcnt = 1;
			break;
		    }
		}
		iscnt = 0;		/* Now, the suffix byte */
		ischr = '\0';
                if (vfcbuf[1] != '\0') {
                    ifcv =  (vfcbuf[1] & 0xe0) >> 5; /* The control bits */
                    switch (ifcv) {
		      case 0: iscnt = (int) vfcbuf[1];
			break;
		      case 4: ischr = vfcbuf[1] & 0x1f;
			break;
		      case 6: ischr = (vfcbuf[1] & 0x1f) + 128;
			break;
		      case 7:
			debug(F101,
			      "zinfill illegal PRN code, byte 1 :",
			      "",
			      vfcbuf[1]
			      );
			iscnt = 1;	/* Ignore code,  apply CRLF */
			break;
		      default :
			debug(F101,
			      "zinfill unknown PRN code, byte 1 :",
			      "",
			      vfcbuf[1]
			      );
			iscnt = 1;
			break;
		    }
		}
		if (ipcnt != 0 || ipchr != '\0') { /* Insert */
		    if (ipcnt > 0) {		   /* CR & n LF's */
			memmove(zinbuffer+ipcnt+1, zinbuffer, zincnt);
			zincnt += ipcnt + 1;
			zinbuffer[0] = '\r';
			for (i = 1; i <= ipcnt; i++)
			  zinbuffer[i] = '\n';
		    } else {		/* Or an ASCII control code */
			memmove(zinbuffer+1, zinbuffer, zincnt);
			zincnt ++;
			zinbuffer[0] = ipchr;
		    }
		}
		if (iscnt != 0 || ischr != '\0') { /* Append */
		    if (iscnt > 0) {
			zinbuffer[zincnt++]='\r';
			for (i = 1; i <= iscnt; i++)
			  zinbuffer[zincnt++]='\n';
		    } else {
			zinbuffer[zincnt++] = ischr;
		    }
		}
	    }
        }
    }
/*
 * And finally return the record
 */
    zincnt--;				/* one less char in buffer */
    return((int)(*zinptr++) & 0377);	/* because we return the first */
}

static int
rinfill() {
    char cchar;
    int linelen;

    if (ispipe[ZRFILE]) {		/* OPEN !READ file */
	int x;
	x = get_subprc_line();
	debug(F101,"rinfill get_subprc_line","",x);
	if (x < 0) return(x);
	rinptr = (CHAR *)sub_buf;
	rincnt = sub_count - 1;
	debug(F101,"rinfill rincount","",rincnt);
	return((int)(*rinptr++) & 0377);
    }
    /* Regular file... */

    debug(F101,"rinfill rfile_bmode","",rfile_bmode);
    if (rfile_bmode != 0) {
	rab_rfile.rab$l_rop = RAB$M_BIO; /* Block mode I/O */
#ifdef DYNAMIC
	rab_rfile.rab$l_ubf = (char *)rinbuffer;
#else
/* NOTE: Might need "(char *)&rinbuffer" here */
	rab_rfile.rab$l_ubf = &rinbuffer;
#endif /* DYNAMIC */
	rab_rfile.rab$w_usz = 512;
	rms_sts = sys$read(&rab_rfile);
	if (!(rms_sts & 1)) {
	    debug(F101,"rinfill sys$read failed, status","",rms_sts);
	    vms_lasterr = rms_sts;
	}
	if (rms_sts == RMS$_EOF)
	  return(-1);			/* End of file */
	if (rms_sts != RMS$_NORMAL)
	  return(-1);			/* Fatal */
	rfile_bcount++;			/* Say another block read */
	debug(F101,"rinfill sys$read ok, rfile_bcount","",rfile_bcount);
	rincnt = 512;
	rinptr = rinbuffer;

	if (rab_rfile.rab$l_bkt != 0) { /* If just a file position... */
	    rfile_bcount = rab_rfile.rab$l_bkt; /* update block counter */
	}
	if (rfile_bcount == xabfhc_rfile.xab$l_ebk) {
	    if (rfile_bmode == 1)	/* BINARY but not LABELED */
	      rincnt = xabfhc_rfile.xab$w_ffb;
	}
	if (rab_rfile.rab$l_bkt != 0) { /* If just a file position... */
	    return(0);			/*...then done                */
	}
	rincnt--;			/* one less char in buffer */
	return((int)(*rinptr++) & 0377); /* because we return the first */
    }
    if (fab_rfile.fab$b_rat & FAB$M_FTN) {
	debug(F100,"rinfill FAB$M_FTN","",0);
#ifdef DYNAMIC
	rab_rfile.rab$l_ubf = (char *)rinbuffer+2;
#else
/* NOTE: Might need "(char *)&rinbuffer" here */
	rab_rfile.rab$l_ubf = &rinbuffer+2;
#endif /* DYNAMIC */
	rab_rfile.rab$w_usz = INBUFSIZE-4; /* space for carriage ctl */
    } else {
	debug(F100,"rinfill not FAB$M_FTN","",0);
#ifdef DYNAMIC
	rab_rfile.rab$l_ubf = (char *)rinbuffer;
#else
/* NOTE: Might need "(char *)&rinbuffer" here */
	rab_rfile.rab$l_ubf = &rinbuffer;
#endif /* DYNAMIC */
	rab_rfile.rab$w_usz = INBUFSIZE-2; /* space for possible CR/LF */
    }
    rab_rfile.rab$l_rop = 0;		/* doing record I/O */
    rms_sts = sys$get(&rab_rfile);
    if (!(rms_sts & 1)) {
	debug(F101,"rinfill sys$get failed, status","",rms_sts);
	vms_lasterr = rms_sts;
    }
    if (rms_sts == RMS$_EOF)
      return(-1);			/* End of file */
    if (rms_sts != RMS$_NORMAL)
      return(-1);			/* Fatal */
/*
 * Do assorted contortions with Fortran carriage control to make it formatted
 * ASCII instead, since many systems don't know about Fortran format in files.
 */
    if (fab_rfile.fab$b_rat & FAB$M_FTN) {
	linelen = rab_rfile.rab$w_rsz-1; /* sans control code */
	cchar = rinbuffer[2];		/* control code */
	switch (cchar) {
	  case '\0':			/* data<CR> */
	  case '+':
	    rinbuffer[linelen+3] = '\r';/*  insert return */
	    rinptr = rinbuffer+3;
	    rincnt = linelen+1;		/* count it */
	    break;
	  case '$':			/* <LF>data<CR> */
	  case ' ':
	    rinbuffer[2] = '\n';	/*  insert newline */
	    rinbuffer[linelen+3] = '\r';/*  insert return */
	    rinptr = rinbuffer+2;
	    rincnt = linelen+2;		/*  count 'em */
	    break;
	  case '0':			/* <LF><CR><LF>data<CR> */
	    rinbuffer[0] = '\n';	/*  insert 1st newline */
	    rinbuffer[1] = '\r';	/*  insert 1st return */
	    rinbuffer[2] = '\n';	/*  insert 2nd newline */
	    rinbuffer[linelen+3] = '\r';/*  insert 2nd return */
	    rinptr = rinbuffer;
	    rincnt = linelen+4;		/*  count 'em */
	    break;
	  case '1':			/* <FF>data<CR> */
	    rinbuffer[2] = '\f';	/*  insert formfeed */
	    rinbuffer[linelen+3] = '\r';/*  insert return */
	    rinptr = rinbuffer+2;
	    rincnt = linelen+2;		/*  count 'em */
	    break;
	  default:			/* <LF>data<CR> */
	    rinbuffer[2] = '\n';	/*  insert newline */
	    rinbuffer[linelen+3] = '\r';/*  insert return */
	    rinptr = rinbuffer+2;
	    rincnt = linelen+2;		/*  count 'em */
	    break;
	}
    } else {
	rincnt = rab_rfile.rab$w_rsz;
	rinptr = rinbuffer;		/* reset pointer */
    }
/*
 * Here we see if we need to insert CR/LF pairs at the record boundary. For
 * the moment, we will add them if the file has "carriage return carriage
 * control" when looked at by a DIRECTORY command. As of edit 036 we also do
 * this for "print file carriage control" files. I'm open to comments de-
 * scribing cases where this doesn't work...
 */
    if (fab_rfile.fab$b_rat & (FAB$M_CR | FAB$M_PRN)) {
	rinbuffer[rincnt] = '\r';
	rinbuffer[rincnt + 1] = '\n';
	rincnt += 2;
    }
    rincnt--;			/* one less char in buffer */
    return((int)(*rinptr++) & 0377); /* because we return the first */
}

/*  Z F S E E K -- Seek to a given position with an input file */
/*                 Assumes block-mode I/O being used           */

int
zfseek(CK_OFF_T pos) {
    CK_OFF_T offset;
    unsigned long errtmp;
    int x;

    debug(F101,"zfseek pos","",pos);
    rab_ifile.rab$l_bkt = (unsigned long) pos >> 9; /* Get block number */
    debug(F101,"zfseek rab_ifile.rab$l_bkt","",rab_ifile.rab$l_bkt);
    rab_ifile.rab$l_bkt++;		/* VBN's are 1-based */
    offset = (unsigned long) pos & 511;	/* Get offset with block */
    debug(F101,"zfseek offset","",offset);
    errtmp = vms_lasterr;		/* Make sure we get the real error */
    vms_lasterr = 0;			/* ... */
    x = zinfill();			/* Read in the block */
    debug(F101,"zfseek zinfill","",x);
    if (x != 0) {			/* Handle any errors */
	rab_ifile.rab$l_bkt = 0;	/* Sequentially from now on */
	debug(F101,"zfseek failed","",vms_lasterr);
	return(-1);
    }
    vms_lasterr = errtmp;
    rab_ifile.rab$l_bkt = 0;		/* Sequentially from now on */
    if (offset != 0) {			/* if not block boundary... */
	zincnt = zincnt - offset;	/* ...adjust count and pointer */
	zinptr = zinptr + offset;
    }
    return(0);
}

/*  Z S O U T  --  Write a string to the given file, buffered.  */

int
zsout(n,s) int n; char *s; {
#ifdef DEBUG
    if (chkfn(n) < 1) return(-1);
#endif
    fputs(s, fp[n]);			/* Don't use fprintf here MM */
    return(0);
}


/*  Z S O U T L  --  Write string to file, with line terminator, buffered.  */

int
zsoutl(n,s) int n; char *s; {
#ifdef DEBUG
    if (chkfn(n) < 1) return(-1);
#endif /* DEBUG */
    if (fputs(s, fp[n]) == EOF)
      return(-1);
    putc('\n', fp[n]);
    return(0);
}


/*  Z S O U T X  --  Write x characters to file, unbuffered.  */

int
zsoutx(n,s,x) int n, x; char *s; {
#ifdef DEBUG
    if (chkfn(n) < 1) return(-1);
#endif
    return(write(fileno(fp[n]),s,x));
}


/*  Z C H O U T  --  Add a character to the given file.  */

int
#ifdef CK_ANSIC
zchout(register int n, char c)
#else
zchout(n,c) register int n; char c;
#endif /* CK_ANSIC */
/* zchout */ {
#ifdef DEBUG
    if (chkfn(n) < 1) return(-1);
#endif
    if (n == ZSFILE) {
    	return(write(fileno(fp[n]),&c,1)); /* Use unbuffered for session log */
    } else {
	if (putc(c,fp[n]) == EOF)	/* If true, maybe there was an error */
	  return(ferror(fp[n]) ? -1 : 0); /* Check to make sure */
	else				/* Otherwise... */
	  return(0);			/* There was no error. */
    }
}

/*  Z O U T D U M P  --  dump buffered output characters to file.  */

/* Buffered file output, buffer dump */

/*
  No, this isn't an entry in the 199x Obfuscated C programming contest, nor
  did we get it at an all-night convenience store.  VMS requires that stream
  format files be written as records, so we have to do _lots_ of contortions
  to make sure we write whole lines as records.  Not pretty.  - TMK
*/
int
zoutdump() {
    int ocnt;
    int wrote_one_line = 0;
    CHAR *optr, *srcptr, *endptr;
    char csave;

    debug(F101,"zoutdump zoutcnt","",zoutcnt);
    debug(F101,"zoutdump ofile_bmode","",ofile_bmode);
/*
  Well, this could be to the console. If it is, chop it into itty-bitty parts
  (the VMS CRTL can't handle a %s spec bigger than 512 bytes) and print it.
*/
    if (cflag == 1) {			/* If we're dumping to console */
	endptr = zoutbuffer + zoutcnt;
	for (optr = zoutbuffer; optr < endptr; optr += 511) {
	    if (optr+511 < endptr) {	/* More than 511, break up */
		csave = *(optr+511);
		*(optr+511) = '\0';
		printf("%s", optr);
		*(optr+511) = csave;
	    } else {
		*endptr = '\0';		/* Make sure null-terminated */
		printf("%s", optr);
	    }
	}
	zoutcnt = 0;
	zoutptr = zoutbuffer;
	return(0);
    }

/* Do we need to processed TYPE LABELED contortions? */

    if (ofile_bmode == XYFT_L) {	/* Is it labeled? */
	int x;
	if (ofile_lblproc == 0)	{	/* I've never gone this way before? */
	    x = do_label_recv();	/* Beyond revolving rainbow door... */
	    if (x == -1)
	      return(-1);		/* Got a hard error in label proc. */
	    if (x == 1 && ofile_dump != 1)
	      return(0);		/* Exit so we can fill up the buffer */
	}
    }
/*
 * Well, we could be lucky...
 */
    if (zoutcnt == 0)
	return(0);
/*
 * Oh well. See if doing binary - that's easy...
 */
    if (ofile_bmode) {
	if (zoutcnt == OBUFSIZE) {
#ifdef DYNAMIC
	    rab_ofile.rab$l_rbf = (char *)zoutbuffer;
#else
/* NOTE: Might need "(char *)&zoutbuffer" here */
	    rab_ofile.rab$l_rbf = &zoutbuffer;
#endif /* DYNAMIC */
	    rab_ofile.rab$w_rsz = OBUFSIZE;
	    if (ofile_ffb != -1 && ofile_dump == 1) {
		/*
		 * Only do this when doing _last_ file segment.
		 */
		xabfhc_ofile.xab$w_ffb = ofile_ffb;
		if (ofile_ffb)
		    rab_ofile.rab$w_rsz -= (512 - ofile_ffb);
		debug(F101,"zoutdump ofile_ffb","",(int)ofile_ffb);
		debug(F101,"zoutdump rab$w_rsz","",rab_ofile.rab$w_rsz);
	    }
	    rms_sts = sys$write(&rab_ofile);
	    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
	    if (rms_sts != RMS$_NORMAL) {
		debug(F101,"zoutdump $write failed, status","",rms_sts);
		return(-1);
	    }
	} else {
#ifdef DYNAMIC
	    rab_ofile.rab$l_rbf = (char *)zoutbuffer;
#else
/* NOTE: Might need "(char *)&zoutbuffer" here */
	    rab_ofile.rab$l_rbf = &zoutbuffer;
#endif
	    rab_ofile.rab$w_rsz = zoutcnt;
	    xabfhc_ofile.xab$w_ffb = (zoutcnt & 511)+1;
	    if (ofile_ffb != -1) {
		xabfhc_ofile.xab$w_ffb = ofile_ffb;
		if (ofile_ffb)
		    rab_ofile.rab$w_rsz -= (512 - ofile_ffb);
		debug(F101,"zoutdump ofile_ffb","",(int)ofile_ffb);
		debug(F101,"zoutdump rab$w_rsz","",rab_ofile.rab$w_rsz);
	    }
	    rms_sts = sys$write(&rab_ofile);
	    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
	    if (rms_sts != RMS$_NORMAL) {
		debug(F101,"zoutdump $write failed, status","",rms_sts);
		return(-1);
	    }
	}
	debug(F100,"zoutdump RMS operations completed ok","",0);
	zoutcnt = 0;
	zoutptr = zoutbuffer;
	return(0);
    }

/*
 * Must be ASCII. This is harder, and weirder... It's actually easier than
 * it looks, but there's (unfortunately) no really easy way to _implement_
 * it. (sigh, whimper, groan)
 */

    srcptr = zoutbuffer;            /* Points to first line in buffer */
    endptr = zoutbuffer + zoutcnt;  /* Points to location after last char */
zoutdump_ascii:
    /* Scan through buffer until we find a CR or we run out of chars */
    for (optr = srcptr; optr < endptr; optr++) if (*optr == CR) break;

    /* If there are at least 2 chars left in the buffer when we stop   */
    /* scanning, then it is assumed the above loop terminated because  */
    /* it found the CR and that both the CR and LF are present in the  */
    /* buffer (situation normal.                                       */
    /* If there are not 2 chars left in the buffer, we have one of two */
    /* cases which we treat identically:                               */
    /*  1) If there are 0 chars left in the buffer, then the line's    */
    /*     terminating CR LF are yet to come. So... we copy the data   */
    /*     to the front of the buffer and exit (next time it should be */
    /*     there.)                                                     */
    /*  2) If there is one char left in the buffer, we have the case of*/
    /*     a line with the CR but no LF present. So... do the same     */
    /*     because the LF will be coming next time.                    */
    if (optr+2 > endptr) {			/* drat! ran off the end */
	if (ofile_dump && (srcptr == endptr)) {
        /* If the beginning and end ptrs are the same, then there the  */
        /* is empty. Good news, 'cause we're clsoing up.               */
	    zoutcnt = 0;        /* No looping, please. */
	    zoutptr = zoutbuffer;
	}
	else if (ofile_dump) {		/* but it's cool, we're closing up */
        /* Oops, we've got a line with no LF and maybe no CR. Well     */
        /* write it out and exit abnormally.                           */
	    rab_ofile.rab$l_rbf = (char *)srcptr;
	    rab_ofile.rab$w_rsz = optr-srcptr;
	    rms_sts = sys$put(&rab_ofile);
	    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
	    zoutcnt = 0;
	    zoutptr = zoutbuffer;
	    if (rms_sts != RMS$_NORMAL) {
		debug(F101, "zoutdump $put failed, status","",rms_sts);
		return(-1);
	    }
	} else if (wrote_one_line) {	/* it's still cool, we did one... */
	    zoutcnt = optr - srcptr;	/* number of chars left */
	    if (optr < endptr) zoutcnt++; /*[jah083] including CR if present */
		if (zoutcnt) memmove(zoutbuffer, srcptr, zoutcnt);
	        /* Move'em to front of buffer*/
	    zoutptr = zoutbuffer+zoutcnt;
	} else {			/* WRONG!!! */
        /* We've got a buffer full of chars with no LF (it may or may  */
        /* not have a terminating CR. In either case its just plain too*/
        /* long. I suppose we could check here for the optr+1 == endptr*/
        /* which indicates that there was a CR but no LF so we could   */
        /* issue a "line barely too long", but, is it useful?          */
	    debug(F100, "zoutdump: line too long","",0);
	    zoutcnt = 0;		/* No looping, please. */
	    zoutptr = zoutbuffer;
	    return(-1);
	}
	debug(F101, "zoutdump exiting, zoutcnt","",zoutcnt);
	return(0);
    }

    /* We now have a line that we can write, so... */

    rab_ofile.rab$l_rbf = (char *)srcptr;
    rab_ofile.rab$w_rsz = optr-srcptr;
    rms_sts = sys$put(&rab_ofile);
    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
    if (rms_sts != RMS$_NORMAL) {
	debug(F101, "zoutdump $put failed, status","",rms_sts);
	return(-1);
    }
	srcptr = optr + 2;		/* Account for CR, LF */
    wrote_one_line = 1;
    goto zoutdump_ascii;
}

/*  C H K F N  --  Internal function to verify file number is ok.  */

/*
 Returns:
  -1: File number n is out of range
   0: n is in range, but file is not open
   1: n in range and file is open
*/
int
chkfn(n) int n; {
    int x;
    if (n != ZDFILE)
      debug(F101,"chkfn","",n);
    switch (n) {
      case ZCTERM:
      case ZSTDIO:
      case ZIFILE:
      case ZOFILE:
      case ZDFILE:
      case ZTFILE:
      case ZPFILE:
      case ZSFILE:
	break;
      case ZSYSFN:			/* System functions */
	return(0);
      case ZRFILE:			/* READ and WRITE files */
      case ZWFILE:
      case ZMFILE:
      case ZDIFIL:
	break;
      default:
	if (n >= ZNFILS) {
	    debug(F101,"chkfn: file number out of range","",n);
	    fprintf(stderr,"?File number out of range - %d\n",n);
	    return(-1);
	}
    }
    x = (fp[n] == NULL) ? 0 : 1;
    if (n != ZDFILE)
      debug(F101,"chkfn return","",x);
    return(x);
}

static int zgetfs_active = 0;
static CK_OFF_T zgfs_size = -1L;
int zgfs_dir = -1;
int zgfs_link = 0;
char linkname[2] = { '\0', '\0' };	/* No symlinks in VMS */

CK_OFF_T
zgetfs(name) char *name; {
    CK_OFF_T x;
    if (!name)
      name = "";
    if (!*name)
      return(-1);
    zgfs_size = -1L;
    zgfs_dir = -1;
    zgetfs_active = 1;
    x = zchki(name);
    zgetfs_active = 0;
    if (zgfs_dir < 0)
      zgfs_dir = isdir(name);
    if (x < 0 && zgfs_size >= 0)
      return(zgfs_size);
    return((x < -2) ? (CK_OFF_T)-1 : x);
}

/*  Z C H K I  --  Check if input file exists and is readable.  */

/*
  Returns:
   >= 0 if the file can be read (returns the size).
     -1 if file doesn't exist or can't be accessed,
     -2 if file exists but is not readable (e.g. a directory file).
     -3 if file exists but protected against read access.
*/
CK_OFF_T
zchki(name) char *name; {
    struct stat statbuf;
    extern int zchkid;
    int x;
    struct FAB fab_chki;
    struct NAMX nam_chki;
    struct XABFHC xabfhc_chki;
    CK_OFF_T iflen = (CK_OFF_T)-1;

    if (!zchkid && !zgetfs_active && isdir(name))
      return(-2);

    if (!name) name = "";
    if (!*name) return(-1);

    if (zgetfs_active) {
        if (stat(name,&statbuf) == 0) {
	    zgfs_link = 0;
#ifdef S_ISDIR
	    zgfs_dir = (S_ISDIR(statbuf.st_mode)) ? 1 : 0;
	    debug(F110,"zgetfs S_ISDIR",name,zgfs_dir);
#else
	    zgfs_dir = isdir(name);
	    debug(F110,"zgetfs isdir",name,zgfs_dir);
#endif /* S_ISDIR */
	    zgfs_size = statbuf.st_size;
	    debug(F110,"zgetfs size",name,zgfs_size);
	    return(zgfs_size);
	}
	/* If stat() failed try RMS... */
    }
    fab_chki = cc$rms_fab;                      /* Initialize FAB. */
    nam_chki = CC_RMS_NAMX;                     /* Initialize NAM[L]. */
    fab_chki.FAB_L_NAMX = &nam_chki;            /* Point FAB to NAM[L]. */

    /* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
    fab_chki.fab$l_dna = (char *) -1;  /* Using NAML for default name. */
    fab_chki.fab$l_fna = (char *) -1;  /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
    FAB_OR_NAML( fab_chki, nam_chki).FAB_OR_NAML_FNA = name;
    FAB_OR_NAML( fab_chki, nam_chki).FAB_OR_NAML_FNS = strlen( name);

    fab_chki.fab$b_fac = FAB$M_BIO;
    fab_chki.fab$l_xab = (char *)&xabfhc_chki;
    xabfhc_chki = cc$rms_xabfhc;
    rms_sts = sys$open(&fab_chki);
    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
    if (rms_sts == RMS$_PRV)			/* No privs */
      return(-3);
    if (rms_sts != RMS$_NORMAL) {
	debug(F101,"zchki $open failed, status","",rms_sts);
	return(-1);
    }
    iflen = ((xabfhc_chki.xab$l_ebk-1)*512)+xabfhc_chki.xab$w_ffb;

    rms_sts = sys$close(&fab_chki);
    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
    if (rms_sts != RMS$_NORMAL) {
	debug(F101,"zchki $close failed, status","",rms_sts);
	return(-1);
    }
    ckstrncpy(nambuf,name,CKMAXPATH);	/* Keep local copy of name */
    debug(F111,"zchki access ok:",name,(int) iflen);
    return( (iflen > -1) ? iflen : 0 );
}

/*  Z C H K O  --  Check if output file can be created.  */

/*
 Returns -1 if write permission for the file would be denied, 0 otherwise.
*/
#include <devdef.h>

int
zchko(name) char *name; {
    extern int zchkod;                  /* Used by IF WRITEABLE */

    struct FAB fab;			/* let RMS do the work */
    struct NAMX nam;
    char expanded_str[NAMX_C_MAXRSS+ 1];

    if (!name) return(-1);              /* Watch out for null pointer. */
    debug(F111,"zchko",name,zchkod);

    fab = cc$rms_fab;                   /* Initialize FAB. */
    nam = CC_RMS_NAMX;                  /* Initialize NAM[L]. */
    fab.FAB_L_NAMX = &nam;              /* Point FAB to NAM[L]. */

    /* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
    fab.fab$l_dna = (char *) -1;  /* Using NAML for default name. */
    fab.fab$l_fna = (char *) -1;  /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
    FAB_OR_NAML( fab, nam).FAB_OR_NAML_FNA = name;
    FAB_OR_NAML( fab, nam).FAB_OR_NAML_FNS = strlen( name);

    fab.fab$l_alq = 1;	       	    /* zero length file may not be writeable */
    fab.fab$b_fac = FAB$M_BIO | FAB$M_PUT;	 /* block i/o setup faster ? */
    fab.fab$l_fop = FAB$M_DLT;	   /* TMD does not check directory file perm */

    nam.NAMX_L_ESA = expanded_str;
    nam.NAMX_B_ESS = sizeof( expanded_str)- 1;

    rms_sts = sys$parse(&fab); 			/* check the file spec */
    if (!(rms_sts & 1)) {
        if (rms_sts == RMS$_SYN) {		/* nzrtol may fix */
            debug(F110,"zchko syntax error",name,0);
            return(0);
        } else {
            CHECK_ERR("zchko ", rms_sts);
            debug(F111,"zchko parse error",name,rms_sts);
            return(-1);
        }
    }						/* file oriented device */
    if ((fab.fab$l_dev & DEV$M_NET) || (fab.fab$l_dev & DEV$M_FOD)) {
        *(nam.NAMX_L_VER+ nam.NAMX_B_VER) = '\0';
        debug(F110,"zchko create",expanded_str,0);
        rms_sts = sys$create(&fab);		/* test write capability */
        if (rms_sts & 1) {
            rms_sts = sys$close(&fab);
            return(0);
        } else {
            CHECK_ERR("zchko ", rms_sts);
            debug(F101,"zchko create error","",rms_sts);
            return(-1);
        }
     } else {
        *(nam.NAMX_L_DIR) = '\0';
        if (fab.fab$l_dev & DEV$M_REC) { 	/* record-oriented device */
            debug(F110,"zchko non-fod",expanded_str,0);
            return(0);				/* omit access test */
        } else {
            debug(F111,"zchko invalid device",expanded_str,fab.fab$l_dev);
            return(-1);
        }
    }
}


/*  Z C H K S P A  --  Check if there is enough space to store the file.  */

/*
 Call with file specification f, size n in bytes.
 Returns -1 on error, 0 if not enough space, 1 if enough space.
*/

int
zchkspa(f,n) char *f; CK_OFF_T n; {

/*
 * This is complicated. The user could have specified an explicit path when
 * sending the file, or could have done a CWD, or could be using the default
 * directory. If not the latter, the path may not even be a disk device, as
 * CWD LPA0: is perfect legal for uploading to the lineprinter. After that,
 * if it's a disk, we should check the user's quota.  However, the user may
 * have SYSPRV, EXQUOTA, BYPASS, or maybe even GRPPRV, and it would be hard
 * to  properly check for all these cases.  So, if the file will fit on the
 * disk, we'll accept it.
 */

    char   *zgtdir();

    struct itmlstdef {
	short int buflen;
	short int itmcod;
	char *bufaddr;
	long int *retlen;
    };

    static char device[64];

    struct dsc$descriptor_s
	dev_desc = {sizeof(device), DSC$K_DTYPE_T, DSC$K_CLASS_S,
		      (char *)&device};
    unsigned long freeblocks, devclass, fileblocks;
    long freelength, classlength;

    struct itmlstdef itmlst[] =
	{4,DVI$_FREEBLOCKS,0,0,4,DVI$_DEVCLASS,0,0,0,0,0,0};

    int rms_sts;

/* First, figure out the device we're interested in */

    ckstrncpy(device,zgtdir(),64);	/* Handles default or CWD */

    if (strchr(f, ':'))			/* If user specified path */
      ckstrncpy(device, f, 64);

    debug(F110,"zchkspa target device is ",device,0);

/* Next, ask for free block count and device type (disk vs. non-disk) */

    itmlst[0].bufaddr = (char *)&freeblocks;
    itmlst[0].retlen = &freelength;
    itmlst[1].bufaddr = (char *)&devclass;
    itmlst[1].retlen = &classlength;

    rms_sts = sys$getdviw(0,0,&dev_desc,&itmlst,0,0,0,0);
    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
    debug(F101,"zchkspa $getdvi returned rms_sts","",rms_sts);

    if (devclass != DC$_DISK)
      return(1);				/* assume space if not disk */

    if (rms_sts != SS$_NORMAL)
      return(1);				/* assume free space if err */

    debug(F101,"zchkspa $getdvi returned freeblocks","",freeblocks);

/* Pad file size if it's a text file */

    if (ofile_bmode == XYFT_T)
      n += (n/40) * 3;

    fileblocks = n / 512 + 1;			/* compute file size in blks */
						/* we may want some fuzz */
    if (fileblocks >= freeblocks)
      return(0);				/* Won't fit */
    else
      return(1);				/* Will fit */
}

/*  Z D E L E T  --  Delete the named file.  */

int
zdelet(name) char *name; {
    return(delete(name));
}

/*  Z R T O L  --  Convert remote filename into local form.  */

VOID
zrtol(name,name2) char *name, *name2; {
    nzrtol(name,name2,1,0,CKMAXPATH);
}
/*  N Z R T O L  --  New zrtol handles native and standard-format pathnames */

VOID
nzrtol(name,name2,fncnv,fnrpath,max)
    char *name, *name2; int fncnv, fnrpath, max;
{ /* nzrtol */
    int start, sfncnv, colon = 0;
    char *np, *vp = NULL, c;
    char *ls = NULL;
    static char *spcl_set = "_-$[]<>:.\";";
    char tmpbuf[ NAMX_C_MAXRSS+ 2], *tmp;
    char * dotp = NULL;

    debug(F110,"nzrtol name",name,0);
    if (!name2) return;
    sfncnv = fncnv;

    tmpbuf[0] = NUL;
    tmpbuf[1] = NUL;
    tmpbuf[2] = NUL;
    start = 0;
    tmp = tmpbuf+start;
    {
	/* Autodetection of path format */
	int lb = 0, rb = 0, sl = 0, bl = 0;
	char *p = name;
	while (*p) {
	    if (*p == '[' || *p == '<' ) lb++;
	    if (*p == ']' || *p == '>' ) rb++;
	    if (*p == ':') colon++;
	    if (*p == '/') sl++;
	    if (*p == '\\') bl++;
	    p++;
	}
	if (lb == 1 && rb == 1 && !sl) { /* VMS detected */
	    fncnv = 0;
	    debug(F100,"nzrtol VMS brackets detected","",0);
	} else if (colon == 1 && !sl) { /* VMS detected */
	    fncnv = 0;
	    debug(F100,"nzrtol VMS colon detected","",0);
	} else if (!lb && !rb && sl > 0) { /* UNIX detected */
	    fncnv = 1;
	    debug(F101,"nzrtol UNIX slashes detected","",sl);
	}
    }
    debug(F101,"nzrtol fncnv","",fncnv);

    /* If converting pathnames, convert to VMS format */

    if (fncnv) {			/* Converting, so assume UNIX format */
	np = name;			/* Set name pointer */
	*tmp++ = '[';			/* Insert opening VMS bracket */
	if (*np == '/') {		/* It's an absolute pathname */
	    np++;			/* Skip past leading slash */
	} else {			/* Relative pathname */
	    *tmp++ = '.';		/* Insert '.' here */
	}
	while (*np) {			/* Convert to VMS format */
	    if (*np == '/') {		/* Have directory separator */
		*tmp = '.';		/* So use this notation in VMS */
		ls = tmp;		/* Remember position of last slash */
	    } else {
		*tmp = *np;
	    }
	    np++;
	    tmp++;
	}
	*tmp = NUL;
	if (ls) {			/* Replace last slash by */
	    *ls = ']';			/* closing directory bracket */
	} else {			/* No slashes */
	    start += 2;			/* So skip past opening "[." */
	}
    } else {				/* Assume VMS format already */
#ifdef COMMENT
/* No, this would ruin any as-name the sender gave */
/* "Be conservative in what you send, liberal in what you receive" */
	int flag = 0;			/* Copy */
	np = name;			/* But strip node and device */
	while (*np) {
	    if (*np == '[')
	      flag = 1;
	    if (flag)
	      *tmp++ = *np;
	    np++;
	}
	if (!flag)
#endif /* COMMENT */
	  ckstrncpy(tmp, name, NAMX_C_MAXRSS); /* Just copy */
    }
    tmpbuf[ NAMX_C_MAXRSS] = NUL;	/* Make sure buffer is terminated */
    tmp = tmpbuf + start;		/* Reset pointer */
    name = tmp;				/* Treat new string as original arg */
    debug(F110,"nzrtol tmp 1",tmp,0);

    /* Now we have VMS path format in tmpbuf */

    if (fnrpath == PATH_OFF) {		/* RECEIVE PATHNAMES OFF */
	zstrip(name,&np);
	ckstrncpy(tmpbuf, np, NAMX_C_MAXRSS);
	tmp = tmpbuf;
	debug(F110,"nzrtol PATH_OFF",tmp,0);
    } else if (fnrpath == PATH_ABS) {	/* RECEIVE PATHNAMES ABSOLUTE */
	/* Nothing to do */
	debug(F110,"nzrtol PATH_ABS",tmp,0);
    } else if (isabsolute(name)) {	/* RECEIVE PATHNAMES RELATIVE */
	int x;
	char * bb = NULL;
	x = strlen(name);
	debug(F111,"nzrtol converting absolute to relative tmp",tmp,x);
#ifdef COMMENT
	if (bb = malloc(x+x+1)) {	/* Be safe */
	    char * b = bb;
	    np = name;
	    while (*np) {
		*b++ = *np;
		if (*np == '[' && *(np+1) != '.')
		  *b++ = '.';
		np++;
	    }
	    *b = NUL;
	    ckstrncpy(tmpbuf, bb, NAMX_C_MAXRSS);
	    tmp = tmpbuf;
	    free(bb);
	}
#else  /* Not COMMENT - From Lucas Hart, Oct 1999 */
       /* strip any node or device and move in place */
        start = ckindex(":",name,-1,1,1);
        np = name + start;		/* ptr to name in tmpbuf */
        bb = tmpbuf;			/* destination */
	if (tmpbuf[0] == '[') {		/* [179] If it starts with a bracket */
	    *bb++ = *np++;
	    if (*np != '.')
	      *bb++ = '.';		/* make relative */
	}
        memmove(bb,np,strlen(np)+1);    /* safe: tmpbuf[NAMX_C_MAXRSS] = NUL */
        tmp = tmpbuf;
#endif  /* COMMENT */
	debug(F110,"nzrtol PATH_REL 1",tmp,0);
    } else {				/* Ditto */
	/* Nothing to do - it's already done */
	debug(F110,"nzrtol PATH_REL 2",tmp,0);
    }
    tmpbuf[ NAMX_C_MAXRSS] = NUL;
    debug(F110,"nzrtol tmp 2",tmp,0);

#ifdef COMMENT				/* Not needed for Edit 166 zmkdir */
    /* But wait, there's more... */
    /* Convert relative name to absolute or else zmkdir won't work */
    {
	char buf2[ NAMX_C_MAXRSS+ 2], *p, *b;
	int n = 0, flag = 0;
	np = tmp;			/* Source pointer */
	b = buf2;			/* Where to put new name */
	while (*np && n < NAMX_C_MAXRSS) { /* And substitute it; */
	    *b++ = *np;
	    n++;
	    if (*np == '[' && *(np+1) == '.') {
		p = zgtdir();		/* Get current directory */
		while (*p && *p != '[')	/* The part inside the brackets */
		  p++;
		if (*p = '[') {		/* Substitute it in */
		    p++;
		    while (*p && *p != ']') {
			*b++ = *p++;
			if (n++ >= NAMX_C_MAXRSS)
			  break;
		    }
		}
	    }
	    np++;
	}
	*b = NUL;
	tmp = buf2;
	debug(F110,"nzrtol tmp 3",tmp,0);
    }
#endif /* COMMENT */
    fncnv = sfncnv;			/* Restore original value of this */
    if (!fncnv) {			/* Now check it; if not converting */
	ckstrncpy(name2,tmp,max);	/* We're done. */
	debug(F110,"nzrtol name2",name2,0);
	return;
    }
    /* Now convert the characters themselves */

    name = tmp;
    for (np = name2; c = *name; name++) {
	if (islower(c))
	  c = toupper(c);
	else if (c == '~' || c == SP)
	  c = '_';
	else if (!isalnum(c) && !strchr(spcl_set,c))
	  c = '$';
	*np++ = c;
    }
    *np = NUL;				/* End of name */
    {					/* Now take care of periods. */
	int x;
	int ndots = 0;
	char * ld = NULL;		/* Last dot */
	char * nld = NULL;		/* Next to last dot */
	x = strlen(name2) - 1;
	for (; x >= 0; x--) {		/* Keep only the last one. */
	    if (name2[x] == ']' ||	/* But only do this */
		name2[x] == ':' ||	/* in the filename part */
		name2[x] == '>'
		)
	      break;
	    if (name2[x] == '.') {	/* Turn prior ones to underscore. */
		ndots++;
		if (ndots == 1)
		  ld = name2+x;
		else if (ndots == 2)
		  nld = name2+x;
		else if (ndots > 2)
		  name2[x] = '_';
	    }
	}
/*
  Finally we check to see if the final dot was really a version-number
  introducer; if so, we turn it into a semicolon and keep the next-to-last
  dot, otherwise we replace the next-to-last dot with an underscore.  Then we
  have exactly one dot in the name.
*/
	if (nld && ld) {		/* We have two dots left */
	    char *sld = ld;		/* Save position of last one */
	    ld++;			/* Point past it */
	    while (*ld <= '9' && *ld >= '0') /* See if only digits follow */
	      ld++;
	    if (*ld) {			/* No */
		*nld = '_';		/* replace previous '.' by '_' */
	    } else {			/* yes */
		*sld = ';';		/* replace '.' by ';' */
	    }
	}
    }
    debug(F110,"nzrtol name2 1",name2,0);

    /* But wait, there's more -- each dotted segment must be <= 39 chars  */

    {
	char buf3[ NAMX_C_MAXRSS+ 2], *p, *b;
	int n = 0;
	for (b = buf3, p = name2; *p; p++) {
	    if (*p == ';'
#ifdef VMSVERSIONS
		&& !vmsrversions
#endif /* VMSVERSIONS */
		) {			/* Discard version number */
		if (rdigits(p+1)) {
		    *b++ = '\0';
		    break;
		}
	    }
	    if (*p == '.' ||		/* Check for segment boundary */
		*p == '[' ||
		*p == ']' ||
		*p == ':' ||
		*p == '<' ||
		*p == '>' ||
		*p == ';')
	      n = 0;
	    if (n < 39) {		/* In segment copy up to 39 chars */
		*b++ = *p;
		n++;
	    }
	}
	*b = '\0';
	ckstrncpy(name2,buf3,max);
    }
    debug(F110,"nzrtol name2 2",name2,0);
}

/*  Z L T O R  --  Convert filename from local format to common form.  */

VOID
zltor(name,name2) char *name, *name2; {
    nzltor(name,name2,1,0,CKMAXPATH);
}

VOID
nzltor(name,name2,fncnv,fnspath,cp_len)
    char *name, *name2; int fncnv, fnspath, cp_len;
/* nzltor */ {
    char *cp, *pp;
    int flag;
    struct FAB fab;
    struct NAMX nam;
    char expanded_name[ NAMX_C_MAXRSS];
    char dirbuf[ NAMX_C_MAXRSS], *p, *q, *q2, *r, *s, *s2;
    int long rms_status;
    int cur_len = 0;

    debug(F111,"zltor fncnv",name,fncnv);

    flag = PARSE_NAME|PARSE_TYPE;
    if (fnspath == PATH_REL || fnspath == PATH_ABS)
      flag |= PARSE_DIRECTORY;
#ifdef VMSVERSIONS
    if (vmssversions)
      flag |= PARSE_VERSION;
#endif /* VMSVERSIONS */

    fab = cc$rms_fab;                   /* Initialize FAB. */
    nam = CC_RMS_NAMX;                  /* Initialize NAM[L]. */
    fab.FAB_L_NAMX = &nam;              /* Point FAB to NAM[L]. */

    /* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
    fab.fab$l_dna = (char *) -1;  /* Using NAML for default name. */
    fab.fab$l_fna = (char *) -1;  /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
    FAB_OR_NAML( fab, nam).FAB_OR_NAML_FNA = name;
    FAB_OR_NAML( fab, nam).FAB_OR_NAML_FNS = strlen( name);

    /* Install the default path name in the FAB or NAML. */
#define NONAME "NONAME"
    FAB_OR_NAML( fab, nam).FAB_OR_NAML_DNA = NONAME;
    FAB_OR_NAML( fab, nam).FAB_OR_NAML_DNS = strlen( NONAME);

    nam.NAMX_L_ESA = expanded_name;
    nam.NAMX_B_ESS = sizeof( expanded_name);

    if (!CHECK_ERR("%%CKERMIT-W-ZLTOR",sys$parse(&fab)))
      return;

    cp = name2;				/* Point to result */
    *cp = '\0';				/* Initialize it to empty */

    if ((PARSE_NODE & flag) && nam.NAMX_B_NODE && /* DECnet node:: */
	cur_len+ nam.NAMX_B_NODE < cp_len) {
	cur_len += nam.NAMX_B_NODE;
	strncat(cp, nam.NAMX_L_NODE, nam.NAMX_B_NODE);
    }
    if ((PARSE_DEVICE & flag) && nam.NAMX_B_DEV && /* Device: */
		cur_len+ nam.NAMX_B_DEV < cp_len) {
	cur_len += nam.NAMX_B_DEV;
	strncat(cp, nam.NAMX_L_DEV, nam.NAMX_B_DEV);
    }

    /* Directory Name [] */

    if ((PARSE_DIRECTORY & flag) && nam.NAMX_B_DIR &&
		cur_len+ nam.NAMX_B_DIR < cp_len) {
	int i; char * tmp;
        q = nam.NAMX_L_DIR;		/* The directory name from RMS */
	i = nam.NAMX_B_DIR;		/* Length; string not NUL-terminated */
	debug(F111,"zltor nam$_dir",q,i);
	if (!q) q = "[]";
	if (!*q) q = "[]";
	if (i < 0) i = 0;
	tmp = NULL;
	if (i > 0) {			/* Copy directory part */
	    if (tmp = malloc(i+1)) {
		p = tmp;
		for (; i > 0 ; i--)
		  *p++ = *q++;
		*p = NUL;
	    }
	}
	q = tmp;
	debug(F111,"zltor directory part",q,i);

	s = zgtdir();			/* Get current directory */
	debug(F110,"zltor zgtdir",s,0);
	if (!s) s = "[]";
	if (!*s) s = "[]";
	s2 = "";
	while (*s && *s != '[')
	  s++;
	if (*s) {
	    s2 = s+1;
	    while (*s2 && *s2 != ']') s2++; /* Closing bracket */
	}
	if (*s2) if (!*(s2+1)) *(s2+1) = NUL;
	debug(F110,"zltor current dir",s,0);

/* First change the VMS pathname to relative format if fnspath == PATH_REL */

/* Note that the user-specified default directory from zgtdir() may
 * differ in case from the actual directory spec from sys$parse(), so a
 * case-insensitive comparison is needed here (between *s and *q).
 */
	p = dirbuf;			/* Result */
	*p++ = *q++;			/* Copy left bracket and... */

	s++;				/* Point past it */
	q2 = q;				/* Remember this place */
	if (fnspath == PATH_REL) {	/* Compare this and current dir */
	    while (((islower( *s) ? toupper( *s) : *s) ==
	     (islower( *q) ? toupper( *q) : *q)) &&
	     *s && *q && *s != ']') {
		s++;
		q++;
	    }
	}
	if ( (*s != ']' && *q != ']' && *q != '.') ||
             (*q == ']' && *s != ']') ) {               /* No match */
	    p-- ;			/* So rewind source pointer */
	} else if (*q == ']' && *s == ']') { /* Current directory */
	    p = dirbuf;			/* So make directory part blank */
	} else {			/* Not current directory */
	    while (*q) *p++ = *q++;	/* so copy the rest */
	}
	*p = NUL;
	debug(F110,"zltor result 1",dirbuf,0);
/*
   VMS directory name is now in dirbuf in either absolute or relative format.
   Now change it to standard (UNIX) format if desired.
*/
	p = dirbuf;			/* Working pointer */
	r = dirbuf;			/* Result pointer */
	if (dirbuf[0]) {
	    extern char whoareu[], * cksysid;
	    /* Converting directory format */
	    debug(F110,"zltor whoareu",whoareu,0);
	    debug(F110,"zltor cksysid",cksysid,0);
	    if (fncnv || (whoareu[0] && strcmp((char *)whoareu,cksysid))) {
		int xflag = 0;
		if (p[1] == '.') {	/* Directory name is relative */
		    r += 2;		/* Point past the leading dot */
		    p += 2;
		}
		while (*p) {		/* Now convert the rest */
		    if (*p == '.' || *p == '[' || *p == ']') {
			if (!xflag) *p = '/';
			if (*p == ']')
			  xflag = 1;
		    }
		    p++;
		}
	    }
	}
	debug(F110,"zltor result 2",r,0);
	if (tmp) free(tmp);
	i = strlen(r);
	if (i > 0) {
	    strncat(cp,r,cp_len);
	    cur_len += i;
	}
    }
    if ((PARSE_NAME & flag) && nam.NAMX_B_NAME &&
		cur_len+ nam.NAMX_B_NAME < cp_len) {
	cur_len += nam.NAMX_B_NAME;
	strncat(cp, nam.NAMX_L_NAME, nam.NAMX_B_NAME);
    }
    if ((PARSE_TYPE & flag) && nam.NAMX_B_TYPE &&
		cur_len+ nam.NAMX_B_TYPE < cp_len) {
	cur_len += nam.NAMX_B_TYPE;
	strncat(cp, nam.NAMX_L_TYPE, nam.NAMX_B_TYPE);
    }
    if ((PARSE_VERSION & flag) && nam.NAMX_B_VER &&
		cur_len+ nam.NAMX_B_VER < cp_len) {
	cur_len += nam.NAMX_B_VER;
	strncat(cp, nam.NAMX_L_VER, nam.NAMX_B_VER);
    }
    if (fncnv && name2[cur_len] == NUL && name2[cur_len-1] == '.')
      name2[cur_len-1] = NUL;
    name2[cp_len-1] = NUL;
    debug(F111,"zltor result",name2,cur_len);
}

/*  C V T D I R  --  Convert directory name from [FOO]BAR.DIR;1 to [FOO.BAR] */

/*
  Call with:
    s = pointer to a string that has already been verified as a directory name
    s2 = pointer to where to put result
    len = size of destination buffer
  Returns:
    n on success, n > 0, size of result
    0 if no conversion was needed
   -1 on failure
*/
int
cvtdir(s,s2,len) char * s, * s2; int len; {
    int i, n;
    char dirbuf[ NAMX_C_MAXRSS+ 1];
    char * p = NULL;			/* Pointer to period */
    char * o = NULL;			/* Pointer to left bracket */
    char * r = NULL;			/* Pointer to right bracket */
    char * v = NULL;			/* Pointer to version */
    char * c = NULL;			/* Pointer to colon */
    char * q = NULL;
    char * t;
    int xx = NAMX_C_MAXRSS+ 1;

    debug(F111,"cvtdir",s,len);

    if (!s || !s2)
      return(-1);
    n = strlen(s);
    if (n < 1)
      return(-1);
    q = malloc(n+1);
    if (!q)
      return(-1);
    t = q;
    strcpy( q, s);
    s = t;
    debug(F111,"cvtdir 2",s,n);

    /* Find special characters.
     * 2010-03-15 SMS.
     * Changed to look for the rightmost dot, because the old scheme
     * (leftmost dot) got confused by directory dots in specs like
     * "[a.b.c]d.DIR;1".
     * If the path argument is supoposed to be legitimate, then it might
     * make more sense to use sys$parse() to parse the thing.
     * Otherwise, it might make some sense to check for realism, like,
     * say, ":" comes before "[", "[" comes before "]", and so on.
     */
    for (i = n; i > 0; i--) {
        if ((i > 0) && (s[ i- 1] == '^'))   /* Skip caret-escaped chars. */
        {
            i--;
            continue;
        }
	if (s[i] == ';')
	{
	    if (v == NULL)
	      v = s+i;                          /* Rightmost semicolon. */
	}
	else if (s[i] == '.')
	{
          if (p == NULL)
	    p = s+i;                            /* Rightmost dot. */
	}
	else if (s[i] == ':')
	  c = s+i;                              /* Leftmost colon. */
	else if (s[i] == ']' || s[i] == '>')
	{
	  if (r == NULL)
	    r = s+i;                            /* Rightmost dir bracket. */
	}
	else if (s[i] == '[' || s[i] == '<')
	  o = s+i;                              /* Leftmost dir bracket. */
    }
    debug(F110,"cvtdir c",c,0);
    debug(F110,"cvtdir r",r,0);
    debug(F110,"cvtdir o",o,0);
    debug(F110,"cvtdir p",p,0);
    debug(F110,"cvtdir v",v,0);

    dirbuf[0] = NUL;

    if (c) {				/* Have colon? */
	*c = NUL;
	ckstrncat(dirbuf,s,xx);
	ckstrncat(dirbuf,":",xx);
	s = c+1;
    }
    debug(F110,"cvtdir 3",dirbuf,0);

    if (o) {				/* Have opening bracket? */
	if (r) {
	    *r = NUL;
	    s = r+1;
	    ckstrncat(dirbuf,"[",xx);
	    ckstrncat(dirbuf,o+1,xx);
	} else
	  return(-1);
    } else
      ckstrncat(dirbuf,"[",xx);
    debug(F110,"cvtdir 4",dirbuf,0);

    /* Look for ".DIR;".
     * (Why look for ".DIR." here, when anything but a semicolon would
     * bewilder the whole dot search?)
     */
    if (p) {
	if (((*(p+1) == 'D') || (*(p+1) == 'd')) &&
	    ((*(p+2) == 'I') || (*(p+2) == 'i')) &&
	    ((*(p+3) == 'R') || (*(p+3) == 'r')) &&
	    (*(p+4) == ';' || *(p+4) == '.' || *(p+4) == NUL))
	  *p = NUL;
	else
	  return(-1);
	ckstrncat(dirbuf,".",xx);
	ckstrncat(dirbuf,s,xx);
    }
    ckstrncat(dirbuf,"]",xx);
    xx = ckstrncpy(s2,dirbuf,len);
    debug(F111,"cvtdir 5", dirbuf, xx);
    return((xx < (int)strlen(dirbuf)) ? -1 : xx);

#ifdef COMMENT
    if (r && p && v) {
	if (*(p+1) == 'D' &&
	    *(p+2) == 'I' &&
	    *(p+3) == 'R' &&
	    *(p+4) == ';') {
	    *r = NUL;
	    strcpy(q,s);
	    strcat(q,".");
	    *p = NUL;
	    strcat(q,p);
	    strcat(q,"]");
	}
	debug(F110,"cvtdir result",q,0);
	return(1);
    }
    return(0);
#endif /* COMMENT */
}

/*  Z C H D I R  --  Change directory.  */

static int setddir = 0;
/*
  This one is for restoring the startup directory in case SYS$SETDDIR() was
  ever called -- see comments below in zchdir().
*/
VOID
zrestoredir() {
    int status;
    struct dsc$descriptor_s indesc;
    if (!setddir)
      return;
    debug(F101,"zrestoredir setddir","",setddir);
    indesc.dsc$w_length = strlen( startupdir);
    indesc.dsc$a_pointer = startupdir;
    indesc.dsc$b_class = DSC$K_CLASS_S;
    indesc.dsc$b_dtype = DSC$K_DTYPE_T;
    status = sys$setddir(&indesc, 0, 0);
    if (!(status & 1)) vms_lasterr = status;
    debug(F111,"zrestoredir sys$setddir",startupdir,status);
    return;
}

int
zchdir(dirnam) char *dirnam; {
#ifndef NOSPL
    extern struct mtab *mactab;             /* Main macro table */
    extern int nmac;                        /* Number of macros */
#endif /* NOSPL */
    char dirbuf[ NAMX_C_MAXRSS+ 1];
    int status;
    struct dsc$descriptor_s indesc;

    if (!dirnam)			/* Watch out for null pointers */
      dirnam = "";
    debug(F110,"zchdir",dirnam,0);

    if (!*dirnam) {			/* No arg so back to home directory */
#ifdef COMMENT
	ckstrcpy(dirbuf, getenv("HOME"), NAMX_C_MAXRSS);
	dirnam = (char *) dirbuf;
	debug(F110,"zchdir home","",0);
	if (!isdir(dirnam))
	  return(0);
#else /* def COMMENT */
	dirnam = "SYS$LOGIN";		/* Supply default */
#endif /* def COMMENT [else] */
    }

#ifdef COMMENT				/* Why is this commented out? */
/* We now assume that dirnam is a valid directory name */
    status = isdir(dirnam);		/* Preverify to prevent C lib traps */
    debug(F111,"zchdir isdir 1",dirnam,status);
    x = strlen(dirnam);
    if (!status) {			/* Not a directory */
	if (dirnam[0] == '[')
	  return(0);
	if (x > 0 && (dirnam[x-1] == ']' || dirnam[x-1] == ':'))
	  return(0);
	if (*dirnam == '.')		/* Be nice - is it a subdirectory */
	  sprintf(dirbuf,"[%s]",dirnam); /* of the current directory? */
	else
	  sprintf(dirbuf,"[.%s]",dirnam);
	status = isdir(dirbuf);
	debug(F111,"zchdir isdir 2",dirnam,status);
	if (!status)
	  return(0);			/* Nope */
	dirnam = dirbuf;		/* Ayup */
    } /* ( We should really save "ayup" comments for the Maine program :-) */

    if (status == 2) {			/* It's [FOO]BAR.DIR;1 */
	/* Must convert to [FOO.BAR] */
	debug(F111,"zchdir malloc",dirnam,x);
	if (cvtdir(dirnam, dirbuf, NAMX_C_MAXRSS+ 1) > 0) {
	    debug(F110,"zchdir cvtdir",dirbuf,0);
	    dirnam = dirbuf;
	}
    }
#endif /* def COMMENT */
/*
  chdir() works as expected, when it works.  It only affects this process
  and those below it, and it handles search lists, hidden devices, etc.
  EXCEPT in VMS 6.2, in which a bug was introduced in the C runtime library.
  So we try chdir() first...
*/
    debug(F110,"zchdir chdir",dirnam,0);
    status = chdir(dirnam);
    debug(F101,"zchdir status","",status);
/*
  chdir() fails in VMS 6.2 when given a logical name like SYS$LOGIN that
  refers to a search list (e.g. for the SYSMGR ID on a cluster) or to a
  hidden device.  In this case SYS$SETDDIR works, but it has the unfortunate
  side effect of changing the default directory for the whole job, not just
  C-Kermit and below.
*/
    if (status == 0) {			/* chdir() worked */
	return(1);
    } else {				/* chdir() failed */
	indesc.dsc$w_length = (int) strlen(dirnam);
	indesc.dsc$a_pointer = dirnam;
	indesc.dsc$b_class = DSC$K_CLASS_S;
	indesc.dsc$b_dtype = DSC$K_DTYPE_T;
	status = sys$setddir(&indesc, 0, 0);
	if (!(status & 1)) vms_lasterr = status;
	debug(F111,"zchdir sys$setddir",dirnam,status);
	if (status == RMS$_NORMAL) {
	    setddir = 1;		/* Set this if setddir() ever used */
#ifndef NOSPL
            if (nmac) {			/* Any macros defined? */
                int k;			/* Yes */
                static on_cd = 0;
                if (!on_cd) {
                    on_cd = 1;
                    k = mlook(mactab,"on_cd",nmac);  /* Look this up */
                    if (k >= 0) {                    /* If found, */
                        if (dodo(k,zgtdir(),0) > -1) /* set it up, */
			  parser(1);	             /* and execute it */
                    }
                    on_cd = 0;
                }
            }
#endif /* NOSPL */
	    return(1);
	} else
	  return(0);
    }
}

/*  Z H O M E  --  Return pointer to user's home (login) directory.  */

char *
zhome() {

/*  return(getenv("HOME")); */          /* No... */
/*  return(getenv("SYS$LOGIN")); */     /* No... */
/*  return("SYS$LOGIN"); */             /* Maybe - but we need a terminator */
    return("SYS$LOGIN:");
}

/*  Z R E L N A M E  --  Make full pathname to relative to given directory */
/*
  Inputs:
    filespec: a full VMS pathname for a file, e.g.      DSK0:[DIR]OOFA.TXT;3
    dir:      a full VMS pathname for a directory, e.g. DSK0:[DIR.SUBDIR]
  Returns:
    Pointer to filespec relative to the given directory.
  Examples:
    1. filespec: DSK0:[OLGA]LOGIN.COM;63
       dir:      DSK0:[OLGA]
       result:   LOGIN.COM;63
    2. filespec: DSK0:[OLGA.LETTERS]JURY.TXT;1
       dir:      DSK0:[OLGA]
       result:   [.LETTERS]JURY.TXT;1
    3. filespec: DSK0:[OLGA.LETTERS.ANGRY]COMPLAINT.TXT;1
       dir:      DSK0:[OLGA]
       result:   [.LETTERS.ANGRY]COMPLAINT.TXT;1
    4. filespec: DSK0:[OLGA]LOGIN.COM;63
       dir:      DSK0:[OLGA.LETTERS.ANGRY]
       result:   [--]LOGIN.COM;63
    5. filespec: DSK0:[OLGA.LETTERS.NICE]PRAISE.TXT;3
       dir:      DSK0:[OLGA.LETTERS.ANGRY]
       result:   [-.NICE]PRAISE.TXT;3
    6. filespec: DSK1:[SYSTOOLS]KERMIT.EXE;1
       dir:      DSK0:[OLGA.LETTERS.ANGRY]
       result:   DSK1:[SYSTOOLS]KERMIT.EXE;1
*/
static char relnambuf[CKMAXPATH];

char *
zrelname(filespec, dir) char *filespec, *dir; {
    char * xs = filespec;		/* Should be a complete filespec, */
    char * s = filespec;		/* like DEV:[DIR]NAME.EXT;n. */
    char * p = dir;			/* Should be complete DEV:[DIR] */
    char * xp = NULL;
    char indir = NUL;

    debug(F110,"zrelname filespec",filespec,0);
    debug(F110,"zrelname dir",dir,0);

    for (; *s; s++,p++) {		/* Compare */
	if (*s == *p) {
	    if (*s == ':') {		/* End of device portion */
		xs = s;
	    } else if (*s == '[') {	/* Begin directory portion */
		indir = ']';
	    } else if (*s == '<') {	/* Ditto */
		indir = '>';
	    } else if (*s == '.' && indir) { /* End of directory segment */
		xs = s;			/* Remember it */
		xp = p;
	    } else if (*s == indir) {	/* End of directory portion */
		xs = s;
		break;
	    } else if (*p == indir) {
		xp = p;
		break;
	    }
	    continue;
	} else if (*s == '.' && indir && *p == indir) {
	    xp = NULL;
	    xs = s;
	} else if (*p == '.' && indir && *s == indir) {
	    xp = p;
	    xs = s;
	}
	break;
    }

    debug(F110,"zrelname xs",xs,0);
    debug(F110,"zrelname xp",xp,0);

    if (xs == filespec || !*xs) {	/* Nothing to strip */
	ckstrncpy(relnambuf,filespec,CKMAXPATH);
    } else if (xp && (*s != indir || *p != indir)) { /* File is above here */
	int i, m = 0;
	char c, * s2 = xp;
	while (c = *s2++) {
	    if (c == '.')		/* Count the levels */
	      m++;
	    else if (c == indir)
	      break;
	}
	relnambuf[0] = (indir == ']') ? '[' : '<'; /* Emit opening bracket */
	for (i = 1; i <= m; i++)	/* then the right number of dashes */
	  relnambuf[i] = '-';
	ckstrncpy(relnambuf+i,xs,CKMAXPATH-i); /* then the rest. */
	debug(F110,"zrelnam result 1",relnambuf,0);
    } else {				/* File is in a subdirectory */
	switch (*xs) {			/* so we can strip some parts */
	  case ':':
	  case ']':
	  case '>':
	    ckstrncpy(relnambuf,xs+1,CKMAXPATH);
	    break;
	  case '.':
	    sprintf(relnambuf,"%c%s",(indir == ']') ? '[' : '<',xs);
	    break;
	  default:
	    ckstrncpy(relnambuf,filespec,CKMAXPATH);
	}
	debug(F110,"zrelnam result 2",relnambuf,0);
    }
    return((char *)relnambuf);
}

/*  Z G T D I R  --  Return pointer to user's current directory.  */

char *
zgtdir() {

/* OLD_VMS (pre V5.0) is defined in the build file */

#ifdef OLD_VMS
    static char *gtdir_buf = 0;
    static char sysdisk[] = "SYS$DISK";
    char tmp_buf[ NAMX_C_MAXRSS+ 1];
    struct dsc$descriptor_s
	tmp_buf_dsc = {sizeof(tmp_buf),DSC$K_DTYPE_T,DSC$K_CLASS_S,&tmp_buf},
	sysdisk_dsc = {sizeof(sysdisk)-1,DSC$K_DTYPE_T,DSC$K_CLASS_S,&sysdisk};
    unsigned short int buf_len;
/*
 * Allocate buffer dynamically, first time through.  This makes the image
 * smaller.
 */
    if (!gtdir_buf) gtdir_buf = malloc( NAMX_C_MAXRSS+ 1);
/*
 * Translate device name.
 */
    lib$sys_trnlog(     &sysdisk_dsc,
			&buf_len,
			&tmp_buf_dsc,
			0,
			0,
			0);
    tmp_buf[buf_len] = '\0';
    ckstrncpy(gtdir_buf, tmp_buf, NAMX_C_MAXRSS);
/*
 * Get directory name.
 */
    sys$setddir(	0,	  /* New dir addr */
			&buf_len, /* length addr */
			&tmp_buf_dsc);
    tmp_buf[buf_len] = '\0';
    strcat(gtdir_buf,tmp_buf);

    return(gtdir_buf);  /* Can't seem to make LINK find getcwd()... */
			/* (wbader: removed &) */
#else /* def OLD_VMS */
    char *getcwd();
    char *buf;

    buf = cwdbuf;
    return(getcwd(buf, NAMX_C_MAXRSS));
#endif /* def OLD_VMS [else] */
}

/*  Z G P I D  --  Get process ID (of Kermit)  */

long
zgpid() {
    unsigned long sts, pid;
    struct itmlstdef {
	short int buflen;
	short int itmcod;
	char *bufaddr;
	long int *retlen;
    };

/* Should maybe this be JPI$_PROC_INDEX instead? */

    struct itmlstdef itmlst[] = {
	4, JPI$_PID, NULL, 0,
	0, 0, 0, 0
    };
    itmlst[0].bufaddr = (char *)&pid;
    sts = sys$getjpiw(0, 0, 0, &itmlst, 0, 0, 0);
    if (!(sts & 1)) vms_lasterr = sts;
    debug(F101,"zgpid sys$getjpiw status", "", sts);
    if (sts != SS$_NORMAL)
      return(0);
    else
      return(pid);
}

/*  Z X C M D  --  Run a system command so its output can be read as a file. */

int
zxcmd(filnum, comand) int filnum; char *comand; {
    char mbxnam[21], inpchan[6] = "NLA0:";
    unsigned long sts, pid;
    int one=1;

    struct dsc$descriptor_s
	mbx_desc = {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0},
	cmd_line = {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0},
	inp_desc = {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0};

    struct itmlstdef {
	short int buflen;
	short int itmcod;
	char *bufaddr;
	long int *retlen;
    };
    struct itmlstdef itmlst[] = {
	4, JPI$_PID, NULL, 0,
	0, 0, 0, 0
    };
    itmlst[0].bufaddr = (char *)&pid;

    if (nopush) {
	debug(F100,"zxcmd fails: nopush","",0);
	return(-1);
    }
    debug(F111,"zxcmd filnum", comand, filnum);
    if (filnum != ZIFILE && filnum != ZRFILE)
      return(0);

    sts = sys$getjpiw(0, 0, 0, &itmlst, 0, 0, 0);
    if (!(sts & 1)) vms_lasterr = sts;
    debug(F101,"zxcmd sys$getjpiw status", "", sts);
    if (sts != SS$_NORMAL)
      return(0);

    sprintf(mbxnam,"KERMIT$MBX_%08X", pid);
    debug(F110,"zxcmd mailbox logical", mbxnam, 0);
    mbx_desc.dsc$w_length = strlen(mbxnam);
    mbx_desc.dsc$a_pointer = mbxnam;

    sts = sys$crembx(0, &mbx_chan, SUB_BUF_SIZE, 0, 0, 0, &mbx_desc, 0);
    if (!(sts & 1)) vms_lasterr = sts;
    debug(F101,"zxcmd sys$crembx status", "", sts);
    if (sts != SS$_NORMAL)
      return(0);

    debug(F101,"zxcmd sys$crembx mbx_chan", "", mbx_chan);
    strcat(mbxnam, ":");
    mbx_desc.dsc$w_length++;

    cmd_line.dsc$w_length = strlen(comand);
    cmd_line.dsc$a_pointer = comand;

    inp_desc.dsc$w_length = strlen(inpchan);
    inp_desc.dsc$a_pointer = inpchan;

    sts = lib$spawn(&cmd_line, &inp_desc, &mbx_desc, &one, 0, &sub_pid,
		    &pexitlong, 0, 0, &mbx_chan);
    if (!(sts & 1)) vms_lasterr = sts;

    debug(F101,"zxcmd lib$spawn status", "", sts);
    if (sts != SS$_NORMAL)
      return(0);

    subprocess_input = 1;
    ispipe[filnum] = 1;			/* Remember this file is a "pipe" */

    sub_count = 0;
    fp[filnum] = fopen("NLA0:","r");	/* It wants a fp, give it one */
    debug(F101,"zxcmd fp[filnum]", "", fp[filnum]);
    fp[ZSYSFN] = fp[filnum];		/* Set ZSYSFN too, so we remember */
    return(1);
}

/*  Z C L O S F  - close the suprocess output file.  */

int
zclosf(filnum) int filnum; {
    unsigned long sts;

    if (subprocess_input != 0) {
	sts = sys$delprc(&sub_pid, 0);
	if (!(sts & 1)) vms_lasterr = sts;
	debug(F101,"zclosf sys$delprc status", "", sts);
	sts = sys$delmbx(mbx_chan);
	if (!(sts & 1)) vms_lasterr = sts;
	debug(F101,"zclosf sys$delmbx status", "", sts);
	sts = sys$dassgn(mbx_chan);
	if (!(sts & 1)) vms_lasterr = sts;
	debug(F101,"zclosf sys$dassgn status", "", sts);
	sub_ptr = sub_buf;		/* flush remaining data */
	sub_count = 1;
	*sub_buf = '\0';
	zincnt = 0;
	fclose(fp[filnum]);		/* Close the place-holders */
	fp[filnum] = fp[ZSYSFN] = NULL;
    }
    debug(F101,"zxcmd pexitlong","",pexitlong);
    subprocess_input = 0;		/* Say we're done */
    pexitstat = (pexitlong & 0x7fff);	/* Set status */
    return(pexitstat & 1 ? 1 : -1);
}

/*  Z X P A N D  --  Expand a wildcard string into an array of strings.  */

/*
  Returns the number of files that match s, with data structures set up
  so that first file (if any) will be returned by the next znext() call.
*/
static int xdironly = 0;
static int xfilonly = 0;
/* static int xmatchdot = 0; */
static int xrecursive = 0;
static int xnobackup = 0;

static int
zxpand(s) char * s; {
    int x;

    if (!s) s = "";
    x = strlen(s);
    debug(F111,"zxpand",s,x);
    if (x <= 0)				/* Nothing asked for, */
      return(0);			/* nothing returned. */
    fcount = (mtchs == NULL &&	/* Kermit */
	      (mtchs = (char **)malloc(MAXWLD * sizeof(*mtchs))) == NULL)
      ? 0
	: fgen(s,mtchs,MAXWLD);		/* Look up the file. */
    if (fcount > 0) {
	mtchptr = mtchs;		/* Save pointer for next. */
	debug(F111,"zxpand fcount",mtchs[0],fcount);
    }
    nxpand = fcount;
    return(fcount);
}

int
nzxpand(s,flags) char * s; int flags; {
    int x;

    debug(F111,"nzxpand",s,flags);
    x = flags & (ZX_DIRONLY|ZX_FILONLY);
    xdironly = (x == ZX_DIRONLY);
    xfilonly = (x == ZX_FILONLY);
    if (xdironly && xfilonly) {
	xdironly = 0;
	xfilonly = 0;
    }
    xrecursive = (flags & ZX_RECURSE);
    xnobackup  = (flags & ZX_NOBACKUP);

    debug(F101,"nzxpand xdironly","",xdironly);
    debug(F101,"nzxpand xfilonly","",xfilonly);
    debug(F101,"nzxpand xrecursive","",xrecursive);
    debug(F101,"nzxpand xnobackup","",xnobackup);

    x = zxpand(s);
    xdironly = 0;
    xfilonly = 0;
    xrecursive = 0;
    xnobackup = 0;
    return(x);
}


/*  Z X R E W I N D  --  Rewinds the zxpand() list */

int
zxrewind() {
    if (!mtchsinit || !mtchs) return(-1);
    fcount = nxpand;
    mtchptr = mtchs;
    return(nxpand);
}


/*  Z N E X T  --  Get name of next file from list created by zxpand(). */

/*
 Returns >0 if there's another file, with its name copied into the arg string,
 or 0 if no more files in list.
*/
int
znext(fn) char *fn; {

    if (fcount-- > 0)
      strcpy(fn,*mtchptr++);
    else
      *fn = '\0';
    debug(F111,"znext",fn,fcount+1);
    return(fcount+1);
}

/*  Z N E W N  --  Make a new name for the given file.  */

VOID
znewn(fn,s) char *fn, **s; {
    static char buf[ NAMX_C_MAXRSS+ 8];
    int x, flag = 0;
    long v = -1L;
    x = strlen(fn) - 1;
    ckstrncpy(buf, fn, NAMX_C_MAXRSS);
    while (x >= 0) {
	if (buf[x] == ';') {
	    v = atol(buf+x+1);
	    sprintf(buf+x+1,"%ld",v+1);
	    flag = 1;
	    break;
	}
	x--;
    }
    if (!flag) strcat(buf,";0");
    *s = buf;
}

#ifdef COMMENT
/*
  This returns the version number the given file, if it exists.  But it causes
  more trouble than it's worth.  For example, if zfnqfp() calls this to get
  the version number and includes it in its result, it breaks the LOG command.
*/
int
getvnum(fn) char * fn; {
    int rc = -1;
    char * p;
    struct dsc$descriptor_s
	file_spec = {0,DSC$K_DTYPE_T,DSC$K_CLASS_S,0},
	result = {0,DSC$K_DTYPE_T,DSC$K_CLASS_D,0},
	deflt = {0,DSC$K_DTYPE_T,DSC$K_CLASS_S,0};
    unsigned long context = 0, status;

    file_spec.dsc$w_length  = strlen(fn);
    file_spec.dsc$a_pointer = fn;

    status = lib$find_file(&file_spec, &result, &context, &deflt);
    if (status == RMS$_NORMAL) {
	if (!(status & 1)) {
	    vms_lasterr = status;
	    debug(F101,"getvnum error", fn, status);
	} else if (result.dsc$a_pointer) {
	    debug(F111,"getvnum", result.dsc$a_pointer, result.dsc$w_length);
	    p = strchr(result.dsc$a_pointer,';');
	    if (p) {
		if (*(p+1)) {
		    if (rdigits(p+1)) {
			rc = atoi(p+1);
		    }
		}
	    }
	}
    }
#ifdef DVI$_ALT_HOST_TYPE
    lib$find_file_end(&context);	/* Only on VMS V4 and later */
#endif /* DVI$_ALT_HOST_TYPE */
    return(rc);
}
#endif /* COMMENT */

/*
  Given a possibly unqualified or relative file specification fn, zfnqfp()
  returns the fully qualified filespec for the same file, returning a struct
  that contains the length (len) of the result, a pointer (fpath) to the
  whole result, and a pointer (fname) to where the filename starts.
*/
struct zfnfp *
zfnqfp(fn, buflen, buf)  char * fn; int buflen; char * buf; {
    int x = 0, y = 0;
    char * cp;
    static struct zfnfp fnfp;

    struct FAB fab;
    struct NAMX nam;
    char expanded_name[NAMX_C_MAXRSS];
    char tmpnam[NAMX_C_MAXRSS+ 16];
    int long rms_status;
    int cur_len = 0;
    int cp_len = buflen;

    if (!fn)
      return(NULL);

    cp = fn;
    debug(F111,"zfnqfp 1",cp,buflen);

#ifdef COMMENT
    /* This works but it messes up LOG commands, etc. */
    if (!strchr(cp,';')) {
	int v;
	v = getvnum(cp);
	if (v > 0) {
	    sprintf(tmpnam,"%s;%d",cp,v);
	    cp = tmpnam;
	    debug(F111,"zfnqfp getvnum",cp,v);
	}
    }
#endif /* COMMENT */

    /* initialize the data structure */

    fnfp.len = buflen;
    fnfp.fpath = buf;
    fnfp.fname = NULL;

    fab = cc$rms_fab;                   /* Initialize the FAB. */
    nam = CC_RMS_NAMX;                  /* Initialize the NAM[L]. */
    fab.FAB_L_NAMX = &nam;              /* Point the FAB to the NAM[L]. */

    /* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
    fab.fab$l_dna = (char *) -1;  /* Using NAML for default name. */
    fab.fab$l_fna = (char *) -1;  /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
    FAB_OR_NAML( fab, nam).FAB_OR_NAML_FNA = cp;
    FAB_OR_NAML( fab, nam).FAB_OR_NAML_FNS = strlen( cp);

    nam.NAMX_L_ESA = expanded_name;
    nam.NAMX_B_ESS = sizeof( expanded_name);

    if (!CHECK_ERR("%%CKERMIT-W-ZFNQFP",sys$parse(&fab)))
      return(NULL);

    /* if not a file oriented device, convert name to upper case */

    if (!(fab.fab$l_dev & DEV$M_NET) && !(fab.fab$l_dev & DEV$M_FOD)) {
        cp = fn;
        while (*cp) {
            if (islower(*cp))
              *cp = toupper(*cp);
            cp++;
        }
        return(NULL);
    }
    cp = buf;
    *cp = '\0';			/* Make a zero length string */
    fnfp.fpath = cp;
    if (nam.NAMX_B_NODE && nam.NAMX_B_NODE < cp_len) /* Node */
      cur_len = nam.NAMX_B_NODE;
    if (nam.NAMX_B_DEV && cur_len+ nam.NAMX_B_DEV < cp_len) /* Device */
      cur_len += nam.NAMX_B_DEV;
    if (nam.NAMX_B_DIR && cur_len+ nam.NAMX_B_DIR < cp_len) /* Directory */
      cur_len += nam.NAMX_B_DIR;
    if (nam.NAMX_B_NAME && cur_len+nam.NAMX_B_NAME < cp_len) { /* Name */
	fnfp.fname = buf + cur_len;
	cur_len += nam.NAMX_B_NAME;
    }
    if (nam.NAMX_B_TYPE && cur_len+ nam.NAMX_B_TYPE < cp_len) /* Extension */
      cur_len += nam.NAMX_B_TYPE;
    if (nam.NAMX_B_VER && cur_len+ nam.NAMX_B_VER < cp_len)
      cur_len += nam.NAMX_B_VER;
    strncat(cp, nam.NAMX_L_NODE, cur_len);
    debug(F111,"zfnqfp 2",buf,cur_len);
    if (cur_len > 1) {
	if (buf[cur_len-1] == ';' && buf[cur_len-2] == '.') {
	    buf[cur_len-2] = NUL;
	    cur_len -= 2;
	}
    }
    fnfp.len = cur_len;
    debug(F111,"zfnqfp 3",buf,cur_len);

    return(&fnfp);
}

/*
 * fgen:
 *  This is the actual name generator.  It is passed a string,
 *  possibly containing wildcards, and an array of character pointers.
 *  It finds all the matching filenames and stores them into the array.
 *  The returned strings are allocated from a static buffer local to
 *  this module (so the caller doesn't have to worry about deallocating
 *  them); this means that successive calls to fgen will wipe out
 *  the results of previous calls.  This isn't a problem here
 *  because we process one wildcard string at a time.
 *
 * Input: a wildcard string, an array to write names to, the
 *        length of the array.
 * Returns: the number of matches.  The array is filled with filenames
 *          that matched the pattern.  If there wasn't enough room in the
 *	    array, -1 is returned.
 */
int
fgen(pat,resarry,len) char *pat, *resarry[]; int len; {
    char * p, * f;
    int i, x, y;
    struct dsc$descriptor_s
	file_spec = {0,DSC$K_DTYPE_T,DSC$K_CLASS_S,0},
	result = {0,DSC$K_DTYPE_T,DSC$K_CLASS_D,0},
	deflt = {0,DSC$K_DTYPE_T,DSC$K_CLASS_S,0};
    unsigned long context = 0, status;
    int count = 0;
    char *def_str = "*.*";
    char dirbuf[ NAMX_C_MAXRSS+ 1];
    char tmpbuf[ NAMX_C_MAXRSS+ 1];
    char patbuf[ NAMX_C_MAXRSS+ 4];

    debug(F101,"fgen len","",len);

    if (!mtchsinit) {			/* Initialize filename array */
	mtchsinit = 1;
	for (i = 0; i < MAXWLD; i++)
	  mtchs[i] = NULL;
    } else {
	for (i = 0; i < lastcount; i++) { /* Free previous filenames */
	    if (resarry[i]) {
		free(resarry[i]);
		resarry[i] = NULL;
	    }
	}
    }
    debug(F111,"fgen pat",pat,xrecursive);
    debug(F101,"fgen xdironly","",xdironly);
    debug(F101,"fgen xfilonly","",xfilonly);

    /* Too bad the LIB$FIND_FILE() flags word doesn't have a "recurse" bit */
    /* If recursive search requested stuff "..." into the directory name */

    if (xrecursive && !ckindex("...",pat,0,0,1)) {
	if (zfnqfp(pat, NAMX_C_MAXRSS, patbuf)) {
	    int x;
	    char * p;
	    x = strlen(patbuf);
	    debug(F110,"fgen patbuf 1",patbuf,x);
	    if (x > 0) {
		p = patbuf + x - 1;
		*(p+4) = '\0';
		while (*p) {
		    *(p+3) = *p;
		    if (*p == ']' || *p == '>') {
			*(p+2) = '.';
			*(p+1) = '.';
			*p = '.';
			break;
		    }
		    p--;
		}
		debug(F110,"fgen patbuf 2",patbuf,x);
		pat = patbuf;
	    }
	}
    }
    file_spec.dsc$w_length  = strlen(pat);
    file_spec.dsc$a_pointer = pat;

    x = file_spec.dsc$w_length -1;	/* kludge to keep zero length name */
    p = def_str;				/* "[foo]"  -> "[foo]*.*" */
    if (pat[x] != ']' && pat[x] != '>')
      p++;					/* "[foo]." -> "[foo].*"  */
    deflt.dsc$w_length  = strlen(p);
    deflt.dsc$a_pointer = p;

    while (count < len
	   && (status = lib$find_file(&file_spec, &result, &context, &deflt))
		== RMS$_NORMAL) {
	if (!(status & 1)) vms_lasterr = status;
	f = tmpbuf;
	strncpy(f, result.dsc$a_pointer, result.dsc$w_length);
	tmpbuf[result.dsc$w_length] = '\0';
	x = isdir(tmpbuf);
	debug(F111,"fgen isdir",f,x);
	if (x != 0 && xfilonly)		/* Want files only and not a file */
	  continue;
	if (x == 0 && xdironly)		/* Want dirs only but not a dir */
	  continue;
	if (xdironly) {
	    if (x == 2) {
		y = cvtdir(tmpbuf, (char *)dirbuf, NAMX_C_MAXRSS+ 1);
		if (y > 0) f = dirbuf;
	    }
	}
	resarry[count] = malloc((int) strlen(f) + 1);
	if (resarry[count]) {
	    strcpy(resarry[count], f);
	    count++;
	    lastcount = count;
	} else {
	    debug(F100,"fgen malloc failure","",0);
	    return(-1);
	}
    }
#ifdef DVI$_ALT_HOST_TYPE
    lib$find_file_end(&context);	/* Only on VMS V4 and later */
#endif /* DVI$_ALT_HOST_TYPE */

    lib$sfree1_dd(&result);

    if (status == RMS$_FNF)		/* File Not Found */
      return((count <= len) ? 0 : -1);
    if (status == RMS$_NMF)		/* No More Found */
      return(count);
    /* Bernd Onasch says that VMS sometimes returns RMS$_NORMAL here, so... */
    if (status == RMS$_NORMAL)
      return(count);

    /* Some other status.  Return 0. */
    /* Improve this later based on results from following debug stmt. */
    debug(F101,"fgen unexpected failure status","",status);
    return(0);
}


/*  Z C O P Y  --  Copy a single file. */

/*
  Call with source and destination names.
  If destination is a directory (specification, not directory file name),
  the source file is copied to that directory with its original name.
  Returns:
   0 on success.
  <0 on failure:
  -2 = source file is a directory file {or link on Unix}.
  -3 = source file not found or not accessible {locked}.
  -4 = permission denied {read, isguest}.
  -5 = source and destination are same file {not used on VMS}
  -6 = i/o error.
  -7 = open output file error {permissions, existing file, directory file}
  -1 = other error.
*/

#include <shrdef.h>
#include <stsdef.h>
#ifdef __DECC
#ifndef BUGFILL7
#ifndef NOCONVROUTINES
#include <conv$routines.h>
#define HAVE_CONVROUTINES
#endif /* NOCONVROUTINES */
#endif /* BUGFILL7 */
#ifndef HAVE_CONVROUTINES
/* 2010-03-09 SMS.
 * Minimal ("#ifndef __NEW_STARLET") declarations to reduce
 * %CC-I-IMPLICITFUNC diagnostics.
 */
unsigned int conv$convert(__unknown_params);
unsigned int conv$pass_files(__unknown_params);
unsigned int conv$pass_options(__unknown_params);
#endif /* ndef HAVE_CONVROUTINES */
#endif /* __DECC */

#ifdef ZCOPY

int
zcopy(source,destination) char *source, *destination; {
    char *src = source, *dst = destination; /* Local pointers to filenames */

    int  rc, n;
    unsigned long int sts;
    unsigned long int options[] = {2,1,1}; /* CONV$ options CREATE SHARE */
    char *p, buf[ NAMX_C_MAXRSS+ 1];	/* File name buffer */


    struct dsc$descriptor_s  ifd = {0,DSC$K_DTYPE_T,DSC$K_CLASS_S,0};
    struct dsc$descriptor_s  ofd = {0,DSC$K_DTYPE_T,DSC$K_CLASS_S,0};

    debug(F110,"zcopy src arg",src,0);
    debug(F110,"zcopy dst arg",dst,0);

#ifdef CK_LOGIN
    if (inserver && isguest)
      return(-4);
#endif /* CK_LOGIN */

    switch (zchki(src)) {
        case -1 : return(-3);	/* not found or not accessible */
        case -2 : return(-2);	/* a directory file */
        case -3 : return(-4);	/* no read permission */
    }

/* If destination is a directory name, append the source file name */

    switch (isdir(dst)) {
      case 0:
	debug(F100,"zcopy destination is not a directory","",0);
	break;
      case 2:
	debug(F100,"zcopy destination is a directory file","",0);
	return(-7);
      case 1: {
	  char *q = NULL;
	  debug(F100,"zcopy destination is a directory spec","",0);
	  zstrip(src,&q);
	  n = strlen(dst);
	  if (strlen(q) + n < sizeof(buf)) {
	      ckstrncpy(buf, dst, NAMX_C_MAXRSS); /* dst dir */
	      p = buf + n - 1;
	      if (*p != ']' && *p != '>' && *p != ':' )
		*(++p) = ':';		/* logical w/o ":" */
	      strcpy(++p,q);		/* nam from src */
	      dst = buf;
	      debug(F110,"zcopy dst result",dst,0);
	      strcpy(destination,dst);
	      break;
	  } else {
	      debug(F100,"zcopy dir+name overflow","",0);
	      return(-7);
	  }
      }
    }
    ifd.dsc$a_pointer = src;
    ifd.dsc$w_length  = strlen(src);
    ofd.dsc$a_pointer = dst;
    ofd.dsc$w_length  = strlen(dst);

    sts = conv$pass_files(&ifd, &ofd);
    if (sts != SS$_NORMAL) {
	debug(F101,"zcopy conv$pass_files error","", sts);
	return(-1);
    }
/*
  CONV option CREATE limits zcopy dst to a new file; to overwrite
  an existing file, delete the old version first
*/
    sts = conv$pass_options(&options);
    if (sts != SS$_NORMAL) {
        debug(F101,"zcopy conv$pass_option error","",sts);
        return(-1);
    }
    sts = conv$convert();

/* CONV$_ errors are shared error codes (facility 178) */

    if (sts != SS$_NORMAL) {
        debug(F101,"zcopy conv$convert error","", sts);
        switch (sts & STS$M_CODE) {
	  case SHR$_OPENOUT:
	    rc = -7;
	    break;
	  case SHR$_OPENIN:
	    rc = -4;
	    break;
	  case SHR$_READERR || SHR$_WRITEERR:
	    rc = -6;
	    break;
	  default : rc = -1;
	}
    } else
      rc = 0;
    debug(F101,"zcopy status","",rc);

#ifdef CKSYSLOG
    if (rc > -1 && ckxsyslog >= SYSLG_FC && ckxlogging) {
	if (rc)
	  syslog(LOG_INFO,"file[] %s: copy to %s failed (%u)", src, dst, sts);
	else
	  syslog(LOG_INFO,"file[] %s: copy to %s ok", src, dst);
    }
#endif /* CKSYSLOG */

    return(rc);
}

#endif /* def ZCOPY */


/*  Z R E N A M E  --  Rename a file.  */

/*  Call with old and new names */
/*  Returns 0 on success, -1 on failure. */
int
zrename(old,new) char *old, *new; {
    int sts;

    sts = rename(old,new);

    return((sts ? -1 : 0));
}

/*  Z C F D A T  --  Return a file's modification time.  */

char *
zfcdat(name) char *name; {
/*
  Returns modification date/time of file whose name is given in the argument
  string.  Return value is a pointer to a string of the form:

    yyyymmdd hh:mm:ss

  for example 19931231 23:59:59, which represents the local time (no timezone
  or daylight savings time finagling required).  Returns an empty string ("")
  on failure.  The text pointed to by the string pointer is in a static
  buffer, and so should be copied to a safe place by the caller before any
  subsequent calls to this function.
*/

/*
  Contributed by William Bader, 9 Nov 93, based on UNIX version: "It would
  probably be possible to get the date by opening the file and requesting a
  NAM block like ckvfio.c does, but stat seems to do the trick."
*/
    struct stat statbuf;
    struct tm *tm;
    static char datebuf[20];

    datebuf[0] = '\0';

    if (name &&
        *name &&
        stat(name,&statbuf) != -1 &&
#ifdef COMMENT
	/* Modification date/time */
        (tm = localtime((const time_t *)&statbuf.st_mtime)))
#else
        /* Creation date/time */
        (tm = localtime((const time_t *)&statbuf.st_ctime)))
#endif /* COMMENT */
      sprintf(datebuf, "%04d%02d%02d %02d:%02d:%02d",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);

    return(datebuf);
}


/*  Z S T I M E  --  Set or compare a file's creation date/time.  */

/*
 * Note: There's an additional value for parameter X on VAX/VMS systems. As
 * it's horribly painful to change a file's creation date after-the-fact we
 * call zstime with an argument  of 2 to pre-set the date when creating the
 * file. An argument of 0 (which the main-line code thinks sets the date of
 * the output file) returns success but does nothing.  Note that an invalid
 * or missing attribute packet will cause $bintim to return an error, which
 * causes the routine to exit. Since we pre-set the binary time to zero, we
 * will create the file "now", or say the incoming file is newer, whichever
 * is appropriate.
 */
static char mth[13][4] = {
    "JAN","FEB","MAR","APR",
    "MAY","JUN","JUL","AUG",
    "SEP","OCT","NOV","DEC",
    ""
};

int
zstime(f,yy,x) char *f; struct zattr *yy; int x; {
    int rms_sts;
    unsigned short attr_pro = 0;
    static char cdate[24];	      /* File date yyyy-mm-dd hh:mm:ss.00 */
    static char mnum[2];
    struct dsc$descriptor_s
	bintim_desc = {sizeof(cdate) - 1, DSC$K_DTYPE_T, DSC$K_CLASS_S,
			 (char *)&cdate};
    unsigned long file_date[2], attr_date[2];
    char *dptr;
    int setperms = 0;
    int setdate = 0;
#ifdef DEBUG
    char xbuf[24];			/* Hex buffer for debugging only */
#endif /* DEBUG */

    debug(F111,"zstime entry",f,x);
    if (yy->date.len == 0) {		/* No date in struct */
	if (yy->lprotect.len != 0) {	/* So go do permissions */
	    goto zsperms;
	} else {
	    debug(F100,"zstime: nothing to do","",0);
	    return(0);
	}
    }

/* First, make a system quadword date from our attribute struct parameter */

    dptr = yy->date.val;
    strncpy(cdate,"dd-mmm-yyyy 00:00:00.00",24);
    attr_date[0]=0;			/* Clear time in case of error */
    attr_date[1]=0;
    strncpy(cdate+7, dptr, 4);		/* yyyy */
    dptr += 4;
    strncpy(mnum, dptr, 2);
    strncpy(cdate+3, mth[atoi(mnum)-1], 3); /* mm */
    dptr += 2;
    strncpy(cdate, dptr, 2);		/* dd */
    dptr += 3;
    strncpy(cdate+12, dptr, 8);		/* hhmmss */
    cdate[23] = '\0';			/* terminate */
    rms_sts = sys$bintim(&bintim_desc, &attr_date); /* Convert to internal */
    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
    if (rms_sts != SS$_NORMAL) {		    /* time format */
	debug(F111,"zstime - $bintim returns",cdate,rms_sts);
	return(-1);
    }
    debug(F110,"zstime built",cdate,0);
    sprintf(cdate, "%08X%08X", attr_date[1], attr_date[0]);
    debug(F110,"zstime $bintim attr_date", cdate, 0);
    setdate = 1;

  zsperms:				/* File protection */
    if (x != 1) {			/* We're not just comparing dates... */
	int i, xx, flag = 0;
	char * s;
#ifdef DEBUG
	if (deblog) {
	    debug(F111,"zstime lperms",yy->lprotect.val,yy->lprotect.len);
	    debug(F111,"zstime gperms",yy->gprotect.val,yy->gprotect.len);
	    debug(F110,"zstime system id",yy->systemid.val,0);
	    sprintf(xbuf,"%X",xabpro_ofile.xab$w_pro);
	    debug(F110,"zstime xabpro_ofile.xab$w_pro 1",xbuf,0);
	}
#endif /* DEBUG */
	if ((yy->lprotect.len > 0 &&	/* Have sys-dependent permissions */
	     yy->systemid.len > 0 &&	/* from A-packet... */
	     !strcmp(yy->systemid.val,"D7"))) { /* AND you are VMS like me */
	    attr_pro = 0;
	    flag = 1;
	    s = yy->lprotect.val;	/* VMS protections */
	    xx = yy->lprotect.len;
	    if (xx < 0)			/* Length < 0 means inheritance */
	      xx = 0 - xx;		/* (ignored in VMS) */
	    if (xx > 4) {		/* Bad format - must be 4 bytes */
		flag = 0;
	    } else {
		for (i = 0; i < xx; i++) {	/* Decode hex string */
		    if (*s <= '9' && *s >= '0') {
			attr_pro = 16 * attr_pro + (int)(*s) - '0';
		    } else if ((*s <= 'F' && *s >= 'A')) {
			attr_pro = 16 * attr_pro + 10 + (int)(*s) - 'A';
		    } else if ((*s <= 'f' && *s >= 'a')) {
			attr_pro = 16 * attr_pro + 10 + (int)(*s) - 'a';
		    } else {
			flag = 0;
			break;
		    }
		    s++;
		}
	    }
#ifdef DEBUG
	    if (deblog) {
		debug(F101,"zstime VMS local protection flag","",flag);
		sprintf(xbuf,"%X",attr_pro);
		debug(F111,"zstime VMS local protection",xbuf,attr_pro);
	    }
#endif /* DEBUG */
	} else if (!flag && yy->gprotect.len > 0) { /* Systems differ */
	    int g;			            /* Use the generic one */
	    unsigned long dfpro = 0L, mask = 0L;
	    unsigned short tmp;
	    tmp = xabpro_ofile.xab$w_pro;
    	    rms_sts = sys$setdfprot(0,&dfpro); /* Get default protection */
	    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
#ifdef DEBUG
	    if (deblog) {
		sprintf(xbuf,"%X",dfpro);
		debug(F111,"zstime sys$setdfprot",xbuf,rms_sts);
	    }
#endif /* DEBUG */
	    if (rms_sts & 1)		/* Succeeded so use it */
	      tmp = dfpro;
	    tmp &= 0xff0f;		/* But remove Owner field */

/* Convert generic protection to number and translate to internal VMS form */

	    g = xunchar(*(yy->gprotect.val));
	    attr_pro = 0x0F;	 	  /* 1111 Initial value = No access */
	    if (g &  1)			  /* 1=Read */
	      attr_pro &= ~XAB$M_NOREAD;  /* 0001 */
	    if (g &  2)			  /* 2=Write */
	      attr_pro &= ~XAB$M_NOWRITE; /* 0010 */
	    if (g &  4)			  /* 4=Execute */
	      attr_pro &= ~XAB$M_NOEXE;   /* 0100 */
	    if (g &  8)			  /* 8=Append */
	      attr_pro &= ~XAB$M_NOWRITE; /* 0010 */
	    if (g & 16)			  /* 16=Delete */
	      attr_pro &= ~XAB$M_NODEL;   /* 1000 */
	    mask = attr_pro    |	  /* Don't give S,G,W any */
	      (attr_pro <<  4) |	  /* more access than Owner... */
	      (attr_pro <<  8) |
	      (attr_pro << 12);
	    attr_pro = attr_pro << 4;	/* Shift to owner field */
	    tmp |= attr_pro;		/* OR in Owner protection */
	    attr_pro = tmp | mask;	/* Mask off any higher permissions */
	    flag = 1;			/* Flag we have the protection value */
#ifdef DEBUG
	    if (deblog) {
		debug(F101,"zstime g attr_pro 4","",attr_pro);
		sprintf(xbuf,"%X",attr_pro);
		debug(F111,"zstime generic protections",xbuf,attr_pro);
	    }
#endif /* DEBUG */
	}
	if (flag)
	  setperms = 1;
    }
    debug(F101,"zstime setperms","",setperms);
    debug(F101,"zstime setdate","",setdate);

    switch (x) {
      case 1:				/* Function code 1, compare dates */
	if (!setdate)			/* Do we have a date? */
	  return(-1);			/* Nothing to do */
	fab_ifile = cc$rms_fab;         /* Initialize the FAB. */
	nam_ifile = CC_RMS_NAMX;        /* Initialize the NAM[L]. */
	fab_ifile.FAB_L_NAMX = &nam_ifile;  /* Point the FAB to the NAM[L]. */

	/* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
	fab_ifile.fab$l_dna = (char *) -1;  /* Using NAML for default name. */
	fab_ifile.fab$l_fna = (char *) -1;  /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
	FAB_OR_NAML( fab_ifile, nam_ifile).FAB_OR_NAML_FNA = f;
	FAB_OR_NAML( fab_ifile, nam_ifile).FAB_OR_NAML_FNS = strlen( f);

	fab_ifile.fab$b_fac = FAB$M_BIO | FAB$M_GET;
	fab_ifile.fab$l_xab = (char *)&xabdat_ifile;
	rab_ifile = cc$rms_rab;
	rab_ifile.rab$l_fab = &fab_ifile;
	xabdat_ifile = cc$rms_xabdat;
	rms_sts = sys$open(&fab_ifile);
	if (!(rms_sts & 1)) vms_lasterr = rms_sts;
	if (rms_sts != RMS$_NORMAL) {
	    debug(F101,"zstime $open failed, status","",rms_sts);
	    return(-1);
	}
	memcpy(file_date, &xabdat_ifile.xab$q_cdt, 8);
	sprintf(cdate, "%08x%08x", file_date[1], file_date[0]);
	debug(F110,"zstime $bintim file_date", cdate, 0);
	rms_sts = sys$close(&fab_ifile);
	if (!(rms_sts & 1)) vms_lasterr = rms_sts;
	if (rms_sts != RMS$_NORMAL) {
	    debug(F101,"zstime $close failed, status","",rms_sts);
	    return(-1);
	}
	if (attr_date[1] < file_date[1]) {
	    debug(F100,"zstime incoming file is older","",0);
	    return(1);
	}
	if (attr_date[1] == file_date[1]) {
	    if (attr_date[0] <= file_date[0]) {
		debug(F100,"zstime incoming file is older, not by much","",0);
		return(1);
	    }
	    debug(F100,"zstime incoming file is newer","",0);
	    return(0);
	}
	break;

      case 0:				/* Function code 0, set attributes */
	return(0);			/* Say we did (see comments above) */

      case 2:				/* Function code 2, really set them */
	if (setdate) {			/* Do we have a date? */
	    debug(F100,"zstime setting date","",0);
	    memcpy(&xabdat_ofile.xab$q_cdt, attr_date, 8); /* File date */
	}
	if (setperms) {			/* File protection... */
	    debug(F100,"zstime setting protection","",0);
	    xabpro_ofile.xab$w_pro = attr_pro;
	}
	return(0);
    }
    return(-1);
}


/*  Z K E R M I N I  --  Find initialization file.  */
/*
  Places name of init file in buffer pointed to by s.
  If no init file found, the device name of the null device is used.
  returns 0 always.
*/
int
zkermini(s, s_len, def) char *s; int s_len; char *def; {
    FILE fd;
    struct dsc$descriptor_s
		dsc_in = {0,DSC$K_DTYPE_T,DSC$K_CLASS_S,0},
		dsc_out = {0,DSC$K_DTYPE_T,DSC$K_CLASS_D,0},
		dsc_def = {0,DSC$K_DTYPE_T,DSC$K_CLASS_S,0};
    int max_len;
    long unsigned int rms_s;
    unsigned long find_file_context = 0;

    struct TRNLIST {
	char *name;			/* ASCII file or logical name */
	unsigned char flag;		/* Zero to use default filename */
    } *p;

    static struct TRNLIST slist[] = {
	{"", 0},			/* Dummy first entry points to file */
	{"ckermit_ini:", 0},		/* CKERMIT_INI: points to directory */
	{"ckermit_init", 1},		/* CKERMIT_INIT points to file      */
	{"sys$login:",   0},		/* CKERMIT.INI in login directory   */
	{"", 0}
    };
    p = slist;				/* Point to list */
    if (rcflag) {			/* Name given on command line? */
	slist[0].name = def;		/* Yes, stuff its name into slist */
	slist[1].name = "";
    } else {				/* No, */
	*p++;				/* skip past dummy entry. */
    }
    while(*(p->name)) {			/* Search the list top to bottom */

	dsc_in.dsc$w_length = strlen(p->name); /* Length of work area */
	dsc_in.dsc$a_pointer = p->name; /* Address of string */

	if (!(p->flag)) {
	    dsc_def.dsc$w_length = strlen(def); /* Length of work area */
	    dsc_def.dsc$a_pointer = def; /* Address of string */
	} else {
	    dsc_def.dsc$w_length = 0;	/* Length of work area */
	    dsc_def.dsc$a_pointer = 0;	/* Address of string */
	}
	rms_s = lib$find_file(
				&dsc_in,	/* File spec */
				&dsc_out,	/* Result file spec */
				&find_file_context, /* Context */
				&dsc_def,	/* Default file spec */
				0,		/* Related spec */
				0,		/* STV error */
				0);		/* Flags */

	if (!(rms_s & 1)) vms_lasterr = rms_s;
	if (rms_s == RMS$_NORMAL) {
	    max_len = ((unsigned short int) dsc_out.dsc$w_length < s_len ?
		       (unsigned short int) dsc_out.dsc$w_length : 0);
	    if (!max_len) {
		fprintf(stderr,
		"%%ZKERMINI out string not long enough, ignoring .ini file\n");
	    } else {
		strncpy(s,dsc_out.dsc$a_pointer,max_len);
		s[max_len] = '\0';
	    }
	    lib$find_file_end(&find_file_context);
	    lib$sfree1_dd(&dsc_out);    /* Return dyno memory */
	    return(1);
	}
	p++;
	lib$find_file_end(&find_file_context);
    }
/*
 * No initialization file found.  We can't return the null string because the
 * runtime library will successfully open it if the file ".;" exists in the
 * user's directory.  Instead we return the name of the null device.
 */
    strcpy(s, "NLA0:");				/* Return null init file */
    lib$sfree1_dd(&dsc_out);
    return(0);
}

#ifdef COMMENT
static int
parse_fname(cp, cp_len, defnam, flag, fncnv, fnspath)
char *cp;		/* Pointer to file spec to parse */
int cp_len;		/* Length of cp field */
char *defnam;		/* Default file spec */
int flag;		/* Flag word PARSE_xxx */
int fncnv;		/* Filename conversion */
int fnspath;		/* Pathname handling */
{
    struct FAB fab;
    struct NAM nam;
    char expanded_name[ NAMX_C_MAXRSS];
    char dirbuf[ NAMX_C_MAXRSS], *p, *q, *q2, *r, *s, *s2;
    int long rms_status;
    int cur_len = 0;

    debug(F110,"zltor entry",defnam,0);

    fab = cc$rms_fab;
    fab.fab$l_nam = &nam;
    fab.fab$l_fna = cp;
    fab.fab$b_fns = strlen(cp);
    if (defnam) {
	fab.fab$b_dns = strlen(defnam);
	fab.fab$l_dna = defnam;
    } else
      fab.fab$l_dna = 0;

    nam = cc$rms_nam;
    nam.nam$l_esa = (char *)&expanded_name;
    nam.nam$b_ess = sizeof(expanded_name);

    if (!CHECK_ERR("%%CKERMIT-W-PARSE, ",
		sys$parse(&fab)))
	return(-1);

    *cp = '\0';				/* Start with an empty result */

    if ((PARSE_NODE & flag) && nam.nam$b_node && /* DECnet node:: */
		cur_len+nam.nam$b_node < cp_len) {
	cur_len += nam.nam$b_node;
	strncat(cp, nam.nam$l_node, (int)nam.nam$b_node);
    }
    if ((PARSE_DEVICE & flag) && nam.nam$b_dev && /* Device: */
		cur_len+nam.nam$b_dev < cp_len) {
	cur_len += nam.nam$b_dev;
	strncat(cp, nam.nam$l_dev, (int)nam.nam$b_dev);
    }

    /* Directory Name [] */

    if ((PARSE_DIRECTORY & flag) && nam.nam$b_dir &&
		cur_len+nam.nam$b_dir < cp_len) {
	int i; char * tmp;
        q = nam.nam$l_dir;		/* The directory name from RMS */
	i = nam.nam$b_dir;		/* Length; string not nul-terminated */
	debug(F111,"zltor nam$_dir",q,i);
	if (!q) q = "[]";
	if (!*q) q = "[]";
	if (i < 0) i = 0;
	tmp = NULL;
	if (i > 0) {			/* Copy directory part */
	    if (tmp = malloc(i+1)) {
		p = tmp;
		for ( ; i > 0 ; i--)
		  *p++ = *q++;
		*p = NUL;
	    }
	}
	q = tmp;
	debug(F111,"zltor directory part",q,i);

	s = zgtdir();			/* Get current directory */
	debug(F110,"zltor zgtdir",s,0);
	if (!s) s = "[]";
	if (!*s) s = "[]";
	s2 = "";
	while (*s && *s != '[')
	  s++;
	if (*s) {
	    s2 = s+1;
	    while (*s2 && *s2 != ']') s2++; /* Closing bracket */
	}
	if (!*s)
	  s = "[]";
	else
	  if (*s2) if (!*(s2+1)) *(s2+1) = NUL;
	debug(F110,"zltor current dir",s,0);

/* First change the VMS pathname to relative format if fnspath == PATH_REL */

	p = dirbuf;			/* Result */
	*p++ = *q++;			/* Copy left bracket and... */

	s++;				/* Point past it */
	q2 = q;				/* Remember this place */
	if (fnspath == PATH_REL) {	/* Compare this and current dir */
	    while (*s == *q && *s && *q && *s != ']') {
		s++;
		q++;
	    }
	}
	if (*s != ']' && *q != ']' && *q != '.') /* No match */
	  q = q2;			/* So rewind source pointer */

	while (*q) *p++ = *q++;		/* Now copy the rest */
	*p = NUL;
	debug(F110,"zltor result 1",dirbuf,0);
/*
   VMS directory name is now in dirbuf in either absolute or relative format.
   Now change it to standard (UNIX) format if desired.
*/
	p = dirbuf;			/* Working pointer */
	r = dirbuf;			/* Result pointer */
	if (fncnv) {			/* Converting directory format */
	    int flag = 0;
	    if (p[1] == '.') {		/* Directory name is relative */
		r += 2;			/* Point past the leading dot */
		p += 2;
	    }
	    while (*p) {		/* Now convert the rest */
		if (*p == '.' || *p == '[' || *p == ']') {
		    if (!flag) *p = '/';
		    if (*p == ']')
		      flag = 1;
		}
		p++;
	    }
	}
	debug(F110,"zltor result 2",r,0);
	if (tmp) free(tmp);
	strncat(cp, r, (int)nam.nam$b_dir);
	cur_len += strlen(r);
    }
    if ((PARSE_NAME & flag) && nam.nam$b_name &&
		cur_len+nam.nam$b_name < cp_len) {
	cur_len += nam.nam$b_name;
	strncat(cp, nam.nam$l_name, (int)nam.nam$b_name);
    }
    if ((PARSE_TYPE & flag) && nam.nam$b_type &&
		cur_len+nam.nam$b_type < cp_len) {
	cur_len += nam.nam$b_type;
	strncat(cp, nam.nam$l_type, (int)nam.nam$b_type);
    }
    if ((PARSE_VERSION & flag) && nam.nam$b_ver &&
		cur_len+nam.nam$b_ver < cp_len) {
	cur_len += nam.nam$b_ver;
	strncat(cp, nam.nam$l_ver, (int)nam.nam$b_ver);
    }
    return(cur_len);
}
#endif /* COMMENT */

/* Z G P E R M  --  Returns the permissions (protection) of the given file. */

static char zgpbuf[24];

char *
zgperm(f) char *f; {
    int x, x1, x2, x3, x4;
    struct FAB fab_perm;
    struct NAMX nam_perm;
    struct XABFHC xabfhc_perm;
    struct XABPRO xabpro_perm;
    struct XABDAT xabdat_perm;
    long iflen = -1L;

    fab_perm = cc$rms_fab;              /* Initialize the FAB. */
    nam_perm = CC_RMS_NAMX;             /* Initialize the NAM[L]. */
    fab_perm.FAB_L_NAMX = &nam_perm;    /* Point the FAB to the NAM[L]. */

    /* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
    fab_perm.fab$l_dna = (char *) -1;  /* Using NAML for default name. */
    fab_perm.fab$l_fna = (char *) -1;  /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
    FAB_OR_NAML( fab_perm, nam_perm).FAB_OR_NAML_FNA = f;
    FAB_OR_NAML( fab_perm, nam_perm).FAB_OR_NAML_FNS = strlen( f);

    fab_perm.fab$b_fac = FAB$M_BIO;
    fab_perm.fab$l_xab = (char *)&xabdat_perm;
    xabdat_perm = cc$rms_xabdat;
    xabdat_perm.xab$l_nxt = (char *)&xabfhc_perm;
    xabfhc_perm = cc$rms_xabfhc;
    xabfhc_perm.xab$l_nxt = (char *)&xabpro_perm;
    xabpro_perm = cc$rms_xabpro;
    rms_sts = sys$open(&fab_perm);
    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
    if (rms_sts == RMS$_PRV)
      return(NULL);
    if (rms_sts != RMS$_NORMAL) {
	debug(F111,"zperm $open failed, status",f,rms_sts);
	return(NULL);
    }
    rms_sts = sys$close(&fab_perm);
    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
    if (rms_sts != RMS$_NORMAL) {
	debug(F101,"zgperm $close failed, status","",rms_sts);
	return(NULL);
    }
    debug(F111,"zgperm xabpro_perm.xab$w_pro",f,xabpro_perm.xab$w_pro);
    sprintf(zgpbuf,"%04X",xabpro_perm.xab$w_pro); /* Convert to hex string */
    debug(F110,"zgperm hex",zgpbuf,0);
    return(zgpbuf);
}

static char zgibuf[24];

char *
ziperm(f) char * f; {
    char * p, pbuf[6];
    int i, x;
    char c;
    p = zgperm(f);
    if (!p)
      return("(,,,)");
    if ((int)strlen(p) != 4)
      return("(,,,)");
    ckstrncpy(pbuf,p,6);
    p = zgibuf;
    *p = '\0';
    for (i = 3; i >= 0; i--) {
	c = pbuf[i];
	if (i < 3)
	  *p++ = ',';
	else
	  *p++ = '(';
	if (c >= '0' && c <= '9')
	  x = c - '0';
	else if (c >= 'A' && c <= 'F')
	  x = c - 'A' + 10;
	else if (c >= 'a' && c <= 'f')
	  x = c - 'a' + 10;
	else
	  return(NULL);
	if (!(x & XAB$M_NOREAD))  *p++ = 'R';
	if (!(x & XAB$M_NOWRITE)) *p++ = 'W';
	if (!(x & XAB$M_NOEXE))   *p++ = 'E';
	if (!(x & XAB$M_NODEL))   *p++ = 'D';
    }
    *p++ = ')';
    *p = '\0';
    debug(F110,"ziperm",zgibuf,0);
    return((char *) zgibuf);
}

/*  Z S A T T R  --  Fill in a Kermit attribute structure for current file.  */

/*
 Fills in a Kermit file attribute structure for the file which is to be sent.
 Returns 0 on success with the structure filled in, or -1 on failure.
 If any string member is null, then it should be ignored.
 If any numeric member is -1, then it should be ignored.
*/
int
zsattr(xx) struct zattr *xx; {
    CK_OFF_T k;
    int x;
    static char mth[13][4] = {
	"JAN","FEB","MAR","APR",
	"MAY","JUN","JUL","AUG",
	"SEP","OCT","NOV","DEC",
	""
    };
    static char recfm[15];	    /* Record format */
    static char cdate[20];          /* Creation date [yy]yymmdd[hh:mm[:ss]] */
    static char creater_id[31];	    /* Creator ID string */
    static char genprot;            /* Generic protection */
    static char gpbuf[2];	    /* String representation thereof */
    static unsigned short lclprot;  /* Local protection */
    static char lpbuf[32];	    /* String representation thereof */
    static long sysparam_size=0;    /* Length of system paramater buffer */
    static char *sysparam_adr=0;    /* Address of system paramater buffer */
    char type;			    /* File type */
    short int asctim_retlen;
    char asctim_buf[24];	    /* Work buffer for ASCTIM() */
    struct dsc$descriptor_s
      asctim_dsc = {sizeof(asctim_buf),DSC$K_DTYPE_T,DSC$K_CLASS_S,NULL};
/*  static long int i;  */
    static unsigned short id_len;
    static struct dsc$descriptor_s id_str =
      {31,DSC$K_DTYPE_T,DSC$K_CLASS_S,creater_id};

/* Zero out strings */

    type = 0;
    recfm[0] = '\0';
    cdate[0] = '\0';
    creater_id[0] = '\0';
    id_len = 0;
    genprot = 0;			/* Blank protections by default */
    gpbuf[0] = '\0';
    lclprot = 0;
    lpbuf[0] = '\0';
    asctim_dsc.dsc$a_pointer = (char *)&asctim_buf;

/* See if we are sending "attributes" from a REMOTE command response */

    if (*nambuf == '\0') {
	xx->lengthk = 1;		/* Number of 1K blocks rounded up */
	xx->type.len = 0;		/* File type can't be filled in here */
	xx->type.val = "";
	xx->date.len = strlen(cdate);	/* File creation date */
	xx->date.val = (char *)&cdate;
	xx->creator.len = strlen(creater_id); /* File creator */
	xx->creator.val = (char *)&creater_id;
	xx->account.len = 0;		/* File account */
	xx->account.val = "";
	xx->area.len = 0;		/* File area */
	xx->area.val = "";
	xx->password.len = 0;		/* Area password */
	xx->password.val = "";
	xx->blksize = -1L;		/* File blocksize */
	xx->xaccess.len = 0;		/* File access */
	xx->xaccess.val = "";
	xx->encoding.len = 1;		/* Transfer syntax */
	xx->encoding.val = "A";		/* ASCII */
	xx->disp.len = 0;		/* Disposition upon arrival */
	xx->disp.val = "";
	xx->lprotect.len = strlen(lpbuf); /* Local protection */
	xx->lprotect.val = (char *)lpbuf;
	xx->gprotect.len = strlen(gpbuf); /* Generic protection */
	xx->gprotect.val = (char *)gpbuf;
	xx->systemid.len = 2;		/* System ID for DEC VMS */
	xx->systemid.val = "D7";
	xx->recfm.len = strlen(recfm);	/* Record format */
	xx->recfm.val = (char *)&recfm;
	xx->sysparam.len = sysparam_size; /* System-dependent parameters */
	xx->sysparam.val = sysparam_adr;
	xx->length = 1;			/* Length */
	return(0);			/* mumble sweet nothings at it */
    }

/* Load the generic protection */

#ifdef COMMENT
    x = xabpro_ifile.xab$w_pro >> XAB$V_WLD;	/* World protection */
#else
    x = (xabpro_ifile.xab$w_pro >> 4) & 0xFF;	/* Owner protection */
#endif /* COMMENT */
    if (!(x & XAB$M_NOREAD))  genprot |= 1;	/* Read */
    if (!(x & XAB$M_NOWRITE)) genprot |= 2+8;	/* Write (and append) */
    if (!(x & XAB$M_NOEXE))   genprot |= 4;	/* Execute */
    if (!(x & XAB$M_NODEL))   genprot |= 16;	/* Delete */
    gpbuf[0] = tochar(genprot);		        /* Generic prot as string */
    gpbuf[1] = '\0';
    lclprot = xabpro_ifile.xab$w_pro;		/* Local-format protection */
    sprintf(lpbuf,"%04X",lclprot);		/* Convert to hex string */

/* Convert creation date from an internal value to common ascii string */

    sys$asctim(&asctim_retlen,&asctim_dsc,&xabdat_ifile.xab$q_cdt,0);
    asctim_buf[asctim_retlen] = '\0';
    debug(F110,"zsattr asctim_buf",asctim_buf,0);
    for (x = 0; strncmp(mth[x], asctim_buf+3,3); x++) /* Find month */
      ;
    strncpy(cdate,asctim_buf+7,4);	/* 'yyyy' */
    sprintf(cdate+4,"%02d",x+1);	/* 'mm' */
    strncpy(cdate+6,asctim_buf+0,2);	/* 'dd' */
    strncpy(cdate+8,asctim_buf+11,9);	/* ' hh:mm:ss' */
    if (cdate[6] == ' ')
	cdate[6] = '0';
    debug(F110,"zsattr cdate",cdate,0);

/* Convert the owner UIC into an alpha name */

    creater_id[0] = '\0';
    rms_sts = sys$idtoasc(xabpro_ifile.xab$l_uic,&id_len,&id_str,0,0,0);
    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
    creater_id[id_len] = '\0';			/* terminating null, please */
    debug(F111,"zsattr $idtoasc owner",creater_id,strlen(creater_id));
    if (rms_sts == SS$_NOSUCHID ||
#ifdef SS$_NORIGHTSDB	/* only vms 5 and higher */
	rms_sts == SS$_NORIGHTSDB ||
#endif /* SS$_NORIGHTSDB */
	rms_sts == SS$_IVIDENT) {
	creater_id[0] = '\0';
	rms_sts = SS$_NORMAL;			/* if unknown, null it out */
    }
    if (!(rms_sts & 1)) {
	debug(F101,"zsattr $idtoasc failed, status","",rms_sts);
	return(-1);				/* fatal */
    }

/* Fill in the record format blockette */

    if (fab_ifile.fab$b_rat & (FAB$M_CR | FAB$M_FTN | FAB$M_PRN)) {
	strcpy(recfm,"AMJ");
    } else {
	strcpy(recfm,"F");
	sprintf(recfm+1,"%05d",xabfhc_ifile.xab$w_lrl);
    }
    debug(F111,"zsattr recfm",recfm,strlen(recfm));

/* Fill in the returned data structure */

    xx->lengthk = (iflen/1024)+1;	/* Number of 1K blocks rounded up */
    xx->type.len = 0;			/* File type can't be filled in here */
    xx->type.val = "";
    xx->date.len = strlen(cdate);	/* File creation date */
    xx->date.val = (char *)&cdate;
    xx->creator.len = strlen(creater_id); /* File creator */
    xx->creator.val = (char *)&creater_id;
    xx->account.len = 0;		/* File account */
    xx->account.val = "";
    xx->area.len = 0;			/* File area */
    xx->area.val = "";
    xx->password.len = 0;		/* Area password */
    xx->password.val = "";
    xx->blksize = -1L;			/* File blocksize */
    xx->xaccess.len = 0;		/* File access */
    xx->xaccess.val = "";
    xx->encoding.len = 1;		/* Transfer syntax */
    xx->encoding.val = "A";		/* ASCII */
    xx->disp.len = 0;			/* Disposition upon arrival */
    xx->disp.val = "";
    xx->lprotect.len = strlen(lpbuf);	/* Local protection */
    xx->lprotect.val = (char *)lpbuf;
    xx->gprotect.len = strlen(gpbuf);	/* Generic protection */
    xx->gprotect.val = (char *)gpbuf;
    xx->systemid.len = 2;		/* System ID for (Open)VMS */
    xx->systemid.val = "D7";
    xx->recfm.len = strlen(recfm);	/* Record format */
    xx->recfm.val = (char *)&recfm;
    xx->sysparam.len = sysparam_size;	/* System-dependent parameters */
    xx->sysparam.val = sysparam_adr;
    xx->length = iflen;			/* Length */
    debug(F111,"zsattr lengthk","",xx->lengthk);
    debug(F111,"zsattr length","",xx->length);
    return(0);
}

/* Z M K D I R  --  Create directory(s) if necessary */
/*
   Call with:
     A pointer to a file specification that might contain directory
     information.  The filename is expected to be included.
     If the file specification does not include any directory separators,
     then it is assumed to be a plain file.
     If one or more directories are included in the file specification,
     this routine tries to create them if they don't already exist.
   Returns:
     0 on success, i.e. the directory was created, or didn't need to be.
    -1 on failure to create the directory
   VMS version by Mark Berryman, Feb 94.
*/
int
zmkdir(path) char *path; {
    struct FAB dir_fab;
    struct NAMX dir_nam;
    struct dsc$descriptor_s expanded_filename;

    char expanded_name[ NAMX_C_MAXRSS];

    dir_fab = cc$rms_fab;               /* Initialize the FAB. */
    dir_nam = CC_RMS_NAMX;              /* Initialize the NAM[L]. */
    dir_fab.FAB_L_NAMX = &dir_nam;      /* Point the FAB to the NAM[L]. */

    /* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
    dir_fab.fab$l_dna = (char *) -1;  /* Using NAML for default name. */
    dir_fab.fab$l_fna = (char *) -1;  /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
    FAB_OR_NAML( dir_fab, dir_nam).FAB_OR_NAML_FNA = path;
    FAB_OR_NAML( dir_fab, dir_nam).FAB_OR_NAML_FNS = strlen( path);

    dir_nam.NAMX_L_ESA = expanded_name;
    dir_nam.NAMX_B_ESS = sizeof( expanded_name);

    rms_sts = sys$parse(&dir_fab,0,0);
    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
    switch (rms_sts) {
        case RMS$_NORMAL :
	    debug(F110,"zmkdir path already exists",path,0);
	    return 0;

        case RMS$_FNM :			/* E.g. too long... (99628) */
	    debug(F110,"zmkdir bad filename",path,0);
	    return 0;			/* Let zrtol() handle it. */

        case RMS$_DNF :			/* Directory Not Found, create it */
            debug(F110,"zmkdir RMS$_DNF",path,0);
            break;

        default :			/*  Any other result is fatal. */
            debug(F111,"zmkdir status",path,rms_sts);
	    return -1;
    }

/*  The parse succeeded but said the directory didn't exist, so create it. */
    expanded_filename.dsc$b_class = DSC$K_CLASS_S;
    expanded_filename.dsc$b_dtype = DSC$K_DTYPE_T;
    expanded_filename.dsc$a_pointer = expanded_name;
    expanded_filename.dsc$w_length = dir_nam.NAMX_B_ESL;

/* Locate the closing bracket, ']' or '>', using the $PARSE results */
/* node names are not allowed, but let lib$create_dir return the error */

    expanded_filename.dsc$w_length = dir_nam.NAMX_B_NODE +
                                     dir_nam.NAMX_B_DEV +
                                     dir_nam.NAMX_B_DIR;

    rms_sts = lib$create_dir(&expanded_filename,0,0,0,0,0);
    expanded_name[expanded_filename.dsc$w_length] = '\0';
    debug(F111,"zmkdir create_dir status",(char *)&expanded_name, rms_sts);
    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
    CHECK_ERR("zmkdir create error", rms_sts);
    return (rms_sts == SS$_CREATED ? 1 : -1);
}

/*  Z R M D I R  --  Delete a directory  */

int
zrmdir(path) char *path; {
    int i;
    int status;
    char *cp;
    char *dir_begin;
    char *dir_end;
    char *dot_last;
    struct FAB fab_dir;
    struct NAMX nam_dir;
    char exp_name[ NAMX_C_MAXRSS];
    char dir_name[ NAMX_C_MAXRSS];

    /* Special ODS5-QIO-compatible name storage. */
#ifdef NAML$C_MAXRSS
    char sys_name[ NAML$C_MAXRSS];
#endif /* def NAML$C_MAXRSS */

    int atr_devchn;                     /* Disk device channel. */
    struct fibdef atr_fib;              /* File Information Block. */
    unsigned short atr_prot;            /* Protection attributes. */

    int lb = 0;                         /* Left bracket count. */
    int rb = 0;                         /* Right bracket count. */

    /* Disk device name descriptor. */
    struct dsc$descriptor_s atr_devdsc =
     { 0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL };

    /* File Information Block descriptor. */
    struct dsc$descriptor atr_fibdsc =
     { sizeof( atr_fib), DSC$K_DTYPE_Z, DSC$K_CLASS_S, NULL };

    /* File name descriptor. */
    struct dsc$descriptor_s atr_fnam =
     { 0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL };

    /* IOSB for QIO[W] miscellaneous ACP operations. */
    struct
    {
        unsigned short  status;
        unsigned short  dummy;
        unsigned int    count;
    } atr_acp_iosb;

    /* Attribute item list: file protection, terminator. */
    struct atrdef atr_atr[ 2] =
     { { ATR$S_FPRO, ATR$C_FPRO, GVTC NULL },
       { 0, 0, GVTC NULL }
     };

    /* Execution begins. */

    /* Make safe, then debug-display path argument. */
    if (path == NULL)
        path = "";
    debug(F110, "zrmdir path 1", path, 0);

    /* Quit early on a null path. */
    if (!*path)
        return(-1);

    /* Convert (plain) "dir_name" to "[.dir_name]". */
    for (cp = path; *cp != '\0'; cp++)
    {
        if (*cp == '^')                 /* Skip caret-escaped characters. */
        {
            cp++;
            continue;
        }
	if (*cp == '[')                 /* Count left brackets. */
            lb++;
	else if (*cp == ']')            /* Count right brackets. */
            rb++;
    }

    /* If bare "name", then construct (relative) VMS dir spec, "[.name]". */
    if (lb != 1 && rb != 1 && cp > path && *(cp- 1) != ':')
    {
	sprintf( dir_name, "[.%s]", path);
        path = dir_name;
    }
    debug(F110, "zrmdir path 2", path, 0);

    fab_dir = cc$rms_fab;               /* Initialize the FAB. */
    nam_dir = CC_RMS_NAMX;              /* Initialize the NAM[L]. */
    fab_dir.FAB_L_NAMX = &nam_dir;      /* Point the FAB to the NAM[L]. */

    /* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
    fab_dir.fab$l_dna = (char *) -1;  /* Using NAML for default name. */
    fab_dir.fab$l_fna = (char *) -1;  /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
    FAB_OR_NAML( fab_dir, nam_dir).FAB_OR_NAML_FNA = path;
    FAB_OR_NAML( fab_dir, nam_dir).FAB_OR_NAML_FNS = strlen( path);

    /* Expanded name storage. */
    nam_dir.NAMX_L_ESA = exp_name;
    nam_dir.NAMX_B_ESS = sizeof( exp_name);

    status = sys$parse( &fab_dir);
    if (!(status& 1)) vms_lasterr = status;
    if (status != RMS$_NORMAL)		/* Does not exist */
        return(-1);
    debug(F100, "zrmdir directory exists", exp_name, 0);

#define DIR_TYPE  ".DIR;1"

    /* Transform "[a.b]" to "[a]b.DIR;1".
     * First, find the last true "." in the directory field.
     */
    dir_begin = nam_dir.NAMX_L_DIR;
    dir_end = dir_begin- 1+ nam_dir.NAMX_B_DIR;
    dot_last = NULL;
    for (cp = dir_end- 1; cp > dir_begin; cp--)
    {
        if (*cp == '.')
        {
            if (*(cp- 1) == '^')        /* Skip an escaped dot. */
            {
                cp--;
            }
            else
            {
                dot_last = cp;          /* Found it. */
                break;
            }
        }
    }

    /* Construct a file spec for the directory file. */
    if (dot_last == NULL)
    {
        /* No last dot.  "[a]" -> "[000000]a.DIR;1". */
        dir_begin++;                     /* Now first char after "[". */
        cp = dir_begin;
        strncpy( dir_name, exp_name, (cp- exp_name));   /* "dev:[". */
        cp = dir_name+ (cp- exp_name);
        strncpy( cp, "000000", 6);              /* "dev:[000000". */
        cp += 6;
        strncpy( cp, dir_end, 1);               /* "dev:[000000]". */
        cp += 1;
        strncpy( cp, dir_begin, (dir_end- dir_begin));  /* "dev:[000000]a". */
        cp += dir_end- dir_begin;
        strcpy( cp, DIR_TYPE);                  /* "dev:[000000]a.DIR;1". */
    }
    else
    {
        /* Last dot found.  "[a.b]" -> "[a]b.DIR;1". */
        *dot_last = *dir_end;                   /* "dev:[a]b". */
        strcpy( dir_end, DIR_TYPE);             /* "dev:[a]b.DIR;1". */
        strcpy( dir_name, exp_name);
    }

    debug(F100, "zrmdir dir file spec", dir_name, 0);
    debug(F101, "zrmdir DECnet =", "", nam_dir.NAMX_V_NODE);

    /* Change protection on the directory file, then delete it. */

    if (nam_dir.NAMX_V_NODE != 0)
    {
        /* DECnet node name detected.  QIO won't work, so use the
         * (fossil) "system()" scheme.  First, SET PROT.
         * ("OWNER:RWED"?  Why not WORLD:RWED?  What if we don't own it?)
         * It's not immediately obvious that SET PROT works any better
         * than QIO on a DECnet file spec, so it may make more sense to
         * discard all this old system() code, and just use the QIO
         * scheme on local files, and delete() on everything.
         * (Who says that "delete() won't do it"?)
         */
	sprintf( exp_name, "set protection=owner:RWED %s", dir_name);
	status = system( exp_name);
	debug(F111, "zrmdir system() status ", exp_name, status);
#ifdef COMMENT
/* If this failed it doesn't necessarily mean there is no Delete access */
	if ((status& 1) != 1)
	  return(-1);
#endif /* COMMENT */
	sprintf( exp_name, "delete %s", dir_name); /* delete() won't do it. */
	status = system( exp_name);
	debug(F111, "zrmdir system() status ", exp_name, status);
	if ((status& 1) != 1)
	  return(-1);
	return(0);
    }
    else
    {
        /* No DECnet node name detected.  Use QIO to set protection. */
        fab_dir = cc$rms_fab;           /* Initialize the FAB. */
        nam_dir = CC_RMS_NAMX;          /* Initialize the NAM[L]. */
        fab_dir.FAB_L_NAMX = &nam_dir;  /* Point the FAB to the NAM[L]. */

        /* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
        fab_dir.fab$l_dna = (char *) -1;    /* Using NAML for default name. */
        fab_dir.fab$l_fna = (char *) -1;    /* Using NAML for file name. */

        /* Special ODS5-QIO-compatible name storage. */
        nam_dir.naml$l_filesys_name = sys_name;
        nam_dir.naml$l_filesys_name_alloc = sizeof( sys_name);
#endif /* def NAML$C_MAXRSS */
        FAB_OR_NAML( fab_dir, nam_dir).FAB_OR_NAML_FNA = dir_name;
        FAB_OR_NAML( fab_dir, nam_dir).FAB_OR_NAML_FNS = strlen( dir_name);

        /* Expanded name storage. */
        nam_dir.NAMX_L_ESA = exp_name;
        nam_dir.NAMX_B_ESS = sizeof( exp_name);

        status = sys$parse( &fab_dir);
        if ((status& STS$M_SEVERITY) == STS$M_SUCCESS)
        {
            debug(F100, "zrmdir dir file exists", exp_name, 0);

            /* Set the address in the device name descriptor. */
            atr_devdsc.dsc$a_pointer = &nam_dir.NAMX_T_DVI[ 1];

            /* Set the length in the device name descriptor. */
            atr_devdsc.dsc$w_length = (unsigned short) nam_dir.NAMX_T_DVI[ 0];

            /* Open a channel to the disk device. */
            status = sys$assign( &atr_devdsc, &atr_devchn, 0, 0);
            if ((status& STS$M_SEVERITY) == STS$M_SUCCESS)
            {
                debug(F011, "zrmdir assign disk",
                 atr_devdsc.dsc$a_pointer, atr_devdsc.dsc$w_length);

                /* Move the directory ID from the NAM[L] to the FIB.
                   Clear the FID in the FIB, as we're using the name.
                */
                for (i = 0; i < 3; i++)
                {
                    atr_fib.FIB_W_DID[ i] = nam_dir.NAMX_W_DID[ i];
                    atr_fib.FIB_W_FID[ i] = 0;
                }

#ifdef NAML$C_MAXRSS

                /* Enable fancy name characters.  Note that "fancy" here does
                   not include Unicode, for which there's no support elsewhere.
                */
                atr_fib.fib$v_names_8bit = 1;
                atr_fib.fib$b_name_format_in = FIB$C_ISL1;

                /* ODS5 Extended names used as input to QIO have peculiar
                   encoding (perhaps to minimize storage?), so the special
                   filesys_name result (typically containing fewer carets) must
                   be used here.
                */
                atr_fnam.dsc$a_pointer = nam_dir.naml$l_filesys_name;
                atr_fnam.dsc$w_length = nam_dir.naml$l_filesys_name_size;

#else /* def NAML$C_MAXRSS */

                /* ODS2-only: Use the whole name. */
                atr_fnam.dsc$a_pointer = nam_dir.NAMX_L_NAME;
                atr_fnam.dsc$w_length =
                 nam_dir.NAMX_B_NAME+ nam_dir.NAMX_B_TYPE+ nam_dir.NAMX_B_VER;

#endif /* def NAML$C_MAXRSS [else] */

                /* Set the address in the FIB descriptor. */
                atr_fibdsc.dsc$a_pointer = (void *) &atr_fib;

                /* Set the address of the prot word in the prot descriptor. */
                atr_atr[ 0].atr$l_addr = GVTC &atr_prot;

                /* Fetch the file (directory) protection attributes. */
                status = sys$qiow( 0,               /* event flag */
                                   atr_devchn,      /* channel */
                                   IO$_ACCESS,      /* function code */
                                   &atr_acp_iosb,   /* IOSB */
                                   0,               /* AST address */
                                   0,               /* AST parameter */
                                   &atr_fibdsc,     /* P1 = File Info Block */
                                   &atr_fnam,       /* P2 = File name */
                                   0,               /* P3 = Rslt nm len */
                                   0,               /* P4 = Rslt nm str */
                                   atr_atr,         /* P5 = Attributes */
                                   0);              /* P6 (not used) */

                debug(F101, "zrmdir access status", "", status);

                /* Mask out or clear No-Delete bits. */
                if ((status & STS$M_SEVERITY) == STS$K_SUCCESS)
                {
                    atr_prot &= 0x7777;     /* Add (S:D, O:D, G:D, W:D) */
                }
                else
                {
                    atr_prot = 0x7777;      /* Set (S:D, O:D, G:D, W:D) */
                }

                /* Try to modify the file (directory) attributes. */
                status = sys$qiow( 0,               /* event flag */
                                   atr_devchn,      /* channel */
                                   IO$_MODIFY,      /* function code */
                                   &atr_acp_iosb,   /* IOSB */
                                   0,               /* AST address */
                                   0,               /* AST parameter */
                                   &atr_fibdsc,     /* P1 = File Info Block */
                                   &atr_fnam,       /* P2 = File name */
                                   0,               /* P3 = Rslt nm len */
                                   0,               /* P4 = Rslt nm str */
                                   atr_atr,         /* P5 = Attributes */
                                   0);              /* P6 (not used) */

                debug(F101, "zrmdir modify status", "", status);
                if ((status & STS$M_SEVERITY) == STS$K_SUCCESS)
                {
                    status = atr_acp_iosb.status;
                    debug(F101, "zrmdir modify iosb status", "", status);
                }
            }
        }
        /* Finally, delete the thing. */
        status = delete( dir_name);
        if (status == 0)
            status = SS$_NORMAL;
        else
            status = vaxc$errno;
        debug(F101, "zrmdir delete status", "", status);
    }
    if ((status&1) != 1)
        return(-1);
    return(0);
}


/*  Z M A I L  --  Send file f as mail to address p.  */
/*
  Returns 0 on success
   2 if mail delivered but temp file can't be deleted
  -2 if mail can't be delivered
*/
int
zmail(p,f) char *p; char *f; {
    char *zmbuf;
    static char spbuf[] = "$ mail %s %s/subj=\"Enclosed file %s\"";
    static char spbuf2[] = "%s;";
    unsigned long int sts;

    zmbuf = malloc(strlen(p)+(2*strlen(f))+sizeof(spbuf));
    sprintf(zmbuf,spbuf, f, p, f);
    sts = system(zmbuf);
    debug(F111,"zmail: system returns status ",zmbuf,sts);
    free(zmbuf);
    if ((sts&1) != 1) {
	debug(F101,"zmail: returning","",-2);
	return(-2);
    }
    zmbuf = malloc(strlen(f)+sizeof(spbuf2));
    sprintf(zmbuf,spbuf2, f);
    sts = delete(zmbuf);
    debug(F111,"zmail: delete returns status ",zmbuf,sts);
    free(zmbuf);
    if (sts) sts = 2;
    debug(F101,"zmail: returning","",sts);
    return(sts);
}

/* Z P R I N T  --  Print file f with options p.  */
/*
  Returns 0 on success, -3 on failure.
*/
int
zprint(p,f) char *p; char *f; {
    char *zmbuf;
    static char spbuf[] = "$ print/delete %s %s";
    unsigned long int sts;

    zmbuf = malloc(strlen(p)+strlen(f)+sizeof(spbuf));
    sprintf(zmbuf,spbuf, p, f);
    sts = system(zmbuf);
    debug(F111,"zprint: system returns status ",zmbuf,sts);
    free(zmbuf);
    debug(F101,"zprint: returning","",(sts&1) ? 0 : -3);
    return((sts&1) ? 0 : -3);
}

/* Z S Y S C M D  --  Execute a DCL command with direct output.  */

/*
 * Since it's really difficult to have an alternate CLI under VMS (since the
 * MCR interface isn't documented and POSIX hasn't published the interface,
 * we'll just assume everybody uses DCL and hand it of to zshcmd().
 */
int
zsyscmd(s) char *s; {
    if (nopush) {
	debug(F100,"zsyscmd fails: nopush","",0);
	return(-1);
    }
    return(zshcmd(s));
}

/*
 * Dummy function, since SIG_DFL seems to be broken with DEC C
 */
VOID
sig_dum() {

}

/* Z S H C M D  --  Execute a default CLI command with direct output.  */

/*
 * As it's _REALLY_ unlikely that the user is using MCR as his default CLI,
 * and DEC doesn't document how to write any other alternate CLIs, use DCL.
 */

#ifndef	SS$_EXPRCLM		/* VMS doesn't return this yet, but let's */
#define SS$_EXPRCLM 10804	/* be forward-thinking and anticpate VMS */
#endif /* SS$_EXPRCLM */	/* V6.0, which will return it. */

int
zshcmd(s) char *s; {
    unsigned long sts, cc;
    void (*cct)();
    void (*sig_dum_ptr)() = sig_dum;
    struct dsc$descriptor_s
      cmd_line = {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0};
    char *i;

    pexitstat = -1;
    if (nopush) {
	debug(F100,"zshcmd fails: nopush","",0);
	return(-1);
    }
    if (!s) s = "";
    debug(F110,"zshcmd",s,0);
    if (check_spawn() != 0) {
	debug(F100,"zshcmd: spawning prohibited by UAF flags","",0);
	return(0);
    }

    i = strstr(zgtdir(),"::");
    if (i != NULL) {
        debug(F100,"zshcmd: spawn prohibited on remote node","",0);
        printf("Cannot SPAWN with remote node as default directory\n");
 	if (*s)
	  printf("therefore, cannot execute the DCL command \"%s\"\n", s);
        return(0);
    }

    cct = signal(SIGINT,sig_dum_ptr);	/* Let inferior process catch ^C */

    cmd_line.dsc$w_length = strlen(s);
    cmd_line.dsc$a_pointer = s;

    if (!(*s))
      printf("Type LOGOUT to return to VMS C-Kermit.\n\n");
    sts = lib$spawn(&cmd_line, 0, 0, 0, 0, 0, &cc, 0, 0, 0, 0, 0);
    if (!(sts & 1)) vms_lasterr = sts;
    signal(SIGINT,cct);
/*
 * Note: We can't check for this beforehand as doing a getjpi for prclm will
 *	 only return the UAF value, not the available value. So we try it and
 *	 print this message if it didn't work.
 */
    if ((sts == SS$_EXQUOTA) || (sts == SS$_EXPRCLM)) {
	printf(
"Your account does not have sufficient quotas to use this command.\n");
	printf(
"Please ask your system manager to increase your UAF PRCLM quota.\n");
    }
    debug(F101,"zshcmd lib$spawn sts", "", sts);
    debug(F101,"zshcmd lib$spawn cc ", "", cc);
    debug(F101,"zshcmd lib$spawn SS$_NORMAL", "", SS$_NORMAL);
    if (sts == SS$_NORMAL) {
	pexitstat = cc;
	return((cc & 1) ? 1 : 0);	/* Success */
    } else {
	return(0);			/* Failure */
    }
}

/*  Z S T R I P  --  Strip device & directory name from file specification.  */

/*  Strip pathname from filename "name", return pointer to result in name2 */

static char work[1100];	/* buffer for use by zstrip and zltor */

VOID
zstrip(name,name2) char *name, **name2; {
    char *cp, *pp;
    char last;
    int len;
    debug(F110,"zstrip entry",name,0);
    pp = work;				/* Default return is empty string */
    *name2 = work;
    *pp = '\0';
    if (!name)
      return;
    if (!*name)
      return;

/*  NODE::DEV:[DIR] terminates on on final ':', '>' or ']'.  */

    for (cp = name; *cp; cp++) {
	last = *cp;
    	if (*cp == '/' || *cp == ':' || *cp == '>' || *cp == ']') /* slash? */
	  pp = work;
	else if (*cp == ';')		/* Chop off any version number */
	  break;
	else				/* Part of filename */
	  *pp++ = *cp;
    }
    *pp = '\0';				/* Terminate the string */
    debug(F000,"zstrip 2",work,last);
#ifdef COMMENT
/*
  This is a bad idea because the result will need to be passed thru zstrip()
  again, but zstrip() isn't designed to call itself.
*/
    if (work[0] == '\0' && last == ':') { /* Result is empty? */
	char * q;			/* Maybe it's a logical name */
	q = (char *)malloc(1100);
	if (q) {
	    ckstrncpy(q,name,1100);
	    len = strlen(q);
	    if (len > 0) {
		if (q[len-1] == ':') {
		    char *t = q;
		    q[len-1] = '\0';
		    while (*t) { if (islower(*t)) *t = toupper(*t); t++; }
		    debug(F110,"zstrip checking",q,0);
		    pp = getenv(q);
		    if (!pp) pp = "";
		    ckstrncpy(work,pp,1100);
		    debug(F110,"zstrip getenv",work,0);
		}
	    }
	    free(q);
	}
    }
    debug(F110,"zstrip 3",work,0);
#endif /* COMMENT */

/* The following should allow us to receive files to LPT:, LTA1:, etc. */

    if (work[0] == '\0') {		/* Still empty? */
	debug(F000,"zstrip last",name,last); /* If it's a device name */
	if (last == ':')		/* put it back */
	  ckstrncpy(work,name,1100);
    }
    debug(F110,"zstrip result",*name2,0);
}

int
zchkpath(s) char *s; {
#ifdef COMMENT
/*
  This needs to be replaced with something more intelligent.
  The idea is to see if the file, whose specification is pointed to by s,
  is in the current directory.  This function should return 0 if it is,
  nonzero otherwise.  Presently we rely on being called with a full
  filespec of the form DISK:[DEV]NAME.TYP;V, so this works more or less
  by accident.  What we really need is to call some kind of VMS service
  to get the NODE::DEV:[DIR] of the file, and compare with the current
  NODE::DEV:[DIR].
*/
    char *p;
    p = zgtdir();			/* Get current dir. */
    debug(F110,"zchkpath file",s,0);
    debug(F110,"zchkpath current dir",p,0);
    return(strncmp(p,s,strlen(p)));	/* Compare it. */
#else /* def COMMENT */
/*
    The purpose of this routine is to determine whether the file specified
    by *s would be in the current default directory or not.

    Inputs:
	*s = filename to check, can be just a filename or full path

    Outputs:
	0 = in the current default directory
        nonzero = not in the current directory

    Algorithm:
	Uses SYS$PARSE.  As long as the current default directory is not
	on a remote DECnet node, SYS$PARSE will return the Directory ID
	of the directory where the parsed filename would be found.  This
	means all we need do is parse both the default directory and the
	filename passed to us and compare DIDs (as well as make sure the
	two DIDs reference the same disk).  If so, it doesn't matter how
	convoluted the filename passed to us is, or what logical names
	may be involved, SYS$PARSE will tell us with certainty if it
	resolves to the current directory or not.

	However, if the current default directory is on a remote DECnet
	node, this information is not available (since SYS$PARSE executes
	locally, not remotely).  In this case, SYS$PARSE will still expand
	the filename as much as possible and then we can simply compare
	the default directory with the directory reference returned by the
	parse of the filename in *s using a simple string compare.  Since
	we cannot evaluate remote logical names, this may cause a false
	negative to occur if the user is playing games with the filename,
	but it guarantees that we do not get a false positive.
 */
    unsigned int dd_len, status;
    struct FAB my_fab;
    struct NAMX dd_nam, s_nam;
    const char *default_dir = "dummy.name";	/* guaranteed to resolve to */
						/* the current default dir. */
    char dd_expanded_name[ NAMX_C_MAXRSS];
    char s_expanded_name[ NAMX_C_MAXRSS];

    my_fab = cc$rms_fab;                /* Initialize the FAB. */
    dd_nam = CC_RMS_NAMX;               /* Initialize the NAM[L]. */
    my_fab.FAB_L_NAMX = &dd_nam;        /* Point the FAB to the NAM[L]. */

    /* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
    my_fab.fab$l_dna = (char *) -1;  /* Using NAML for default name. */
    my_fab.fab$l_fna = (char *) -1;  /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
    FAB_OR_NAML( my_fab, dd_nam).FAB_OR_NAML_FNA = (char *)default_dir;
    FAB_OR_NAML( my_fab, dd_nam).FAB_OR_NAML_FNS = strlen( default_dir);

    dd_nam.NAMX_L_ESA = dd_expanded_name;
    dd_nam.NAMX_B_ESS = sizeof( dd_expanded_name);

/* PARSE the current default directory */
    status = sys$parse(&my_fab,0,0);
    if (!(status & 1)) {                  /* any $PARSE errors are fatal */
        return (status);
    }

    s_nam = CC_RMS_NAMX;
    s_nam.NAMX_L_ESA = s_expanded_name;
    s_nam.NAMX_B_ESS = sizeof( s_expanded_name);

    my_fab.FAB_L_NAMX = &s_nam;

    /* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
    my_fab.fab$l_dna = (char *) -1;  /* Using NAML for default name. */
    my_fab.fab$l_fna = (char *) -1;  /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
    FAB_OR_NAML( my_fab, s_nam).FAB_OR_NAML_FNA = s;
    FAB_OR_NAML( my_fab, s_nam).FAB_OR_NAML_FNS = strlen( s);

/* PARSE the filename in question */
    status = sys$parse(&my_fab,0,0);
    if (!(status & 1)) {                  /* any $PARSE errors are fatal */
        return (status);
    }

   if (dd_nam.NAMX_B_NODE == 0) {	/* default dir is not via DECnet */
	if (dd_nam.NAMX_T_DVI[0] != s_nam.NAMX_T_DVI[0])
		return (1);		/* disk name sizes do not match*/
	if (strncmp( &dd_nam.NAMX_T_DVI[1], &s_nam.NAMX_T_DVI[1],
			dd_nam.NAMX_T_DVI[0] ))
		return (1);		/* disk names do not match */

/*	At this point it is known that the current default directory is
	a local directory and that the device portion of the default
	directory and that of the filename passed to us are identical.
	Now compare the major and minor number of the DID (but not the RVN,
	we do not care if the result is on another disk of a bound volumset)
	of the two parse results.  If they are the same, the filename passed
	to us is definitely a reference to the default directory.
 */
	if ( (dd_nam.NAMX_W_DID_NUM == s_nam.NAMX_W_DID_NUM) &&
	     (dd_nam.NAMX_W_DID_SEQ == s_nam.NAMX_W_DID_SEQ) )
	     return (0);		/* in the same dir */
	return (1);			/* not the same dir */
   }

/*
   The default file spec contains a DECnet node reference.  SYS$PARSE does
   not return as much data when done across DECnet, mainly because the parse
   is still executed on the local node, not the remote node.  The result of
   the parse will still expand the string as much as possible however
   (.e.g., if one does a SET DEFAULT to NODE::DEV:[DIR] then a result of the
    parse of FILENAME.EXT will return NODE::DEV:[DIR]FILENAME.EXT).  Because
   of this, we simply compare the two resultant strings from the two SYS$PARSE
   operations in order to check the path of a file that contains a node name.
   This still has the benefit of no longer requiring that a full path name
   be passed to this routine.
 */
   dd_len =				/* only want node::dev:[dir] */
       dd_nam.NAMX_L_NAME - dd_nam.NAMX_L_NODE;
   dd_expanded_name[ dd_len ] = '\0';	/* terminate after the ] */
   s_expanded_name[ s_nam.NAMX_L_NAME - s_nam.NAMX_L_NODE ] = '\0';
   return (strncmp (dd_expanded_name, s_expanded_name, dd_len) );
#endif /* def COMMENT [else] */
}

#ifdef OLD_VMS
#ifndef VMS_V46    /* rename() included in VAXCRTL as of VMS v4.6*/

static VOID
descname(desc,name) struct dsc$descriptor_s *desc; char *name; {
    desc->dsc$w_length = strlen(name);	/* Length of name */
    desc->dsc$a_pointer = name;		/* Address */
    desc->dsc$b_class = DSC$K_CLASS_S;	/* String descriptor class */
    desc->dsc$b_dtype = DSC$K_DTYPE_T;	/* ASCII string data type */
}

/* VMS version of RENAME */
int /* ? */
rename(oldname, newname) char oldname[], newname[]; {
    struct dsc$descriptor_s old_desc, new_desc;
    int lib$rename_file();

    /* Build string descriptors */

    descname(&old_desc, oldname);
    descname(&new_desc, newname);

    /* Call lib$rename_file routine */

    return(lib$rename_file(&old_desc, &new_desc, 0,0,0,0,0,0,0,0,0,0));
}
#endif /* !VMS_46 */
#endif /* OLD_VMS */

/*
 * Check to see if we have spawn priv's.
 */
int
check_spawn() {
    struct itmlstdef {
	short int buflen;
	short int itmcod;
	char *bufaddr;
	long int *retlen;
    };

    struct itmlstdef itmlst[] =
	{4,JPI$_UAF_FLAGS,0,0,0,0,0,0};

    long uaf_flags_size;
    unsigned long uaf_flags;

    itmlst[0].bufaddr = (char *)&uaf_flags;
    itmlst[0].retlen = &uaf_flags_size;


    vms_status = sys$getjpiw(0, 0, 0, &itmlst, 0, 0, 0);
    if (!(vms_status & 1)) vms_lasterr = vms_status;
    if ((vms_status) != SS$_NORMAL)
      return(-1);				/* Assume the worst... */

#ifdef UAI$M_CAPTIVE
    if (uaf_flags & UAI$M_CAPTIVE) {
	printf(
"\nThis command cannot be executed. Your account is CAPTIVE.\n\n");
	return(-1);
    }
#endif  /* UAI$M_CAPTIVE */
#ifdef	UAI$M_RESTRICTED			/* for pre-V5.2 systems */
    if (uaf_flags & UAI$M_RESTRICTED) {
	printf(
"\nThis command cannot be executed. Your account is CAPTIVE.\n\n");
	return(-1);
    }
#endif	/* uai$v_restricted */
    return(0);
}

/*
 * Stuff having to do with SET FILE TYPE LABELED
 */
char *
get_vms_vers() {
    static char sysver[9];
    int len;
    struct itmlst {
		  short int buflen;
		  short int code;
		  char *bufadr;
		  int *retlen;
		  } vms_sysver[2];

    vms_sysver[0].buflen = 8;
    vms_sysver[0].code = SYI$_VERSION;
    vms_sysver[0].bufadr = (char *)&sysver;
    vms_sysver[0].retlen = &len;
    vms_sysver[1].buflen = 0;
    vms_sysver[1].code = 0;
    sys$getsyiw(0,0,0,&vms_sysver,0,0,0);
    sysver[8]='\0';
    len = 7;
    while (sysver[len] == ' ') {
	sysver[len] = '\0';
	len--;
    }
    return(sysver);
}

/* Find where a relative file specification starts in the expanded file
 * name.  Returns the number of characters preceeding the relative portion
 */

int
fnd_rel(name) char *name; {

    int ix=0;
    char relnam[ NAMX_C_MAXRSS];
    char *ip;

    nzltor(name, relnam, 0, PATH_REL, NAMX_C_MAXRSS);

    debug(F111, " fnd_rel: absolute name and length", name, strlen(name));
    debug(F111, " fnd_rel: relative name and length", relnam, strlen(relnam));

    if ( (*relnam == '[') || (*relnam == '<') ) {
        ip = strstr(name, &relnam[1]);
        if (ip) ix = ip - name;
    } else {                       /* no common path; send as PATH OFF */
        ip = strstr(name, relnam);
        if (ip) ix = ip - name -1;  /* do_label_rcv will use the bracket */
    }
    return(ix);
}

int
do_label_send(name) char *name; {
    int pad_size;
    int rel_dspec;
    extern int fnspath;
    char * zp;				/* To shut up compilers whining */
					/* about signed vs unsigned chars */
    zp = (char *)zinptr;
    zp += sprintf(zp,"KERMIT LABELED FILE:02D704VERS");
    zp += sprintf(zp,"%08d%s", strlen(get_vms_vers()), get_vms_vers());
    zp += sprintf(zp,"05KVERS00000008%08ld", vernum);
    if (fnspath == PATH_REL) {
        rel_dspec = fnd_rel(name);
        debug(F101," do_label_send: rel dir at char","", rel_dspec);
        zp += sprintf(zp,"07REL_DIR00000008%08ld", rel_dspec);
    }
    zp += sprintf(zp,"07VMSNAME%08d", strlen(name));
    zp += sprintf(zp,"%s", name);
    zp += sprintf(zp,"07VMSFILE%08d", 70);
    memmove(zp, &xabpro_ifile.xab$w_pro, 2);
    zp += 2;
    memmove(zp, &xabpro_ifile.xab$l_uic, 4);
    zp += 4;
    memmove(zp, &fab_ifile.fab$b_rfm, 1);
    zp += 1;
    memmove(zp, &fab_ifile.fab$b_org, 1);
    zp += 1;
    memmove(zp, &fab_ifile.fab$b_rat, 1);
    zp += 1;
    memmove(zp, &xuchar, 4);	/* Dummy for file chars. */
    zp += 4;
    memmove(zp, &fab_ifile.fab$b_fsz, 1);
    zp += 1;
    memmove(zp, &xabfhc_ifile.xab$w_lrl, 2);
    zp += 2;
    memmove(zp, &fab_ifile.fab$w_mrs, 2);
    zp += 2;
    memmove(zp, &xabfhc_ifile.xab$l_ebk, 4);
    zp += 4;
    memmove(zp, &xabfhc_ifile.xab$w_ffb, 2);
    zp += 2;
    memmove(zp, &xabfhc_ifile.xab$l_hbk, 4);
    zp += 4;
    memmove(zp, &fab_ifile.fab$w_deq, 2);
    zp += 2;
    memmove(zp, &fab_ifile.fab$b_bks, 1);
    zp += 1;
    memmove(zp, &fab_ifile.fab$w_gbc, 2);
    zp += 2;
    memmove(zp, &xabfhc_ifile.xab$w_verlimit, 2);
    zp += 2;
    memmove(zp, &fab_ifile.fab$b_rfm+1, 1);	/* This is fab$b_journal */
    zp += 1;
    memmove(zp, &xabdat_ifile.xab$q_cdt, 8);
    zp += 8;
    memmove(zp, &xabdat_ifile.xab$q_rdt, 8);
    zp += 8;
    memmove(zp, &xabdat_ifile.xab$w_rvn, 2);
    zp += 2;
    memmove(zp, &xabdat_ifile.xab$q_edt, 8);
    zp += 8;
    memmove(zp, &xabdat_ifile.xab$q_bdt, 8);
    zp += 8;
    if (xabpro_ifile.xab$w_acllen != 0) {
        debug(F101, "do_label_send: acllen", "", xabpro_ifile.xab$w_acllen);
	zp += sprintf(zp,"06VMSACL%08d", xabpro_ifile.xab$w_acllen);
	memmove(zp, &aclbuf, xabpro_ifile.xab$w_acllen);
	zp += xabpro_ifile.xab$w_acllen;
    }
    zp += sprintf(zp,"04DATA00000000");
    zincnt = (zp - (char *)zinbuffer);	/* How big */
    zinptr = zinbuffer;			/* Reset pointer for readout */
    return(1);
}

/*
  D O _ L A B E L _ R E C V

  Note that we don't honor SET FILE COLLISION APPEND for labeled receives --
  the whole point of labeled receives is to generate an exact copy of the
  source file, attributes and all.
*/
#define CK_LBLBUFLEN 16
#define CK_VMSFILELEN 70

int
do_label_recv() {

    extern int fnrpath;			/* (look at this later...) */

    char *recv_ptr;
    char buffer[CK_LBLBUFLEN+1];
    char vmsfile[CK_VMSFILELEN];
    char *filptr = vmsfile;
    int lblen, alen;
    int gotname = 0, gotfile = 0, gotacl = 0, gotrel = 0;
    char *i, *j;
    unsigned short jnlflg;

    debug(F101,"do_label_recv: options","",ofile_lblopts);
    ofile_lblproc = 1;			/* Don't come here again */

    if (strncmp((char *)zoutbuffer,"KERMIT LABELED FILE:02D704VERS",30) != 0)
      return(0);			/* Just continue if unlabeled */

    recv_ptr = (char *)zoutbuffer+30;	/* start at front of buffer */

    memcpy(buffer, recv_ptr, 8);
    recv_ptr += 8;
    buffer[8] = '\0';
    lblen = atoi(buffer);
    if (lblen > CK_LBLBUFLEN) {
	debug(F101,"do_label_recv: lblen too long 1","",lblen);
	return(-1);
    }
    memcpy(buffer, recv_ptr, lblen);
    recv_ptr += lblen;
    buffer[lblen] = '\0';
    debug(F110,"do_label_recv: file created under {Open}VMS: ",buffer,0);

    memcpy(buffer, recv_ptr, 7);
    recv_ptr += 7;
    if (strncmp(buffer, "05KVERS", 7) != 0) {
	debug(F100,"do_label_recv: lost sync at KVERS","",0);
	return(-1);
    }
    memcpy(buffer, recv_ptr, 8);
    recv_ptr += 8;
    buffer[8] = '\0';
    lblen = atoi(buffer);
    if (lblen > CK_LBLBUFLEN) {
	debug(F101,"do_label_recv: lblen too long 2","",lblen);
	return(-1);
    }
    memcpy(buffer, recv_ptr, lblen);
    recv_ptr += lblen;
    buffer[lblen] = '\0';
    debug(F110,"do_label_recv: file created with C-Kermit/VMS: ",buffer,0);

  next_label:
    memcpy(buffer, recv_ptr, 2);
    recv_ptr += 2;
    buffer[2] = '\0';
    lblen = atoi(buffer);
    if (lblen == 0) {
	debug(F100,"do_label_recv: lost sync at next_label: ","",0);
	return(-1);
    } else if (lblen > CK_LBLBUFLEN) {
	debug(F101,"do_label_recv: lblen too long 3","",lblen);
	return(-1);
    }
    memcpy(buffer, recv_ptr, lblen);
    recv_ptr += lblen;
    buffer[lblen] = '\0';
    debug(F110,"do_label_recv: found tag: ",buffer,0);
    if (strcmp(buffer, "VMSNAME") == 0) {
	memcpy(buffer, recv_ptr, 8);
        recv_ptr += 8;
	buffer[8] = '\0';
	lblen = atoi(buffer);
	if (lblen > CKMAXPATH) {	/* fdc 23 Jun 96 */
	    debug(F101,"do_label_recv: lblen too long 4","",lblen);
	    return(-1);
	}
	memcpy(ofile_vmsname, recv_ptr, lblen);
        recv_ptr += lblen;
	ofile_vmsname[lblen] = '\0';
	gotname++;
	debug(F110,"do_label_recv: loaded file name block as: ",
	      ofile_vmsname,
	      0
	      );
	debug(F110,"do_label_recv: ofile_vmsname 2",ofile_vmsname,0);
        if ((ofile_lblopts & LBL_PTH) == 0) {
            if ( (fnrpath == PATH_REL) && (gotrel > 0) ) {
                i = strrchr(ofile_vmsname, '[');
                if (i != NULL) {
                    ofile_vmsname[0] = '[';
                } else {
                    ofile_vmsname[0] = '<';
                }
                ckstrncpy(&ofile_vmsname[1],
			  &ofile_vmsname[gotrel],
			  CKMAXPATH
			  );
            } else {
                i = strstr(ofile_vmsname, "::");
		if (i != NULL) {
		    i += 2;
		    memmove(ofile_vmsname, i, strlen(ofile_vmsname));
                }
	        i = strrchr(ofile_vmsname, ':');
	        j = strrchr(ofile_vmsname, ']');
	        if (j == NULL)
		j = strrchr(ofile_vmsname, '>');
	        if (j > i)
		  i = j;
	        if (i) {			/* fdc 6-12-96 */
		    i++;
		    memmove(ofile_vmsname, i, strlen(ofile_vmsname));
	        }
            }
        } else {
            i = strstr(ofile_vmsname, "::");
	    if (i != NULL) {
	        i += 2;
	        memmove(ofile_vmsname, i, strlen(ofile_vmsname));
	    }
	}
	debug(F110,"do_label_recv: ofile_vmsname 3",ofile_vmsname,0);
	if (strchr(ofile_vmsname, ';') != NULL) {
	    for (alen = strlen(ofile_vmsname);
		 ofile_vmsname[alen] != ';' && alen > 0;
		 alen--)
	      ;
	    ofile_vmsname[alen] = '\0';
	}
	debug(F110,"do_label_recv: resultant filespec: ",ofile_vmsname,0);
	goto next_label;
    } else if (strcmp(buffer, "REL_DIR") == 0) {
	memcpy(buffer, recv_ptr, 8);
	recv_ptr += 8;
	buffer[8] = '\0';
	lblen = atoi(buffer);
	if (lblen > CK_LBLBUFLEN) {
	    debug(F101,"do_label_recv: lblen too long 5","",lblen);
	    return(-1);
	}
        memcpy(buffer, recv_ptr, lblen);
        recv_ptr += lblen;
        buffer[lblen] = '\0';
        gotrel = atoi(buffer);
	if ( (gotrel < 3) || (gotrel > NAMX_C_MAXRSS) ) {
	    debug(F101,
		  "do_label_recv: rel dir head position wrong",
		  "",
		  gotrel
		  );
	    return(-1);
	}
	debug(F101,"do_label_recv: relative directory position","", gotrel);
	goto next_label;
    } else if (strcmp(buffer, "VMSFILE") == 0) {
	memcpy(buffer, recv_ptr, 8);
	recv_ptr += 8;
	buffer[8] = '\0';
	lblen = atoi(buffer);
	if (lblen > CK_VMSFILELEN) {
	    debug(F101,"do_label_recv: lblen too long 5","",lblen);
	    return(-1);
	}
	memcpy(vmsfile, recv_ptr, lblen);
	recv_ptr += lblen;
	vmsfile[lblen] = '\0';
	gotfile++;
	debug(F100,"do_label_recv: loaded file attribute block","",0);
	goto next_label;
    } else if (strcmp(buffer, "VMSACL") == 0) {
	memcpy(buffer, recv_ptr, 8);
	recv_ptr += 8;
	buffer[8] = '\0';
	ofile_acllen = atoi(buffer);
	if (ofile_acllen > sizeof(ofile_vmsacl) ) {
	    debug(F101,"do_label_recv: ofile_acllen too long","",ofile_acllen);
	    return(-1);
	}
	memcpy(ofile_vmsacl, recv_ptr, ofile_acllen);
	recv_ptr += ofile_acllen;
/*	ofile_vmsacl[ofile_acllen] = '\0'; */	/* ACL buffer is binary */
	gotacl++;
	debug(F100,"do_label_recv: loaded file ACL block","",0);
	goto next_label;
    } else if (strcmp(buffer, "DATA") == 0) {
	memcpy(buffer, recv_ptr, 8);
	recv_ptr += 8;
	buffer[8] = '\0';
	lblen = atoi(buffer);
	if (lblen != 0) {
	    debug(F101,"do_label_recv: length of DATA tag not zero","",lblen);
	    return(-1);
	}
	debug(F100,"do_label_recv: positioned at start of file data","",0);
	goto all_set;
    } else {
	debug(F110,"do_label_recv: unrecognized label: ",buffer,0);
	memcpy(buffer, recv_ptr, 8);
	recv_ptr += 8;
	buffer[8] = '\0';
	lblen = atoi(buffer);
	recv_ptr += lblen;
	goto next_label;
    }
  all_set:
    if (gotfile != 1 || gotname != 1) {
	debug(F100,"do_label_recv: missing one or more required labels","",0);
	return(-1);
    }

/* Prep the characteristics */

    if ((ofile_lblopts & LBL_NAM) != 0) {
	/* Install the path name in the FAB or NAML. */
#ifdef NAML$C_MAXRSS
	fab_ofile.fab$l_dna = (char *) -1;  /* Using NAML for default name. */
	fab_ofile.fab$l_fna = (char *) -1;  /* Using NAML for file name. */
#endif /* def NAML$C_MAXRSS */
	FAB_OR_NAML( fab_ofile, nam_ofile).FAB_OR_NAML_FNA = ofile_vmsname;
	FAB_OR_NAML( fab_ofile, nam_ofile).FAB_OR_NAML_FNS =
	 strlen( ofile_vmsname);
    }
    fab_ofile.fab$b_fac = FAB$M_BIO | FAB$M_PUT;
    fab_ofile.fab$l_fop = FAB$M_MXV;
    fab_ofile.fab$l_xab = (char *)&xabdat_ofile;
    rab_ofile = cc$rms_rab;
    rab_ofile.rab$l_fab = &fab_ofile;
    xabdat_ofile = cc$rms_xabdat;
    xabdat_ofile.xab$l_nxt = (char *)&xabrdt_ofile;
    xabrdt_ofile = cc$rms_xabrdt;
    xabrdt_ofile.xab$l_nxt = (char *)&xabfhc_ofile;
    xabfhc_ofile = cc$rms_xabfhc;
    xabfhc_ofile.xab$l_nxt = (char *)&xabpro_ofile;
    xabpro_ofile = cc$rms_xabpro;
    xabpro_ofile.xab$l_nxt = (char *)&xaball_ofile;
    xaball_ofile = cc$rms_xaball;

/* Load 'em up */

    memmove(&xabpro_ofile.xab$w_pro, filptr, 2);
    filptr += 2;
    if ((ofile_lblopts & LBL_OWN) != 0)
      memmove(&xabpro_ofile.xab$l_uic, filptr, 4);
    filptr += 4;
    memmove(&fab_ofile.fab$b_rfm, filptr, 1);
    filptr += 1;
    memmove(&fab_ofile.fab$b_org, filptr, 1);
    filptr += 1;
    memmove(&fab_ofile.fab$b_rat, filptr, 1);
    filptr += 1;
    filptr += 4;			/* reserved */
    memmove(&fab_ofile.fab$b_fsz, filptr, 1);
    filptr += 1;
    memmove(&xabfhc_ofile.xab$w_lrl, filptr, 2);
    filptr += 2;
    memmove(&fab_ofile.fab$w_mrs, filptr, 2);
    filptr += 2;
    memmove(&xabfhc_ofile.xab$l_ebk, filptr, 4);
    filptr += 4;
/* preserve this as RMS won't remember it for us */
    memmove(&ofile_ffb, filptr, 2);
    filptr += 2;
    memmove(&xaball_ofile.xab$l_alq, filptr, 4);
    filptr += 4;
    memmove(&xaball_ofile.xab$w_deq, filptr, 2);
    filptr += 2;

#ifdef BUGFILL7
/*
  When DEC C first came out for the VAX, the xab$b_bkz definition was missing
  and they used xaballdef$$_fill_7 instead.  But that was a long time ago.
  (This change made for C-Kermit 6.0, 19 Sep 96.)
*/
#ifdef COMMENT
    memmove((char *)&xaball_ofile.xaballdef$$_fill_7, (char *)filptr, 1);
#else
    {
	char * s1, * s2;
	s1 = (char *)&xaball_ofile.xaballdef$$_fill_7;
	s2 = (char *)filptr;
	*s1 = *s2;
    }
#endif /* COMMENT */
#else
    memmove(&xaball_ofile.xab$b_bkz, filptr, 1);
#endif /* BUGFILL7 */

    filptr += 1;
    memmove(&fab_ofile.fab$w_gbc, filptr, 2);
    filptr += 2;
    memmove(&xabfhc_ofile.xab$w_verlimit, filptr, 2);
    filptr += 2;
    memmove(&jnlflg, filptr, 1);
    if (jnlflg !=0)
      debug(F100,"do_label_recv: journaling status removed for file","",0);
    filptr += 1;
    memmove(&xabdat_ofile.xab$q_cdt, filptr, 8);
    filptr += 8;
    memmove(&revdat, filptr, 8);
    filptr += 8;
    memmove(&revnum, filptr, 2);
    filptr += 2;
    memmove(&xabdat_ofile.xab$q_edt, filptr, 8);
    filptr += 8;
    if ((ofile_lblopts & LBL_BCK) != 0)
	memmove(&xabdat_ofile.xab$q_bdt, filptr, 8);
    filptr += 8;

/* ACL's? */

    if ((ofile_lblopts & LBL_ACL) != 0 && gotacl != 0) {
	xabpro_ofile.xab$l_aclbuf = (char *)&ofile_vmsacl;
	xabpro_ofile.xab$w_aclsiz = ofile_acllen;
    }

/* Try to create the file */

    rms_sts = sys$create(&fab_ofile);
    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
    if (!(rms_sts & 1)) {
	debug(F101,"do_label_recv: $create failed, status","",rms_sts);
	return(-1);
    }
    if ((ofile_lblopts & LBL_ACL) != 0 && gotacl != 0) {
	if (!(xabpro_ofile.xab$l_aclsts & 1)) {
	    debug(F101,"do_label_recv: ACL chain failed, status",
		  "",
		  xabpro_ofile.xab$l_aclsts
		  );
	    return(-1);
	}
    }
    rms_sts = sys$connect(&rab_ofile);
    if (!(rms_sts & 1)) vms_lasterr = rms_sts;
    if (!(rms_sts & 1)) {
	debug(F101,"do_label_recv: $connect failed, status","",rms_sts);
	return(-1);
    }
/*
  Slide the remainder of the data to the head of the buffer and adjust the
  counter and pointer. This will cause the buffer to be re-filled to the full
  32Kb capacity, which is necessary for proper operation of zoutdump().
*/
    zoutcnt -= ((char *)recv_ptr - (char *)zoutbuffer);
    debug(F101,"do_label_recv: zoutcnt","",zoutcnt);
    memcpy(zoutbuffer, recv_ptr, zoutcnt);
    zoutptr = zoutbuffer + zoutcnt;
    return(1);				/* Go fill some more */
}

static char xxvmsmsg[PMSG_MSG_SIZE];

char *
ckvmserrstr(x) unsigned long x; {
    long int n = 0;
    struct dsc$descriptor_s b = {
	PMSG_BUF_SIZE - 1,
	DSC$K_DTYPE_T,
	DSC$K_CLASS_S,
	NULL
    };
    if (x < 1) x = vms_lasterr;
    b.dsc$a_pointer = (char *)&xxvmsmsg;
    if (!((vms_status = sys$getmsg(x, &n, &b, 0, 0)) & 1))
      return("%CKERMIT-W-UNKERR, Unknown error");
    xxvmsmsg[n] = '\0';
    return((char *)xxvmsmsg);
}

/* End of CKVFIO.C */


/* DEAD CODE follows... */

#ifdef CK_TMPDIR
#ifdef COMMENT /* FDC */		/* Frank's version */
int
isdir(s) char *s; {
    int x;
    int i, s_len;
    char *full_name = NULL;
    char *p = NULL;

    static char dot_dir[] = ".dir";
    static char zero_dir[] = "[000000]";

    char name_buf[255];			/* Was 512 but that's too big */
    char tmpbuf[255];			/* for unsigned char... */

    struct FAB fab;
    struct NAM nam;

    struct dsc$descriptor_s indesc;
    $DESCRIPTOR(lnmtable, "LNM$FILE_DEV");
    int new_len;
    int serial_num;
    struct {
	short length;
	short code;
	char *address;
	int *len;
	int term;
    } itemlist;

    if (!s) return(0);
    if (!*s) return(0);

    /* Determine if this is something we can SET DEFAULT to...  */

    s_len = strlen(s);
    debug(F111,"isdir",s,s_len);

    /* Does it look like a directory name? */

    if (s[s_len-1] != ':' && s[s_len-1] != ']' && s[s_len-1] != '>') {
#ifdef COMMENT
/*
  Bad idea.  No, the user really has to include the colon, otherwise the
  intention is ambiguous.
*/
	p = tmpbuf;			/* No, maybe it's a logical name */
	ckstrncpy(tmpbuf,getenv(s),255);
	if (*p) {
	    s = p;
	    s_len = strlen(s);
	    if (s < 1)			/* No definition */
	      return(0);

	    /* It is a logical name, but does it look like a dir name? */
	    if (s[s_len-1] != ':' && s[s_len-1] != ']' && s[s_len-1] != '>')
	      return(0);		/* No, it doesn't */
	}
#else /* def COMMENT */

/*  Check for "DEV:[FOO]BLAH.DIR;1"... */

	if (zchki(s) == -2) {		/* It's a directory */
	    int i;
	    char * p = NULL;		/* Pointer to period */
	    debug(F111,"isdir zchki",s,-2);
	    /* Note: This does not handle "FOO.DIR.1"... */
	    for (i = s_len; i > 0; i--) {
		if (s[i] == '.')
		  p = s+i;
		else if (s[i] == ']' || s[i] == '>')
		  break;
	    }
	    debug(F110,"isdir p",p,0);
	    if (p) {
		if ((*(p+1) == 'D' || *(p+1) == 'd') &&
		    (*(p+2) == 'I' || *(p+2) == 'i') &&
		    (*(p+3) == 'R' || *(p+3) == 'r') &&
		    (*(p+4) == ';' || *(p+4) == NUL || *(p+4) == SP))
		  return(2);
	    }
	}
	return(0);
#endif /* def COMMENT [else] */
    }
    /* Check that the directory part is valid... */

    if (s[s_len-1] == ']' || s[s_len-1] == '>') {
        fab = cc$rms_fab;
        nam = cc$rms_nam;

        fab.fab$l_dna = 0;
        fab.fab$b_dns = 0;

        fab.fab$l_fna = s;
        fab.fab$b_fns = s_len;

        fab.fab$l_fop = 0;
        fab.fab$w_ifi = 0;
        fab.fab$l_nam = &nam;

        nam.nam$l_esa = name_buf;
        nam.nam$b_ess = sizeof(name_buf);
        nam.nam$b_nop = 0;
        nam.nam$l_rlf = 0;
        nam.nam$l_rsa = 0;
        nam.nam$b_rsl = 0;
        nam.nam$l_fnb = 0;

        i = sys$parse(&fab, 0, 0);
	if (!(i & 1)) vms_lasterr = i;
#ifdef COMMENT
	printf("parse returned %d 0x%x, nam fnb is %d 0x%x\n",
	       i, i, nam.nam$l_fnb, nam.nam$l_fnb);
#endif /* def COMMENT */
        if ((i & 1) == 0) return(0);
    }

    /* Check that the logical name is valid */

    i = s_len - 1;
    while (i >= 0 && s[i] != ':') i--;

    if (i >= 0 && s[i] == ':') {
    	if (i == 0) return(0);		/* Single colon (:) */
    	if (s[i-1] == ':') {
    	    if (i > 1) return(1);	/* DECnet node name (blah::) */
    	    else return(0);		/* or :: alone. */
    	}
	s_len = i;
	full_name = malloc(s_len + 1);
	if (!full_name) return(0);
	/* Logicals must be upper case */
	for (i = 0; i < s_len; i++) {
	    full_name[i] = s[i];
	    if (full_name[i] >= 'a' && full_name[i] <= 'z')
	      full_name[i] -= ('a' - 'A');
	}
	indesc.dsc$w_length = s_len;
	indesc.dsc$a_pointer = full_name;
	indesc.dsc$b_class = DSC$K_CLASS_S;
	indesc.dsc$b_dtype = DSC$K_DTYPE_T;

	itemlist.length = new_len = sizeof(name_buf);
	itemlist.code = LNM$_STRING;
	itemlist.address = name_buf;
	itemlist.len = &new_len;
	itemlist.term = 0;

	i = sys$trnlnm(0, &lnmtable, &indesc, 0, &itemlist);
	if (!(i & 1)) vms_lasterr = i;
	if (i != SS$_NORMAL || new_len < 0) new_len = 0;
	if (new_len >= sizeof(name_buf)) new_len = sizeof(name_buf) - 1;
	name_buf[new_len] = '\0';

#ifdef COMMENT
	printf("trnlnm result %d 0x%x, '%.*s'\n", i, i, new_len, name_buf);
#endif /* def COMMENT */

	if (new_len == 0) {
	    /* Could still be a device name. */
	    /* Only disks have serial numbers... */
	    serial_num = 0;
	    itemlist.length = new_len = sizeof(serial_num);
	    itemlist.code = DVI$_SERIALNUM;
	    itemlist.address = (char *) &serial_num;
	    itemlist.len = &new_len;
	    itemlist.term = 0;
	    i = sys$getdvi(0, 0, &indesc, &itemlist, 0, 0, 0, 0);
	    if (!(i & 1)) vms_lasterr = i;
#ifdef COMMENT
	    printf("getdvi ret %d 0x%x, serial %d len %d\n",
		   i, i, serial_num, new_len);
#endif /* def COMMENT */
	    free(full_name);
	    return(((i & 1) == 1 && new_len > 0) ? 1: 0);

	} else if (name_buf[new_len-1] == ':' ||
			name_buf[new_len-1] == ']' ||
			name_buf[new_len-1] == '>') {
	    /* Check returned value */
	    if (new_len > 2 &&
	        (name_buf[new_len-1] == ']' || name_buf[new_len-1] == '>') &&
	        name_buf[new_len-2] == '.') {
	    	/* Remove trailing dot in directory of logical name */
	    	name_buf[new_len-2] = name_buf[new_len-1];
	    	name_buf[new_len-1] = '\0';
	    }
	    free(full_name);
	    return( isdir(name_buf) );
	} else {
	    /* Logical name is just a random string signifying nothing */
	    free(full_name);
	    return(0);
	}
    }
    return(1);
}
#endif /* def COMMENT */ /* FDC */
#endif /* CK_TMPDIR */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* 2010-03-15 SMS.
 * Unit test main program.
 *
 * cc ckvfio.c /obj = testn /def = (UNIT_TEST)
 * cc ckvfio.c /obj = testnl /def = (UNIT_TEST, _LARGEFILE)
 * link testn  ! (Expect several %LINK-W-USEUNDEF complaints.)
 * link testnl
 *
 */


#ifdef UNIT_TEST

/* Globals. */

unsigned int vms_status;
unsigned int vms_lasterr;

int binary = 0;
int zchkid = 0;
int zchkod = 0;
int zincnt, zoutcnt;
CHAR *zinptr, *zoutptr;
CHAR *zinbuffer, *zoutbuffer;

char whoareu[ 16] = "U1";               /* UNIX. */
char *cksysid = "D7";                   /* VMS. */

int deblog = 1;

/* MAIN. */

int main( int argc, char **argv)
{
    int sts;
    int argi1;
    int argi2;
    int argi3;

    if (argc > 1)
    {
        argi1 = strtol( argv[ 1], NULL, 10);
        if (argi1 == 1)
        {
            if (argc < 3)
            {
                printf( " arg1 = %d.  Insuff args (%d < 3).\n", argi1, argc);
            }
            else
            {
                sts = isdir( argv[ 2]);
                printf( " sts = %d.\n", sts);
            }
        }
        else if (argi1 == 2)
        {
            if (argc < 4)
            {
                printf( " arg1 = %d.  Insuff args (%d < 4).\n", argi1, argc);
            }
            else
            {
                argi2 = strtol( argv[ 2], NULL, 10);
                sts = zopeni( argi2, argv[ 3]);
                printf( " sts = %d.\n", sts);
            }
        }
        else if (argi1 == 3)
        {
            if (argc < 4)
            {
                printf( " arg1 = %d.  Insuff args (%d < 4).\n", argi1, argc);
            }
            else
            {
                struct zattr zz;
                struct filinfo fcb;

                argi2 = strtol( argv[ 2], NULL, 10);
                sts = zopeno( argi2, argv[ 3], &zz, &fcb);
                printf( " sts = %d.\n", sts);
            }
        }
        else if (argi1 == 4)
        {
            if (argc < 3)
            {
                printf( " arg1 = %d.  Insuff args (%d < 3).\n", argi1, argc);
            }
            else
            {
                sts = zchki( argv[ 2]);
                printf( " sts = %d.\n", sts);
            }
        }
        else if (argi1 == 5)
        {
            if (argc < 3)
            {
                printf( " arg1 = %d.  Insuff args (%d < 3).\n", argi1, argc);
            }
            else
            {
                sts = zchko( argv[ 2]);
                printf( " sts = %d.\n", sts);
            }
        }
        else if (argi1 == 6)
        {
            if (argc < 5)
            {
                printf( " arg1 = %d.  Insuff args (%d < 5).\n", argi1, argc);
            }
            else
            {
                argi2 = strtol( argv[ 2], NULL, 10);
                argi3 = strtol( argv[ 3], NULL, 10);
                char name2[ NAMX_C_MAXRSS];

                nzltor( argv[ 4], name2, argi2, argi3, NAMX_C_MAXRSS);
                printf( " nameq: >%s<.\n", name2);
            }
        }
        else if (argi1 == 7)
        {
            if (argc < 3)
            {
                printf( " arg1 = %d.  Insuff args (%d < 3).\n", argi1, argc);
            }
            else
            {
                char name2[ NAMX_C_MAXRSS];

                sts = cvtdir( argv[ 2], name2, NAMX_C_MAXRSS);
                printf( " sts = %d.\n", sts);
            }
        }
        else if (argi1 == 8)
        {
            if (argc < 3)
            {
                printf( " arg1 = %d.  Insuff args (%d < 3).\n", argi1, argc);
            }
            else
            {
                sts = zmkdir( argv[ 2]);
                printf( " sts = %d.\n", sts);
            }
        }
        else if (argi1 == 9)
        {
            if (argc < 3)
            {
                printf( " arg1 = %d.  Insuff args (%d < 3).\n", argi1, argc);
            }
            else
            {
                sts = zrmdir( argv[ 2]);
                printf( " sts = %d.\n", sts);
            }
        }
        else if (argi1 == 10)
        {
            if (argc < 3)
            {
                printf( " arg1 = %d.  Insuff args (%d < 3).\n", argi1, argc);
            }
            else
            {
                sts = zchkpath( argv[ 2]);
                printf( " sts = %d.\n", sts);
            }
        }
    }
}

/* Dummy run-time library functions. */

#define MIN( a, b) ((a) < (b) ? (a) : (b))

int ckstrncpy(char * dest, const char * src, int len)
{
    strncpy( dest, src, len);
    return MIN( strlen( src), len);
}

int ckstrncat(char * dest, const char * src, int len)
{
    strncat( dest, src, len);
    return MIN( strlen( src), len);
}

int dodebug( int f, char *s1, char *s2, CK_OFF_T n)
{
#if __USE_OFF64_T
    printf( " %s %s %lld (%%x%016llx).\n", s1, s2, n, n);
#else /* __USE_OFF64_T */
    printf( " %s %s %d (%%x%08x).\n", s1, s2, n, n);
#endif /* __USE_OFF64_T [else] */
    return 0;
}

int print_msg( char *msg)
{
    printf( " v_s = %d (%%x%08x), msg: >%s<.\n",
     vms_status, vms_status, msg);
    return 0;
}

#endif /* def UNIT_TEST */
