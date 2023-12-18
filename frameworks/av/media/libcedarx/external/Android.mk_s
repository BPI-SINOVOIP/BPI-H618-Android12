LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/../config.mk

MY_SDK="notdef"

ifeq ($(CONF_ANDROID_VERSION), 4.2)
MY_SDK=ajb42
else ifeq ($(CONF_ANDROID_VERSION), 4.4)
MY_SDK=akk44
else ifeq ($(CONF_ANDROID_VERSION), 5.0)
MY_SDK=alp50
else ifeq ($(CONF_ANDROID_VERSION), 5.1)
MY_SDK=alp50
else ifeq ($(CONF_ANDROID_VERSION), 6.0)
MY_SDK=amm60
else ifeq ($(CONF_ANDROID_VERSION), 7.0)
MY_SDK=anougat70
else ifeq ($(CONF_ANDROID_VERSION), 7.1)
MY_SDK=anougat70
else ifeq ($(CONF_ANDROID_VERSION), 8.0)
MY_SDK=anougat70
else ifeq ($(CONF_ANDROID_VERSION), 8.1)
MY_SDK=anougat70
else ifeq ($(CONF_ANDROID_VERSION), 9)
MY_SDK=anougat70
endif

###################### audio midware and libs begin ##########################

define define-audio-lib
include $$(CLEAR_VARS)

LOCAL_MODULE := $1
LOCAL_SRC_FILES := lib32/arm-linux-androideabi/$1.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 32
include $(BUILD_PREBUILT)
endef

AUD_LIBS := \
	libadecoder \
	libaencoder \
	libaw_aacdec \
	libaw_alacdec \
	libaw_amrdec \
	libaw_mp3dec \
	libaw_apedec \
	libaw_dsddec \
	libaw_flacdec \
	libaw_g729dec \
	libaw_oggdec \
	libaw_wavdec \
        libaw_wvdev \
	libaw_atrcdec \
	libaw_cookdec \
	libaw_radec \
	libaw_siprdec \
	libaw_opusdec \
	libpassthrud \
	libpassthrua

$(foreach lib, $(AUD_LIBS),\
    $(eval $(call define-audio-lib,$(lib))))

###################### audio midware and libs end   ##########################

include $(CLEAR_VARS)
LOCAL_MODULE := liblive555
LOCAL_SRC_FILES := lib32/$(MY_SDK)/liblive555.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 32
include $(BUILD_PREBUILT)

ifeq ($(CONF_RTP), yes)
include $(CLEAR_VARS)
LOCAL_MODULE := librtp
LOCAL_SRC_FILES := lib32/$(MY_SDK)/librtp.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 32
include $(BUILD_PREBUILT)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libsubdecoder
LOCAL_SRC_FILES := lib32/$(MY_SDK)/libsubdecoder.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 32
include $(BUILD_PREBUILT)

ifeq ($(CONF_ANDROID_VERSION), 5.0)

include $(CLEAR_VARS)
LOCAL_MODULE := liblive555
LOCAL_SRC_FILES := lib64/$(MY_SDK)/liblive555.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 64
include $(BUILD_PREBUILT)

ifeq ($(CONF_RTP), yes)
include $(CLEAR_VARS)
LOCAL_MODULE := librtp
LOCAL_SRC_FILES := lib64/$(MY_SDK)/librtp.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 64
include $(BUILD_PREBUILT)
endif

endif

ifeq ($(CONF_ANDROID_VERSION), 6.0)

include $(CLEAR_VARS)
LOCAL_MODULE := liblive555
LOCAL_SRC_FILES := lib64/$(MY_SDK)/liblive555.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 64
include $(BUILD_PREBUILT)

ifeq ($(CONF_RTP), yes)
include $(CLEAR_VARS)
LOCAL_MODULE := librtp
LOCAL_SRC_FILES := lib64/$(MY_SDK)/librtp.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 64
include $(BUILD_PREBUILT)
endif

endif

