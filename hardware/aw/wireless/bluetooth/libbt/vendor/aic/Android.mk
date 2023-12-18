LOCAL_PATH := hardware/aic/libbt

ifneq (,$(wildcard $(LOCAL_PATH)))

include $(CLEAR_VARS)

ifneq ($(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR),)
  bdroid_C_INCLUDES := $(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR)
  bdroid_CFLAGS += -DHAS_BDROID_BUILDCFG
else
  bdroid_C_INCLUDES :=
  bdroid_CFLAGS += -DHAS_NO_BDROID_BUILDCFG
endif

TARGET_CHIP_AICBT := aic8800
BDROID_DIR := $(TOP_DIR)system/bt

LOCAL_CFLAGS += -DUSE_BLUETOOTH_AIC8800

LOCAL_CFLAGS += \
        -Wall \
        -Werror \
        -Wno-switch \
        -Wno-unused-function \
        -Wno-unused-parameter \
        -Wno-unused-variable \

LOCAL_SRC_FILES := \
        src/bt_vendor_aic.c \
        src/hardware.c \
        src/hardware_uart.c \
        src/hardware_usb.c \
        src/userial_vendor.c \
        src/upio.c \
        src/aic_socket.c \
        src/bt_list.c \
        src/bt_skbuff.c \
        src/hci_h5.c \
        src/aic_parse.c \
        src/aic_btservice.c \
        src/aic_heartbeat.c \
        src/aic_poll.c \
        src/aic_btsnoop_net.c

LOCAL_SRC_FILES += \
        codec/plc/sbcplc.c \
        codec/sbc/sbc.c \
        codec/sbc/sbc_primitives.c \
        codec/sbc/sbc_primitives_mmx.c \
        codec/sbc/sbc_primitives_neon.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/codec/sbc \
        $(LOCAL_PATH)/codec/plc \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/hci/include \
        $(BDROID_DIR)/include \
        $(BDROID_DIR)/device/include \
        $(BDROID_DIR)

LOCAL_C_INCLUDES += $(bdroid_C_INCLUDES)
LOCAL_CFLAGS += $(bdroid_CFLAGS)

LOCAL_HEADER_LIBRARIES := libutils_headers

ifneq ($(BOARD_HAVE_BLUETOOTH_AICBT_A2DP_OFFLOAD),)
  LOCAL_STATIC_LIBRARIES := libbt-aicbt_a2dp
endif

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog

LOCAL_MODULE := libbt-aic
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := aic
LOCAL_PROPRIETARY_MODULE := true

include $(LOCAL_PATH)/vnd_buildcfg.mk

include $(BUILD_SHARED_LIBRARY)

endif
