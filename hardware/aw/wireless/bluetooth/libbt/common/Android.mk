LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

BDROID_DIR := system/bt

LOCAL_C_INCLUDES += \
    hardware/aw/wireless/hwinfo \
    hardware/libhardware_legacy/include \
    $(BDROID_DIR)/include \
    $(BDROID_DIR)/hci/include \
    $(BDROID_DIR)/osi/include \
    $(BDROID_DIR)/stack/include

LOCAL_SRC_FILES := bt_vendor.c
LOCAL_HEADER_LIBRARIES := libutils_headers
LOCAL_WHOLE_STATIC_LIBRARIES := libhwinfo

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog

LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := allwinner
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libbt-package
LOCAL_MODULE_OWNER := google
LOCAL_MODULE_TAGS := optional

LOCAL_VENDOR_LIST := hardware/broadcom/libbt:libbt-broadcom
LOCAL_VENDOR_LIST += hardware/realtek/bluetooth/libbt-vendor:libbt-realtek
LOCAL_VENDOR_LIST += hardware/xradio/bt/libbt-vendor:libbt-xradio
LOCAL_VENDOR_LIST += hardware/sprd/libbt:libbt-sprd
LOCAL_VENDOR_LIST += hardware/aic/libbt:libbt-aic

LOCAL_REQUIRED_MODULES :=

$(foreach e,$(LOCAL_VENDOR_LIST),\
    $(eval LOCAL_VENDOR_PATH   := $(shell echo $(e) | awk -F: '{print $$1}')) \
    $(eval LOCAL_VENDOR_MODULE := $(shell echo $(e) | awk -F: '{print $$2}')) \
    $(if $(wildcard $(LOCAL_VENDOR_PATH)),\
        $(eval LOCAL_REQUIRED_MODULES += $(LOCAL_VENDOR_MODULE)) \
    ) \
)

include $(BUILD_PHONY_PACKAGE)
