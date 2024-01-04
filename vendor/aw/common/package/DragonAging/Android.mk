LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_JAVA_LIBRARIES := android-support-v13
LOCAL_STATIC_JAVA_LIBRARIES := libsysapi
LOCAL_SRC_FILES := $(call all-java-files-under, src) \

LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_PACKAGE_NAME := DragonAgingTV

# We mark this out until Mtp and MediaMetadataRetriever is unhidden.
#LOCAL_SDK_VERSION := current
LOCAL_CERTIFICATE := platform
LOCAL_STATIC_JAVA_LIBRARIES = android-support-v4

LOCAL_PROGUARD_FLAG_FILES := proguard.cfg

#for H6
ifeq (petrel, $(TARGET_BOARD_PLATFORM))
    ifeq (,$(filter-out 7%,$(PLATFORM_VERSION)))
        DRAGONAGING_SRC_FILES := $(call all-java-files-under, platform/H6-AndroidN)
    else ifeq (,$(filter-out 9%,$(PLATFORM_VERSION)))
        DRAGONAGING_SRC_FILES := $(call all-java-files-under, platform/H6-AndroidP)
        DRAGONAGING_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static
    endif
else ifeq (dolphin, $(TARGET_BOARD_PLATFORM))
    ifeq (,$(filter-out 7%,$(PLATFORM_VERSION)))
        DRAGONAGING_SRC_FILES := $(call all-java-files-under, platform/H3-AndroidN)
    else ifeq (,$(filter-out 10%,$(PLATFORM_VERSION)))
        DRAGONAGING_SRC_FILES := $(call all-java-files-under, platform/H3-AndroidQ)
        DRAGONAGING_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static
    endif
else ifeq (cupid, $(TARGET_BOARD_PLATFORM))
    ifeq (,$(filter-out 10%,$(PLATFORM_VERSION)))
        DRAGONAGING_SRC_FILES := $(call all-java-files-under, platform/H616-AndroidQ)
        DRAGONAGING_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static
    endif
else ifeq (eros, $(TARGET_BOARD_PLATFORM))
    ifeq (,$(filter-out 10%,$(PLATFORM_VERSION)))
        DRAGONAGING_SRC_FILES := $(call all-java-files-under, platform/H313-AndroidQ)
        DRAGONAGING_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static
    endif
else ifeq (titan, $(TARGET_BOARD_PLATFORM))
    ifeq (,$(filter-out 10%,$(PLATFORM_VERSION)))
        DRAGONAGING_SRC_FILES := $(call all-java-files-under, platform/TITAN-AndroidQ)
        DRAGONAGING_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static
    endif
else ifeq (apollo, $(TARGET_BOARD_PLATFORM))
    ifeq (,$(filter-out 10%,$(PLATFORM_VERSION)))
        DRAGONAGING_SRC_FILES := $(call all-java-files-under, platform/APOLLO-AndroidQ)
        DRAGONAGING_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static
    else ifeq (,$(filter-out 12%,$(PLATFORM_VERSION)))
        DRAGONAGING_SRC_FILES := $(call all-java-files-under, platform/APOLLO-AndroidS)
        DRAGONAGING_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static
    endif
endif

ifneq ($(DRAGONAGING_SRC_FILES),)
    LOCAL_SRC_FILES += $(DRAGONAGING_SRC_FILES)
    LOCAL_STATIC_JAVA_LIBRARIES += $(DRAGONAGING_STATIC_JAVA_LIBRARIES)
else
    # universal implement for un-configured platform
    $(warning DragonAging: No platform configured, use universal implement!)
    LOCAL_SRC_FILES += $(call all-java-files-under, platform/Universal)
    LOCAL_STATIC_JAVA_LIBRARIES += services.core softwinner.audio.static
endif


include $(BUILD_PACKAGE)

# Use the following include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))

include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := libsysapi:libs/sysapi.jar
include $(BUILD_MULTI_PREBUILT)
