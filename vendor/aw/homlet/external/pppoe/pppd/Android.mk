LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	main.c \
	magic.c \
	fsm.c \
	lcp.c \
	ipcp.c \
	upap.c \
	chap-new.c \
	ccp.c \
	ecp.c \
	auth.c \
	options.c \
	sys-linux.c \
	chap_ms.c \
	demand.c \
	utils.c \
	tty.c \
	eap.c \
	chap-md5.c \
	pppcrypt.c \
	openssl-hash.c \
	pppox.c \
	src/if.c \
	src/debug.c \
	src/common.c \
	src/ppp.c \
	src/discovery.c \
	src/plugin.c

# options.c:623:21: error: passing 'const char *' to parameter of type 'char *' discards qualifiers.
# # [-Werror,-Wincompatible-pointer-types-discards-qualifiers]
LOCAL_CLANG_CFLAGS += -Wno-incompatible-pointer-types-discards-qualifiers

LOCAL_SHARED_LIBRARIES := \
	libcutils liblog libcrypto libdl

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS := -DANDROID_CHANGES -DCHAPMS=1 -DMPPE=1 -Iexternal/openssl/include -DPLUGIN=1 \
                -Wno-unused-parameter -Wno-empty-body -Wno-missing-field-initializers -Wno-attributes \
                -Wno-sign-compare -Wno-pointer-sign -Werror -Wno-format-security -Wno-macro-redefined \
                -Wno-implicit-function-declaration -Wno-implicit-int

# Turn off warnings for now until this is fixed upstream. b/18632512
LOCAL_CFLAGS += -Wno-unused-variable

LOCAL_MODULE:= pppoe

include $(BUILD_EXECUTABLE)
