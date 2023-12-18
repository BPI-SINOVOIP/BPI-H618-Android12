!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! 2007-02-27 SMS.
! Actual VMS dependencies.

.FIRST
    @ ! echo: write to stdout; tab: some spacing
    @ echo = "write sys$output"
    @ tab = "    "
.include ccflags.mms

SHAREOPTS = KERMIT.OPT/OPTION

.IFDEF INT_FTP
CKCFTP_OBJ = ckcftp.obj
.ENDIF

OBJECT_MODULES = ckcfn2.obj, ckcfn3.obj, ckcfns.obj, $(CKCFTP_OBJ), -
                 ckclib.obj, ckcmai.obj, ckcnet.obj, ckcpro.obj, -
                 ckctel.obj, ckcuni.obj, ckuath.obj, ckucmd.obj, -
                 ckudia.obj, ckuscr.obj, ckusig.obj, ckuus2.obj, -
                 ckuus3.obj, ckuus4.obj, ckuus5.obj, ckuus6.obj, -
                 ckuus7.obj, ckuusr.obj, ckuusx.obj, ckuusy.obj, -
                 ckuxla.obj, ckvcon.obj, ckvfio.obj, ckvioc.obj, -
                 ckvrtl.obj, ckvtio.obj, ck_crp.obj, ck_ssl.obj

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
! Rule Section:
!
.C.OBJ :
    @ echo tab + "Compiling ''f$trnlnm("K")'$(MMS$SOURCE)" - "K:"
    @ $(CC) $(CCFLAGS) /object=$(MMS$TARGET) $(MMS$SOURCE)

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
! Dependencies Section:
!
ALL :   wermit.exe  ckvcvt.exe
    @ continue

WERMIT :   wermit.exe
    @ continue

CKVCVT :   ckvcvt.exe
    @ continue

wermit.exe : $(OBJECT_MODULES)
    @ echo tab + "Linking $(MMS$TARGET_NAME)"

       $(LINK) $(LINKFLAGS) -
        /exec=wermit.exe $(SHAREOPTS)

ckcpro.c : K:ckcpro.w ckwart.exe K:ckcdeb.h K:ckcasc.h K:ckcker.h
    @ echo "CKWART $(MMS$SOURCE) CKCPRO.C"
    @ ckwart = "$" + f$parse("CKWART.EXE",,,"DEVICE") + -
        f$parse("CKWART.EXE",,,"DIRECTORY") + "CKWART"
    @ ckwart K:ckcpro.w ckcpro.c

ckvcvt.exe : ckvcvt.obj
    @ echo tab + "Linking $(MMS$TARGET_NAME)"
        $(LINK) $(LINKFLAGS) ckvcvt.obj,aux.opt/opt

ckwart.exe : ckwart.obj
    @ echo tab + "Linking $(MMS$TARGET_NAME)"
        $(LINK) /nodebug/nomap ckwart.obj,aux.opt/opt


! Object file dependencies:

ckcfn2.obj : K:ckcfn2.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcasc.h K:ckcker.h K:ckcxla.h K:ckuxla.h -
             K:ckcuni.h K:ckcnet.h K:ckvioc.h K:ckctel.h

ckcfn3.obj : K:ckcfn3.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcasc.h K:ckcker.h K:ckcxla.h K:ckuxla.h -
             K:ckcuni.h

ckcfns.obj : K:ckcfns.c K:ckcsym.h K:ckcasc.h K:ckcdeb.h -
             K:ckclib.h K:ckcker.h K:ckcxla.h K:ckuxla.h -
             K:ckcuni.h K:ckcnet.h K:ckvioc.h K:ckctel.h

ckcftp.obj : K:ckcftp.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcsig.h K:ckcasc.h K:ckcker.h K:ckucmd.h -
             K:ckuusr.h K:ckcnet.h K:ckvioc.h K:ckctel.h -
             K:ckcxla.h K:ckuxla.h K:ckcuni.h K:ckuath.h -
             K:ckvrtl.h K:ck_ssl.h

ckclib.obj : K:ckclib.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcasc.h

ckcmai.obj : K:ckcmai.c K:ckcsym.h K:ckcasc.h K:ckcdeb.h -
             K:ckclib.h K:ckcker.h K:ckcnet.h K:ckvioc.h -
             K:ckctel.h K:ck_ssl.h K:ckuusr.h K:ckucmd.h -
             K:ckuath.h K:ckcsig.h

ckcnet.obj : K:ckcnet.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcker.h K:ckcasc.h K:ckcnet.h K:ckvioc.h -
             K:ckctel.h K:ck_ssl.h K:ckuusr.h K:ckucmd.h -
             K:ckuath.h K:ckcsig.h K:ckvrtl.h

ckcpro.obj :     ckcpro.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcasc.h K:ckcker.h K:ckcnet.h K:ckvioc.h -
             K:ckctel.h
	@ echo "Compiling $(MMS$SOURCE)"
	@ $(CC) $(CCFLAGS)/INCLUDE_DIRECTORY=K: $(MMS$SOURCE)

ckctel.obj : K:ckctel.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcker.h K:ckcnet.h K:ckvioc.h K:ckctel.h -
             K:ckuath.h K:ck_ssl.h

ckcuni.obj : K:ckcuni.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcker.h K:ckucmd.h K:ckcxla.h K:ckuxla.h -
             K:ckcuni.h

ckuath.obj : K:ckuath.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcker.h K:ckuusr.h K:ckucmd.h K:ckcnet.h -
             K:ckvioc.h K:ckctel.h K:ckuath.h K:ckuat2.h -
             K:ck_ssl.h

ckucmd.obj : K:ckucmd.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcker.h K:ckcnet.h K:ckvioc.h K:ckctel.h -
             K:ckucmd.h K:ckuusr.h K:ckcasc.h

ckudia.obj : K:ckudia.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcasc.h K:ckcker.h K:ckucmd.h K:ckcnet.h -
             K:ckvioc.h K:ckctel.h K:ckuusr.h K:ckcsig.h

ckuscr.obj : K:ckuscr.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcasc.h K:ckcker.h K:ckuusr.h K:ckucmd.h -
             K:ckcnet.h K:ckvioc.h K:ckctel.h K:ckcsig.h

ckusig.obj : K:ckusig.c K:ckcsym.h K:ckcasc.h K:ckcdeb.h -
             K:ckclib.h K:ckcker.h K:ckcnet.h K:ckvioc.h -
             K:ckctel.h K:ckuusr.h K:ckucmd.h K:ckcsig.h

ckuus2.obj : K:ckuus2.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcnet.h K:ckvioc.h K:ckctel.h K:ckcasc.h -
             K:ckcker.h K:ckuusr.h K:ckucmd.h K:ckcxla.h -
             K:ckuxla.h K:ckcuni.h

ckuus3.obj : K:ckuus3.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcasc.h K:ckcker.h K:ckcxla.h K:ckuxla.h -
             K:ckcuni.h K:ckcnet.h K:ckvioc.h K:ckctel.h -
             K:ckuath.h K:ck_ssl.h K:ckuusr.h K:ckucmd.h

ckuus4.obj : K:ckuus4.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcasc.h K:ckcker.h K:ckcnet.h K:ckvioc.h -
             K:ckctel.h K:ckuusr.h K:ckucmd.h K:ckuver.h -
             K:ckcxla.h K:ckuxla.h K:ckcuni.h K:ckuath.h -
             K:ck_ssl.h

ckuus5.obj : K:ckuus5.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcasc.h K:ckcker.h K:ckuusr.h K:ckucmd.h -
             K:ckcnet.h K:ckvioc.h K:ckctel.h K:ckcxla.h -
             K:ckuxla.h K:ckcuni.h K:ck_ssl.h

ckuus6.obj : K:ckuus6.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcasc.h K:ckcker.h K:ckuusr.h K:ckucmd.h -
             K:ckcxla.h K:ckuxla.h K:ckcuni.h K:ckcnet.h -
             K:ckvioc.h K:ckctel.h

ckuus7.obj : K:ckuus7.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcasc.h K:ckcker.h K:ckcxla.h K:ckuxla.h -
             K:ckcuni.h K:ckcnet.h K:ckvioc.h K:ckctel.h -
             K:ckuusr.h K:ckucmd.h K:ckuath.h K:ck_ssl.h

ckuusr.obj : K:ckuusr.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcasc.h K:ckcker.h K:ckcnet.h K:ckvioc.h -
             K:ckctel.h K:ckuusr.h K:ckucmd.h K:ckcxla.h -
             K:ckuxla.h K:ckcuni.h

ckuusx.obj : K:ckuusx.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcasc.h K:ckcker.h K:ckuusr.h K:ckucmd.h -
             K:ckcxla.h K:ckuxla.h K:ckcuni.h K:ckcsig.h

ckuusy.obj : K:ckuusy.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcasc.h K:ckcker.h K:ckucmd.h K:ckcnet.h -
             K:ckvioc.h K:ckctel.h K:ckuusr.h K:ckcxla.h -
             K:ckuxla.h K:ckcuni.h K:ck_ssl.h

ckuxla.obj : K:ckuxla.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcker.h K:ckucmd.h K:ckcxla.h K:ckuxla.h -
             K:ckcuni.h

ckvcon.obj : K:ckvcon.c K:ckcdeb.h K:ckclib.h K:ckcasc.h -
             K:ckcker.h K:ckucmd.h K:ckcnet.h K:ckvioc.h -
             K:ckctel.h K:ckvvms.h K:ckcxla.h K:ckuxla.h -
             K:ckcuni.h

ckvcvt.obj : K:ckvcvt.c

ckvfio.obj : K:ckvfio.c K:ckcdeb.h K:ckclib.h K:ckcasc.h -
             K:ckcker.h K:ckuusr.h K:ckucmd.h K:ckvvms.h

ckvioc.obj : K:ckvioc.c K:ckcdeb.h K:ckclib.h K:ckvioc.h -
             K:ckcker.h K:ckuusr.h K:ckucmd.h K:ckvvms.h

ckvrtl.obj : K:ckvrtl.c K:ckvrtl.h

ckvtio.obj : K:ckvtio.c K:ckcdeb.h K:ckclib.h K:ckcasc.h -
             K:ckcker.h K:ckvvms.h K:ck_ssl.h K:ckcnet.h -
             K:ckvioc.h K:ckctel.h

ckwart.obj : K:ckwart.c K:ckcsym.h K:ckcdeb.h K:ckclib.h

ck_crp.obj : K:ck_crp.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcnet.h K:ckvioc.h K:ckctel.h

ck_ssl.obj : K:ck_ssl.c K:ckcsym.h K:ckcdeb.h K:ckclib.h -
             K:ckcnet.h K:ckvioc.h K:ckctel.h K:ckuath.h -
             K:ckcker.h K:ckucmd.h K:ck_ssl.h

!! end of ckvker.mms !!
