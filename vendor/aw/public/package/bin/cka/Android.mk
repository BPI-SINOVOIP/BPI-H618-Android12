LOCAL_PATH:= $(call my-dir)

# ========================================================
# nano
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	ckcmai.c ckclib.c ckutio.c ckufio.c \
	ckcfns.c ckcfn2.c ckcfn3.c ckuxla.c \
	ckcpro.c ckucmd.c ckuus2.c ckuus3.c \
	ckuus4.c ckuus5.c ckuus6.c ckuus7.c \
	ckuusx.c ckuusy.c ckuusr.c ckucns.c \
	ckudia.c ckuscr.c ckcnet.c ckusig.c \
	ckctel.c ckcuni.c ckupty.c ckcftp.c \
	ckuath.c ck_crp.c ck_ssl.c
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)

LOCAL_C_INCLUDES += bionic/libc/stdio

LOCAL_CFLAGS += \
	-DFNFLOAT -DCK_NEWTERM -DTCPSOCKET \
	-DLINUXFSSTND -DNOCOTFMC -DUSE_STRERROR \
	-DHAVE_PTMX -D_LARGEFILE_SOURCE -DNO_OPENPTY \
	-D_FILE_OFFSET_BITS=64 -DPOSIX -DUSE_FILE_R\
	-DKTARGET=\"android\" -DNO_DNS_SRV -DNOIKSD \
	-DNOTIMESTAMP -DNOZLOCALTIME -DNOUUCP \
	-DNO_NL_LANGINFO -DNO_LOCALE
LOCAL_CFLAGS += -Wno-unused-variable -Wno-dangling-else -Wno-unused-parameter \
				-Wno-format-security -Wno-char-subscripts -Wno-format -Wno-missing-braces \
				-Wno-unused-label -Wno-constant-logical-operand -Wno-logical-op-parentheses \
				-Wno-date-time -Wno-unused-function -Wno-implicit-function-declaration \
				-Wno-logical-not-parentheses -Wno-parentheses \
				-Wno-missing-field-initializers -Wno-string-compare -Wno-pointer-sign -Wno-uninitialized \
				-Wno-non-literal-null-conversion -Wno-empty-body -Wno-pointer-bool-conversion \
				-Wno-unused-value -Wno-implicit-int

LOCAL_CFLAGS += -DCK_SYSINI=\"/vendor/etc/kermit/kermrc\"

LOCAL_MODULE := kermit
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
LOCAL_REQUIRED_MODULES := kermrc
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := kermrc
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_ETC)/kermit
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := kermrc
LOCAL_MODULE_OWNER := kermit
include $(BUILD_PREBUILT)
