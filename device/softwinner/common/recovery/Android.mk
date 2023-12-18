LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CLANG := true
LOCAL_CFLAGS += -Wall
ifeq ($(TARGET_OTA_RESTORE_BOOT_STORAGE_DATA), true)
    LOCAL_CPPFLAGS += -DRESTORE_BOOT_STORAGE_DATA=true
else
    LOCAL_CPPFLAGS += -DRESTORE_BOOT_STORAGE_DATA=false
endif
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := librecovery_updater_common
LOCAL_SRC_FILES := \
    recovery_updater.cpp \
    BurnNandBoot.cpp \
    BurnSdBoot.cpp \
    Utils.cpp \
    UpdateCustom.cpp

LOCAL_STATIC_LIBRARIES := \
    libbase \
    libziparchive \
    libotautil \
    android.hardware.boot@1.1

LOCAL_C_INCLUDES += \
    bootable/recovery \
    bootable/recovery/edify/include \
    bootable/recovery/otautil \
    bootable/recovery/updater/include \
    system/extras/boot_control_copy


include $(BUILD_STATIC_LIBRARY)

