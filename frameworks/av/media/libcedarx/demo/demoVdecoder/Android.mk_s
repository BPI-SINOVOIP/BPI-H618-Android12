LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

CEDARC = $(TOP)/frameworks/av/media/libcedarc/
CEDARX = $(LOCAL_PATH)/../../
################################################################################
## set flags for golobal compile and link setting.
################################################################################

CONFIG_FOR_COMPILE =
CONFIG_FOR_LINK =

LOCAL_CFLAGS += $(CONFIG_FOR_COMPILE)
LOCAL_MODULE_TAGS := optional

## set the include path for compile flags.
LOCAL_SRC_FILES:= $(notdir $(wildcard $(LOCAL_PATH)/*.c))

LOCAL_C_INCLUDES := $(SourcePath) \
                    $(LOCAL_PATH)/../../ \
                    $(CEDARC)/include \
                    $(CEDARC)/vdecoder/include \
                    $(CEDARX) \
                    $(CEDARX)/libcore/base/include \
                    $(CEDARX)/libcore/parser/include \
                    $(CEDARX)/libcore/stream/include \
                    $(CEDARX)/libcore/common/iniparser \
                    $(CEDARX)/external/include/adecoder \
                    $(CEDARX)/external/include

LOCAL_SHARED_LIBRARIES := \
            libcutils \
            libutils \
            liblog \
            libvdecoder \
            libcdx_base \
            libcdx_stream \
            libcdx_parser \
            libVE \
            libMemAdapter \
            libcdx_common \
            libcrypto

#LOCAL_32_BIT_ONLY := true
LOCAL_MODULE:= demoVdecoder

include $(BUILD_EXECUTABLE)
