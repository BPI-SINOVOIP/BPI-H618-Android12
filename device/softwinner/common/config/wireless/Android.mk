# Main work:
# 1. Use BOARD_WIRELESS_FILES to generate copy-package
# 2. Use BOARD_WIRELESS_PACKAGES as requested for wireless-package
# 3. Use BOARD_WIRELESS_PROPERTIES to generate property-package
# 3. Use BOARD_WIRELESS_SYSTEM_PROPERTIES to generate property-system-package
# 4. Add copy-package, BOARD_WIRELESS_PACKAGES, property-package & property-system-package to wireless-package

$(call define-copy-target,$(BOARD_WIRELESS_FILES))

LOCAL_PATH := device/softwinner/common/config/wireless

include $(CLEAR_VARS)
LOCAL_MODULE := wireless-package
LOCAL_MODULE_OWNER := google
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(BOARD_WIRELESS_PACKAGES)
LOCAL_REQUIRED_MODULES += $(call get-copy-target,$(BOARD_WIRELESS_FILES))
LOCAL_REQUIRED_MODULES += wireless-properity
LOCAL_REQUIRED_MODULES += wireless-system-properity
include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := wireless-properity
LOCAL_MODULE_CLASS:= ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/etc/init
LOCAL_MODULE_STEM := init.wireless.property.rc

GEN := $(local-generated-sources-dir)/init.wireless.property.rc
$(GEN): $(LOCAL_PATH)/prop_gen.sh
	@echo "Generator: $@"
	@$< "$(BOARD_WIRELESS_PROPERTIES)" $@

LOCAL_PREBUILT_MODULE_FILE := $(GEN)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := wireless-system-properity
LOCAL_MODULE_CLASS:= ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/init
LOCAL_MODULE_STEM := init.wireless.property.rc

GEN := $(local-generated-sources-dir)/init.wireless.property.rc
$(GEN): $(LOCAL_PATH)/prop_gen.sh
	@echo "Generator: $@"
	@$< "$(BOARD_WIRELESS_SYSTEM_PROPERTIES)" $@

LOCAL_PREBUILT_MODULE_FILE := $(GEN)
include $(BUILD_PREBUILT)
