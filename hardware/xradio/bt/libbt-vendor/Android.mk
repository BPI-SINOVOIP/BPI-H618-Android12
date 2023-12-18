LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifneq ($(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR),)
  bdroid_C_INCLUDES := $(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR)
  bdroid_CFLAGS += -DHAS_BDROID_BUILDCFG
else
  bdroid_C_INCLUDES :=
  bdroid_CFLAGS += -DHAS_NO_BDROID_BUILDCFG
endif

BDROID_DIR := system/bt

LOCAL_SRC_FILES := \
        src/socket.c \
        src/bt_vendor_xr.c \
        src/hardware.c \
        src/userial_vendor.c \
        src/upio.c \
        src/cmd.c \
        src/conf.c \
        src/fw_debug.c \
        src/sdd/efuse.c \
        src/sdd/sdd.c \
        src/sdd/mix_sdd_efuse.c

LOCAL_C_INCLUDES += \
        hardware/libhardware_legacy/include \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/include \
        $(BDROID_DIR)/hci/include \
        $(BDROID_DIR)/osi/include \
        $(BDROID_DIR)/stack/include

LOCAL_CFLAGS := \
    -Wno-unused-function \
    -Wno-unused-parameter \
    -Wno-unused-variable

LOCAL_CFLAGS += -Wno-implicit-function-declaration

LOCAL_CFLAGS += -DXR829

BUILD_VERSION = "\"1.1.16"\"
LOCAL_CFLAGS += -DBUILD_VERSION=$(BUILD_VERSION)
BUILD_TIME = "\"`date '+%Y-%m-%d %H:%M:%S'`"\"
LOCAL_CFLAGS += -DBUILD_TIME=$(BUILD_TIME)
GIT_VERSION = "\"`cd $(LOCAL_PATH);git show -s --pretty=format:%h`"\"
LOCAL_CFLAGS += -DGIT_VERSION=$(GIT_VERSION)

LOCAL_HEADER_LIBRARIES := libutils_headers liblog_headers

LOCAL_SHARED_LIBRARIES := \
        libhardware_legacy \
        libcutils \
        liblog

LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

include $(LOCAL_PATH)/vnd_buildcfg.mk

include $(BUILD_SHARED_LIBRARY)
