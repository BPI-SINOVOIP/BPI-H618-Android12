$!
$! CKVKER.COM - C-Kermit 8.0 Construction for (Open)VMS
$!
$! Version 1.23, 12 Nov 2002
$!
$! 04-Oct-97 1.17 lh  Updated to CKVKER.COM 1.17
$!                    C_Kermit 6.1.193.Alpha07
$! 25-Dec-98 1.18 fdc Change C-Kermit version number to 7.0.195.
$!  6-Jan-99 1.19 fdc Add new CKCLIB and CKCTEL modules.
$! 21-Mar-99 1.20 lh  Update to CKVKER 1.20 (add X,  ucxv5, if_dot_h)
$!  6-Sep-99 1.21 wb  Update for ckcuni.[ch]
$! 12-Dec-01 1.22 fdc Update version numbers.
$! 12-Nov-02 1.23 lh  apply various CKVKER edits
$!                      1.23 TCPSOCKET
$!                      1.24, 1.25, 1.31  UCX V5.0,V5.1
$!                      1.27 if_dot_h UCX only
$!                      1.28 DECC /UNSIGNED_CHAR
$!                      1.32 ckuusy.c dependencies
$!                      1.33 VMS60 symbol
$!                           VAX C version test with a search library
$!
$! A modified version of CKVKER.COM, originally based on
$! CKVKER.COM  Version 1.15, 05 December 1996 for C-Kermit 6.0.192
$! and updated to include subsequent CKVKER.COM revs.
$!
$! CKVOLD.COM uses DCL syntax compatible with VAX/VMS 4.x; it may also
$! be used in place of CKVKER.COM with later {Open}VMS releases.
$!
$! It has been used to build C-Kermit v 6.0.192 with
$!     VMS 4.4, VAX C 2.4; NONET, WINTCP v3.2
$!              gcc  1.42; NONET
$!
$! See CKVKER.COM for documention on building VMS C-Kermit
$! in general
$!
$! modifications specific to VMS 4 are
$!   - the compiler macro "OLD_VMS" is automatically defined
$!   - a compiler macro "OLD_TWG" is used for code modifications
$!     specific to WINTCP v3.2.  If that is defined in P3,
$!     the symbol "oldtwg" then selects the link libraries for
$!     that TCPIP package
$!
$!    OLD_TWG and oldtwg may serve as a model for TCPIP
$!    packages on older systems that do not have the standard
$!    configuration of CKVKER.  However, features implemented
$!    in the various TCPIP packages may differ from the current
$!    versions, so CKCNET.H and CKCNET.C may also need to be
$!    examined for appropriate options/actions with respect to
$!    other packages.
$!
$!  Lucas Hart, Oregon State University, Nov 1996.
$!
$     if p1 .eqs. "" then goto Skip_Help
$     if (f$locate(",",p1).ne.f$length(p1)) then goto Bad_param
$     p1 = f$edit(p1,"UPCASE")
$     if (f$locate("H",p1).eq.f$length(p1)) .and. -
         (f$locate("?",p1).eq.f$length(p1)) then goto Skip_Help
$!
$Help:
$type sys$input
   Usage:
       $ @[directory]ckvker p1 [ p2 [ p3 [ p4 [ p5] ] ] ]

       P1 = Build options
       P2 = Compiler selection
       P3 = C-Kermit DEFINES
       P4 = Additional compiler qualifiers
       P5 = Link qualifiers

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
       "" = Null place holder; use the defaults

   P2 compiler_selection
       D = DEC C
       V = VAX C
       G = GNU C
       "" = Use the first compiler found, searching in the above order

   P3    C-Kermit options (enclosed in quotes if more than one) including
          NOPUSH                for security
          NODEBUG               to reduce size of executable

          ""      Empty string; only needed as a spaceholder
                  when other parameters follow.

         Note: Default C-Kermit options that can be negated, NO.....,
         (see CKCCFG.DOC), as well as options that are not previously
         defined, should be affected by specifying the flag here.
         However, CKCDEB.H must be edited to "undefine" other defaults,
         e.g. CK_CURSES.

   P4    Compiler qualifiers (enclosed in quotes)if desired; provides
         additional flexibility, e.g., to change the compiler optimization,
         "/OPT=LEV=2"

   P5    Linker qualifiers, e.g., the linker default is to search the
         system shareable image library, IMAGELIB.OLB,  before the
         object module STARLET.OLB.  To build an image that may run on
         older system you might want to try linking /NOSYSSHR
 Example:

      $ @ckvker snmd ""  "NOPUSH, NODEBUG" "/NOOPT"

      NOPUSH  - Disallow access to DCL from within Kermit.
      NODEBUG - Remove debugging code to make C-Kermit smaller and faster.

      This procedure should be stored in the same directory as the source
      files.  You can SET DEFAULT to that directory and execute the procedure
      as shown above, or you can SET DEFAULT to a separate directory and run
      run the procedure out of the source directory, e.g.:

      SET DEFAULT DKA300:[KERMIT.ALPHA]
      @DKA300:[KERMIT.SOURCE]CKVKER

      This puts the object and executable files into your current directory.
      Thus you can have (e.g.) and Alpha and an VAX build running at the
      same time from the same source.  Similarly, you can define a logical
      name for the source directory:

      DEFINE CK_SOURCE DKA300:[KERMIT.SOURCE]

      and then no matter which directory you run this procedure from, it
      will look in the CK_SOURCE logical for the source files.

   NOTES:
      If adding flags here makes ccopt too long for Multinet/Wollongong
      you will have to do it in CKCDEB.H.

      The empty string, "", is needed only as a space holder if additional
      strings follow.

   Works like MAKE in that only those source modules that are newer than the
   corresponding object modules are recompiled.  Changing the C-Kermit command
   line DEFINES or compiler options does not affect previously compiled
   modules. To force a particular module to be recompiled, delete the object
   file first.  To force a full rebuild:

   $  @ckvker c
   $  @ckvker <desired-options>

   To use in batch, set up the appropriate directories and submit
   (/NOLIST and /NOMAP will be defined unless P1 includes "D")
    e.g., submit CKVKER /parameters=(SL,"","NODEBUG")

   See the CKVINS.DOC and CKVKER.BWR files for further information.

$Exit
$!
$!
$! Uses MMS if it is installed, unless the M option is included.  If CKVKER.MMS
$! is missing, you'll get an error; if MMS is not runnable for some reason
$! (privilege, image mismatch, etc), you'll also get an error.  In either case,
$! simply bypass MMS by including the M option.
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
$! more than one TCP/IP product is installed, the search proceeds in this
$! order: MULTINET, TCPWARE, WINTCP, DEC_TCPIP, CMU_TCPIP.
$!
$! Should work for all combinations of VAXC/DECC/GCC, VAX/Alpha, and any of
$! the following TCP/IP products: DEC TCP/IP (UCX), Cisco (TGV) MultiNet,
$! Attachmate (Wollongong) WINTCP (Pathway), Process Software TCPware,
$! or CMU/Tektronix TCP/IP (except CMU/Tek is not available for the Alpha).
$! VAX C is supported back to version 3.1, and DEC C back to 1.3.
$! Tested on VMS versions back to 5.4, but should work back to VAX/VMS 5.0,
$! and hopefully even into the 4.x's.
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
$!  fdc = Frank da Cruz, Columbia U, fdc@columbia.edu
$!  mf  = Mike Freeman, Bonneville Power Authority, freeman@columbia.edu
$!  cf  = Carl Friedberg, Comet & Company, carl@comets.com
$!  hg  = Hunter Goatley, Process Software, goathunter@goat.process.com
$!  lh  = Lucas Hart, Oregon State U, hartl@ucs.orst.edu
$!  js  = John Santos, Evans Griffiths & Hart, john@egh.com
$!  jw  = Joellen Windsor, U of Arizona, windsor@ccit.arizona.edu
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
$! 04-Nov-86 1.11 lh  A. Allow CFLAG options as command-line parameter p3
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
$! 05-Nov-86 1.12 fdc Clean up and amplify help text and add VMS >= 7.0 test.
$! 06-Nov-86 1.12 hg  Remove extraneous comma in VMS >= 7.0 test.
$! 08-Nov-96 1.13 js  Fixes to CMU/Tek build.
$! 23-Nov-96 1.14 lh  Fixes for VMS V7, VAXCRTL links for all TCP/IP packages,
$!                    improved batch operation, add P5 for link options,
$!                    catch commas in P1.
$! 05-Dec-96 1.15 lh  Fixes to work with GCC.
$!
$!
$Skip_Help:
$!
$ On Control_Y then $ goto CY_Exit
$! On ERROR then $ goto The_exit
$ On SEVERE_ERROR then $ goto The_exit
$! On Warning then goto warning_exit
$!
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
$ alpha=0
$ debug=0
$ noshare=1
$ decc=0
$ vaxc=0
$ verify=0
$ gnuc=0
$ oldmath=0
$ mathlevel=0
$ vmsv4=0
$ vmsv6=0
$ vmsv7=0
$ havetcp=0
$ ucxv5=0
$ if_dot_h=0
$ oldvms=0
$ oldsrc=0
$ oldobj=0
$ nomms=0
$ mmsclm=255        ! maximum command length limit for MMS/MMK (estimate)
$ do_ckvcvt=0
$!
$! Find out which OpenVMS version we are running
$! (do not use IF ... ENDIF for the VMS 4 test and exit)
$!
$ sys_ver = f$edit(f$getsyi("version"),"compress")
$ if f$extract(0,1,sys_ver) .eqs. "V" then goto Production_version
$     type sys$input
WARNING: You appear to be running a Field Test version of VMS.
         Please exercise caution until you have verified proper operation.

$Production_version:
$!
$     dot = f$locate(".",sys_ver)
$     sys_maj = 0+f$extract(dot-1,1,sys_ver)
$     sys_min = 0+f$extract(dot+1,1,sys_ver)
$!
$     if sys_maj .eq. 4 then if (sys_min/2)*2 .ne. sys_min then -
        sys_min = sys_min - 1
$     if sys_maj .ne. 4 then goto Supported_version
$     vmsv4 = 1
$     say f$fao("!/!_You are running VAX VMS ''sys_ver'")
$     type sys$input

WARNING: VMS C-Kermit is maintained with more recent VMS releases.
         Please exercise caution until you have verified proper operation.

$!
$!
$Supported_version:
$!
$     vms_ver = "VMS_V''sys_maj'''sys_min'"
$
$! VMSV70 must be defined if the VMS version is 7.0 OR GREATER, so we know
$! we can include <strings.h>. We need this information again, so use
$! a variable.
$
$	if vms_ver .ges."VMS_V60" then vmsv6=1
$	if vms_ver .ges."VMS_V70" then vmsv7=1
$!
$!
$! Set the Kermit Source Path KSP: to be the same path as this procedure
$! if the user has not specified another source with a CK_SOURCE logical
$!
$     if f$trnlnm("CK_SOURCE") .nes. ""  then goto Set_source
$!
$     source_device = f$parse(f$environment("procedure"),,,"device")
$     source_directory = f$parse(f$environment("procedure"),,,"directory")
$     define KSP 'source_device''source_directory
$Goto Build_opt
$!
$Set_source:
$     user_source = f$trnlnm("CK_SOURCE")
$     define KSP 'user_source'
$Goto Build_opt
$!
$Build_opt:
$ if p1.eqs."" then goto Compiler_opt
$     if f$locate("V",p1).ne.f$length(p1) then verify=1
$     if verify .eq. 1 then set verify
$     if f$locate("D",p1).ne.f$length(p1) then debug=1
$     if f$locate("S",p1).ne.f$length(p1) then noshare=0
$     if f$locate("L",p1).ne.f$length(p1) then mathlevel=1
$     if f$locate("M",p1).ne.f$length(p1) then nomms=1
$     if f$locate("N",p1).ne.f$length(p1) then net_option="NONET"
$     if f$locate("C",p1).ne.f$length(p1) then goto clean
$     if f$locate("X",p1).ne.f$length(p1) then do_ckvcvt=1
$!
$     if f$locate("O",p1).ne.f$length(p1) then mmsclm = 1024
$     if f$locate("W",p1).ne.f$length(p1) then On Warning then goto warning_exit
$! we have already tested for "H", help
$Goto Compiler_opt
$!
$Compiler_opt:
$!
$!  customize for local configuration
$!
$! TWG 3.2 (WINTCP, May 1988)
$!   "OLD_TWG" is a macro used in CKCNET
$!   oldtwg is used to customize the link libraries (see WINTCP_opt)
$!   and we overide any net_option symbol
$!
$     oldtwg=0
$     if p3.nes."" then p3 = f$edit(p3,"UPCASE")
$     if p3.nes."" then if f$locate("OLD_TWG",p3).ne.f$length(p3) then oldtwg=1
$     if oldtwg then net_option = "WINTCP"
$!
$!
$     cln_def = ""
$     if p3 .nes. "" then cln_def = ","+p3  ! comma delimited string
$!
$     cln_qua = ""
$     if p4 .nes. "" then cln_qua = p4
$!
$!
$     if debug.eq.1 then ccopt = "/noopt/deb"
$     if debug.eq.1 then  lopt = "/deb/map/full/sym"
$!
$     if p5 .nes. "" then p5 = f$edit(p5,"UPCASE")
$     if p5 .nes. "" then lopt = lopt + p5
$!
$! Check for MMK/MMS
$!
$     if nomms .eq. 0 then goto Check_mms
$     if nomms .eq. 1 then goto Build_opt_files
$!
$Check_mms:
$     if f$search("sys$system:mms.exe") .nes. "" then make = "MMS"
$     if (verify) .and.(make.nes."") then make = "MMS/LOG/VERIFY"
$     if f$type(MMK) .eqs. "STRING" then make = "MMK"
$     if make .nes. "" then say "Using ''make' utility"
$Goto Build_opt_files
$!
$! Build the option-file
$!
$Build_opt_files:
$     open/write aoptf aux.opt
$     if do_ckvcvt then goto end_init_optf
$     open/write optf kermit.opt
$     write optf "ckcmai.obj"
$     write optf "ckclib.obj"
$     write optf "ckcfn2.obj"
$     write optf "ckcfn3.obj"
$     write optf "ckcfns.obj"
$     write optf "ckcuni.obj"
$     write optf "ckcpro.obj"
$     write optf "ckucmd.obj"
$     write optf "ckudia.obj"
$     write optf "ckuscr.obj"
$     write optf "ckuus2.obj"
$     write optf "ckuus3.obj"
$     write optf "ckuus4.obj"
$     write optf "ckuus5.obj"
$     write optf "ckuus6.obj"
$     write optf "ckuus7.obj"
$     write optf "ckuusr.obj"
$     write optf "ckuusx.obj"
$     write optf "ckuusy.obj"
$     write optf "ckuxla.obj"
$     write optf "ckvcon.obj"
$     write optf "ckcnet.obj"
$     write optf "ckctel.obj"
$     write optf "ckvfio.obj"
$     write optf "ckvtio.obj"
$     write optf "ckvioc.obj"
$     write optf "ckusig.obj"
$     if (vmsv4) then write optf "ckvold.obj"
$     write optf "Identification=""Kermit 8.0.206"""
$end_init_optf:
$!
$!
$! Look for old math-library to allow transfer of the production Kermit
$! to old-fashioned VMS systems
$!
$     if f$search("SYS$SHARE:FORTRAN$MTHRTL-VMS.EXE") .nes. "" then -
        oldmath=1
$     if mathlevel.eq.1  then oldmath=0
$     if   oldmath.eq.0  then goto Compiler_type
$!
$     define/nolog mthrtl fortran$mthrtl-vms
$     define/nolog vmthrtl fortran$vmthrtl-vms
$     type sys$input
NOTE: You have currently DEC Fortran V6.0 or later installed, but the
      old versions of the Math libraries are still available on your
      system. We will link C-Kermit with these older, pre-Fortan V6
      libraries so that it will run on systems which don't have Fortran
      V6 installed. C-Kermit does not use any features of the new
      libraries.

      You will receive %LINK-I-IDMISMCH informational messages during
      linking, but these can be safely ignored.
$Goto Compiler_type
$!
$Compiler_type:
$!
$! Look for the compiler used
$!
$ if p2.eqs."" then goto DECC
$     p2 = f$edit(p2,"UPCASE")
$     if f$locate("G",p2).ne.f$length(p2) then goto gnuc
$     if f$locate("V",p2).ne.f$length(p2) then goto vaxc
$     if f$locate("D",p2).ne.f$length(p2) then goto decc
$Goto DECC
$!
$DECC:
$ if f$search("SYS$SYSTEM:DECC$COMPILER.EXE").eqs."" then goto VAXC
$     say "DECC compiler found"
$     cc_ver = "DECC"
$     ccopt = "/decc/unsigned_char"+ccopt
$Goto compile
$!
$VAXC:
$ if f$search("SYS$SYSTEM:VAXC.EXE").eqs."" then goto GNUC
$     say "VAXC compiler found, checking version..."
$     vaxc_h = "SYS$LIBRARY:"
$     if f$trnlnm("VAXC$INCLUDE") .nes. "" then vaxc_h = "VAXC$INCLUDE:"
$     cc_ver = "VAXC023"
$     if f$search("''vaxc_h'fscndef.h") .nes. "" then cc_ver = "VAXC024"
$     if f$search("''vaxc_h'ppl$routines.h") .nes. "" then cc_ver = "VAXC030"
$     if f$search("''vaxc_h'xabrudef.h") .nes. "" then cc_ver = "VAXC031"
$     if (cc_ver .lts. "VAXC031") then vaxc = 2
$     if (cc_ver .eqs. "VAXC031") then goto Check_dual
$     type sys$input
WARNING: Your system has an older version of the C compiler.
         VMS C-Kermit was designed to be compiled under VAX C V3.1 or
         newer or DEC C V1.3 or newer.  It has not been verified to
         build properly under older compilers, athough pre-built C-Kermit
         versions should run properly.  Please exercise caution until you
         have verified proper operation.

$Goto Check_dual
$!
$Check_dual:
$!  If both DECC and VAXC are in this system, then use the /vaxc qualifier
$     if f$search("SYS$SYSTEM:DECC$COMPILER.EXE").nes."" then -
        ccopt = "/vaxc" + ccopt
$Goto compile
$
$!
$GNUC:
$     if f$trnlnm("GNU_CC").eqs."" then goto NoCC
$     say "GNUC compiler"
$     CC="GCC"
$     cc_ver="GNUC"+f$trnlnm("GNU_CC_VERSION")
$!
$Version_Loop:              ! convert period separator to underscore
$     dot = f$locate(".",cc_ver)
$     if dot .eq. f$length(cc_ver) then goto End_Version_Loop
$     cc_ver[dot,1] := "_"
$goto Version_Loop
$!
$End_Version_Loop:
$!
$     if debug.eq.0 then ccopt = "/nolist/optimize=4"
$     if .not. do_ckvcvt then write optf "gnu_cc:[000000]gcclib.olb/lib"
$     write aoptf "gnu_cc:[000000]gcclib.olb/lib"
$     noshare=1
$Goto compile
$!
$NoCC:
$! No compiler found - Warning and Exit
$!
$     if .not. do_ckvcvt then close optf
$     close aoptf
$     type sys$input
FATAL: No C-compiler found - Can't build Kermit on this system.

$Goto The_exit
$!
$COMPILE:
$     say "C compiler: ''cc_ver', options: ''ccopt', command: ''CC'"
$     if vmsv4 then goto VAX_arch
$!
$     if f$getsyi("HW_MODEL").ge.1024 then alpha=1
$     if alpha .ne. 1 then goto VAX_arch
$!
$     say f$fao("!/Operating System: OpenVMS(tm) Alpha!/")
$! alpha this is only option...recover a bit of DCL real estate
$     ccopt = ccopt-"/decc"
$Goto Set_share
$!
$VAX_arch:
$     if vms_ver .lts. "VMS_V55" then -
        say f$fao("!/Operating System: VAX/VMS(tm)!/")
$     if vms_ver .ges. "VMS_V55" then -
        say f$fao("!/Operating System: OpenVMS(tm) VAX(tm)!/")
$Goto Set_share
$!
$Set_share:
$! cc_ver could start with VAXC; DECC; or GNUC
$!
$! The linker default is to link /SYSSHR
$! Only VAXCRTL and (and the TCPIP library if appropriate)
$! is affected by the "S" build flag, unless the line with
$! /NOSYSSHR is uncommented
$!
$!$ if (cc_ver.gts."DECC" .and. f$search("sys$share:vaxcrtl.exe").eqs."") .or. -
$!     (vms_ver .lts. "VMS_V52") .or. noshare.eq.1
$!
$! VMS V4 DCL doesn't have an if ... then ... else ... endif
$!
$     if alpha.eq.0 then -
        if (cc_ver.gts."DECC" .and. f$search("sys$share:vaxcrtl.exe").eqs."") -
        .or. (vmsv4.eq.0 .and. vms_ver .lts. "VMS_V52") .or. noshare.eq.1 -
        then goto noshare_opt
$!
$     noshare = 0                      ! Alpha default is SHARE
$     if f$locate("/NOSYSS",p5).ne.f$length(p5) then noshare = 1
$     if noshare .eq. 1 then goto noshare_opt
$!
$     share_opt  = "VMSSHARE"
$     share_text = "shareable libs and"
$goto end_share_opt
$!
$noshare_opt:
$     share_opt  = "NOVMSSHARE"
$     share_text = "system OLBs and"
$!
$end_share_opt:
$!
$!      Find out which network to use
$!
$! Type:
$!    net_option = "NONET"
$! before running this procedure to build C-Kermit without TCP/IP network
$! support on a system that has a TCP/IP package installed, or use the
$! N command-line option to force NONET.
$!
$     if .not. do_ckvcvt then goto check_net_option
$     net_option = "NONET"
$Goto Net_Done
$!
$check_net_option:
$     if f$type(net_option) .nes. "STRING" then goto Find_net
$!
$     say "Network option override = ''net_option'"
$     net_option = f$edit(net_option,"UPCASE")
$Goto Net_Done
$!
$Find_Net:
$!
$     net_option = ""
$     if f$search("MULTINET:MULTINET_SOCKET_LIBRARY.EXE") .nes. "" then -
        net_option = "MULTINET"
$     if net_option .nes "" then goto Net_Done
$!
$     if f$search("TCPWARE:UCX$IPC.OLB") .nes. ""  then -
        net_option = "TCPWARE"
$     if net_option .nes "" then goto Net_Done
$!
$     if f$search("TWG$TCP:[NETDIST.LIB]TWGLIB.OLB") .nes. ""  then -
        net_option = "WINTCP"
$     if net_option .nes "" then goto Net_Done
$!
$     if f$search("SYS$LIBRARY:UCX$ACCESS_SHR.EXE") .nes. ""  then -
        net_option = "DEC_TCPIP"
$     if net_option .nes "" then goto Net_Done
$!
$     if f$search(f$parse(f$trnlnm("LIBCMU"),-
                         "cmuip_root:[syslib]libcmu.olb")) .nes. "" -
        then net_option = "CMU_TCPIP"
$     if net_option .nes "" then goto Net_Done
$!
$     if net_option .eqs. "" then net_option = "NONET"
$     if net_option .nes "" then goto Net_Done
$!
$Net_Done:
$!
$!    The net may have been selected manually or automatically
$!    First, the name of the link library for the specified TCPIP
$!    package will be put in the option file, kermit.opt, and then
$!    the VAXCRTL link object, if needed, will be added to the option
$!    file, and placed in the auxillary option file, aux.opt.
$!
$     if net_option .eqs. "NONET"     then goto NoNet_opt
$     havetcp = 1
$     if net_option .eqs. "MULTINET"  then goto Multinet_opt
$     if net_option .eqs. "TCPWARE"   then goto TCPware_opt
$     if net_option .eqs. "WINTCP"    then goto WINTCP_opt
$     if net_option .eqs. "DEC_TCPIP" then goto UCX_opt
$     if net_option .eqs. "CMU_TCPIP" then goto CMU_opt
$Goto Unknown_net
$!
$!
$NONet_opt:
$     net_name = "no"
$     havetcp = 0
$Goto End_Net_opt
$!
$!
$Multinet_opt:
$     net_name = "MultiNet"
$     write optf "multinet:multinet_socket_library.exe/share"
$Goto End_Net_opt
$!
$TCPware_opt:
$     net_name = "Process Software TCPware"
$     write optf "tcpware:ucx$ipc.olb/library"
$     net_option = "TCPWARE,DEC_TCPIP"
$Goto End_Net_opt
$!
$WINTCP_opt:
$     net_name = "WIN/TCP"
$     define/nolog vaxc$include twg$tcp:[netdist.include],sys$library
$     define/nolog sys twg$tcp:[netdist.include.sys]
$     if oldtwg  then goto Oldtwg_opt
$     write optf "twg$common:[netdist.lib]twglib.olb/library"
$Goto End_Net_opt
$!
$Oldtwg_opt:
$     define/nolog vms twg$tcp:[netdist.include.vms]
$     write optf "twg$tcp:[netdist.lib]LIBNET.OLB/library"
$     write optf "twg$tcp:[netdist.lib]LIBNETACC.OLB/library"
$     write optf "twg$tcp:[netdist.lib]LIBNET.OLB/library"
$Goto End_Net_opt
$!
$UCX_opt:
$     net_name = "DEC TCP/IP Services for OpenVMS(tm)"
$     if f$search("SYS$LIBRARY:TCPIP$IPC_SHR.EXE") .nes. "" then ucxv5 = 1
$     if (alpha.eq.0) .and. (ucxv5 .eq.0) then -
        write optf "sys$library:ucx$ipc.olb/library"
$     if (alpha.eq.0) .and. (ucxv5 .eq.1) then -
        write optf "sys$library:tcpip$ipc_shr.exe/share" !1.31
$Goto End_Net_opt
$!
$CMU_opt:
$     net_name = "CMU-OpenVMS/IP"
$     libcmu = f$search(f$parse(f$trnlnm("LIBCMU"), -
                               "cmuip_root:[syslib]libcmu.olb"))
$     write optf "''libcmu'/library"
$Goto End_Net_opt
$!
$Unknown_net:
$     say "Unknown net_option: ''net_option', building as NONET"
$     net_option = "NONET"
$     net_name = "no"
$     havetcp = 0
$Goto End_Net_opt
$!
$End_Net_opt:
$!
$! First specify the appropriate VAXCRTL then
$! close the option-files
$!
$! CKVKER.COM does not specify vaxcrtl for TCPWARE; delete here
$! or is it version/architecture dependent?
$!
$     if do_ckvcvt then goto close_aoptf
$     if (noshare.eq.1) .and. -
        (cc_ver.gts."DECC" .or. net_option .eqs. "CMU_TCPIP" .or. -
        net_option .eqs. "TCPWARE")  then -
        write  optf "sys$share:vaxcrtl.olb/lib"
$     if (noshare.eq.0) .and. -
        (cc_ver.gts."DECC" .or. net_option .eqs. "CMU_TCPIP" .or. -
        net_option .eqs. "TCPWARE")  then -
        write  optf "sys$share:vaxcrtl.exe/share"
$     close optf
$!
$close_aoptf:
$     if (noshare.eq.1) .and. cc_ver.gts."DECC" then -
        write aoptf "sys$share:vaxcrtl.olb/lib"
$     if (noshare.eq.0) .and.(cc_ver.gts."DECC") then -
        write aoptf "sys$share:vaxcrtl.exe/share"
$     close aoptf
$!
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
$     if cc_ver.gts."DECC"  then goto Set_cflags
$!
$     if f$search("SYS$COMMON:[DECC$LIB.REFERENCE.DECC$RTLDEF]IF.H") -
        .nes. "" then if_dot_h = 1
$     if f$search("SYS$LIBRARY:TCPIP$IPC_SHR.EXE") .nes. "" then ucxv5 = 1
$     if net_option .nes. "DEC_TCPIP" then ucxv5 = 0
$     if net_option .nes. "DEC_TCPIP" then if_dot_h = 0
$!
$!   These packages provide their own header files
$     if (net_option .nes. "MULTINET") .and. (net_option .nes. "WINTCP") -
        then goto DEC_headers
$!
$     say "Adding /PREFIX for DECC and Multinet.."
$     ccopt = ccopt + "/PREF=(AL,EX=("
$     ccopt = ccopt + "accept,bind,connect,listen,select,"
$     ccopt = ccopt + "socket,recv,send,sendmsg,getservbyname,"
$     ccopt = ccopt + "getpeername,getsockname,getsockopt,setsockopt,"
$     ccopt = ccopt + "gethostbyname,gethostbyaddr,inet_addr,"
$     ccopt = ccopt + "inet_ntoa,htons,ntohs))"
$Goto Set_cflags
$!
$DEC_headers:
$!   use DEC C header files
$!   OpenVMS >= 7.0 has a system ioctl; that is used by both
$!   DEC_TCPIP and TCPware
$     if vmsv7 .and. -
        (f$locate("DEC_TCPIP",net_option) .ne. f$length(net_option)) -
        then goto Special_case
$!
$! the rest of the DEC C
$     ccopt = ccopt + "/PREFIX_LIBRARY_ENTRIES=(ALL_ENTRIES)"
$Goto Set_cflags
$!
$Special_case:
$     ccopt = ccopt + "/PREFIX_LIBRARY_ENTRIES=(AL,EX=ioctl)"
$Goto Set_cflags
$!
$Set_cflags:
$! CFLAGS equivalent - local site options are added here
$!
$     if (vaxc .eq. 2) then cln_def = cln_def+",VAXCV2"
$     if vmsv4  then cln_def = cln_def+",OLD_VMS"
$     if vmsv6  then cln_def = cln_def+",VMSV60"
$     if vmsv7  then cln_def = cln_def+",VMSV70"
$     if ucxv5 then cln_def = cln_def+",UCX50"
$     if if_dot_h then cln_def = cln_def+",IF_DOT_H"
$     if havetcp then cln_def = cln_def+",TCPSOCKET"
$!
$     ccdef="/def=(''net_option',''cc_ver',''vms_ver',''share_opt'"+-
            "''cln_def')''cln_qua'"
$! say "length of ccopt is ''f$length(ccopt)'"
$! say "length of ccdef is ''f$length(ccdef)'"
$     ccopt = ccopt + ccdef
$     mmscln = f$length(ccopt)
$     if make .eqs "" .or. mmscln .lt. mmsclm  then goto build
$say "Warning: The 'ccopt' command is ''mmscln' characters which could cause the"
$say "         ''make' procedure to fail. You may continue on by restarting"
$say "         with the either O (over-ride) flag or M (no MM_) flag set."
$Goto The_exit
$
$!
$Build:
$     say "Compiling Kermit sources ..."
$     set noon
$     tempsymb = "CCOPT = ""''ccopt'"""
$     say "Kermit Source Path = ''f$trnlnm(""KSP"")'"
$     set on
$     if .not. do_ckvcvt then say -
"Building WERMIT with ''share_text' ''net_name' network support at ''f$time()"
$!
$     say f$fao("!/cc   options : ''ccopt'")
$     say f$fao(  "link options : ''lopt'!/")
$!
$     if Make.eqs."" then goto Build_direct
$     if Make.nes."" then goto Build_MMx
$!
$Build_direct:
$! Build the thing plain
$!
$! To facilitate batch mode compilation, append /NOLIST and /NOMAP
$! to the compiler and linker options (default for INTERACTIVE or MMx)
$!
$     batch_mode = 0
$     if f$mode() .eqs. "BATCH" then batch_mode = 1
$     if batch_mode then ccopt = ccopt + "/NOLIST"
$     if batch_mode .and. (debug .eq. 0) then  lopt =  lopt + "/NOMAP"
$!
$     if do_ckvcvt then goto CKVCVT
$     say "  Compiling CKWART at     ''f$time()"
$!
$! Note the use of single quotes (') apostrophe than double ('') in quoting
$! CCOPT, to prevent CCOPT from being expanded prior to the MAKE call, which
$! could result in the string being too long.  Using ' rather than '' forces
$! evaluation of CCOPT to occure in the MAKE routine itself.
$!
$   CALL MAKE ckwart.OBJ "'CC' 'CCOPT' KSP:ckwart" -
              KSP:ckwart.c
$   say "  Linking   CKWART at     ''f$time()"
$!
$   call make ckwart.exe "LINK  ckwart,aux.opt/opt/NOMAP" ckwart.obj
$   say "  Running   CKWART at     ''f$time()"
$   ckwart = "$" +f$parse("CKWART.EXE",,,"DEVICE") +-
            f$parse("CKWART.EXE",,,"DIRECTORY") + "CKWART"
$   CALL MAKE ckcpro.c "ckwart  KSP:ckcpro.w ckcpro.c" -
             " KSP:ckcpro.w "
$   say "  Compiling WERMIT files  ''f$time()"
$!
$! Note how MAKE args are combined in quotes to get around the limitation
$! on the number of arguments to a DCL procedure.
$!
$   CALL MAKE ckcmai.OBJ "'CC' 'CCOPT' KSP:ckcmai" -
          KSP:ckcmai.c -
          "KSP:ckcker.h KSP:ckcdeb.h KSP:ckcsym.h" -
          "KSP:ckcasc.h KSP:ckcnet.h KSP:ckcsig.h" -
          "KSP:ckuusr.h KSP:ckvioc.h KSP:ckucmd.h"
$   CALL MAKE ckclib.OBJ "'CC' 'CCOPT' KSP:ckclib" -
          KSP:ckclib.c -
          "KSP:ckcdeb.h KSP:ckcsym.h KSP:ckcasc.h"
$   CALL MAKE ckcfn2.OBJ "'CC' 'CCOPT' KSP:ckcfn2" -
          KSP:ckcfn2.c -
          "KSP:ckcker.h KSP:ckcdeb.h KSP:ckcsym.h" -
          "KSP:ckcasc.h KSP:ckcxla.h KSP:ckuxla.h"
$   CALL MAKE ckcfn3.OBJ "'CC' 'CCOPT' KSP:ckcfn3" -
          KSP:ckcfn3.c -
          "KSP:ckcker.h KSP:ckcdeb.h KSP:ckcsym.h" -
          "KSP:ckcasc.h KSP:ckcxla.h KSP:ckuxla.h"
$   CALL MAKE ckcfns.OBJ "'CC' 'CCOPT' KSP:ckcfns" -
          KSP:ckcfns.c -
          "KSP:ckcker.h KSP:ckcdeb.h KSP:ckcsym.h" -
          "KSP:ckcasc.h KSP:ckcxla.h KSP:ckuxla.h"
$   CALL MAKE ckcuni.OBJ "CC 'CCOPT' KSP:ckcuni" -
          KSP:ckcuni.c -
          "KSP:ckcker.h KSP:ckcdeb.h KSP:ckcsym.h" -
          "KSP:ckcasc.h KSP:ckcxla.h KSP:ckuxla.h"
$   CALL MAKE ckcpro.OBJ "'CC' 'CCOPT'/INCL=KSP: ckcpro" -
          ckcpro.c -
          "KSP:ckcker.h KSP:ckcdeb.h KSP:ckcasc.h" -
          "KSP:ckcsym.h KSP:ckcnet.h KSP:ckvioc.h"
$   CALL MAKE ckucmd.OBJ "'CC' 'CCOPT' KSP:ckucmd" -
          KSP:ckucmd.c -
          "KSP:ckcasc.h KSP:ckucmd.h KSP:ckcdeb.h" -
          "KSP:ckcsym.h KSP:ckcker.h KSP:ckuusr.h" -
          "KSP:ckucmd.h"
$   CALL MAKE ckudia.OBJ "'CC' 'CCOPT' KSP:ckudia" -
          KSP:ckudia.c -
          "KSP:ckcker.h KSP:ckcdeb.h KSP:ckucmd.h" -
          "KSP:ckcasc.h KSP:ckcsig.h KSP:ckcnet.h" -
          "KSP:ckcsym.h KSP:ckuusr.h KSP:ckvioc.h"
$   CALL MAKE ckuscr.OBJ "'CC' 'CCOPT' KSP:ckuscr" -
          KSP:ckuscr.c -
          "KSP:ckcker.h KSP:ckcdeb.h KSP:ckcasc.h" -
          "KSP:ckcsig.h KSP:ckcsym.h KSP:ckuusr.h" -
          "KSP:ckcnet.h KSP:ckvioc.h KSP:ckucmd.h"
$   CALL MAKE ckuus2.OBJ "'CC' 'CCOPT' KSP:ckuus2" -
          KSP:ckuus2.c -
          "KSP:ckucmd.h KSP:ckcker.h KSP:ckuusr.h" -
          "KSP:ckcdeb.h KSP:ckcxla.h KSP:ckuxla.h" -
          "KSP:ckcasc.h KSP:ckcsym.h KSP:ckcnet.h" -
          "KSP:ckvioc.h"
$   CALL MAKE ckuus3.OBJ "'CC' 'CCOPT' KSP:ckuus3" -
          KSP:ckuus3.c -
          "KSP:ckucmd.h KSP:ckcker.h KSP:ckuusr.h" -
          "KSP:ckcdeb.h KSP:ckcxla.h KSP:ckuxla.h" -
          "KSP:ckcasc.h KSP:ckcnet.h KSP:ckcsym.h" -
          "KSP:ckvioc.h"
$   CALL MAKE ckuus4.OBJ "'CC' 'CCOPT' KSP:ckuus4" -
          KSP:ckuus4.c -
          "KSP:ckucmd.h KSP:ckcker.h KSP:ckuusr.h" -
          "KSP:ckcdeb.h KSP:ckcxla.h KSP:ckuxla.h" -
          "KSP:ckcasc.h KSP:ckcnet.h KSP:ckcsym.h" -
          "KSP:ckuver.h KSP:ckvioc.h"
$   CALL MAKE ckuus5.OBJ "'CC' 'CCOPT' KSP:ckuus5" -
          KSP:ckuus5.c -
          "KSP:ckucmd.h KSP:ckcker.h KSP:ckuusr.h" -
          "KSP:ckcdeb.h KSP:ckcxla.h KSP:ckcsym.h" -
          "KSP:ckcasc.h KSP:ckcnet.h KSP:ckvioc.h" -
          "KSP:ckuxla.h"
$   CALL MAKE ckuus6.OBJ "'CC' 'CCOPT' KSP:ckuus6" -
          KSP:ckuus6.c -
          "KSP:ckucmd.h KSP:ckcker.h KSP:ckuusr.h" -
          "KSP:ckcdeb.h KSP:ckcasc.h KSP:ckcsym.h" -
          "KSP:ckcxla.h KSP:ckcnet.h KSP:ckvioc.h" -
          "KSP:ckuxla.h"
$   CALL MAKE ckuus7.OBJ "'CC' 'CCOPT' KSP:ckuus7" -
          KSP:ckuus7.c -
          "KSP:ckucmd.h KSP:ckcker.h KSP:ckuusr.h" -
          "KSP:ckcdeb.h KSP:ckcxla.h KSP:ckuxla.h" -
          "KSP:ckcasc.h KSP:ckcnet.h KSP:ckcsym.h" -
          "KSP:ckvioc.h"
$   CALL MAKE ckuusr.OBJ "'CC' 'CCOPT' KSP:ckuusr" -
          KSP:ckuusr.c -
          "KSP:ckucmd.h KSP:ckcker.h KSP:ckuusr.h" -
          "KSP:ckcdeb.h KSP:ckcxla.h KSP:ckuxla.h" -
          "KSP:ckcasc.h KSP:ckcnet.h KSP:ckcsym.h" -
          "KSP:ckvioc.h"
$   CALL MAKE ckuusx.OBJ "'CC' 'CCOPT' KSP:ckuusx" -
          KSP:ckuusx.c -
          "KSP:ckucmd.h KSP:ckcker.h KSP:ckuusr.h" -
          "KSP:ckcdeb.h KSP:ckcxla.h KSP:ckcsym.h" -
          "KSP:ckcasc.h KSP:ckvvms.h KSP:ckuxla.h"
$   CALL MAKE ckuusy.OBJ "'CC' 'CCOPT' KSP:ckuusy" -
          KSP:ckuusy.c -
          "KSP:ckucmd.h KSP:ckcker.h KSP:ckuusr.h" -
          "KSP:ckcdeb.h KSP:ckcasc.h KSP:ckcsym.h" -
          "KSP:ckcnet.h KSP:ckvioc.h KSP:ckuxla.h"
$   CALL MAKE ckuxla.OBJ "'CC' 'CCOPT' KSP:ckuxla" -
          KSP:ckuxla.c -
          "KSP:ckucmd.h KSP:ckcker.h KSP:ckcdeb.h" -
          "KSP:ckcxla.h KSP:ckuxla.h KSP:ckcsym.h"
$   CALL MAKE ckvcon.OBJ "'CC' 'CCOPT' KSP:ckvcon" -
          KSP:ckvcon.c -
          "KSP:ckcdeb.h KSP:ckcasc.h KSP:ckcker.h" -
          "KSP:ckcnet.h KSP:ckvvms.h KSP:ckcxla.h" -
          "KSP:ckucmd.h KSP:ckvioc.h KSP:ckuxla.h"
$   CALL MAKE ckcnet.OBJ "'CC' 'CCOPT' KSP:ckcnet" -
          KSP:ckcnet.c -
          "KSP:ckcdeb.h KSP:ckcker.h KSP:ckcnet.h" -
          "KSP:ckcsym.h KSP:ckuusr.h KSP:ckcsig.h" -
          "KSP:ckvioc.h KSP:ckucmd.h"
$   CALL MAKE ckctel.OBJ "'CC' 'CCOPT' KSP:ckctel" -
          KSP:ckctel.c -
          "KSP:ckcsym.h KSP:ckcdeb.h KSP:ckcker.h" -
          "KSP:ckcnet.h KSP:ckctel.h"
$   CALL MAKE ckvfio.OBJ "'CC' 'CCOPT' KSP:ckvfio" -
          KSP:ckvfio.c -
          "KSP:ckcdeb.h KSP:ckuver.h KSP:ckcasc.h" -
          "KSP:ckcker.h KSP:ckvvms.h"
$   CALL MAKE ckvtio.OBJ "'CC' 'CCOPT' KSP:ckvtio" -
          KSP:ckvtio.c -
          "KSP:ckcdeb.h KSP:ckcnet.h KSP:ckuver.h" -
          "KSP:ckcasc.h KSP:ckcker.h KSP:ckvvms.h" -
          "KSP:ckvioc.h"
$   CALL MAKE ckvioc.OBJ "'CC' 'CCOPT' KSP:ckvioc" -
          KSP:ckvioc.c -
          "KSP:ckvioc.h KSP:ckcdeb.h"
$   CALL MAKE ckusig.OBJ "'CC' 'CCOPT' KSP:ckusig" -
          KSP:ckusig.c -
          "KSP:ckcdeb.h KSP:ckcasc.h KSP:ckcker.h" -
          "KSP:ckcnet.h KSP:ckuusr.h KSP:ckcsig.h" -
          "KSP:ckcsym.h KSP:ckvioc.h KSP:ckucmd.h"
$   if vmsv4 then -
    CALL MAKE ckvold.OBJ "'CC' 'CCOPT' KSP:ckvold" -
          KSP:ckvold.c
$!
$   say "  Linking   WERMIT at     ''f$time()"
$   CALL MAKE wermit.exe "LINK/exe=wermit.exe 'lopt' kermit.opt/opt" *.obj
$   say "Done building WERMIT at   ''f$time()"
$ goto Build_Complete
$!
$CKVCVT:
$     say "Building    CKVCVT at   ''f$time()"
$     say "  Compiling CKVCVT at   ''f$time()"
$     oldsrc = ""
$     oldobj = ""
$     if .not.vmsv4 then goto bld_ckvcvt
$         oldsrc =  "KSP:ckvold.c"
$         oldobj =  "ckvold.obj"
$         lopt = lopt + " ckvold.obj,"
$         CALL MAKE ckvold.OBJ "'CC' 'CCOPT' KSP:ckvold" -
              KSP:ckvold.c
$bld_ckvcvt:
$     CALL MAKE ckvcvt.OBJ "'CC' 'CCOPT' KSP:ckvcvt" -
             " KSP:ckvcvt.c  ''oldsrc'"
$     say "  Linking   CKVCVT at   ''f$time()"
$!
$     CALL MAKE ckvcvt.exe "LINK/exe=ckvcvt 'lopt' ckvcvt.obj,aux.opt/opt" -
              "ckvcvt.obj ''oldobj' "
$     say "Done building CKVCVT at ''f$time()"
$     if vmsv4.eq.0 then goto Build_complete
$ type sys$input

      Any link warning about an undefined symbol ZSYSCMD in CKVOLD
      may be safely ignored.  The SYSTEM function in CKVOLD which
      references ZSYSCMD is not used by CKVCVT, only by WERMIT.

$Goto Build_complete
$!
$!
$Build_MMx:
$!
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
$     open/write mmstemp ccflags.mms
$     ccopt = "CCFLAGS="+ccopt
$     write/symbol mmstemp ccopt
$     close mmstemp
$     target = "wermit"
$     if do_ckvcvt then target = "ckvcvt"
$     'Make' 'target' /des=KSP:ckvker.mms/ignore=warn -
         /macro=(cc="''CC'", linkflags="''lopt'")
$
$     say "Done building ''f$edit(target,""UPCASE"")' at ''f$time()"
$Goto Build_complete
$!
$Build_complete:
$!
$     if (noshare .ne. 1) then goto fini
$     type sys$input

 A link warning about an undefined symbol LIB$FIND_IMAGE_SYMBOL means
 you should link with the shareable library; add S to first parameter
 of CKVKER (and, if P5 is /NOSYSSHARE, omit that) and relink.

$!
$fini:
$     if f$search("kermit.opt") .nes. ""  then purge kermit.opt
$     if f$search("aux.opt") .nes. ""     then purge aux.opt
$     if f$search("ccflags.mms") .nes. "" then purge ccflags.mms
$     if f$search("wermit.exe") .nes. "" then -
        set file/protection=(g:re,w:re) wermit.exe
$     if f$search("ckvcvt.exe") .nes. "" then -
        set file/protection=(g:re,w:re) ckvcvt.exe
$     say f$fao("!/Kermit build completed  ''f$time()'!/")
$goto The_exit
$!
$CLEAN:
$     if f$search("ckwart.exe").nes."" then delete/noconf/log ckwart.exe;*
$     if f$search("*.obj")    .nes. "" then delete/noconf/log    *.obj;*
$     if f$search("ckcpro.c") .nes. "" then delete/noconf/log ckcpro.c;*
$     say f$fao("Cleanup done!/")
$ goto The_exit
$!
$!
$CY_exit:
$     $status = %x10000004
$!
$The_exit:
$     if f$trnlnm("KSP") .nes. "" then deassign KSP
$     if oldmath.eq.1 then deass mthrtl
$     if oldmath.eq.1 then deass vmthrtl
$!
$     x = f$verify(save_verify_procedure,save_verify_image)
$ exit $status
$!
$Bad_param:
$     say "ERROR: The first parameter should not include commas"
$     say "       P1 = "+ P1
$     say "       you may have used commas instead of spaces to
$     say "       separate parameters"
$Exit
$!
$MAKE: SUBROUTINE   !SUBROUTINE TO CHECK DEPENDENCIES
$! P1 = What we are trying to make
$! P2 = Command to make it
$! P3 - P8  What it depends on
$
$     If F$Search(P1) .Eqs. "" Then Goto Makeit
$     Time = F$CvTime(F$File(P1,"RDT"))
$arg=3
$Make_Loop:
$     Argument = P'arg
$     If Argument .Eqs. "" Then Goto Make_exit
$     El=0
$Loop2:
$     File = F$Element(El," ",Argument)
$     If File .Eqs. " " Then Goto Endl
$     AFile = ""
$Loop3:
$     OFile = AFile
$     AFile = F$Search(File)
$     If AFile .Eqs. "" .Or. AFile .Eqs. OFile Then Goto NextEl
$     If F$CvTime(F$File(AFile,"RDT")) .ges. Time Then Goto Makeit
$     Goto Loop3
$NextEL:
$     El = El + 1
$     Goto Loop2
$EndL:
$     arg=arg+1
$     If arg .Le. 8 Then Goto Make_Loop
$ Goto Make_Exit
$
$Makeit:
$     say P2
$     'P2'
$Make_Exit:
$     exit
$ENDSUBROUTINE
$!
$warning_exit:
$     status = $status
$     sev = $severity
$     set noon
$     xtext = f$message(status)
$     say "Warning:"
$     say "''xtext'"
$goto the_exit
$!

