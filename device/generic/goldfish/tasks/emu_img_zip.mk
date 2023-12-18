# Rules to generate a zip file that contains google emulator images
# and other files for distribution

ifeq ($(filter $(TARGET_PRODUCT), qemu_trusty_arm64),)
ifeq ($(filter $(MAKECMDGOALS), sdk win_sdk sdk_repo goog_emu_imgs),)
emulator_img_source_prop := $(TARGET_OUT_INTERMEDIATES)/source.properties
$(emulator_img_source_prop): $(PRODUCT_SDK_ADDON_SYS_IMG_SOURCE_PROP)
	@echo Generate $@
	$(hide) mkdir -p $(dir $@)
	$(hide) sed \
		-e 's/$${PLATFORM_VERSION}/$(PLATFORM_VERSION)/' \
		-e 's/$${PLATFORM_SDK_VERSION}/$(PLATFORM_SDK_VERSION)/' \
		-e 's/$${PLATFORM_VERSION_CODENAME}/$(subst REL,,$(PLATFORM_VERSION_CODENAME))/' \
		-e 's/$${TARGET_ARCH}/$(TARGET_ARCH)/' \
		-e 's/$${TARGET_CPU_ABI}/$(TARGET_CPU_ABI)/' \
		-e 's/$${SYSTEM_IMAGE_TAG_ID}/$(SYSTEM_IMAGE_TAG_ID)/' \
		-e 's/$${SYSTEM_IMAGE_TAG_DISPLAY}/$(SYSTEM_IMAGE_TAG_DISPLAY)/' \
		$< > $@ && sed -i -e '/^AndroidVersion.CodeName=\s*$$/d' $@

INTERNAL_EMULATOR_PACKAGE_FILES := \
        $(target_notice_file_txt) \
        $(emulator_img_source_prop) \
        $(PRODUCT_OUT)/system/build.prop \

ifneq ($(filter $(TARGET_PRODUCT), sdk_goog3_x86 sdk_goog3_x86_64 sdk_goog3_x86_arm),)
    INTERNAL_EMULATOR_PACKAGE_FILES += \
        $(HOST_OUT_EXECUTABLES)/dex2oats \
        $(HOST_OUT_EXECUTABLES)/dex2oatds
endif

ifeq ($(BUILD_QEMU_IMAGES),true)
ifeq ($(BOARD_AVB_ENABLE),true)
INTERNAL_EMULATOR_PACKAGE_FILES += \
        $(PRODUCT_OUT)/VerifiedBootParams.textproto
endif
endif

INTERNAL_EMULATOR_PACKAGE_SOURCE := $(PRODUCT_OUT)/emulator

INSTALLED_QEMU_SYSTEMIMAGE := $(PRODUCT_OUT)/system-qemu.img
FINAL_INSTALLED_QEMU_SYSTEMIMAGE := $(INTERNAL_EMULATOR_PACKAGE_SOURCE)/$(TARGET_CPU_ABI)/system.img
$(eval $(call copy-one-file,$(INSTALLED_QEMU_SYSTEMIMAGE),$(FINAL_INSTALLED_QEMU_SYSTEMIMAGE)))

INSTALLED_QEMU_RAMDISKIMAGE := $(PRODUCT_OUT)/ramdisk-qemu.img
FINAL_INSTALLED_QEMU_RAMDISKIMAGE := $(INTERNAL_EMULATOR_PACKAGE_SOURCE)/$(TARGET_CPU_ABI)/ramdisk.img
$(eval $(call copy-one-file,$(INSTALLED_QEMU_RAMDISKIMAGE),$(FINAL_INSTALLED_QEMU_RAMDISKIMAGE)))

INSTALLED_QEMU_VENDORIMAGE := $(PRODUCT_OUT)/vendor-qemu.img
FINAL_INSTALLED_QEMU_VENDORIMAGE := $(INTERNAL_EMULATOR_PACKAGE_SOURCE)/$(TARGET_CPU_ABI)/vendor.img
$(eval $(call copy-one-file,$(INSTALLED_QEMU_VENDORIMAGE),$(FINAL_INSTALLED_QEMU_VENDORIMAGE)))


INTERNAL_EMULATOR_PACKAGE_FILES += device/generic/goldfish/data/etc/encryptionkey.img
INTERNAL_EMULATOR_PACKAGE_FILES += device/generic/goldfish/data/etc/userdata.img

INTERNAL_EMULATOR_FEATURE_DIR := .
ifneq ($(filter sdk_phone64_% sdk_gphone64_%, $(TARGET_PRODUCT)),)
INTERNAL_EMULATOR_FEATURE_DIR := 64bit
endif

ifeq ($(TARGET_BUILD_VARIANT),user)
INTERNAL_EMULATOR_PACKAGE_FILES += device/generic/goldfish/data/etc/google/$(INTERNAL_EMULATOR_FEATURE_DIR)/user/advancedFeatures.ini
ifeq ($(TARGET_ARCH),arm64)
INTERNAL_EMULATOR_PACKAGE_FILES += device/generic/goldfish/data/etc/google/$(INTERNAL_EMULATOR_FEATURE_DIR)/user/arm64/advancedFeatures.ini
endif
else
INTERNAL_EMULATOR_PACKAGE_FILES += device/generic/goldfish/data/etc/google/$(INTERNAL_EMULATOR_FEATURE_DIR)/userdebug/advancedFeatures.ini
ifeq ($(TARGET_ARCH),arm64)
INTERNAL_EMULATOR_PACKAGE_FILES += device/generic/goldfish/data/etc/google/$(INTERNAL_EMULATOR_FEATURE_DIR)/userdebug/arm64/advancedFeatures.ini
endif
endif



name := sdk-repo-linux-system-images-$(FILE_NAME_TAG)


INTERNAL_EMULATOR_PACKAGE_TARGET := $(PRODUCT_OUT)/$(name).zip

ifeq ($(TARGET_ARCH), x86)
EMULATOR_KERNEL_ARCH := x86_64
EMULATOR_KERNEL_DIST_NAME := kernel-ranchu-64
else
ifeq ($(TARGET_ARCH), x86_64)
EMULATOR_KERNEL_ARCH := $(TARGET_ARCH)
EMULATOR_KERNEL_DIST_NAME := kernel-ranchu
else
ifeq ($(TARGET_ARCH), arm64)
EMULATOR_KERNEL_ARCH := $(TARGET_ARCH)
EMULATOR_KERNEL_DIST_NAME := kernel-ranchu
else
ifeq ($(TARGET_ARCH), arm)
EMULATOR_KERNEL_ARCH := $(TARGET_ARCH)
EMULATOR_KERNEL_DIST_NAME := kernel-ranchu
EMULATOR_KERNEL_VERSION := 3.18
EMULATOR_KERNEL_FILE := prebuilts/qemu-kernel/$(EMULATOR_KERNEL_ARCH)/$(EMULATOR_KERNEL_VERSION)/kernel-qemu2
else
$(error unsupported arch: $(TARGET_ARCH))
endif # arm
endif # arm64
endif # x86_64
endif # x86

$(INTERNAL_EMULATOR_PACKAGE_TARGET): $(INTERNAL_EMULATOR_PACKAGE_FILES) $(FINAL_INSTALLED_QEMU_SYSTEMIMAGE) $(FINAL_INSTALLED_QEMU_RAMDISKIMAGE) $(FINAL_INSTALLED_QEMU_VENDORIMAGE) $(EMULATOR_KERNEL_FILE)
	@echo "Package: $@"
	$(hide) mkdir -p $(INTERNAL_EMULATOR_PACKAGE_SOURCE)/$(TARGET_CPU_ABI)
	$(hide) $(foreach f,$(INTERNAL_EMULATOR_PACKAGE_FILES), $(ACP) $(f) $(INTERNAL_EMULATOR_PACKAGE_SOURCE)/$(TARGET_CPU_ABI)/$(notdir $(f));)
	$(hide) ($(ACP) $(EMULATOR_KERNEL_FILE) $(INTERNAL_EMULATOR_PACKAGE_SOURCE)/$(TARGET_CPU_ABI)/${EMULATOR_KERNEL_DIST_NAME})
	$(hide) $(ACP) -r $(PRODUCT_OUT)/data $(INTERNAL_EMULATOR_PACKAGE_SOURCE)/$(TARGET_CPU_ABI)
	$(hide) $(SOONG_ZIP) -o $@ -C $(INTERNAL_EMULATOR_PACKAGE_SOURCE) -D $(INTERNAL_EMULATOR_PACKAGE_SOURCE)/$(TARGET_CPU_ABI)

.PHONY: emu_img_zip
emu_img_zip: $(INTERNAL_EMULATOR_PACKAGE_TARGET)

endif
endif
