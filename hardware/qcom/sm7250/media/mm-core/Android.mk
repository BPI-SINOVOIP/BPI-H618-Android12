ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

OMXCORE_CFLAGS := -g -O3 -DVERBOSE
OMXCORE_CFLAGS += -O0 -fno-inline -fno-short-enums
OMXCORE_CFLAGS += -D_ANDROID_
OMXCORE_CFLAGS += -U_ENABLE_QC_MSG_LOG_

ifeq ($(VIDC_STUB_HAL),true)
OMXCORE_CFLAGS += -DVIDC_STUB_HAL
endif

#===============================================================================
#             Figure out the targets
#===============================================================================

MPEGH_TARGET_LIST := kona lito bengal
ifeq ($(call is-board-platform-in-list, $(MPEGH_TARGET_LIST)), true)
OMXCORE_CFLAGS += -DAUDIO_MPEGH_ENABLED
endif

ifeq ($(filter $(TARGET_BOARD_PLATFORM), bengal),$(TARGET_BOARD_PLATFORM))
OMXCORE_CFLAGS += -D_BENGAL_
else ifeq ($(filter $(TARGET_BOARD_PLATFORM), $(MSMSTEPPE)),$(TARGET_BOARD_PLATFORM))
OMXCORE_CFLAGS += -D_STEPPE_
else
OMXCORE_CFLAGS += -D_DEFAULT_
endif

ifeq ($(call is-platform-sdk-version-at-least,27),true) # O-MR1
OMXCORE_CFLAGS += -D_ANDROID_O_MR1_DIVX_CHANGES
endif

#===============================================================================
#             Deploy the headers that can be exposed
#===============================================================================

LOCAL_COPY_HEADERS_TO   := mm-core/omxcore
LOCAL_COPY_HEADERS      := inc/OMX_Audio.h
LOCAL_COPY_HEADERS      += inc/OMX_Component.h
LOCAL_COPY_HEADERS      += inc/OMX_ContentPipe.h
LOCAL_COPY_HEADERS      += inc/OMX_Core.h
LOCAL_COPY_HEADERS      += inc/OMX_Image.h
LOCAL_COPY_HEADERS      += inc/OMX_Index.h
LOCAL_COPY_HEADERS      += inc/OMX_IVCommon.h
LOCAL_COPY_HEADERS      += inc/OMX_Other.h
LOCAL_COPY_HEADERS      += inc/OMX_QCOMExtns.h
LOCAL_COPY_HEADERS      += inc/OMX_Types.h
LOCAL_COPY_HEADERS      += inc/OMX_Video.h
LOCAL_COPY_HEADERS      += inc/qc_omx_common.h
LOCAL_COPY_HEADERS      += inc/qc_omx_component.h
LOCAL_COPY_HEADERS      += inc/qc_omx_msg.h
LOCAL_COPY_HEADERS      += inc/QOMX_AudioExtensions.h
LOCAL_COPY_HEADERS      += inc/QOMX_AudioIndexExtensions.h
LOCAL_COPY_HEADERS      += inc/OMX_CoreExt.h
LOCAL_COPY_HEADERS      += inc/QOMX_CoreExtensions.h
LOCAL_COPY_HEADERS      += inc/QOMX_FileFormatExtensions.h
LOCAL_COPY_HEADERS      += inc/QOMX_IVCommonExtensions.h
LOCAL_COPY_HEADERS      += inc/QOMX_SourceExtensions.h
LOCAL_COPY_HEADERS      += inc/QOMX_VideoExtensions.h
LOCAL_COPY_HEADERS      += inc/OMX_IndexExt.h
LOCAL_COPY_HEADERS      += inc/OMX_VideoExt.h
LOCAL_COPY_HEADERS      += inc/QOMX_StreamingExtensions.h
LOCAL_COPY_HEADERS      += inc/QCMediaDefs.h
LOCAL_COPY_HEADERS      += inc/QCMetaData.h

#===============================================================================
#             LIBRARY for Android apps
#===============================================================================

LOCAL_C_INCLUDES        := $(LOCAL_PATH)/src/common
LOCAL_C_INCLUDES        += $(QCOM_MEDIA_ROOT)/libplatformconfig

LOCAL_HEADER_LIBRARIES := \
        libutils_headers \
        libomxcore_headers

LOCAL_EXPORT_HEADER_LIBRARY_HEADERS := libomxcore_headers

ifeq ($(TARGET_ENABLE_VIDC_INTSAN), true)
LOCAL_SANITIZE := integer_overflow
ifeq ($(TARGET_ENABLE_VIDC_INTSAN_DIAG), true)
$(warning INTSAN_DIAG_ENABLED)
LOCAL_SANITIZE_DIAG := integer_overflow
endif
endif

LOCAL_PRELINK_MODULE    := false
LOCAL_MODULE            := libOmxCore
LOCAL_LICENSE_KINDS     := SPDX-license-identifier-BSD SPDX-license-identifier-MIT
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE       := $(LOCAL_PATH)/../NOTICE
LOCAL_MODULE_TAGS       := optional
LOCAL_VENDOR_MODULE     := true
LOCAL_SHARED_LIBRARIES  := liblog libdl libcutils
ifeq ($(call is-board-platform-in-list, $(MSM_VIDC_TARGET_LIST)),true)
ifeq ($(VIDC_STUB_HAL),false)
LOCAL_SHARED_LIBRARIES  += libplatformconfig
endif
endif
LOCAL_CFLAGS            := $(OMXCORE_CFLAGS)

LOCAL_SRC_FILES         := src/common/omx_core_cmp.cpp
LOCAL_SRC_FILES         += src/common/qc_omx_core.c
ifneq (,$(filter lito bengal kona $(MSMSTEPPE),$(TARGET_BOARD_PLATFORM)))
LOCAL_SRC_FILES         += src/registry_table_android.c
else
LOCAL_SRC_FILES         += src/default/qc_registry_table_android.c
endif

include $(BUILD_SHARED_LIBRARY)

#===============================================================================
#             LIBRARY for command line test apps
#===============================================================================

include $(CLEAR_VARS)

LOCAL_C_INCLUDES        := $(LOCAL_PATH)/src/common
LOCAL_C_INCLUDES        += $(QCOM_MEDIA_ROOT)/libplatformconfig

LOCAL_HEADER_LIBRARIES := \
        libutils_headers \
        libomxcore_headers

LOCAL_EXPORT_HEADER_LIBRARY_HEADERS := libomxcore_headers

LOCAL_PRELINK_MODULE    := false
LOCAL_MODULE            := libmm-omxcore
LOCAL_LICENSE_KINDS     := SPDX-license-identifier-BSD SPDX-license-identifier-MIT
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE       := $(LOCAL_PATH)/../NOTICE
LOCAL_MODULE_TAGS       := optional
LOCAL_VENDOR_MODULE     := true
LOCAL_SHARED_LIBRARIES  := liblog libdl libcutils
ifeq ($(call is-board-platform-in-list, $(MSM_VIDC_TARGET_LIST)),true)
ifeq ($(VIDC_STUB_HAL),false)
LOCAL_SHARED_LIBRARIES  += libplatformconfig
endif
endif
LOCAL_CFLAGS            := $(OMXCORE_CFLAGS)

ifeq ($(TARGET_ENABLE_VIDC_INTSAN), true)
LOCAL_SANITIZE := integer_overflow
ifeq ($(TARGET_ENABLE_VIDC_INTSAN_DIAG), true)
$(warning INTSAN_DIAG_ENABLED)
LOCAL_SANITIZE_DIAG := integer_overflow
endif
endif

LOCAL_SRC_FILES         := src/common/omx_core_cmp.cpp
LOCAL_SRC_FILES         += src/common/qc_omx_core.c
ifneq (,$(filter lito bengal kona $(MSMSTEPPE),$(TARGET_BOARD_PLATFORM)))
LOCAL_SRC_FILES         += src/$(MM_CORE_TARGET)/registry_table.c
else
LOCAL_SRC_FILES         += src/$(MM_CORE_TARGET)/default/qc_registry_table.c
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libomxcore_headers
LOCAL_LICENSE_KINDS := SPDX-license-identifier-BSD SPDX-license-identifier-MIT
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/../NOTICE
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc
LOCAL_VENDOR_MODULE := true

include $(BUILD_HEADER_LIBRARY)

endif #BUILD_TINY_ANDROID
