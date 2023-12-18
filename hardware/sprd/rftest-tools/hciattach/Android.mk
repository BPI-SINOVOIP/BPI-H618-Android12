LOCAL_PATH := $(call my-dir)

#
# hciattach
#

include $(CLEAR_VARS)

BDROID_DIR := $(TOP_DIR)system/bt

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/lib \
    $(BDROID_DIR)/hci/include \
    $(BDROID_DIR)/stack/include \

LOCAL_SRC_FILES := \
    hciattach.c \
    hciattach_sprd.c

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libcutils \
    liblog \
    libdl

LOCAL_CFLAGS += -Wno-unused-function
LOCAL_CFLAGS += -Wno-missing-field-initializers

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := hciattach
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_EXECUTABLE)

