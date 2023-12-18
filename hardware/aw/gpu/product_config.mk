#
# Copyright (C) 2017-2019 Allwinner Technology Co., Ltd. All rights reserved.
#

GPU_ROOT := $(shell dirname $(lastword $(MAKEFILE_LIST)))

PRODUCT_GPU_FILES      :=
PRODUCT_GPU_PACKAGES   :=
PRODUCT_GPU_PROPERTIES :=
TARGET_COPY_OUT_VENDOR := vendor

CONFIG_TABLE := GPU_ARCH GLES_VERSION SUPPORT_AEP VULKAN_VERSION GPU_PUBLIC_INCLUDE

# TARGET_GPU_TYPE  GPU_ARCH  GLES_VERSION SUPPORT_AEP VULKAN_VERSION GPU_PUBLIC_INCLUDE
SEARCH_TABLES := \
    mali400    mali-utgard   2.0 null null include/hal_public/hal_mali_utgard.h            \
    mali450    mali-utgard   2.0 null null include/hal_public/hal_mali_utgard.h            \
    mali-t720  mali-midgard  3.1 1    null include/hal_public/hal_mali_midgard.h           \
    mali-t760  mali-midgard  3.2 1    null include/hal_public/hal_mali_midgard.h           \
    sgx544     img-sgx       2.0 null null include/hal_public/hal_img_sgx.h                \
    mali-g31   mali-bifrost  3.2 1    1.1 mali-bifrost/gralloc/src/mali_gralloc_buffer.h  \
    ge8300     img-rgx       3.2 1    1.1  include/hal_public/hal_img_rgx.h

POS_X  = $(if $(findstring $1,$2),$(call POS_X,$1,$(wordlist 2,$(words $2),$2),x $3),$3)
POS_N  = $(words $(call POS_X,$1,$2))

BASE_IDX := $(call POS_N,$(TARGET_GPU_TYPE),$(SEARCH_TABLES))

ifeq ($(BASE_IDX),0)
    $(error TARGET_GPU_TYPE($(TARGET_GPU_TYPE)) is invalid!)
endif

$(foreach k,$(CONFIG_TABLE),\
    $(eval BASE_IDX := $(shell expr $(BASE_IDX) + 1)) \
    $(eval $(k) := $(word $(BASE_IDX),$(SEARCH_TABLES))) \
    $(if $(subst null,,$(k)),,$(eval $(k) := )))

$(call soong_config_add,gpu,public_include_file,$(GPU_PUBLIC_INCLUDE))

GPU_COPY_ROOT_DIR := $(GPU_ROOT)/$(GPU_ARCH)/$(TARGET_GPU_TYPE)/$(TARGET_ARCH)

ifeq ($(wildcard $(GPU_COPY_ROOT_DIR)/lib/*.so),)
    $(error There is no libraries in $(GPU_COPY_ROOT_DIR)!)
endif

PRODUCT_GPU_FILES += $(call find-copy-subdir-files,"egl.cfg",$(GPU_ROOT)/$(GPU_ARCH),$(TARGET_COPY_OUT_SYSTEM)/lib/egl)

# For Mali GPUs
ifeq ($(filter-out mali-%, $(GPU_ARCH)),)
    PRODUCT_GPU_PACKAGES += gralloc.$(TARGET_BOARD_PLATFORM)
    PRODUCT_GPU_FILES    += $(GPU_COPY_ROOT_DIR)/lib/libGLES_mali.so:$(TARGET_COPY_OUT_VENDOR)/lib/egl/libGLES_mali.so
    PRODUCT_GPU_FILES    += $(GPU_COPY_ROOT_DIR)/lib/libGLES_mali.so:$(TARGET_COPY_OUT_VENDOR)/lib/hw/vulkan.$(TARGET_BOARD_PLATFORM).so
    ifeq ($(TARGET_ARCH),arm64)
        PRODUCT_GPU_FILES  += $(GPU_COPY_ROOT_DIR)/lib64/libGLES_mali.so:$(TARGET_COPY_OUT_VENDOR)/lib64/egl/libGLES_mali.so
    endif
# For IMG GPUs
else ifeq ($(filter-out img-%, $(GPU_ARCH)),)
    PRODUCT_GPU_FILES += \
        $(GPU_COPY_ROOT_DIR)/init.gpu.img.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.gpu.img.rc \
	$(GPU_COPY_ROOT_DIR)/init/android.hardware.graphics.allocator@4.0-service.img.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/android.hardware.graphics.allocator@4.0-service.img.rc \
	$(GPU_COPY_ROOT_DIR)/init/android.hardware.graphics.allocator@4.0.img.xml:$(TARGET_COPY_OUT_VENDOR)/etc/vintf/manifest/android.hardware.graphics.allocator@4.0.img.xml \
	$(GPU_COPY_ROOT_DIR)/init/android.hardware.graphics.mapper@4.0-passthrough.img.xml:$(TARGET_COPY_OUT_VENDOR)/etc/vintf/manifest/android.hardware.graphics.mapper@4.0-passthrough.img.xml \
	$(call find-copy-subdir-files,"powervr.ini",$(GPU_COPY_ROOT_DIR),$(TARGET_COPY_OUT_VENDOR)/etc) \
        $(GPU_COPY_ROOT_DIR)/bin/pvrsrvctl:$(TARGET_COPY_OUT_VENDOR)/bin/pvrsrvctl \
        $(GPU_COPY_ROOT_DIR)/bin/pvrtld:$(TARGET_COPY_OUT_VENDOR)/bin/pvrtld \
        $(GPU_COPY_ROOT_DIR)/bin/pvrhtb2txt:$(TARGET_COPY_OUT_VENDOR)/bin/pvrhtb2txt \
        $(GPU_COPY_ROOT_DIR)/bin/pvrdebug:$(TARGET_COPY_OUT_VENDOR)/bin/pvrdebug \
        $(GPU_COPY_ROOT_DIR)/bin/hwperfbin2jsont:$(TARGET_COPY_OUT_VENDOR)/bin/hwperfbin2jsont \
        $(GPU_COPY_ROOT_DIR)/bin/pvrhtbd:$(TARGET_COPY_OUT_VENDOR)/bin/pvrhtbd \
        $(GPU_COPY_ROOT_DIR)/bin/pvrhwperf:$(TARGET_COPY_OUT_VENDOR)/bin/pvrhwperf \
        $(GPU_COPY_ROOT_DIR)/bin/pvrlogdump:$(TARGET_COPY_OUT_VENDOR)/bin/pvrlogdump \
        $(GPU_COPY_ROOT_DIR)/bin/pvrlogsplit:$(TARGET_COPY_OUT_VENDOR)/bin/pvrlogsplit \
        $(GPU_COPY_ROOT_DIR)/bin/android.hardware.graphics.allocator@4.0-service:$(TARGET_COPY_OUT_VENDOR)/bin/hw/android.hardware.graphics.allocator@4.0-service \
	$(GPU_COPY_ROOT_DIR)/lib/libusc.so:$(TARGET_COPY_OUT_VENDOR)/lib/libusc.so \
        $(GPU_COPY_ROOT_DIR)/lib/libglslcompiler.so:$(TARGET_COPY_OUT_VENDOR)/lib/libglslcompiler.so \
        $(GPU_COPY_ROOT_DIR)/lib/libIMGegl.so:$(TARGET_COPY_OUT_VENDOR)/lib/libIMGegl.so \
        $(GPU_COPY_ROOT_DIR)/lib/libPVRScopeServices.so:$(TARGET_COPY_OUT_VENDOR)/lib/libPVRScopeServices.so \
        $(GPU_COPY_ROOT_DIR)/lib/libpvrANDROID_WSEGL.so:$(TARGET_COPY_OUT_VENDOR)/lib/libpvrANDROID_WSEGL.so \
        $(GPU_COPY_ROOT_DIR)/lib/libsrv_um.so:$(TARGET_COPY_OUT_VENDOR)/lib/libsrv_um.so \
        $(GPU_COPY_ROOT_DIR)/lib/gralloc.sunxi.so:$(TARGET_COPY_OUT_VENDOR)/lib/hw/gralloc.$(TARGET_BOARD_PLATFORM).so \
	$(GPU_COPY_ROOT_DIR)/lib/android.hardware.graphics.mapper@4.0-impl.so:$(TARGET_COPY_OUT_VENDOR)/lib/hw/android.hardware.graphics.mapper@4.0-impl.so
    ifeq ($(GPU_ARCH),img-sgx)
        PRODUCT_GPU_FILES += \
            $(GPU_COPY_ROOT_DIR)/lib/libpvr2d.so:$(TARGET_COPY_OUT_VENDOR)/lib/libpvr2d.so \
            $(GPU_COPY_ROOT_DIR)/lib/libsrv_init.so:$(TARGET_COPY_OUT_VENDOR)/lib/libsrv_init.so \
            $(GPU_COPY_ROOT_DIR)/lib/libEGL_POWERVR_SGX544_115.so:$(TARGET_COPY_OUT_VENDOR)/lib/egl/libEGL_POWERVR_SGX544_115.so \
            $(GPU_COPY_ROOT_DIR)/lib/libGLESv1_CM_POWERVR_SGX544_115.so:$(TARGET_COPY_OUT_VENDOR)/lib/egl/libGLESv1_CM_POWERVR_SGX544_115.so \
            $(GPU_COPY_ROOT_DIR)/lib/libGLESv2_POWERVR_SGX544_115.so:$(TARGET_COPY_OUT_VENDOR)/lib/egl/libGLESv2_POWERVR_SGX544_115.so
    else ifeq ($(GPU_ARCH),img-rgx)
        PRODUCT_GPU_FILES += \
	    $(GPU_COPY_ROOT_DIR)/lib/libufwriter.so:$(TARGET_COPY_OUT_VENDOR)/lib/libufwriter.so \
            $(GPU_COPY_ROOT_DIR)/lib/libEGL_POWERVR_ROGUE.so:$(TARGET_COPY_OUT_VENDOR)/lib/egl/libEGL_POWERVR_ROGUE.so \
            $(GPU_COPY_ROOT_DIR)/lib/libGLESv1_CM_POWERVR_ROGUE.so:$(TARGET_COPY_OUT_VENDOR)/lib/egl/libGLESv1_CM_POWERVR_ROGUE.so \
            $(GPU_COPY_ROOT_DIR)/lib/libGLESv2_POWERVR_ROGUE.so:$(TARGET_COPY_OUT_VENDOR)/lib/egl/libGLESv2_POWERVR_ROGUE.so \
            $(GPU_COPY_ROOT_DIR)/lib/memtrack_aidl.sunxi.so:$(TARGET_COPY_OUT_VENDOR)/lib/hw/memtrack_aidl.sunxi.so \
	    $(GPU_COPY_ROOT_DIR)/lib/vulkan.sunxi.so:$(TARGET_COPY_OUT_VENDOR)/lib/hw/vulkan.$(TARGET_BOARD_PLATFORM).so \
	    $(GPU_COPY_ROOT_DIR)/lib/libPVROCL.so:$(TARGET_COPY_OUT_VENDOR)/lib/libPVROCL.so \
            $(GPU_COPY_ROOT_DIR)/firmware/rgx.fw.22.102.54.38:$(TARGET_COPY_OUT_VENDOR)/etc/firmware/rgx.fw.22.102.54.38 \
            $(GPU_COPY_ROOT_DIR)/firmware/rgx.sh.22.102.54.38:$(TARGET_COPY_OUT_VENDOR)/etc/firmware/rgx.sh.22.102.54.38
        ifeq ($(TARGET_ARCH),arm64)
        endif
    endif
endif

# configure gpu opengles version:
# 131072=opengles 2.0
# 196608=opengles 3.0
# 196609=opengles 3.1
# 196610=opengles 3.2
ifeq ($(GLES_VERSION),2.0)
    PRODUCT_GPU_PROPERTIES += ro.opengles.version=131072
else ifeq ($(GLES_VERSION),3.0)
    PRODUCT_GPU_PROPERTIES += ro.opengles.version=196608
else ifeq ($(GLES_VERSION),3.1)
    PRODUCT_GPU_PROPERTIES += ro.opengles.version=196609
else ifeq ($(GLES_VERSION),3.2)
    PRODUCT_GPU_PROPERTIES += ro.opengles.version=196610
endif

# AEP (Android Extension Pack)
ifeq ($(SUPPORT_AEP),1)
    PRODUCT_GPU_FILES += frameworks/native/data/etc/android.hardware.opengles.aep.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.opengles.aep.xml
endif

ifeq ($(VULKAN_VERSION),1.1)
     PRODUCT_GPU_FILES += frameworks/native/data/etc/android.hardware.vulkan.compute-0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.vulkan.compute.xml
     PRODUCT_GPU_FILES += frameworks/native/data/etc/android.hardware.vulkan.version-1_1.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.vulkan.version.xml
     PRODUCT_GPU_FILES += frameworks/native/data/etc/android.hardware.vulkan.level-0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.vulkan.level.xml
     PRODUCT_GPU_FILES += frameworks/native/data/etc/android.software.vulkan.deqp.level-2021-03-01.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.vulkan.deqp.level.xml
endif


PRODUCT_GPU_FILES += frameworks/native/data/etc/android.software.opengles.deqp.level-2021-03-01.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.opengles.deqp.level.xml
