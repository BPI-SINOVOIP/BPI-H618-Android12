LOCAL_PATH := external/ppp/pppd

LOCAL_BUILD_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	auth.c \
	ccp.c \
	chap-md5.c \
	chap-new.c \
	chap_ms.c \
	demand.c \
	eap.c \
	ecp.c \
	eui64.c \
	fsm.c \
	ipcp.c \
	ipv6cp.c \
	lcp.c \
	magic.c \
	options.c \
	pppcrypt.c \
	pppox.c \
	session.c \
	sys-linux.c \
	tty.c \
	upap.c \
	utils.c

# options.c:623:21: error: passing 'const char *' to parameter of type 'char *' discards qualifiers.
# [-Werror,-Wincompatible-pointer-types-discards-qualifiers]
LOCAL_CLANG_CFLAGS += -Wno-incompatible-pointer-types-discards-qualifiers

LOCAL_SHARED_LIBRARIES := \
	libcutils liblog libcrypto

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS := -DCHAPMS=1 -DMPPE=1 -DINET6=1 -DUSE_OPENSSL=1 -Wno-unused-parameter -Wno-empty-body -Wno-missing-field-initializers -Wno-attributes -Wno-sign-compare -Wno-pointer-sign -Werror

# Turn off warnings for now until this is fixed upstream. b/18632512
LOCAL_CFLAGS += -Wno-unused-variable

LOCAL_MODULE:= pppd_vendor
LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE_CLASS := EXECUTABLES

LOCAL_CFLAGS += -DHAVE_PATHS_H
LOCAL_C_INCLUDES += $(LOCAL_BUILD_PATH)

LOCAL_GENERATED_SOURCES := $(local-generated-sources-dir)/main.c

$(LOCAL_GENERATED_SOURCES): $(LOCAL_BUILD_PATH)/auto-gen-source.sh $(LOCAL_PATH)
	@echo "Generator: $@"
	@$< $^ $@

include $(BUILD_EXECUTABLE)
