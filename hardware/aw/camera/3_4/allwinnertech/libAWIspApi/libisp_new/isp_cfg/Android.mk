
LIBISP_FLAGS = -Wall -Wno-unused-parameter -Wno-macro-redefined -Wno-unused-parameter -Wno-extra-tokens \
		-Wno-null-arithmetic -Wno-format -Wno-reorder -Wno-unused-variable -Wno-writable-strings \
		-Wno-logical-op-parentheses -Wno-sign-compare -Wno-unused-parameter -Wno-unused-value \
		-Wno-unused-function -Wno-parentheses -Wno-extern-c-compat -Wno-null-conversion \
		-Wno-sometimes-uninitialized -Wno-gnu-designator -Wno-unused-label -Wno-pointer-arith -Wno-empty-body -fPIC

#########################################
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS += $(LIBISP_FLAGS)

LOCAL_MODULE := libisp_ini

LOCAL_MODULE_TAGS := optional

LOCAL_PROPRIETARY_MODULE := true

LOCAL_SRC_FILES := \
    isp_ini_parse.c

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/SENSOR_H

LOCAL_SHARED_LIBRARIES := \
    libcutils liblog

include $(BUILD_SHARED_LIBRARY)


