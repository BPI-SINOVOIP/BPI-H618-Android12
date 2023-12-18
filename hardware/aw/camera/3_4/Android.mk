ifeq ($(USE_CAMERA_HAL_3_4), true)

v4l2_local_path := $(call my-dir)
top_path := $(v4l2_local_path)/../../../..

# Common setting.
# ==============================================================================
#v4l2_cflags := -D__USE_ONE_THREAD__
#v4l2_cflags := -D__USE_MULTI_THREAD__
v4l2_cflags := \

v4l2_shared_libs := \
  libbase \
  libcamera_metadata \
  libcutils \
  libhardware \
  liblog \
  libsync \
  libutils \
  libbinder \
  libui \
  libexpat \
  libvencoder.vendor\
  libion

v4l2_static_libs := \
  android.hardware.camera.common@1.0-helper

v4l2_cflags += -fno-short-enums -Wall -Wextra -fvisibility=hidden -Wc++11-narrowing -DTARGET_BOARD_PLATFORM=$(TARGET_BOARD_PLATFORM) -Wno-unused-parameter -Wno-macro-redefined -Wno-unused-parameter -Wno-extra-tokens -Wno-null-arithmetic -Wno-format -Wno-reorder -Wno-unused-variable -Wno-writable-strings -Wno-logical-op-parentheses -Wno-sign-compare -Wno-unused-parameter -Wno-unused-value -Wno-unused-function -Wno-parentheses -Wno-extern-c-compat -Wno-null-conversion  -Wno-sometimes-uninitialized -Wno-gnu-designator -Wno-unused-label -Wno-pointer-arith -Wno-empty-body -fPIC -Wno-missing-field-initializers -Wno-pessimizing-move -Wno-unused-private-field -Wno-user-defined-warnings

#v4l2_cflags := -DTARGET_BOARD_PLATFORM=$(TARGET_BOARD_PLATFORM) -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)


# Include the build/core/pathmap.mk mapping from pathmap_INCL
v4l2_c_includes := $(call include-path-for, camera) \
	$(v4l2_local_path) \
	$(v4l2_local_path)/include \
	$(top_path)/system/core/libion/kernel-headers \
	$(top_path)/hardware/interfaces/camera/common/1.0/default/include \
	$(top_path)/frameworks/av/media/libcedarc/include \
	$(top_path)/frameworks/native/include \
#	$(v4l2_local_path)/../../../../frameworks/native/libs/nativewindow/include \
#	$(v4l2_local_path)/../../../../frameworks/native/libs/nativebase/include \

v4l2_src_files := \
  common.cpp \
  camera.cpp \
  capture_request.cpp \
  format_metadata_factory.cpp \
  metadata/boottime_state_delegate.cpp \
  metadata/enum_converter.cpp \
  metadata/metadata.cpp \
  metadata/metadata_reader.cpp \
  memory/ion_alloc.c\
  camera_config.cpp \
  request_tracker.cpp \
  static_properties.cpp \
  stream_format.cpp \
  v4l2_camera.cpp \
  v4l2_camera_hal.cpp \
  v4l2_gralloc.cpp \
  v4l2_metadata_factory.cpp \
  v4l2_stream.cpp \
  v4l2_wrapper.cpp \
  stream_manager.cpp \
  camera_stream.cpp \

v4l2_test_files := \
  format_metadata_factory_test.cpp \
  metadata/control_test.cpp \
  metadata/default_option_delegate_test.cpp \
  metadata/enum_converter_test.cpp \
  metadata/ignored_control_delegate_test.cpp \
  metadata/map_converter_test.cpp \
  metadata/menu_control_options_test.cpp \
  metadata/metadata_reader_test.cpp \
  metadata/metadata_test.cpp \
  metadata/no_effect_control_delegate_test.cpp \
  metadata/partial_metadata_factory_test.cpp \
  metadata/property_test.cpp \
  metadata/ranged_converter_test.cpp \
  metadata/slider_control_options_test.cpp \
  metadata/state_test.cpp \
  metadata/tagged_control_delegate_test.cpp \
  metadata/tagged_control_options_test.cpp \
  metadata/v4l2_control_delegate_test.cpp \
  request_tracker_test.cpp \
  static_properties_test.cpp

#   PLATFORM    ISP_VERSION VIDEO_NUM
SEARCH_ISP_TABLE := \
    ceres       isp_new         2\
    pluto       isp_new         2\
    epic        isp_new         2\

POS_X     = $(if $(findstring $1,$2),$(call POS_X,$1,$(wordlist 2,$(words $2),$2),x $3),$3)
POS_N     = $(words $(call POS_X,$1,$2))

PLAT_IDX := $(call POS_N,$(TARGET_BOARD_PLATFORM),$(SEARCH_ISP_TABLE))
ISP_VERSION = isp_old
VIDEO_NUM   =  1
ifneq ($(PLAT_IDX),0)
VERSION_IDX    := $(shell expr $(PLAT_IDX) + 1)
NUM_IDX        := $(shell expr $(PLAT_IDX) + 2)
ISP_VERSION    := $(word $(VERSION_IDX),$(SEARCH_ISP_TABLE))
VIDEO_NUM      := $(word $(NUM_IDX),$(SEARCH_ISP_TABLE))
endif

#$(info  Platform:$(TARGET_BOARD_PLATFORM), IspVersion:$(ISP_VERSION), VideoNum:$(VIDEO_NUM))
v4l2_cflags += -D__VIDEO_NUM__=$(VIDEO_NUM)

v4l2_c_includes += \
  $(v4l2_local_path)/allwinnertech/libAWIspApi

v4l2_shared_libs += \
  libAWIspApi

include $(call all-makefiles-under,$(v4l2_local_path))

# ==============================================================================
include $(CLEAR_VARS)

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_PATH := $(v4l2_local_path)
LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_RELATIVE_PATH := hw
# Android O use vendor/lib/hw dir for modules
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CFLAGS += $(v4l2_cflags)
LOCAL_SHARED_LIBRARIES := $(v4l2_shared_libs)
LOCAL_STATIC_LIBRARIES := \
  $(v4l2_static_libs) \

ifneq ($(GPU_PUBLIC_INCLUDE),)
LOCAL_C_INCLUDES += hardware/aw/gpu
LOCAL_C_INCLUDES += hardware/aw/gpu/include
LOCAL_CFLAGS += -DHAL_PUBLIC_UNIFIED_ENABLE
LOCAL_CFLAGS += -DGPU_PUBLIC_INCLUDE=\"$(GPU_PUBLIC_INCLUDE)\"
endif
LOCAL_C_INCLUDES += $(v4l2_c_includes)
LOCAL_SRC_FILES := $(v4l2_src_files)

include $(BUILD_SHARED_LIBRARY)

# Unit tests for V4L2 Camera HAL.
# ==============================================================================
ifeq ($(USE_CAMERA_HAL_V4L2_TEST), true)
include $(CLEAR_VARS)
LOCAL_PATH := $(v4l2_local_path)
LOCAL_MODULE := camera.v4l2_test
LOCAL_CFLAGS += $(v4l2_cflags)
LOCAL_SHARED_LIBRARIES := $(v4l2_shared_libs)
LOCAL_STATIC_LIBRARIES := \
  libBionicGtestMain \
  libgmock \
  $(v4l2_static_libs) \

LOCAL_C_INCLUDES += $(v4l2_c_includes)
LOCAL_SRC_FILES := \
  $(v4l2_src_files) \
  $(v4l2_test_files) \

include $(BUILD_NATIVE_TEST)
endif #USE_CAMERA_HAL_V4L2_TEST

endif # USE_CAMERA_V4L2_HAL
