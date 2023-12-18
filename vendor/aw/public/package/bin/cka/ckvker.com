$!
$! CKVKER.COM - C-Kermit 9.0 Construction for (Open)VMS
$!
$! Version 1.46, 18-Mar-2010
$!
$! DCL usage requires VMS 5.0 or higher - use CKVOLD.COM for VMS 4.x.
$!
$ if p1 .eqs. "" then goto Skip_Help
$ if (f$locate(",",p1).ne.f$length(p1)) then goto Bad_param
$ if p1.nes."" then p1 = f$edit(p1,"UPCASE")
$ if f$locate("H",p1).eq.f$length(p1) .and. -
     f$locate("?",p1).eq.f$length(p1) then goto Skip_Help
$Help:
$type sys$input
   Usage:
       $ @[directory]ckvker [ p1 [ p2 [ p3 [ p4 [ p5 ] ] ] ] ]

       P1 = Build options
       P2 = Compiler selection
       P3 = C-Kermit DEFINES
       P4 = Additional compiler qualifiers (like /LIST/SHOW=INCLUDE)
       P5 = Link Qualifiers

   P1 Build options (no white space, or enclose in quotes):
       S = share (VAX C default is noshare, i.e. no shared VAXCRTL)
       L = RTL version level -- link with latest math RTL
       D = compile&link /DEBUG (create map files, etc)
       C = clean (remove object files, etc.)
       M = don't use MMS or MMK; use the DCL MAKE subroutine herein
       N = build with no network support
       O = override the limit on MMS/MMK command line length
       V = turn on verify
       W = exit on warnings
       H = display help message
       X = build CKVCVT rather than C-Kermit
       F = DISABLE large-file support (see LARGE_FILE NOTES below)
       I = DISABLE internal FTP (it's enabled by default in network builds)
       "" = Null place holder; use the defaults

   P2 compiler_selection
       D = DEC C
       V = VAX C
       G = GNU C
       "" = Use the first compiler found, searching in the above order

   P3    C-Kermit options (enclosed in quotes if more than one) including:
          CK_SSL   Includes SSL/TLS support for secure connections.
          NOPUSH   prevents escape to DCL or running external programs.
          NODEBUG  To reduce size of executable.
          INTSELECT If you get %CC-W-PTRMISMATCH on statements with select().
          NEEDUINT If you get complaints about "u_int" not defined (TCPware5.4)
          OLDFIB   If you get %CC-I-NONSEQUITUR on statements with ut_fib.
          OLDIP    Use a very old TCP/IP run-time scheme (where "very old" is
                   not precisely known, but pre-dates VMS V5.4, VAX C V3.1-051,
                   UCX V1.3).
          BUGFILL7 If you get %CC-E-NEEDMEMBER, '"xab$b_bkz" is not a member
                   of "xaball_ofile"'.  (Effectively implies NOCONVROUTINES.)
          NEEDUINT If you get complaints about "u_int" not defined (TCPware5.4)
          NOCONVROUTINES If <conv$routines.h> not found (VMS 6.1 / UCX 4.1).
          ""       Empty string.  Include this if you want none of these
                   options, and other parameters follow.

         To build with SSL, include CK_SSL in P3.  If you have a vendor
         supplied SSL library such as HP SSL V1.3 or Compaq SSL V1.0A, that
         should do it.  Otherwise if you have installed OpenSSL you'll also
         need to define the logical names OPENSSL and SSLLIB to point to
         OpenSSL and libs.  OPENSSL_DISABLE_OLD_DES_SUPPORT might be necessary
         in P3 also if you get tons of compilation errors, but starting in
         C-Kermit 9.0 Alpha.03 this should be added automatically by this
         procedure for any OpenSSL later than 0.9.7.
         NOTE: SSL-enabled binaries are restricted by USA export law.

   P4    Compiler qualifiers (enclosed in quotes) if desired; provides
         additional flexibility, e.g., to change the compiler optimization,
         "/OPT=LEV=2" or "/CHECK" to add runtime array bounds checking, etc,
         in DECC.

   P5    Link qualifiers, e.g., the linker default is to search the
         system shareable image library, IMAGELIB.OLB,  before the
         object module STARLET.OLB.  To build an image that may run on
         older system you might want to try linking /NOSYSSHR

 Example:

      $ @ckvker snmd ""  "NOPUSH, NODEBUG" "/NOOPT"

      NOPUSH  - Disallow access to DCL from within Kermit.
      NODEBUG - Remove debugging code to make C-Kermit smaller and faster.

 How to use this procedure:

      This procedure should be stored in the same directory as the source
      files.  You can SET DEFAULT to that directory and execute the procedure
      as shown above, or you can SET DEFAULT to a separate directory and run
      run the procedure out of the source directory, e.g.:

      SET DEFAULT DKA300:[KERMIT.ALPHA]
      @DKA300:[KERMIT.SOURCE]CKVKER

      This puts the object and executable files into your current directory.
      Thus you can have (e.g.) and Alpha and an VAX build running at the
      same time from the same source on a shared disk.  Alternatively, you
      can define a logical name for the source directory:

      DEFINE CK_SOURCE DKA300:[KERMIT.SOURCE]

      and then no matter which directory you run this procedure from, it
      will look in the CK_SOURCE logical for the source files.

   NOTES:
      If adding flags here makes ccopt too long for Multinet/Wollongong
      you will have to do it in CKCDEB.H.

      The empty string, "", is needed only as a place holder if additional
      strings follow.

      This procedure defines a process logical name, "K".  If this
      interferes with an existing definition of "K", consider using
      SPAWN to give this procedure its own process.

      If more than one TCP/IP stack is installed, the first one found is
      used in the build.  To force a different one, do:

        $ net_option = "DEC_TCPIP"

      (or other) prior to invoking this command procedure (see CKVINS.TXT
      for a complete list).

   Works like MAKE in that only those source modules that are newer than the
   corresponding object modules are recompiled.  Changing the C-Kermit command
   line DEFINES or compiler options does not affect previously compiled
   modules. To force a particular module to be recompiled, delete the object
   file first.  To force a full rebuild:

   $  @ckvker c
   $  @ckvker <desired-options>

   To use in batch, set up the appropriate directories and submit
   (/NOLIST and /NOMAP are not needed unless P1 includes "D")
    e.g., submit CKVKER /parameters=(SL,"","NODEBUG")

   See the CKVINS.TXT and CKVBWR.TXT files for further information.

$Exit
$!
$!
$! Uses MMS if it is installed, unless the M option is included.  If CKVKER.MMS
$! is missing, you'll get an error; if MMS is not runnable for some reason
$! (privilege, image mismatch, etc), you'll also get an error.  In either case,
$! simply bypass MMS by including the M option in P1.
$!
$! For network-type selection, you may also type (at the DCL prompt, prior
$! to running this procedure):
$!
$!   net_option = "BLAH"
$!
$! where BLAH (uppercase, in quotes) is NONET, MULTINET, TCPWARE, WINTCP,
$! DEC_TCPIP, or CMU_TCPIP, to force selection of a particular TCP/IP
$! product, but only if the product's header files and libraries are installed
$! on the system where this procedure is running.
$!
$! By default, this procedure builds C-Kermit with support for the TCP/IP
$! network type that is installed on the system where this procedure is run,
$! and tries to link statically with old libraries.  If the system is a VAX, a
$! VAX binary is created; if it is an Alpha, an Alpha binary is created.  If
$! IA64, an IA64 binary is created.  If more than one TCP/IP product is
$! installed, the search proceeds in this order: MULTINET, TCPWARE, WINTCP,
$! DEC_TCPIP, CMU_TCPIP.
$!
$! Should work for all combinations of VAXC/DECC/GCC, VAX/Alpha/IA64, and any
$! of the following TCP/IP products: DEC TCP/IP (UCX), Cisco (TGV) MultiNet,
$! Attachmate (Wollongong) WINTCP (Pathway), Process Software TCPware, or
$! CMU/Tektronix TCP/IP (except CMU/Tek is not available for the Alpha).  VAX
$! C is supported back to version 3.1, and DEC C back to 1.3.  Tested on VMS
$! versions back to 5.4, but should work back to VAX/VMS 5.0.  Use CKVOLD.COM
$! for pre-VMS-5.0 builds since this procedure uses DCL features introduced in
$! VMS 5.0.
$!
$! WOLLONGONG/ATTATCHMATE/WINTCP/PATHWAY BUILDS:
$! You also need to edit TWG$COMMON:[NETDIST.MISC]DEF.COM.
$! Comment out the following lines:
$!
$!   37   $ define decc$system_include   twg$tcp:[netdist.include],      -
$!   38   $                              twg$tcp:[netdist.include.sys]
$!
$! ERRORS:
$! 1. At link time, you might see messages like:
$!    %LINK-I-OPENIN, Error opening SYS$COMMON:[SYSLIB]VAXCRTLG.OLB; as input,
$!    %RMS-E-FNF, file not found
$!    %LINK-I-OPENIN, Error opening SYS$COMMON:[SYSLIB]VAXCRTL.OLB; as input,
$!    %RMS-E-FNF, file not found
$!    This generally indicates that the logical name(s) LNK$LIBRARY* is
$!    defined and the runtime libraries are in SYS$SHARE but are not in
$!    SYS$COMMON:[SYSLIB].  In the one case where this was observed, the
$!    messages turned out to be harmless, since the runtime library is being
$!    properly located in the .OPT file generated by this procedure.
$! 2. In newer configurations, you might get a link-time message to the effect
$!    that DECC$IOCTL is multiply defined (e.g. VMS 7.0 / DECC 5.3 / UCX or
$!    TCPware of recent vintage), since the ioctl() function is now supplied
$!    as of VMS 7.0.  This message should be harmless.
$! 3. The compiler might warn that routines like bzero and bcopy are not
$!    declared, or that they have been declared twice.  If the affected module
$!    (usually ckcnet.c) builds anyway, and runs correctly, ignore the
$!    warnings.  If it crashes at runtime, some (more) adjustments will be
$!    needed at the source-code level.
$!
$! This procedure is intended to replace the many and varied Makefiles, MMS
$! and MMK files, and so on, and to combine all of their features into one.
$! It was written by Martin Zinser, Gesellschaft fuer Schwerionenforschung
$! GSI, Darmstadt, Germany, m.zinser@gsi.de (preferred) or eurmpz@eur.sas.com,
$! in September 1996, based on all of the older versions developed by:
$!
$!   Mark Berryman, Science Applications Int'l. Corp., San Diego, CA
$!   Frank da Cruz, Columbia University, New York City <fdc@columbia.edu>
$!   Mike Freeman, Bonneville Power Administration
$!   Tarjei T. Jensen, Norwegian Hydrographic Service
$!   Terry Kennedy, Saint Peters College, Jersey City NJ <terry@spcvxa.spc.edu>
$!   Mike O'Malley, Digital Equipment Corporation
$!   Piet W. Plomp, ICCE, Groningen University, The Netherlands
$!     (piet@icce.rug.nl, piet@asterix.icce.rug.nl)
$!   James Sturdevant, CAP GEMINI AMERICA, Minneapolis, MN
$!   Lee Tibbert, DEC <tibbert@cosby.enet.dec.com>
$!   Bernie Volz, Process Software <volz@process.com>
$!
$! Modification history:
$!  jw  = Joellen Windsor, U of Arizona, windsor@ccit.arizona.edu
$!  fdc = Frank da Cruz, Columbia U, fdc@columbia.edu
$!  mf  = Mike Freeman, Bonneville Power Authority, freeman@columbia.edu
$!  cf  = Carl Friedberg, Comet & Company, carl@comets.com
$!  hg  = Hunter Goatley, Process Software, goathunter@goat.process.com
$!  lh  = Lucas Hart, Oregon State U, hartl@ucs.orst.edu
$!  js  = John Santos, Evans Griffiths & Hart, john@egh.com
$!  dbs = David B Sneddon, dbsneddon@bigpond.com
$!  mv  = Martin Vorlaender, martin@radiogaga.harz.de
$!  jaltman = Jeff Altman, Columbia U <jaltman@columbia.edu>
$!
$! 23-Sep-96 1.01 fdc Shorten and fix syntax of MultiNet
$!                    /PREFIX_LIBRARIES_ENTRIES clause, remove ccopt items to
$!                    make string short enough.
$! 26-Sep-96 1.02 jw  o Create a temporary file for the CCFLAGS=ccopt macro and
$!                    "prepend" it to ckvker.mms to reduce the MMS command
$!                    line length
$!                    o Optionally, use the current level of the Fortran
$!                    runtime library and not the "lowest common denominator".
$!                    When using the "lowest common denominator," be sure to
$!                    DEASSIGN the logicals before exit.
$!                    o  Continue to operate on WARNING messages.
$!                    o  Implement some .COM file debugging qualifiers:
$!                    o  Modify .h file dependencies
$! 06-Oct-96 1.03 fdc Add 'N' command-line switch for no nets, make 'C' list
$!                    the files it deletes, clean up comments & messages, etc.
$! 09-Oct-96 1.04 cf  Change error handling to use ON WARNING only; add "V"
$!                    option to enable verify; fix CKWART so it doesn't come
$!                    up in /debug; remove /DECC from alphas as this is the
$!                    case anyway add /LOG to MMS to get more info
$! 20-Oct-96 1.05 fdc Numerous changes suggested by lots of people to make it
$!                    work in more settings.
$! 21-Oct-96 1.06 jw  o Put the /vaxc qualifier in ccopt when both DECC and
$!                    VAXC are present and user forces use of VAXC.
$!                    o When forcing VAXC and building NOSHARE, add
$!                    sys$share:vaxcrtl.olb/lib to kermit.opt
$!                    o Purge rather than delete kermit.opt, aux.opt, and
$!                    ccflags.mms so we will have them for reference.
$! 21-Oct-96 1.07 hg  Adapt for TCPware and for MMK.
$! 21-Oct-96 1.08 mf  Smooth out a couple differences between MMS and MMK.
$! 21-Oct-96 1.09 hg  Fixes to fdc's interpretation of 1.08.
$! 25-Oct-96 1.10 jw  o Allow compilation source in a centrally-located path
$!                    o Pretty up write of ccopt to sys$output
$!                    o Deassign logicals on warning exit
$! 04-Nov-96 1.11 lh  A. Allow CFLAG options as command-line parameter p3
$!                    (may require adding "ifndef NOopt" to "#define opt"
$!                    construction wherever the VMS default is set, e.g.,
$!                    in CKCDEB.H).
$!                    B. Spiff up:
$!                    (a) Line length limit for Multinet - arbitrary
$!                    (b) Ioctl of VMS v7, DEC_TCPIP, DECC
$!                    (c) Add a P4 option
$!                    (d) Check for command-line length
$!                    (e) Try to set up W for user selection of termination on
$!                        warning, per Joellen's comments.
$!                    C. Some cosmetic changes:
$!                    Change (b) from  net_option {.eqs.} "DEC_TCPIP" to
$!                    {includes string} per jas; move help text to start;
$!                    add VAXC N vaxcrtl link.
$!                    {what about missing net_option share/noshare options?}
$!                    Test for CK_SOURCE to define a source directory different
$!                    from the CKVKER.COM directory
$! 05-Nov-96 1.12 fdc Clean up and amplify help text and add VMS >= 7.0 test.
$! 06-Nov-96 1.12 hg  Remove extraneous comma in VMS >= 7.0 test.
$! 08-Nov-96 1.13 js  Fixes to CMU/Tek build.
$! 23-Nov-96 1.14 lh  Fixes for VMS V7, VAXCRTL links for all TCP/IP packages,
$!                    improved batch operation, add P5 for link options,
$!                    catch commas in P1.
$! 05-Dec-96 1.15 lh  Fixes to work with GCC.
$! 20-Aug-97 1.16 fdc Change version number to 6.0.193.
$! 20-Sep-97 1.16 js  More fixes to CMU/Tek build.
$!  3-Dec-97 1.17 lh  VAX build default, link /NOSYSSHARE; Alpha /SYSSHARE
$! 25-Dec-98 1.18 fdc Change C-Kermit version number to 7.0.195.
$!  6-Jan-99 1.19 fdc Add new CKCLIB and CKCTEL modules.
$!  8-Feb-99 1.20 fdc Add UCX 5.0 detection, add separate P1 (X) for CKVCVT.
$! 18-May-99 1.21 bt  Fix TWG/WINTCP/PathWay 3.1 support.
$! 17-Jul-99 1.22 fdc Can't remember.
$! 22-Jul-99 1.23 fdc Define TCPSOCKET for all TCP/IP builds.
$! 25-Jul-99 1.24 dbs Use SYS$LIBRARY:TCPIP$IPC_SHR.EXE for UCX V5.0.
$! 26-Jul-99 1.25 dbs Now check the share/noshare stuff for UCX V5.0.
$!  3-Aug-99 1.26 fdc Compile and link CKCUNI module.
$! 11-Aug-99 1.27 fdc Don't set if_dot_h for non-UCX builds.
$! 20-Sep-99 1.28 fdc Add /UNSIGNED_CHAR to ccopt for DECC.
$!  8-Dec-00 1.29 fdc Update version numbers.
$! 28-Jun-01 1.30 fdc Update version numbers.
$! 22-Nov-01 1.31 dbs Fix for UCX 5.1. Use TCPIP$IPC_SHR.EXE since there is
$!			no .OLB to use.
$!  9-Jan-02 1.32 fdc Add ckcxla.h to depencies for ckuusy.c.
$!  8-Feb-02 1.33 fdc Update version numbers, add VMS60 symbol definition.
$! 24-Oct-02 1.34 fdc Update version number.
$! 10-Nov-02 1.35 mv, jaltman  Add SSL support.
$! 16-Nov-02 1.36 mv  Update SSL support to include Compaq SSL
$! 17-Nov-02 1.37 lh  Allow for VAX C 2.x
$! 25-Jun-03 1.38 mv  Update OpenSSL support.  Include/exclude HTTP client
$!                    support based on VMS version.  Get Kermit version
$!                    number from CKCMAI.C
$! 06-Apr-04 1.39 fdc CK version = 8.0.211.  Allow for VMS 8.x.
$! 05-Jan-05 1.40 dbs Fix problem where COMPAQ_SSL was not defined but later
$!                    referenced generating an undefined symbol error.
$! 01-Feb-07 1.40+sms Added P1 options F (large-file) and I (internal
$!                    FTP).  Added IA64 announcement, and changed most
$!                    uses of "alpha" to "non_vax".  Changed to use
$!                    actual VMS source dependencies.  Shortened logical
$!                    name "KSP" to "K" to shorten DCL commands for the
$!                    internal CALL MAKE scheme.  Added CKVRTL.C/H for
$!                    utime() for __CRTL_VER < 70300000.
$! 25-Jan-10 1.41 fdc Update comments about building with SSL.
$! 15-Feb-10 1.42 fdc Update C-Kermit version and comments about large files.
$! 22-Feb-10 1.43 mv  Automatically detect SSL version.
$! 09-Mar-10 1.44 sms Added/documented P3 options INTSELECT, OLDFIB, OLDIP.
$!                    Disabled (commented out) automatic definition of
$!                    NOSETTIME for VMS before V7.2 (vms_ver .lts. "VMS_V72").
$! 18-Mar-10 1.45 fdc Configure for large files by default for VMS 7.3 and
$!                    later on non-VAX.
$! 18-Mar-10 1.46 fdc Include FTP client by default in non-nonet builds.
$!                    Fixed C-Kermit version number ck_version.
$!
$Skip_Help:
$!
$ On Control_C then $ goto CY_Exit
$ On Control_Y then $ goto CY_Exit
$! On ERROR then $ goto The_exit
$ On SEVERE_ERROR then $ goto The_exit
$! On Warning then goto warning_exit
$ save_verify_image = f$environment("VERIFY_IMAGE")
$ save_verify_procedure = f$verify(0)
$!
$ say == "Write sys$output"
$ procedure = f$environment("PROCEDURE")
$ procname = f$element(0,";",procedure)
$ node = f$getsyi("NODENAME")
$ say "Starting ''procedure' on ''node' at ''f$time()'"
$ ccopt = ""
$ lopt = ""
$ make = ""
$ CC = "CC"
$ non_vax=0
$ debug=0
$ noshare=1
$ decc=0
$ vaxc=0
$ verify=0
$ gnuc=0
$ oldmath=0
$ mathlevel=0
$ vmsv6=0
$ vmsv7=0
$ vmsv8=0
$ havetcp=0
$ ucxv5=0
$ if_dot_h=0
$ nomms=0
$ mmsclm=264        ! maximum command length limit for MMS/MMK (estimate)
$ do_ckvcvt=0
$ ssl=0
$ internal_ftp=1    ! FTP client enabled by default
$ large_file=1      ! large file support enabled by default
$!
$! Find out which OpenVMS version we are running
$! (do not use IF ... ENDIF for the VMS 4 test and exit)
$!
$ sys_ver = f$edit(f$getsyi("version"),"compress")
$ if f$extract(0,1,sys_ver) .eqs. "V" then goto Production_version
$ type sys$input
WARNING: You appear to be running a Field Test version of VMS.
         Please exercise caution until you have verified proper operation.

$Production_version:
$!
$ dot = f$locate(".",sys_ver)
$ sys_maj = 0+f$extract(dot-1,1,sys_ver)
$ sys_min = 0+f$extract(dot+1,1,sys_ver)
$!
$ if sys_maj .eq. 4 then if (sys_min/2)*2 .ne. sys_min then -
       sys_min = sys_min - 1
$   if sys_maj .ne. 4 then goto Supported_version
$     say ""
$     say "         You are running VMS ''sys_ver'"
$     type sys$input

WARNING: CKVKER.COM will not build VMS C-Kermit using that version of VMS.
         Prebuilt images should run properly, or try CKVOLD.COM.
         Please exercise caution until you have verified proper operation.

$!
$goto The_exit
$!
$Supported_version:
$!
$ vms_ver = "VMS_V''sys_maj'''sys_min'"
$
$! VMSV70 must be defined if the VMS version is 7.0 OR GREATER, so we know
$! we can include <strings.h>.
$
$ if vms_ver .ges."VMS_V60" then vmsv6 = 1
$ if vms_ver .ges."VMS_V70" then vmsv7 = 1
$ if vms_ver .ges."VMS_V80" then vmsv8 = 1
$!
$!
$! Set the Kermit Source Path K: to be the same path as this procedure
$! if the user has not specified another source with a CK_SOURCE logical
$!
$ if f$trnlnm("CK_SOURCE") .eqs. ""
$ then
$    source_device = f$parse(f$environment("procedure"),,,"device")
$    source_directory = f$parse(f$environment("procedure"),,,"directory")
$    define K 'source_device''source_directory
$ else
$   user_source = f$trnlnm("CK_SOURCE")
$   define K 'user_source'
$ endif
$!
$ if p1.nes.""
$   then
$    p1 = f$edit(p1,"UPCASE")
$    if f$locate("V",p1).ne.f$length(p1) then verify=1
$    if f$locate("D",p1).ne.f$length(p1) then debug=1
$    if f$locate("S",p1).ne.f$length(p1) then noshare=0
$    if f$locate("L",p1).ne.f$length(p1) then mathlevel=1
$    if f$locate("M",p1).ne.f$length(p1) then nomms=1
$    if f$locate("N",p1).ne.f$length(p1)
$    then
$      net_option="NONET"
$      internal_ftp=0
$    endif
$    if f$locate("C",p1).ne.f$length(p1) then goto clean
$    if f$locate("X",p1).ne.f$length(p1) then do_ckvcvt=1
$    if f$locate("F",p1).ne.f$length(p1) then large_file=0
$    if f$locate("I",p1).ne.f$length(p1) then internal_ftp=0
$!
$    if f$locate("O",p1).ne.f$length(p1) then mmsclm = 1024
$    if f$locate("W",p1).ne.f$length(p1) then On Warning then goto warning_exit
$ endif
$
$ compaq_ssl = 0
$ if p3 .nes. "" .and. f$locate("CK_SSL",p3) .ne. f$length(p3) then ssl=1
$ if ssl
$   then
$     if f$trnlnm("openssl") .eqs. "" .or. -
         (f$trnlnm("ssllib") .eqs. "" .and. -
         f$trnlnm("sslinclude") .eqs. "" .and. -
         f$trnlnm("ssl$include") .eqs. "")
$     then
$       type sys$input
FATAL: You specified that OpenSSL be used, but the required logical names
       have not been defined.

$       goto The_exit
$     endif
$     compaq_ssl = f$trnlnm("ssl$include") .nes. ""
$     ssl_text = "SSL support and"
$   else
$     ssl_text = ""
$   endif
$!
$ cln_def = ""
$ if p3 .nes. "" then cln_def = ","+p3  ! comma delimited string
$ cln_qua = ""
$ if p4 .nes. "" then cln_qua = p4
$!
$!
$ if ssl
$   then
$     openssl_cmd = ""
$     if f$type(openssl) .eqs. "STRING" then openssl_cmd = openssl
$     if openssl_cmd .eqs. "" .and. compaq_ssl then -
        openssl_cmd := $ssl$exe:openssl.exe
$     if openssl_cmd .eqs. ""
$       then
$         write sys$input
FATAL: Cannot determine the OpenSSL version installed. Please define the
command openssl.

$         goto The_exit
$       endif
$     define/user sys$output openssl_version.tmp
$     openssl_cmd version
$     close/nolog LOG
$     open/read LOG openssl_version.tmp
$     read LOG line
$     close LOG
$     delete_ openssl_version.tmp;
$     ssl_version = f$element(1," ",f$edit(line,"compress"))
$     if ssl_version .ges. "0.9.7"
$       then
$         cln_def = cln_def + ",OPENSSL_DISABLE_OLD_DES_SUPPORT"
$       endif
$     say "OpenSSL ''ssl_version' found"
$   endif
$!
$ if debug.eq.1
$ then
$   ccopt = "/noopt/deb"
$   lopt  = "/deb/map/full/sym"
$ endif
$!
$ if p5 .nes. ""
$ then
$   p5   = f$edit(p5,"UPCASE")
$   lopt = lopt + p5
$ endif
$
$! Check for MMK/MMS
$!
$ if nomms .eq. 0
$ then
$     if f$search("sys$system:mms.exe") .nes. ""
$       then
$         make = "MMS"
$         if (verify) then make = "MMS/LOG/VERIFY"
$       endif
$     if f$type(MMK) .eqs. "STRING" then make = "MMK"
$     if make .nes. "" then say "Using ''make' utility"
$ endif
$!
$! Find out which Kermit version we are building
$! (from CKCMAI.C's ck_s_ver variable declaration)
$!
$ ck_version = "9.0.299"
$ search /exact /nostatistic /output=ck_version.tmp -
      K:ckcmai.c "char *ck_s_ver = "
$ open /read /error=end_version VERSION_TMP ck_version.tmp
$ read /error=end_version VERSION_TMP line
$ close VERSION_TMP
$ delete ck_version.tmp;
$ ck_version = f$element(1,"""",line)
$end_version:
$!
$! Build the option-file
$!
$ open/write aoptf aux.opt
$ if .not. do_ckvcvt
$ then
$   open/write optf kermit.opt
$   write optf "ckcmai.obj"
$   write optf "ckclib.obj"
$   write optf "ckcuni.obj"
$   write optf "ckcfn2.obj"
$   write optf "ckcfn3.obj"
$   write optf "ckcfns.obj"
$   write optf "ckcpro.obj"
$   write optf "ckucmd.obj"
$   write optf "ckudia.obj"
$   write optf "ckuscr.obj"
$   write optf "ckuus2.obj"
$   write optf "ckuus3.obj"
$   write optf "ckuus4.obj"
$   write optf "ckuus5.obj"
$   write optf "ckuus6.obj"
$   write optf "ckuus7.obj"
$   write optf "ckuusr.obj"
$   write optf "ckuusx.obj"
$   write optf "ckuusy.obj"
$   write optf "ckuxla.obj"
$   write optf "ckcnet.obj"
$   write optf "ckctel.obj"
$   write optf "ckvfio.obj"
$   write optf "ckvtio.obj"
$   write optf "ckvcon.obj"
$   write optf "ckvioc.obj"
$   write optf "ckusig.obj"
$   if internal_ftp
$   then
$     write optf "ckcftp.obj"
$     write optf "ckvrtl.obj"
$   endif
$   write optf "Identification=""Kermit ''ck_version'"""
$
$   if ssl
$     then
$       write optf "ckuath.obj"
$       write optf "ck_crp.obj"
$       write optf "ck_ssl.obj"
$       if compaq_ssl
$         then
$           write optf "sys$share:ssl$libssl_shr32.exe/shareable"
$           write optf "sys$share:ssl$libcrypto_shr32.exe/shareable"
$         else
$           write optf "ssllib:libssl/library"
$           write optf "ssllib:libcrypto/library"
$       endif
$   endif
$ endif
$!
$! Look for old math-library to allow transfer of the production Kermit
$! to old-fashioned VMS systems
$!
$ if (mathlevel .eq. 0) .and. -
      (f$search("SYS$SHARE:FORTRAN$MTHRTL-VMS.EXE") .nes. "")
$ then
$   oldmath = 1
$   define/nolog mthrtl fortran$mthrtl-vms
$   define/nolog vmthrtl fortran$vmthrtl-vms
$   type sys$input
NOTE: You have currently DEC Fortran V6.0 or later installed, but the
      old versions of the Math libraries are still available on your
      system. We will link C-Kermit with these older, pre-Fortan V6
      libraries so that it will run on systems which don't have Fortran
      V6 installed. C-Kermit does not use any features of the new
      libraries.

      You will receive %LINK-I-IDMISMCH informational messages during
      linking, but these can be safely ignored.
$ endif
$!
$! Look for the compiler used
$!
$ if p2.nes.""
$   then
$     p2 = f$edit(p2,"UPCASE")
$     if f$locate("G",p2).ne.f$length(p2) then goto gnuc
$     if f$locate("V",p2).ne.f$length(p2) then goto vaxc
$     if f$locate("D",p2).ne.f$length(p2) then goto decc
$ endif
$!
$DECC:
$ if f$search("SYS$SYSTEM:DECC$COMPILER.EXE").nes.""
$ then
$   say "DECC compiler found"
$   cc_ver = "DECC"
$   ccopt = "/decc/unsigned_char"+ccopt
$   goto compile
$ endif
$!
$VAXC:
$ if f$search("SYS$SYSTEM:VAXC.EXE").nes.""
$ then
$   say "VAXC compiler found, checking version..."
$!
$   if f$trnlnm("VAXC$INCLUDE") .eqs. ""
$   then
$     vaxc_h = "SYS$LIBRARY:"
$   else
$     vaxc_h = "VAXC$INCLUDE:"  ! keep as logical name, may be a search list
$   endif
$   cc_ver = "VAXC023"
$   if f$search("''vaxc_h'fscndef.h") .nes. "" then cc_ver = "VAXC024"
$   if f$search("''vaxc_h'ppl$routines.h") .nes. "" then cc_ver = "VAXC030"
$   if f$search("''vaxc_h'xabrudef.h") .nes. "" then cc_ver = "VAXC031"
$   if (cc_ver .lts. "VAXC031") then vaxc = 2
$   if (cc_ver .nes. "VAXC031")
$   then
$     type sys$input
WARNING: Your system has an older version of the C compiler.
         VMS C-Kermit was designed to be compiled under VAX C V3.1 or
         newer or DEC C V1.3 or newer.  It has not been verified to
         build properly under older compilers, athough pre-built C-Kermit
         versions should run properly.  Please exercise caution until you
         have verified proper operation.

$   endif
$!  If both DECC and VAXC are in this system, then use the /vaxc qualifier
$   if f$search("SYS$SYSTEM:DECC$COMPILER.EXE").nes."" then -
           ccopt = "/vaxc" + ccopt
$   goto compile
$ endif
$!
$GNUC:
$ if f$trnlnm("GNU_CC").nes.""
$ then
$   say "GNUC compiler found"
$   CC="GCC"
$   cc_ver="GNUC"+f$trnlnm("GNU_CC_VERSION")
$!
$Version_Loop:              ! convert period separator to underscore
$   dot = f$locate(".",cc_ver)
$   if dot .eq. f$length(cc_ver) then goto End_Version_Loop
$   cc_ver[dot,1] := "_"
$   goto Version_Loop
$End_Version_Loop:
$!
$   if debug.eq.0 then ccopt = "/nolist/optimize=4"
$   if .not. do_ckvcvt then write optf "gnu_cc:[000000]gcclib.olb/lib"
$   write aoptf "gnu_cc:[000000]gcclib.olb/lib"
$   noshare=1
$   goto compile
$ endif
$!
$! No compiler found - Warning and Exit
$!
$ if .not. do_ckvcvt then close optf
$ close aoptf
$ type sys$input
FATAL: No C-compiler found - Can't build Kermit on this system.

$ goto The_exit
$!
$COMPILE:
$!
$! say "C compiler: ''cc_ver', options: ''ccopt', command: ''CC'"
$ hw_mdl=f$getsyi("HW_MODEL")
$ if hw_mdl.ge.1024
$ then
$  non_vax=1
$  if hw_mdl.ge.4096
$  then
$   say f$fao("!/Operating System: OpenVMS(tm) IA64!/")
$  else
$   say f$fao("!/Operating System: OpenVMS(tm) Alpha!/")
$  endif
$! non-vax this is only option...recover a bit of DCL real estate
$  ccopt = ccopt-"/decc"
$! say ccopt
$ else
$  say f$fao("!/Operating System: OpenVMS(tm) VAX(tm)!/")
$ endif
$!
$! cc_ver could start with VAXC; DECC; or GNUC
$!
$! The VMS linker default is to link /SYSSHR
$!
$! C-Kermit default is to link w/ shareable libraries on non-VAX
$! and w/ object libraries on VAX. For backwards compatibility, use
$! p1 "S", to use shareable libraries on VAX, and p5 "/NOSYSSHARE" to
$! use object libraries on non-VAX.
$!
$ if (cc_ver.gts."DECC" .and. f$search("sys$share:vaxcrtl.exe").eqs."") .or. -
     (non_vax.eq.0 .and. vms_ver .lts. "VMS_V52") .or. -
     (non_vax.eq.0 .and. noshare.eq.1) .or. -
     (non_vax.eq.1 .and. (f$locate("/NOSYSS",p5).ne.f$length(p5)) )
$   then
$     noshare = 1
$     share_opt = "NOVMSSHARE"
$     share_text = "system OLBs and"
$   else
$     noshare = 0
$     share_opt = "VMSSHARE"
$     share_text = "shareable libs and"
$ endif
$!
$!      Find out which network to use
$!
$! Type:
$!    net_option = "NONET"
$! before running this procedure to build C-Kermit without TCP/IP network
$! support on a system that has a TCP/IP package installed, or use the
$! N command-line option to force NONET.
$!
$!
$ if f$search("SYS$LIBRARY:TCPIP$IPC_SHR.EXE") .nes. "" then ucxv5 = 1
$ if do_ckvcvt
$ then
$   net_option = "NONET"
$   goto Net_Done
$ endif
$!
$ if f$type(net_option) .eqs. "STRING"
$ then
$   say "Network option override = ''net_option'"
$   net_option = f$edit(net_option,"UPCASE")
$   goto Net_Done
$ endif
$!
$ net_option = "NONET"
$ if f$search("MULTINET:MULTINET_SOCKET_LIBRARY.EXE") .nes. ""
$ then
$   net_option = "MULTINET"
$ else
$  if f$search("TCPWARE:UCX$IPC.OLB") .nes. ""
$  then
$    net_option = "TCPWARE"
$  else
$   if f$search("TWG$TCP:[NETDIST.LIB]TWGLIB.OLB") .nes. ""
$   then
$     net_option = "WINTCP"
$   else
$    if (f$search("SYS$LIBRARY:UCX$ACCESS_SHR.EXE") .nes. "") -
       .or. (f$search("SYS$LIBRARY:TCPIP$ACCESS_SHR.EXE") .nes. "")
$    then
$      net_option = "DEC_TCPIP"
$    else
$     if f$search(f$parse(f$trnlnm("LIBCMU"),-
                         "cmuip_root:[syslib]libcmu.olb")) .nes. ""
$     then
$       net_option = "CMU_TCPIP"
$     endif ! CMU TCP/IP
$    endif ! DEC TCP/IP
$   endif ! Wollongong
$  endif ! Process
$ endif ! MultiNet
$!
$!
$ Net_Done:
$!
$ if f$type(net_option) .eqs. ""
$ then
$   net_option = "NONET"
$ endif
$!
$ if net_option .eqs. "NONET"
$ then
$   net_name = "no"
$   internal_ftp = 0
$ else
$  havetcp = 1
$  if net_option .eqs. "MULTINET"
$  then
$    net_name = "MultiNet"
$    write optf "multinet:multinet_socket_library.exe/share"
$  else
$   if net_option .eqs. "TCPWARE"
$   then
$    net_name = "Process Software TCPware"
$    write optf "tcpware:ucx$ipc.olb/library"
$    net_option = "TCPWARE,DEC_TCPIP"
$   else
$    if net_option .eqs. "WINTCP"
$    then
$      net_name = "WIN/TCP"
$      define/nolog vaxc$include twg$tcp:[netdist.include],sys$library
$      @twg$tcp:[netdist.misc]def
$      if noshare .eq. 0
$        then write optf "twglib/share"
$        else write optf "twg$common:[netdist.lib]twglib.olb/library"
$      endif
$    else
$     if net_option .eqs. "DEC_TCPIP"				!+1.24
$     then
$       net_name = "DEC TCP/IP Services for OpenVMS(tm)"
$       if non_vax.eq.0
$         then
$         if ucxv5
$           then
$           write optf "sys$library:tcpip$ipc_shr.exe/share" !1.31
$         else write optf "sys$library:ucx$ipc.olb/library"
$         endif
$       endif							!-1.24
$     else
$      if net_option .eqs. "CMU_TCPIP"
$      then
$        net_name = "CMU-OpenVMS/IP"
$        libcmu = f$search(f$parse(f$trnlnm("LIBCMU"), -
                                   "cmuip_root:[syslib]libcmu.olb"))
$        write optf "''libcmu'/library"
$      else
$        say "Unknown net_option: ''net_option'"
$        net_option = "NONET"
$        net_name = "no"
$      endif ! None
$     endif ! CMU TCP/IP
$    endif ! DEC TCP/IP
$   endif ! Wollongong
$  endif ! Process
$ endif ! MultiNet
$
$ if f$search("SYS$COMMON:[DECC$LIB.REFERENCE.DECC$RTLDEF]IF.H") -
   .nes. "" then if_dot_h = 1
$ if f$search("SYS$LIBRARY:TCPIP$IPC_SHR.EXE") .nes. "" then ucxv5 = 1
$ if net_option .nes. "DEC_TCPIP" then ucxv5 = 0
$ if net_option .nes. "DEC_TCPIP" then if_dot_h = 0
$!
$! Now specify the appropriate VAXCRTL
$! then close the option-files
$!
$ if (noshare.eq.1) .and. -
   ((cc_ver.gts."DECC") .or. (net_option .eqs. "CMU_TCPIP"))
$  then
$    if .not. do_ckvcvt then write optf "sys$share:vaxcrtl.olb/lib"
$    write aoptf "sys$share:vaxcrtl.olb/lib"
$  endif
$!
$ if (noshare.eq.0) .and. -
   ((cc_ver.gts."DECC") .or. (net_option .eqs. "CMU_TCPIP"))
$ then
$   if .not. do_ckvcvt then write  optf "sys$share:vaxcrtl.exe/share"
$   write aoptf "sys$share:vaxcrtl.exe/share"
$ endif
$!
$! Close the option-files
$!
$ if .not. do_ckvcvt then close optf
$ close aoptf
$!
$! Set compile prefix as a function of the TCP/IP stack for DEC C.  The
$! /PREFIX_LIBRARY_ENTRIES business is needed for MultiNet 3.2 and earlier,
$! but is not needed for 4.0.  Not sure about WINTCP.  Not sure where the
$! cutoff is.  CAUTION: There are limits on how long statements can be, and
$! how long string constants can be, and how long strings can be, even when
$! formed as below, by repeated concatenation.  These limits start out at
$! 254 or so, and go up to maybe 1023.  Don't add anything to these
$! strings (spaces, etc) that doesn't need to be there.
$!
$! also, DEC C and VMS >= 7.0 has its own ioctl
$!
$ if cc_ver.eqs."DECC"
$ then
$   if (net_option .eqs. "MULTINET") .or. (net_option .eqs. "WINTCP")
$   then
$     say "Adding /PREFIX for DECC and Multinet.."
$     ccopt = ccopt + "/PREF=(AL,EX=("
$     ccopt = ccopt + "accept,bind,connect,listen,select,"
$     ccopt = ccopt + "socket,recv,send,sendmsg,getservbyname,"
$     ccopt = ccopt + "getpeername,getsockname,getsockopt,setsockopt,"
$     ccopt = ccopt + "gethostbyname,gethostbyaddr,inet_addr,"
$     ccopt = ccopt + "inet_ntoa,inet_aton,htons,ntohs))"
$   else
$     if vms_ver .ges."VMS_V70" .and. -
         f$locate("DEC_TCPIP",net_option) .ne. f$length(net_option)
$     then
$       ccopt = ccopt + "/PREFIX_LIBRARY_ENTRIES=(AL,EX=ioctl)"
$     else
$       ccopt = ccopt + "/PREFIX_LIBRARY_ENTRIES=(ALL_ENTRIES)"
$     endif
$   endif
$ endif
$!
$! CFLAGS equivalent - local site options are added here
$!
$ if (vaxc .eq. 2) then cln_def = cln_def+",VAXCV2"
$ if vmsv6 then cln_def = cln_def+",VMSV60"
$ if vmsv7 then cln_def = cln_def+",VMSV70"
$ if vmsv8 then cln_def = cln_def+",VMSV80"
$ if ucxv5 then cln_def = cln_def+",UCX50"
$ if if_dot_h then cln_def = cln_def+",IF_DOT_H"
$ if havetcp then cln_def = cln_def+",TCPSOCKET"
$ if vms_ver .lts. "VMS_V62" then cln_def = cln_def+",NOHTTP,NOCMDATE2TM"
$!!! if vms_ver .lts. "VMS_V72" then cln_def = cln_def+",NOSETTIME"
$ if ssl .and. .not. compaq_ssl then -
    cln_def = cln_def+",OPENSSL_DISABLE_OLD_DES_SUPPORT"
$!
$ if_def=""
$ if internal_ftp
$ then
$  if_def=",NEWFTP"
$ endif
$!
$! LARGE_FILE NOTES :
$! Large file support can be configured on 64-bit platforms (not VAX)
$! in VMS 7.3 or later.  Strictly speaking, to build for this in VMS 7.3
$! we also need update ECO VMS73_ACRTL-V0200 to the C runtime library.
$! If at link time FSEEKO and FTELLO are not defined, it means this ECO
$! was not applied.  In that case clean out the .OBJ files and rebuild
$! with F option in P1 to disable large file support.
$!
$ if large_file.eq.0
$ then
$   say "Large file support disabled because: command line"
$ endif
$!
$ lf_def=""
$ if large_file .and. non_vax.eq.0 ! Disable for VAX
$ then
$  large_file=0
$  say "Large file support disabled becuase: VAX"
$ endif
$!
$ if large_file .and. vms_ver .lts. "VMS_V73" ! Disable for VMS pre-7.3
$ then
$  large_file=0
$  say "Large file support disabled because: VMS pre-7.3"
$ endif
$!
$ if large_file .and. non_vax
$ then    ! For Kermit:       For VMS:
$  lf_def=",_LARGEFILE_SOURCE,_LARGEFILE"
$  say "Large file support enabled"
$ endif
$!
$ ccdef="/def=(''net_option',''cc_ver',''vms_ver',''share_opt'"+-
        "''if_def' ''lf_def' ''cln_def')''cln_qua'"
$! say "length of ccopt is ''f$length(ccopt)'"
$! say "length of ccdef is ''f$length(ccdef)'"
$ ccopt = ccopt + ccdef
$ mmscln = f$length(ccopt)
$ if make .nes "" .and. mmscln .ge. mmsclm
$   then
$say "Warning: The 'ccopt' command is ''mmscln' characters which could make"
$say " the ''make' procedure fail. You may continue on by restarting with"
$say " either O (over-ride) flag or M (no MM_) flag set."
$ goto The_exit
$ endif
$!
$! To facilitate batch mode compilation, append /NOLIST and /NOMAP to
$! the compiler and linker options (not needed for INTERACTIVE or MMx)
$!
$ if (f$mode() .eqs. "BATCH") .and. (make .eqs. "")
$ then
$   ccopt = ccopt + "/NOLIST"
$   if debug .eq. 0 then lopt =  lopt + "/NOMAP"
$ endif
$!
$ say "Compiling Kermit sources ..."
$ set noon
$ tempsymb = "CCOPT = ""''ccopt'"""
$ write/symbol sys$output tempsymb
$ say "Kermit Source Path = ''f$trnlnm(""K"")'"
$ set on
$ if .not. do_ckvcvt then say -
 "Building WERMIT ''ck_version' with:"
$ if .not. do_ckvcvt then say -
 "''share_text' ''ssl_text' ''net_name' network support at ''f$time()"
$! if vmsv7 then say "VMSV7 detected"
$! if ucxv5 then say "UCXV5 detected"
$! if if_dot_h then say "<if.h> detected"
$ if Make.eqs.""
$ then
$!
$   if do_ckvcvt then goto CKVCVT
$!
$! Build the thing plain
$!
$   say ""
$   show symb ccopt
$   show symb lopt
$   say ""
$   say "  Compiling CKWART at ''f$time()"
$!
$! Note the use of single quotes (') apostrophe than double ('') in quoting
$! CCOPT, to prevent CCOPT from being expanded prior to the MAKE call, which
$! could result in the string being too long.  Using ' rather than '' forces
$! evaluation of CCOPT to occur in the MAKE routine itself.
$!
$   CALL MAKE ckwart.OBJ "'CC' 'CCOPT' K:ckwart" -
          "K:ckwart.c K:ckcsym.h K:ckcdeb.h K:ckclib.h"
$   say "  Linking   CKWART at ''f$time()"
$!
$   CALL MAKE ckwart.exe "LINK  ckwart,aux.opt/opt/NOMAP" ckwart.obj
$   say "  Running   CKWART at ''f$time()"
$   ckwart = "$" +f$parse("CKWART.EXE",,,"DEVICE") +-
            f$parse("CKWART.EXE",,,"DIRECTORY") + "CKWART"
$   CALL MAKE ckcpro.c "ckwart  K:ckcpro.w ckcpro.c" -
          "K:ckcpro.w"
$   say f$fao("!/  Compiling WERMIT files at ''f$time()")
$!
$! Note how MAKE args are combined in quotes to get around the limitation
$! on the number of arguments to a DCL procedure.
$!
$   CALL MAKE ckcfn2.OBJ "'CC' 'CCOPT' K:ckcfn2" -
          "K:ckcfn2.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcasc.h K:ckcker.h K:ckcxla.h K:ckuxla.h" -
          "K:ckcuni.h K:ckcnet.h K:ckvioc.h K:ckctel.h"
$!
$   CALL MAKE ckcfn3.OBJ "'CC' 'CCOPT' K:ckcfn3" -
          "K:ckcfn3.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcasc.h K:ckcker.h K:ckcxla.h K:ckuxla.h" -
          "K:ckcuni.h"
$!
$   CALL MAKE ckcfns.OBJ "'CC' 'CCOPT' K:ckcfns" -
          "K:ckcfns.c K:ckcsym.h K:ckcasc.h K:ckcdeb.h" -
          "K:ckclib.h K:ckcker.h K:ckcxla.h K:ckuxla.h" -
          "K:ckcuni.h K:ckcnet.h K:ckvioc.h K:ckctel.h"
$!
$   if internal_ftp
$   then
$     CALL MAKE ckcftp.obj "'CC' 'CCOPT' K:ckcftp" -
            "K:ckcftp.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
            "K:ckcsig.h K:ckcasc.h K:ckcker.h K:ckucmd.h" -
            "K:ckuusr.h K:ckcnet.h K:ckvioc.h K:ckctel.h" -
            "K:ckcxla.h K:ckuxla.h K:ckcuni.h K:ckuath.h" -
            "K:ck_ssl.h K:ckvrtl.h"
$!
$     CALL MAKE ckvrtl.obj "'CC' 'CCOPT' K:ckvrtl" -
            "K:ckvrtl.c K:ckvrtl.h"
$   endif
$!
$   CALL MAKE ckclib.OBJ "'CC' 'CCOPT' K:ckclib" -
          "K:ckclib.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcasc.h"
$!
$   CALL MAKE ckcmai.OBJ "'CC' 'CCOPT' K:ckcmai" -
          "K:ckcmai.c K:ckcsym.h K:ckcasc.h K:ckcdeb.h" -
          "K:ckclib.h K:ckcker.h K:ckcnet.h K:ckvioc.h" -
          "K:ckctel.h K:ck_ssl.h K:ckuusr.h K:ckucmd.h" -
          "K:ckuath.h K:ckcsig.h"
$!
$   CALL MAKE ckcnet.OBJ "'CC' 'CCOPT' K:ckcnet" -
          "K:ckcnet.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcker.h K:ckcasc.h K:ckcnet.h K:ckvioc.h" -
          "K:ckctel.h K:ck_ssl.h K:ckuusr.h K:ckucmd.h" -
          "K:ckuath.h K:ckcsig.h K:ckvrtl"
$!
$   CALL MAKE ckcpro.OBJ "'CC' 'CCOPT'/INCL=K: ckcpro" -
          "ckcpro.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcasc.h K:ckcker.h K:ckcnet.h K:ckvioc.h" -
          "K:ckctel.h"
$!
$   CALL MAKE ckctel.OBJ "'CC' 'CCOPT' K:ckctel" -
          "K:ckctel.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcker.h K:ckcnet.h K:ckvioc.h K:ckctel.h" -
          "K:ckuath.h K:ck_ssl.h"
$!
$   CALL MAKE ckcuni.OBJ "'CC' 'CCOPT' K:ckcuni" -
          "K:ckcuni.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcker.h K:ckucmd.h K:ckcxla.h K:ckuxla.h" -
          "K:ckcuni.h"
$!
$   CALL MAKE ckuath.obj "'CC' 'CCOPT' K:ckuath" -
          "K:ckuath.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcker.h K:ckuusr.h K:ckucmd.h K:ckcnet.h" -
          "K:ckvioc.h K:ckctel.h K:ckuath.h K:ckuat2.h" -
          "K:ck_ssl.h"
$!
$   CALL MAKE ckucmd.OBJ "'CC' 'CCOPT' K:ckucmd" -
          "K:ckucmd.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcker.h K:ckcnet.h K:ckvioc.h K:ckctel.h" -
          "K:ckucmd.h K:ckuusr.h K:ckcasc.h"
$!
$   CALL MAKE ckudia.OBJ "'CC' 'CCOPT' K:ckudia" -
          "K:ckudia.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcasc.h K:ckcker.h K:ckucmd.h K:ckcnet.h" -
          "K:ckvioc.h K:ckctel.h K:ckuusr.h K:ckcsig.h"
$!
$   CALL MAKE ckuscr.OBJ "'CC' 'CCOPT' K:ckuscr" -
          "K:ckuscr.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcasc.h K:ckcker.h K:ckuusr.h K:ckucmd.h" -
          "K:ckcnet.h K:ckvioc.h K:ckctel.h K:ckcsig.h"
$!
$   CALL MAKE ckusig.OBJ "'CC' 'CCOPT' K:ckusig" -
          "K:ckusig.c K:ckcsym.h K:ckcasc.h K:ckcdeb.h" -
          "K:ckclib.h K:ckcker.h K:ckcnet.h K:ckvioc.h" -
          "K:ckctel.h K:ckuusr.h K:ckucmd.h K:ckcsig.h"
$!
$   CALL MAKE ckuus2.OBJ "'CC' 'CCOPT' K:ckuus2" -
          "K:ckuus2.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcnet.h K:ckvioc.h K:ckctel.h K:ckcasc.h" -
          "K:ckcker.h K:ckuusr.h K:ckucmd.h K:ckcxla.h" -
          "K:ckuxla.h K:ckcuni.h"
$!
$   CALL MAKE ckuus3.OBJ "'CC' 'CCOPT' K:ckuus3" -
          "K:ckuus3.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcasc.h K:ckcker.h K:ckcxla.h K:ckuxla.h" -
          "K:ckcuni.h K:ckcnet.h K:ckvioc.h K:ckctel.h" -
          "K:ckuath.h K:ck_ssl.h K:ckuusr.h K:ckucmd.h"
$!
$   CALL MAKE ckuus4.OBJ "'CC' 'CCOPT' K:ckuus4" -
          "K:ckuus4.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcasc.h K:ckcker.h K:ckcnet.h K:ckvioc.h" -
          "K:ckctel.h K:ckuusr.h K:ckucmd.h K:ckuver.h" -
          "K:ckcxla.h K:ckuxla.h K:ckcuni.h K:ckuath.h" -
          "K:ck_ssl.h"
$!
$   CALL MAKE ckuus5.OBJ "'CC' 'CCOPT' K:ckuus5" -
          "K:ckuus5.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcasc.h K:ckcker.h K:ckuusr.h K:ckucmd.h" -
          "K:ckcnet.h K:ckvioc.h K:ckctel.h K:ckcxla.h" -
          "K:ckuxla.h K:ckcuni.h K:ck_ssl.h"
$!
$   CALL MAKE ckuus6.OBJ "'CC' 'CCOPT' K:ckuus6" -
          "K:ckuus6.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcasc.h K:ckcker.h K:ckuusr.h K:ckucmd.h" -
          "K:ckcxla.h K:ckuxla.h K:ckcuni.h K:ckcnet.h" -
          "K:ckvioc.h K:ckctel.h"
$!
$   CALL MAKE ckuus7.OBJ "'CC' 'CCOPT' K:ckuus7" -
          "K:ckuus7.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcasc.h K:ckcker.h K:ckcxla.h K:ckuxla.h" -
          "K:ckcuni.h K:ckcnet.h K:ckvioc.h K:ckctel.h" -
          "K:ckuusr.h K:ckucmd.h K:ckuath.h K:ck_ssl.h"
$!
$   CALL MAKE ckuusr.OBJ "'CC' 'CCOPT' K:ckuusr" -
          "K:ckuusr.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcasc.h K:ckcker.h K:ckcnet.h K:ckvioc.h" -
          "K:ckctel.h K:ckuusr.h K:ckucmd.h K:ckcxla.h" -
          "K:ckuxla.h K:ckcuni.h"
$!
$   CALL MAKE ckuusx.OBJ "'CC' 'CCOPT' K:ckuusx" -
          "K:ckuusx.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcasc.h K:ckcker.h K:ckuusr.h K:ckucmd.h" -
          "K:ckcxla.h K:ckuxla.h K:ckcuni.h K:ckcsig.h"
$!
$   CALL MAKE ckuusy.OBJ "'CC' 'CCOPT' K:ckuusy" -
          "K:ckuusy.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcasc.h K:ckcker.h K:ckucmd.h K:ckcnet.h" -
          "K:ckvioc.h K:ckctel.h K:ckuusr.h K:ckcxla.h" -
          "K:ckuxla.h K:ckcuni.h K:ck_ssl.h"
$!
$   CALL MAKE ckuxla.OBJ "'CC' 'CCOPT' K:ckuxla" -
          "K:ckuxla.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcker.h K:ckucmd.h K:ckcxla.h K:ckuxla.h" -
          "K:ckcuni.h"
$!
$   CALL MAKE ckvcon.OBJ "'CC' 'CCOPT' K:ckvcon" -
          "K:ckvcon.c K:ckcdeb.h K:ckclib.h K:ckcasc.h" -
          "K:ckcker.h K:ckucmd.h K:ckcnet.h K:ckvioc.h" -
          "K:ckctel.h K:ckvvms.h K:ckcxla.h K:ckuxla.h" -
          "K:ckcuni.h"
$!
$   CALL MAKE ckvfio.OBJ "'CC' 'CCOPT' K:ckvfio" -
          "K:ckvfio.c K:ckcdeb.h K:ckclib.h K:ckcasc.h" -
          "K:ckcker.h K:ckuusr.h K:ckucmd.h K:ckvvms.h"
$!
$   CALL MAKE ckvioc.OBJ "'CC' 'CCOPT' K:ckvioc" -
          "K:ckvioc.c K:ckcdeb.h K:ckclib.h K:ckvioc.h" -
          "K:ckcker.h K:ckuusr.h K:ckucmd.h K:ckvvms.h"
$!
$   CALL MAKE ckvtio.OBJ "'CC' 'CCOPT' K:ckvtio" -
          "K:ckvtio.c K:ckcdeb.h K:ckclib.h K:ckcasc.h" -
          "K:ckcker.h K:ckvvms.h K:ck_ssl.h K:ckcnet.h" -
          "K:ckvioc.h K:ckctel.h"
$!
$   CALL MAKE ck_crp.obj "'CC' 'CCOPT' K:ck_crp" -
          "K:ck_crp.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcnet.h K:ckvioc.h K:ckctel.h"
$!
$   CALL MAKE ck_ssl.obj "'CC' 'CCOPT' K:ck_ssl" -
          "K:ck_ssl.c K:ckcsym.h K:ckcdeb.h K:ckclib.h" -
          "K:ckcnet.h K:ckvioc.h K:ckctel.h K:ckuath.h" -
          "K:ckcker.h K:ckucmd.h K:ck_ssl.h"
$!
$   say "  Linking WERMIT at ''f$time()"
$   CALL MAKE wermit.exe "LINK/exe=wermit.exe 'lopt' kermit.opt/opt" *.obj
$   say "Done building WERMIT at ''f$time()"
$ goto The_exit
$!
$!
$CKVCVT:
$   say f$fao("!/Building CKVCVT at ''f$time()")
$   say "  Compiling CKVCVT at ''f$time()"
$   CALL MAKE ckvcvt.OBJ "'CC' 'CCOPT' K:ckvcvt" -
              K:ckvcvt.c
$   say "  Linking   CKVCVT at ''f$time()"
$   CALL MAKE ckvcvt.exe "LINK 'lopt' ckvcvt.obj,aux.opt/opt" ckvcvt.obj
$   write sys$output "Done building CKVCVT at ''f$time()"
$ else
$! ccopt gets _very_ loooong.  Shorten the MMS command line by prepending the
$! CCFLAGS macro to the mms file.  Note that the CC command line may now be
$! "at risk."  The OpenVMS User's Manual states:
$!
$!    Include no more than 127 elements (parameters, qualifiers, and
$!    qualifier values) in each command line.
$!
$!    Each element in a command must not exceed 255 characters.
$!    The entire command must not exceed 1024 characters after all symbols
$!    and lexical functions are converted to their values.
$!
$   open/write mmstemp ccflags.mms
$   ccopt = "CCFLAGS="+ccopt
$   write/symbol mmstemp ccopt
$   close mmstemp
$!
$ ftp_mac=""
$ if internal_ftp then ftp_mac=", INT_FTP=1"
$!
$   target = "WERMIT"
$   if do_ckvcvt then target = "CKVCVT"
$   'Make' 'target' /des=K:ckvker.mms/ignore=warn -
          /macro=(cc="''CC'", linkflags="''lopt'" 'ftp_mac')
$ endif	! make/mms
$ if (noshare .eq. 1)
$ then
$   type sys$input

 A link warning about an undefined symbol LIB$FIND_IMAGE_SYMBOL means
 you should link with the shareable library; add S to first parameter
 of CKVKER (and, if P5 is /NOSYSSHARE, omit that) and relink.

$ endif
$ if f$search("kermit.opt") .nes. "" then purge kermit.opt
$ if f$search("aux.opt") .nes. "" then purge aux.opt
$ if f$search("ccflags.mms") .nes. "" then purge ccflags.mms
$ if f$search("wermit.exe") .nes. "" then -
     set file/protection=(g:re,w:re) wermit.exe
$ if f$search("ckvcvt.exe") .nes. "" then -
     set file/protection=(g:re,w:re) ckvcvt.exe
$ say "Kermit build completed"
$goto The_exit
$!
$CLEAN:
$ if f$search("ckwart.exe") .nes. "" then delete/noconf/log ckwart.exe;*
$ if f$search("*.obj")      .nes. "" then delete/noconf/log *.obj;*
$ if f$search("ckcpro.c")   .nes. "" then delete/noconf/log ckcpro.c;*
$ say "Cleanup done"
$ say ""
$ goto The_exit
$!
$CY_exit:
$ $status = %x10000004
$!
$The_exit:
$ if f$trnlnm("VERSION_TMP") .nes. "" then close VERSION_TMP
$ if f$trnlnm("K", "LNM$PROCESS") .nes. "" then deassign K
$ if oldmath.eq.1 then deass mthrtl
$ if oldmath.eq.1 then deass vmthrtl
$!
$ x = f$verify(save_verify_procedure,save_verify_image)
$ exit $status
$!
$Bad_param:
$ write sys$output "ERROR: The first parameter should not include commas"
$ write sys$output "       P1 = "+ P1
$ write sys$output "       you may have used commas instead of spaces to
$ write sys$output "       seperate parameters"
$Exit
$!
$MAKE: SUBROUTINE   !SUBROUTINE TO CHECK DEPENDENCIES
$! P1 = What we are trying to make
$! P2 = Command to make it
$! P3 - P8  What it depends on
$
$ If F$Search(P1) .Eqs. "" Then Goto Makeit
$ Time = F$CvTime(F$File(P1,"RDT"))
$arg=3
$Make_Loop:
$       Argument = P'arg
$       If Argument .Eqs. "" Then Goto Make_exit
$       El=0
$Loop2:
$       File = F$Element(El," ",Argument)
$       If File .Eqs. " " Then Goto Endl
$       AFile = ""
$Loop3:
$       OFile = AFile
$       AFile = F$Search(File)
$       If AFile .Eqs. "" .Or. AFile .Eqs. OFile Then Goto NextEl
$       If F$CvTime(F$File(AFile,"RDT")) .Ges. Time Then Goto Makeit
$       Goto Loop3
$NextEL:
$       El = El + 1
$       Goto Loop2
$EndL:
$ arg=arg+1
$ If arg .Le. 8 Then Goto Make_Loop
$ Goto Make_Exit
$
$Makeit:
$ say P2
$ 'P2'
$Make_Exit:
$ exit
$ENDSUBROUTINE
$!
$warning_exit:
$ status = $status
$ sev = $severity
$ set noon
$ xtext = f$message(status)
$ say "Warning:"
$ say "''xtext'"
$ goto the_exit
$!
