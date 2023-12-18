WIDEVINE_SUPPORTED_ARCH := arm x86
LOCAL_PATH:= $(call my-dir)

#####################################################################
#android.hardware.drm@1.4-service.widevine
include $(CLEAR_VARS)

LOCAL_MODULE := android.hardware.drm@1.4-service.widevine
LOCAL_INIT_RC := android.hardware.drm@1.4-service.widevine.rc
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_CHECK_ELF_FILES := false
#LOCAL_STRIP_MODULE := true
LOCAL_VINTF_FRAGMENTS := manifest_android.hardware.drm@1.4-service.widevine.xml
include $(BUILD_PREBUILT)


#android.hardware.drm@1.4-service-lazy.widevine
include $(CLEAR_VARS)

LOCAL_MODULE := android.hardware.drm@1.4-service-lazy.widevine
LOCAL_INIT_RC := android.hardware.drm@1.4-service-lazy.widevine.rc
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_CFLAGS := -DLAZY_SERVICE
LOCAL_OVERRIDES_MODULES := android.hardware.drm@1.4-service.widevine
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := android.hardware.drm@1.0 android.hardware.drm@1.1 android.hardware.drm@1.2 android.hardware.drm@1.3 android.hardware.drm@1.4 libbase libbinder libc++ libc libdl libhidlbase libhidltransport libhwbinder liblog libm libutils libwvhidl
LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
#LOCAL_STRIP_MODULE := true
LOCAL_VINTF_FRAGMENTS := manifest_android.hardware.drm@1.4-service.widevine.xml
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

#####################################################################
#libwvhidl.so
include $(CLEAR_VARS)

LOCAL_MODULE := libwvhidl
LOCAL_SRC_FILES := lib32/libwvhidl.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 32
LOCAL_PROPRIETARY_MODULE := true
#LOCAL_STRIP_MODULE := true
LOCAL_SHARED_LIBRARIES := android.hardware.drm@1.0 android.hardware.drm@1.1 android.hardware.drm@1.2 android.hardware.drm@1.3 android.hardware.drm@1.4 android.hidl.memory@1.0 libbase libc++ libc libcrypto libcutils libdl libhidlbase libhidlmemory liblog libm libprotobuf-cpp-lite libutils
include $(BUILD_PREBUILT)

#64
include $(CLEAR_VARS)
LOCAL_MODULE := libwvhidl
LOCAL_SRC_FILES := lib64/libwvhidl.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 64
LOCAL_PROPRIETARY_MODULE := true
#LOCAL_STRIP_MODULE := true
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

#####################################################################
#libwvdrmengine.so
include $(CLEAR_VARS)

LOCAL_MODULE := libwvdrmengine
LOCAL_SRC_FILES := lib32/libwvdrmengine.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 32
LOCAL_MODULE_RELATIVE_PATH := mediadrm
LOCAL_PROPRIETARY_MODULE := true
#LOCAL_STRIP_MODULE := true
LOCAL_SHARED_LIBRARIES := libbase libc++ libc libcrypto libdl libhidlbase liblog libm libprotobuf-cpp-lite libstagefright_foundation libutils
include $(BUILD_PREBUILT)

#64
include $(CLEAR_VARS)
LOCAL_MODULE := libwvdrmengine
LOCAL_SRC_FILES := lib64/libwvdrmengine.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 64
LOCAL_MODULE_RELATIVE_PATH := mediadrm
LOCAL_PROPRIETARY_MODULE := true
#LOCAL_STRIP_MODULE := true
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

#####################################################################
#libvtswidevine.so
include $(CLEAR_VARS)

LOCAL_MODULE := libvtswidevine
LOCAL_SRC_FILES := lib32/libvtswidevine.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 32
LOCAL_MODULE_RELATIVE_PATH := drm-vts-test-libs
LOCAL_PROPRIETARY_MODULE := true
#LOCAL_STRIP_MODULE := true
LOCAL_SHARED_LIBRARIES := libbase libc++ libc libcrypto libdl libhidlbase liblog libm libssl libutils
include $(BUILD_PREBUILT)

#64
include $(CLEAR_VARS)
LOCAL_MODULE := libvtswidevine
LOCAL_SRC_FILES := lib64/libvtswidevine.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 64
LOCAL_MODULE_RELATIVE_PATH := drm-vts-test-libs
LOCAL_PROPRIETARY_MODULE := true
#LOCAL_STRIP_MODULE := true
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

#####################################################################
#liboemcrypto.so
ifeq ($(BOARD_WIDEVINE_OEMCRYPTO_LEVEL),1)
include $(CLEAR_VARS)

LOCAL_MODULE := liboemcrypto
LOCAL_SRC_FILES := lib32/secure/liboemcrypto.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 32
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SHARED_LIBRARIES := libbase libc++ libc libcrypto libcutils libdl libhidlbase liblog libm libteec
#LOCAL_STRIP_MODULE := true
include $(BUILD_PREBUILT)
#64
include $(CLEAR_VARS)
LOCAL_MODULE := liboemcrypto
LOCAL_SRC_FILES := lib64/secure/liboemcrypto.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := 64
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CHECK_ELF_FILES := false
#LOCAL_STRIP_MODULE := true
include $(BUILD_PREBUILT)
endif# liboemcrypto
