LOCAL_PATH:= $(call my-dir)

#####################################################################
# libplayreadydrmplugin.so
####################################################################
include $(CLEAR_VARS)

LOCAL_MODULE := libplayreadydrmplugin

ifeq ($(BOARD_PLAYREADY_USE_SECUREOS), 1)
LOCAL_SRC_FILES := \
    secure/lib32/libplayreadydrmplugin.so
else
LOCAL_SRC_FILES := \
    nonsecure/lib32/libplayreadydrmplugin.so
endif
LOCAL_SHARED_LIBRARIES := libc++ libc libcutils libdl liblog libm libplayreadypk libprotobuf-cpp-lite libstagefright_foundation libutils
ifeq ($(BOARD_PLAYREADY_USE_SECUREOS), 1)
LOCAL_SHARED_LIBRARIES += libteec
endif
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 32
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := mediadrm
include $(BUILD_PREBUILT)


#64
include $(CLEAR_VARS)

LOCAL_MODULE := libplayreadydrmplugin

ifeq ($(BOARD_PLAYREADY_USE_SECUREOS), 1)
LOCAL_SRC_FILES := \
    secure/lib64/libplayreadydrmplugin.so
else
LOCAL_SRC_FILES := \
    nonsecure/lib64/libplayreadydrmplugin.so
endif
LOCAL_SHARED_LIBRARIES := libc++ libc libcutils libdl liblog libm libplayreadypk libprotobuf-cpp-lite libstagefright_foundation libutils
ifeq ($(BOARD_PLAYREADY_USE_SECUREOS), 1)
LOCAL_SHARED_LIBRARIES += libteec
endif
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 64
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CHECK_ELF_FILES := false
LOCAL_MODULE_RELATIVE_PATH := mediadrm
include $(BUILD_PREBUILT)

#####################################################################
# libplayreadypk.so
#####################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := libplayreadypk
ifeq ($(BOARD_PLAYREADY_USE_SECUREOS), 1)
LOCAL_SRC_FILES := \
    secure/lib32/libplayreadypk.so
else
LOCAL_SRC_FILES := \
    nonsecure/lib32/libplayreadypk.so
endif
LOCAL_SHARED_LIBRARIES := libc++ libc libcurl libcutils libdl liblog libm libutils
ifeq ($(BOARD_PLAYREADY_USE_SECUREOS), 1)
LOCAL_SHARED_LIBRARIES += libteec
endif
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 32
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_PREBUILT)


#64
include $(CLEAR_VARS)
LOCAL_MODULE := libplayreadypk
ifeq ($(BOARD_PLAYREADY_USE_SECUREOS), 1)
LOCAL_SRC_FILES := \
    secure/lib64/libplayreadypk.so
else
LOCAL_SRC_FILES := \
    nonsecure/lib64/libplayreadypk.so
endif
LOCAL_SHARED_LIBRARIES := libc++ libc libcurl libcutils libdl liblog libm libutils
ifeq ($(BOARD_PLAYREADY_USE_SECUREOS), 1)
LOCAL_SHARED_LIBRARIES += libteec
endif
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 64
LOCAL_CHECK_ELF_FILES := false
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_PREBUILT)
####################################################################
