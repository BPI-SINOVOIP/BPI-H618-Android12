LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

SPRD_SUITE_CFLAGS :=

ifeq ($(PLATFORM_VERSION),4.4.4)
SPRD_SUITE_CFLAGS += -DANDROID_4_4_4
endif

ifeq ($(PLATFORM_VERSION),6.0)
SPRD_SUITE_CFLAGS += -DANDROID_6_0
endif

LOCAL_SRC_FILES := \
    osi/src/list.c \
    osi/src/allocator.c \
    osi/src/hash_map.c \
    osi/src/hash_functions.c \
    osi/src/allocation_tracker.c \
    osi/src/semaphore.c \
    osi/src/fixed_queue.c \
    osi/src/reactor.c \
    osi/src/thread.c \
    osi/src/eager_reader.c \
    osi/src/future.c \
    main/src/sprd_suite.c \
    main/src/buffer_allocator.c \
    main/src/vendor_h4.c \
    main/src/vendor_hci.c \
    main/src/vendor_hcif.c \
    main/src/vendor_hcicmds.c \
    main/src/vendor_test.c \
    main/src/vendor_suite.c

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/main/include \
    $(LOCAL_PATH)/osi/include \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    liblog \
    libc \
    libdl

LOCAL_CFLAGS := $(SPRD_SUITE_CFLAGS) \
    -std=c99 -Wall
LOCAL_CLANG_CFLAGS += -Wno-error=typedef-redefinition

LOCAL_MODULE := libbt-sprd_suite
#LOCAL_MODULE_TAGS := debug
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)
