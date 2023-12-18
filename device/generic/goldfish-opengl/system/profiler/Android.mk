LOCAL_PATH := $(call my-dir)

### profiler ###########################################
$(call emugl-begin-shared-library,libGoldfishProfiler)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 30 && echo isApi30OrHigher),isApi30OrHigher)
    LOCAL_CFLAGS += -DLOG_TAG=\"emuglProfiler\"

    LOCAL_SHARED_LIBRARIES := liblog \
        libbase

    LOCAL_SRC_FILES := \
        profiler.cpp \
        perfetto.cpp

else
    LOCAL_SRC_FILES := \
        profiler_stub.cpp

endif

$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

$(call emugl-end-module)