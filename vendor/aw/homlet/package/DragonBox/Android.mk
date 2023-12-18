LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_JAVA_LIBRARIES := android-support-v13
LOCAL_SRC_FILES := $(call all-java-files-under, src) \

#for A20
ifeq (A20, $(SW_CHIP_PLATFORM))
    DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/A20)
else ifeq (A10, $(SW_CHIP_PLATFORM))
    DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/A10)
else ifeq (A31, $(SW_CHIP_PLATFORM))
    DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/A31)
else ifeq (A80, $(SW_CHIP_PLATFORM))
    DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/A80)
else ifeq (H8, $(SW_CHIP_PLATFORM))
    DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/H8)
else ifeq (H64, $(SW_CHIP_PLATFORM))
    DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/H64)
else ifeq (H5, $(SW_CHIP_PLATFORM))
    ifeq (,$(filter-out 4.4%,$(PLATFORM_VERSION)))
        DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/H5-Android4.4)
    else ifeq (,$(filter-out 7%,$(PLATFORM_VERSION)))
        DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/H5-AndroidN)
        LOCAL_JAVA_LIBRARIES:= org.apache.http.legacy.boot
    endif
else ifeq (petrel, $(TARGET_BOARD_PLATFORM))
    ifeq (,$(filter-out 4.4%,$(PLATFORM_VERSION)))
        ifeq (,$(filter-out yst-petrel_ai_p1,$(TARGET_BUSINESS_PRODUCT)))
            DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/H6-Android4.4-yst)
        else
            DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/H6-Android4.4)
        endif
    else ifeq (,$(filter-out 7%,$(PLATFORM_VERSION)))
        DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/H6-AndroidN)
        LOCAL_JAVA_LIBRARIES:= org.apache.http.legacy.boot
    else ifeq (,$(filter-out 9%,$(PLATFORM_VERSION)))
        DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/H6-AndroidP)
        LOCAL_JAVA_LIBRARIES:= org.apache.http.legacy.boot
        DRAGONBOX_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static libsystemmix
    endif
else ifeq (dolphin, $(TARGET_BOARD_PLATFORM))
    ifeq (,$(filter-out 4.4%,$(PLATFORM_VERSION)))
        DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/H3-Android4.4)
    else ifeq (,$(filter-out 7%,$(PLATFORM_VERSION)))
        DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/H3-AndroidN)
        LOCAL_JAVA_LIBRARIES:= org.apache.http.legacy.boot
        DRAGONBOX_STATIC_JAVA_LIBRARIES := utdid
    else ifeq (,$(filter-out 10%,$(PLATFORM_VERSION)))
        DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/H3-AndroidQ)
        LOCAL_JAVA_LIBRARIES:= org.apache.http.legacy
        DRAGONBOX_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static libsystemmix
    endif
else ifeq (cupid, $(TARGET_BOARD_PLATFORM))
    ifeq (,$(filter-out 10%,$(PLATFORM_VERSION)))
        DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/H616-AndroidQ)
        LOCAL_JAVA_LIBRARIES:= org.apache.http.legacy
        DRAGONBOX_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static libsystemmix
    endif
else ifeq (eros, $(TARGET_BOARD_PLATFORM))
    ifeq (,$(filter-out 10%,$(PLATFORM_VERSION)))
        DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/H313-AndroidQ)
        LOCAL_JAVA_LIBRARIES:= org.apache.http.legacy
        DRAGONBOX_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static libsystemmix
    endif
else ifeq (ares, $(TARGET_BOARD_PLATFORM))
    ifeq (,$(filter-out 11%,$(PLATFORM_VERSION)))
        DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/TV303-AndroidR)
        LOCAL_JAVA_LIBRARIES:= org.apache.http.legacy
        DRAGONBOX_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static libsystemmix softwinner.display.output
    endif
else ifeq (titan, $(TARGET_BOARD_PLATFORM))
    ifeq (,$(filter-out 10%,$(PLATFORM_VERSION)))
        DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/TITAN-AndroidQ)
        LOCAL_JAVA_LIBRARIES:= org.apache.http.legacy
        DRAGONBOX_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static libsystemmix
    endif
else ifeq (apollo, $(TARGET_BOARD_PLATFORM))
    ifeq (,$(filter-out 10%,$(PLATFORM_VERSION)))
        DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/APOLLO-AndroidQ)
        LOCAL_JAVA_LIBRARIES:= org.apache.http.legacy
        DRAGONBOX_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static libsystemmix
    else ifeq (,$(filter-out 12%,$(PLATFORM_VERSION)))
        LOCAL_ENFORCE_USES_LIBRARIES := false
        LOCAL_DEX_PREOPT := false
        DRAGONBOX_SRC_FILES := $(call all-java-files-under, platform/H618-AndroidS)
        LOCAL_JAVA_LIBRARIES:= org.apache.http.legacy
        DRAGONBOX_STATIC_JAVA_LIBRARIES := services.core softwinner.audio.static libsystemmix softwinner.display.output
    endif
endif

ifneq ($(DRAGONBOX_SRC_FILES),)
    LOCAL_SRC_FILES += $(DRAGONBOX_SRC_FILES)
    LOCAL_STATIC_JAVA_LIBRARIES += $(DRAGONBOX_STATIC_JAVA_LIBRARIES)
else
    # universal implement for un-configured platform
    $(warning DragonBox: No platform configured, use universal implement!)
    LOCAL_SRC_FILES += $(call all-java-files-under, platform/Universal)
    LOCAL_JAVA_LIBRARIES:= org.apache.http.legacy
    LOCAL_STATIC_JAVA_LIBRARIES += services.core softwinner.audio.static libsystemmix
endif


LOCAL_PACKAGE_NAME := DragonBox

LOCAL_REQUIRED_MODULES :=liballwinnertech_read_private_dragonbox

# We mark this out until Mtp and MediaMetadataRetriever is unhidden.
#LOCAL_SDK_VERSION := current
LOCAL_CERTIFICATE := platform

LOCAL_PROGUARD_FLAG_FILES := proguard.cfg
#LOCAL_PROGUARD_ENABLED := full obfuscation
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_STATIC_JAVA_LIBRARIES += mysql

LOCAL_PRIVATE_PLATFORM_APIS := true

include $(BUILD_PACKAGE)

include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := mysql:mysql-connector-java-5.1.48-bin.jar
include $(BUILD_MULTI_PREBUILT)

#for H3
ifeq (dolphin, $(TARGET_BOARD_PLATFORM))
    ifneq (,$(filter 7%,$(PLATFORM_VERSION)))
        include $(CLEAR_VARS)
        LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := utdid:utdid4all-2.0.5-ali-h3n.jar
        include $(BUILD_MULTI_PREBUILT)
    endif
endif

# Use the following include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))
