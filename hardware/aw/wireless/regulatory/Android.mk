LOCAL_PATH := $(call my-dir)

include $(call all-subdir-makefiles)

include $(CLEAR_VARS)
LOCAL_MODULE := regulatory-package
LOCAL_MODULE_OWNER := allwinner
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := crda crda.uevent regdbdump
LOCAL_REQUIRED_MODULES += allwinner.key.pub.pem regulatory.bin

VERSION_NUM := $(shell printf "$(PLATFORM_VERSION)" | hexdump -n 1 -v -e '1/1 "%d"')
VERSION_NUM := $(shell [ $(VERSION_NUM) -ge 65 -a $(VERSION_NUM) -le 90 ] && expr $(VERSION_NUM) - 71 || echo $(PLATFORM_VERSION))
DB_FIRMWARE_SUPPORT := $(shell [ $(VERSION_NUM) -ge 11 ] && echo true || echo false)
ifeq ($(DB_FIRMWARE_SUPPORT),true)
LOCAL_REQUIRED_MODULES := regulatory.db regulatory.db.p7s
endif

LOCAL_REQUIRED_MODULES += dom2reg
include $(BUILD_PHONY_PACKAGE)
