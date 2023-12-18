LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := misc.img
LOCAL_MODULE_CLASS:= ETC
LOCAL_MODULE_PATH := $(PRODUCT_OUT)

GEN := $(local-generated-sources-dir)/$(LOCAL_MODULE)

$(GEN): $(LOCAL_PATH)/gen.sh
	$< $@

LOCAL_PREBUILT_MODULE_FILE := $(GEN)

include $(BUILD_PREBUILT)
