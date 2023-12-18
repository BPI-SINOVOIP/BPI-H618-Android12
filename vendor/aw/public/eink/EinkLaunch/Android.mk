# don't use LOCAL_PATH for include mk will overwrite it
LOCAL_PATH_PRIVATE := $(call my-dir)

# not work, so manual include mk
#include $(call all-makefiles-under, $(LOCAL_PATH))

# build jni
$(info building jni ... )
#include $(LOCAL_PATH_PRIVATE)/app/src/main/jni/Android.mk

# build apk
$(info building apk ... )
include $(LOCAL_PATH_PRIVATE)/app/src/main/Android.mk
