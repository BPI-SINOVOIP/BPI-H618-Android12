CAMERA_HAL_LOCAL_PATH := $(call my-dir)

# Prevent the HAL from building on devices not specifically
# requesting to use it.
ifeq ($(USE_CAMERA_HAL_AUTO_1_0), true)
ifneq ($(filter venus%,$(TARGET_BOARD_PLATFORM)),)
include $(call all-subdir-makefiles)
endif
ifneq ($(filter mercury%,$(TARGET_BOARD_PLATFORM)),)
include $(call all-subdir-makefiles)
endif
ifneq ($(filter t7%,$(TARGET_BOARD_PLATFORM)),)
include $(call all-subdir-makefiles)
endif
ifneq ($(filter t8%,$(TARGET_BOARD_PLATFORM)),)
include $(call all-subdir-makefiles)
endif
ifneq ($(filter t3%,$(TARGET_BOARD_PLATFORM)),)
include $(call all-subdir-makefiles)
endif
LOCAL_PATH := $(CAMERA_HAL_LOCAL_PATH)

########## configure CONF_ANDROID_VERSION ##########
android_version = $(shell echo $(PLATFORM_VERSION) | cut -c 1)

#LOCAL_CFLAGS += -DUSE_SHARE_BUFFER

############################################################################
#####---A64---
ifeq (tulip,$(TARGET_BOARD_PLATFORM))

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libSmileEyeBlink
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__A64__
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/libfacedetection \
    hardware/aw/camera/SceneFactory

CAMERA_SHARED_LIBRARIES += \
    libvencoder \
    libfacedetection \
    libSmileEyeBlink \
    libapperceivepeople

############################################################################
#####---H6---
else ifeq (petrel,$(TARGET_BOARD_PLATFORM))

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libSmileEyeBlink
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
CAMERA_CFLAGS += -DUSE_IOMMU_DRIVER
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__H6__
CAMERA_CFLAGS += -DUSE_IOMMU_DRIVER
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/libfacedetection \
    hardware/aw/camera/SceneFactory \
    system/media/camera/include/system \

#CAMERA_SHARED_LIBRARIES += \
    #libvencoder \
    #libfacedetection \
    #libSmileEyeBlink \
    #libapperceivepeople

############################################################################
#####---H616---
else ifeq (cupid,$(TARGET_BOARD_PLATFORM))

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libSmileEyeBlink
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
LOCAL_CHECK_ELF_FILES := false
CAMERA_CFLAGS += -DUSE_IOMMU_DRIVER
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__H616__
CAMERA_CFLAGS += -DUSE_IOMMU_DRIVER
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/libfacedetection \
    hardware/aw/camera/SceneFactory \
    system/media/camera/include/system \

#CAMERA_SHARED_LIBRARIES += \
    #libvencoder \
    #libfacedetection \
    #libSmileEyeBlink \
    #libapperceivepeople

############################################################################
#####---T5---
else ifeq (mercury,$(TARGET_BOARD_PLATFORM))

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)
#include $(CLEAR_VARS)
#LOCAL_MODULE := libSmileEyeBlink
#LOCAL_MODULE_SUFFIX := .so
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
#LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
#LOCAL_MULTILIB:= both
#LOCAL_MODULE_TAGS := optional
#include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__T5__
CAMERA_CFLAGS += -DUSE_IOMMU_DRIVER
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/libfacedetection \
    hardware/aw/camera/SceneFactory \
    hardware/aw/camera/auto_1_0/allwinnertech/libAWIspApi

CAMERA_SHARED_LIBRARIES += \
    libcamera_decorder
    
CAMERA_SHARED_LIBRARIES += libAWIspApi

#CAMERA_SHARED_LIBRARIES += \
    #libvencoder \
    #libfacedetection \
    #libSmileEyeBlink \
    #libapperceivepeople

############################################################################
#####---T7---
else ifeq (t7,$(TARGET_BOARD_PLATFORM))

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
#include $(CLEAR_VARS)
#LOCAL_MODULE := libSmileEyeBlink
#LOCAL_MODULE_SUFFIX := .so
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
#LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
#LOCAL_MULTILIB:= both
#LOCAL_MODULE_TAGS := optional
#include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__T7__
#CAMERA_CFLAGS += -DUSE_IOMMU_DRIVER
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/libfacedetection \
    hardware/aw/camera/SceneFactory \
    hardware/aw/camera/1_0/allwinnertech/libAWIspApi

CAMERA_SHARED_LIBRARIES += libAWIspApi

#CAMERA_SHARED_LIBRARIES += \
    #libvencoder \
    #libfacedetection \
    #libSmileEyeBlink \
    #libapperceivepeople

############################################################################
#####---T8---
else ifeq (t8,$(TARGET_BOARD_PLATFORM))

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
#include $(CLEAR_VARS)
#LOCAL_MODULE := libSmileEyeBlink
#LOCAL_MODULE_SUFFIX := .so
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
#LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
#LOCAL_MULTILIB:= both
#LOCAL_MODULE_TAGS := optional
#include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__T8__
#CAMERA_CFLAGS += -DUSE_IOMMU_DRIVER
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/libfacedetection \
    hardware/aw/camera/SceneFactory \
    hardware/aw/camera/1_0/allwinnertech/libAWIspApi

#CAMERA_SHARED_LIBRARIES += libAWIspApi

#CAMERA_SHARED_LIBRARIES += \
    #libvencoder \
    #libfacedetection \
    #libSmileEyeBlink \
    #libapperceivepeople

############################################################################
#####---H3---
else ifeq (dolphin,$(TARGET_BOARD_PLATFORM))

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)
#include $(CLEAR_VARS)
#LOCAL_MODULE := libSmileEyeBlink
#LOCAL_MODULE_SUFFIX := .so
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
#LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
#LOCAL_MULTILIB:= both
#LOCAL_MODULE_TAGS := optional
#include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__H3__
#CAMERA_CFLAGS += -DUSE_IOMMU_DRIVER
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/libfacedetection \
    hardware/aw/camera/SceneFactory \
    hardware/aw/camera/1_0/allwinnertech/libAWIspApi

#CAMERA_SHARED_LIBRARIES += libAWIspApi

#CAMERA_SHARED_LIBRARIES += \
    #libvencoder \
    #libfacedetection \
    #libSmileEyeBlink \
    #libapperceivepeople

############################################################################
#####---T3---
else ifeq (t3,$(TARGET_BOARD_PLATFORM))

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
#include $(CLEAR_VARS)
#LOCAL_MODULE := libSmileEyeBlink
#LOCAL_MODULE_SUFFIX := .so
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
#LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
#LOCAL_MULTILIB:= both
#LOCAL_MODULE_TAGS := optional
#include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__T3__
#CAMERA_CFLAGS += -DUSE_IOMMU_DRIVER
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/libfacedetection \
    hardware/aw/camera/SceneFactory \
    hardware/aw/camera/1_0/allwinnertech/libAWIspApi

#CAMERA_SHARED_LIBRARIES += libAWIspApi

#CAMERA_SHARED_LIBRARIES += \
    #libvencoder \
    #libfacedetection \
    #libSmileEyeBlink \
    #libapperceivepeople

############################################################################
#####---A50---
else ifeq (venus,$(TARGET_BOARD_PLATFORM))

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
#include $(CLEAR_VARS)
#LOCAL_MODULE := libSmileEyeBlink
#LOCAL_MODULE_SUFFIX := .so
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
#LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
#LOCAL_MULTILIB:= both
#LOCAL_MODULE_TAGS := optional
#include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__A50__
CAMERA_CFLAGS += -DUSE_IOMMU_DRIVER
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/1_0/libfacedetection \
    hardware/aw/camera/1_0/SceneFactory \
    hardware/aw/camera/1_0/allwinnertech/libAWIspApi

CAMERA_SHARED_LIBRARIES += libAWIspApi

############################################################################
#####---A83---
else ifeq (octopus,$(TARGET_BOARD_PLATFORM))

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libSmileEyeBlink
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__A83__
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/libfacedetection \
    hardware/aw/camera/SceneFactory

CAMERA_SHARED_LIBRARIES += \
    libvencoder \
    libfacedetection \
    libSmileEyeBlink \
    libapperceivepeople

############################################################################
#####---A33---
else ifeq (astar,$(TARGET_BOARD_PLATFORM))

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libSmileEyeBlink
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__A33__
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/libfacedetection \
    hardware/aw/camera/SceneFactory

CAMERA_SHARED_LIBRARIES += \
    libvencoder \
    libfacedetection \
    libSmileEyeBlink \
    libapperceivepeople

############################################################################
#####---A80---
else ifeq (kylin,$(TARGET_BOARD_PLATFORM))

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libSmileEyeBlink
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__A80__
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/libfacedetection \
    hardware/aw/camera/SceneFactory

CAMERA_SHARED_LIBRARIES += \
    libvencoder \
    libfacedetection \
    libSmileEyeBlink \
    libapperceivepeople

############################################################################
#####---A63---
else ifeq (uranus,$(TARGET_BOARD_PLATFORM))

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libSmileEyeBlink
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__A63__
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/libfacedetection \
    hardware/aw/camera/SceneFactory

CAMERA_SHARED_LIBRARIES += \
    libvencoder \
    libfacedetection \
    libSmileEyeBlink \
    libapperceivepeople

############################################################################
#####---vr9---
else ifeq (neptune,$(TARGET_BOARD_PLATFORM))

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libSmileEyeBlink
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__A63__
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/libfacedetection \
    hardware/aw/camera/SceneFactory

CAMERA_SHARED_LIBRARIES += \
    libvencoder \
    libfacedetection \
    libSmileEyeBlink \
    libapperceivepeople

############################################################################
#####---universal implement---
else

$(warning Use universal implement for camera hal 1.0, please confirm!)

include $(CLEAR_VARS)
LOCAL_MODULE := libfacedetection
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libfacedetection.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libfacedetection.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
#include $(CLEAR_VARS)
#LOCAL_MODULE := libSmileEyeBlink
#LOCAL_MODULE_SUFFIX := .so
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_SRC_FILES_32 := lib32/facedetection/libSmileEyeBlink.so
#LOCAL_SRC_FILES_64 := lib64/facedetection/libSmileEyeBlink.so
#LOCAL_MULTILIB:= both
#LOCAL_MODULE_TAGS := optional
#include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libapperceivepeople
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/facedetection/libapperceivepeople.so
LOCAL_SRC_FILES_64 := lib64/facedetection/libapperceivepeople.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

CAMERA_CFLAGS += -D__UNIVERSAL__
#CAMERA_CFLAGS += -DUSE_IOMMU_DRIVER
CAMERA_C_INCLUDES += \
    frameworks/av/media/libcedarc/include \
    hardware/aw/camera/libfacedetection \
    hardware/aw/camera/SceneFactory \
    hardware/aw/camera/1_0/allwinnertech/libAWIspApi

#CAMERA_SHARED_LIBRARIES += libAWIspApi

#CAMERA_SHARED_LIBRARIES += \
    #libvencoder \
    #libfacedetection \
    #libSmileEyeBlink \
    #libapperceivepeople

endif
############################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := libproc
LOCAL_MODULE_SUFFIX := .so
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/proc/libproc.so
LOCAL_SRC_FILES_64 := lib64/proc/libproc.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
# TODO revert this after fix elf check fail
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE := libhdr
LOCAL_MODULE_SUFFIX := .so
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_32 := lib32/hdr/libhdr.so
LOCAL_SRC_FILES_64 := lib64/hdr/libhdr.so
LOCAL_MULTILIB:= both
LOCAL_MODULE_TAGS := optional
# TODO revert this after fix elf check fail
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
# LOCAL_MODULE_RELATIVE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true

LOCAL_CFLAGS += -Wall  -Wno-unused-parameter -Wno-macro-redefined -Wno-unused-parameter -Wno-extra-tokens -Wno-null-arithmetic -Wno-format -Wno-reorder -Wno-unused-variable -Wno-writable-strings -Wno-logical-op-parentheses -Wno-sign-compare \
                -Wno-unused-parameter -Wno-unused-value -Wno-unused-function -Wno-parentheses -Wno-extern-c-compat -Wno-null-conversion -Wno-sometimes-uninitialized -Wno-gnu-designator

LOCAL_SHARED_LIBRARIES += \
    libbinder \
    libutils \
    libcutils \
    libui \
    liblog \
    libexpat

LOCAL_SHARED_LIBRARIES += \
    libhdr \
    libvdecoder.vendor \
    libvencoder.vendor \
    libproc

LOCAL_C_INCLUDES +=                                 \
    frameworks/base/core/jni/android/graphics         \
    frameworks/native/include/media/openmax            \
    hardware/libhardware/include/hardware            \
    frameworks/native/include                        \
    frameworks/native/libs/nativewindow/include		\
    frameworks/native/include/media/hardware         \
    system/media/camera/include/                \
    hardware/aw/camera/auto_1_0/include        \
    hardware/aw/camera/auto_1_0        \
    hardware/aw/camera/auto_1_0/allwinnertech/CameraDecoder \
    hardware/aw/camera/auto_1_0/allwinnertech/AWCamRecorder \
    system/core/libion/kernel-headers \
    system/core/libion/include \
    system/core/include \
    system/core/include/utils \
    system/core/liblog/include \
    hardware/aw/camera/auto_1_0/allwinnertech/deinterlace \
    $(TARGET_HARDWARE_INCLUDE)

ifneq ($(GPU_PUBLIC_INCLUDE),)
LOCAL_C_INCLUDES += hardware/aw/gpu
LOCAL_C_INCLUDES += hardware/aw/gpu/include
LOCAL_CFLAGS += -DHAL_PUBLIC_UNIFIED_ENABLE
LOCAL_CFLAGS += -DGPU_PUBLIC_INCLUDE=\"$(GPU_PUBLIC_INCLUDE)\"
endif

LOCAL_SRC_FILES := \
    memory/memoryAdapter.c \
    memory/ionMemory/ionAlloc.c \
    CameraManager.cpp \
    G2dApi.cpp \
    HALCameraFactory.cpp \
    PreviewWindow.cpp \
    CameraParameters.cpp \
    CallbackNotifier.cpp \
    CCameraConfig.cpp \
    BufferListManager.cpp \
    OSAL_Mutex.c \
    OSAL_Queue.c \
    scaler.c \
    CameraDebug.cpp \
    SceneFactory/HDRSceneMode.cpp \
    SceneFactory/NightSceneMode.cpp \
    SceneFactory/SceneModeFactory.cpp \
    allwinnertech/deinterlace/DiProcess.cpp

ifeq ($(USE_IOMMU),true)
    LOCAL_CFLAGS += -DUSE_IOMMU
endif

# choose hal for new driver or old
SUPPORT_NEW_DRIVER := Y

ifeq ($(SUPPORT_NEW_DRIVER),Y)
LOCAL_CFLAGS += -DSUPPORT_NEW_DRIVER -DTARGET_BOARD_PLATFORM=$(TARGET_BOARD_PLATFORM) -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
LOCAL_SRC_FILES += \
    CameraHardware2.cpp \
    V4L2CameraDevice2.cpp
else
LOCAL_SRC_FILES += \
    CameraHardware.cpp \
    V4L2CameraDevice.cpp
endif

########## configure CONF_ANDROID_VERSION ##########
android_version = $(shell echo $(PLATFORM_VERSION) | cut -c 1)

LOCAL_CFLAGS += $(CAMERA_CFLAGS)
LOCAL_C_INCLUDES += $(CAMERA_C_INCLUDES)
LOCAL_SHARED_LIBRARIES += $(CAMERA_SHARED_LIBRARIES)

LOCAL_CFLAGS += -Wno-error=format-security
LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

endif # USE_CAMERA_HAL_AUTO_1_0
