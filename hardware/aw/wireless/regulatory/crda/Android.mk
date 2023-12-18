LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libreg
LOCAL_SRC_FILES := upstream/reglib.c upstream/keys-ssl.c
LOCAL_C_INCLUDES := external/openssl/include
LOCAL_CFLAGS := -DUSE_OPENSSL -DPUBKEY_DIR=\"/vendor/etc/crda\" -Wno-unused-variable
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcrypto libnl
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := regdbdump
LOCAL_SRC_FILES := upstream/regdbdump.c
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcrypto libnl
LOCAL_STATIC_LIBRARIES := libreg
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := crda
LOCAL_SRC_FILES := upstream/crda.c
LOCAL_C_INCLUDES := external/libnl/include
LOCAL_CFLAGS := -DCONFIG_LIBNL20
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcrypto libnl
LOCAL_STATIC_LIBRARIES := libreg
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := crda.uevent
LOCAL_SRC_FILES := crda-uevent.c
LOCAL_MODULE_CLASS := EXECUTABLES
GEN := $(local-generated-sources-dir)/libcrda.c
$(GEN): $(LOCAL_PATH)/upstream/crda.c
	@echo "Generator: $@"
	@cp $< $@
	@sed -i 's|main(|crda(|g' $@
	@sed -i '/regdb_paths\[\]/a "/vendor/etc/crda/regulatory.bin",' $@
LOCAL_GENERATED_SOURCES := $(GEN)
LOCAL_STATIC_LIBRARIES := libreg
LOCAL_SHARED_LIBRARIES := libcrypto libnl libcutils liblog
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CFLAGS := -DCONFIG_LIBNL20
LOCAL_C_INCLUDES := external/libnl/include
LOCAL_C_INCLUDES += system/core/libutils/include $(LOCAL_PATH)/upstream
LOCAL_INIT_RC := crda.uevent.rc
include $(BUILD_EXECUTABLE)
