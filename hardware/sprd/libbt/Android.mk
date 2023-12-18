LOCAL_PATH := $(call my-dir)

ifeq ($(BOARD_HAVE_BLUETOOTH_SPRD),true)

include $(CLEAR_VARS)

SPRD_WCNBT_CHISET := marlin3
BDROID_DIR := $(TOP_DIR)system/bt

LOCAL_SRC_FILES := \
        src/bt_vendor_sprd.c \
        src/hardware.c \
        src/userial_vendor.c \
        src/upio.c \
        src/conf.c \
        src/sitm.c \
        conf/sprd/$(SPRD_WCNBT_CHISET)/src/$(SPRD_WCNBT_CHISET).c \
        src/bt_vendor_sprd_hci.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/ \
        $(BDROID_DIR)/hci/include \
        $(BDROID_DIR)/include \
        $(BDROID_DIR)/stack/btm \
        $(BDROID_DIR)/stack/include \
        $(BDROID_DIR)/gki/ulinux \
        $(LOCAL_PATH)/conf/sprd/$(SPRD_WCNBT_CHISET)/include

LOCAL_ALGO_SRC_FILES:= \
        src/bt_vendor_sprd_ssp.c \
        src/algorithms/p_256_ecc_pp.c \
        src/algorithms/p_256_curvepara.c \
        src/algorithms/p_256_multprecision.c \
        src/algorithms/lmp_ecc.c \
        src/algorithms/algo_api.c \
        src/algorithms/algo_utils.c

LOCAL_ALGO_C_INCLUDES := $(LOCAL_PATH)/src/algorithms

LOCAL_SRC_FILES += $(LOCAL_ALGO_SRC_FILES)
LOCAL_C_INCLUDES += $(LOCAL_ALGO_C_INCLUDES)


LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog

LOCAL_HEADER_LIBRARIES:= libbluetooth_headers

## Special configuration ##
ifeq ($(BOARD_SPRD_WCNBT_MARLIN), true)
    ifneq ($(strip $(WCN_EXTENSION)),true)
        LIBBT_CFLAGS += -DSPRD_WCNBT_MARLIN_15A
    else
        LIBBT_CFLAGS += -DGET_MARLIN_CHIPID
    endif
endif

ifneq ($(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR),)
  bdroid_C_INCLUDES := $(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR)
  bdroid_CFLAGS += -DHAS_BDROID_BUILDCFG
else
  bdroid_C_INCLUDES :=
  bdroid_CFLAGS += -DHAS_NO_BDROID_BUILDCFG
endif

LOCAL_C_INCLUDES += $(bdroid_C_INCLUDES)
LOCAL_CFLAGS := $(LIBBT_CFLAGS) $(bdroid_CFLAGS)
LOCAL_CFLAGS += -Wno-unused-variable
LOCAL_CFLAGS += -Wno-unused-value
LOCAL_CFLAGS += -Wno-unused-function
LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := sprd
LOCAL_PROPRIETARY_MODULE := true

include $(LOCAL_PATH)/vnd_buildcfg.mk

include $(BUILD_SHARED_LIBRARY)

endif
