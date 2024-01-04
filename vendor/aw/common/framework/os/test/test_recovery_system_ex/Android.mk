LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    md5.cpp \
    main.cpp

LOCAL_SHARED_LIBRARIES := \
    libc \
    libandroid_runtime \
    libnativehelper \
    libutils \
    libbinder \
    libui \
    libcutils 
    
LOCAL_C_INCLUDES += \
    frameworks/base/core/jni \
    $(JNI_H_INCLUDE)

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= test_recovery_system_ex

include $(BUILD_EXECUTABLE)
