
#include <fcntl.h>
#include <string.h>
#include <android/log.h>
#include <jni.h>

#include <sys/types.h>
#include <fcntl.h>
#include "cutils/properties.h"
#include "JNIHelp.h"
#include <android_runtime/AndroidRuntime.h>
#include "HdmiCtsUtils.h"
#include "edid/edid.h"

JNIEnv* jniEnv;

#define DEBUG

#define HDMICAPABILITY_OBJECT_PATH \
    "com/softwinner/utils/HdmiCapability"
#define HDMIOUTPUTCONFIG_OBJECT_PATH \
    "com/softwinner/utils/HdmiOutputConfig"

inline void setIntToJavaObject(JNIEnv *env,
        jclass toclazz, jobject toobject, const char *name, int value)
{
    int result = 0;
    jfieldID fieldid = env->GetFieldID(toclazz, name, "I");
    if (fieldid == NULL) {
        ALOGE("GetFieldID: can't find '%s'", name);
        goto errout__;
    }
    env->SetIntField(toobject, fieldid, value);
    return;
errout__:
    ALOGE("setIntToJavaObject: error at '%s'", name);
}

#define __SET_INT_VALUE(_env, _toclass, _toobj, _fptr, _name) \
do {                                                          \
        setIntToJavaObject(_env,                              \
            _toclass, _toobj, #_name, _fptr->_name);          \
} while (0)

static jobject generateHdimCapabilityObject(JNIEnv *env, struct HdmiCapability *capability)
{
    jclass clazz = env->FindClass(HDMICAPABILITY_OBJECT_PATH);
    jobject outobj = env->AllocObject(clazz);

    if (!outobj) {
        env->DeleteLocalRef(clazz);
        ALOGE("generateHdimCapabilityObject: error on AllocObject");
        return NULL;
    }

    __SET_INT_VALUE(env, clazz, outobj, capability, deviceType);
    __SET_INT_VALUE(env, clazz, outobj, capability, supportedColorSpace);
    __SET_INT_VALUE(env, clazz, outobj, capability, supportedColorDepth);
    __SET_INT_VALUE(env, clazz, outobj, capability, supportedColorimetry);
    __SET_INT_VALUE(env, clazz, outobj, capability, supportedValueRange);
    __SET_INT_VALUE(env, clazz, outobj, capability, supportedScanMode);
    __SET_INT_VALUE(env, clazz, outobj, capability, supportedHdrMode);
    __SET_INT_VALUE(env, clazz, outobj, capability, supportedAspectRatio);

    env->DeleteLocalRef(clazz);
    return outobj;
}

static jobject generateHdimOutputConfigObject(JNIEnv *env, struct HdmiOutputConfig *config)
{
    jclass clazz = env->FindClass(HDMIOUTPUTCONFIG_OBJECT_PATH);
    jobject outobj = env->AllocObject(clazz);

    if (!outobj) {
        env->DeleteLocalRef(clazz);
        ALOGE("generateHdimOutputConfigObject: error on AllocObject");
        return NULL;
    }

    __SET_INT_VALUE(env, clazz, outobj, config, deviceType);
    __SET_INT_VALUE(env, clazz, outobj, config, hdmiMode);
    __SET_INT_VALUE(env, clazz, outobj, config, colorSpace);
    __SET_INT_VALUE(env, clazz, outobj, config, colorDepth);
    __SET_INT_VALUE(env, clazz, outobj, config, colorimetry);
    __SET_INT_VALUE(env, clazz, outobj, config, valueRange);
    __SET_INT_VALUE(env, clazz, outobj, config, scanMode);
    __SET_INT_VALUE(env, clazz, outobj, config, hdrMode);
    __SET_INT_VALUE(env, clazz, outobj, config, aspectRatio);

    env->DeleteLocalRef(clazz);
    return outobj;
}

static jobject com_softwinner_utils_HdmiCtsUtils_getHdmiCapability(
        JNIEnv *env, jclass clazz)
{
    struct HdmiCapability capability;
    struct edid_info edidinfo;
    read_edid_info(&edidinfo);

    memset(&capability, 0, sizeof(capability));
    HdmiCapabilityFromEdidInfo(&capability, &edidinfo);
    return generateHdimCapabilityObject(env, &capability);
}

static void update_mode_arrary(jint *buf, jint *length, int mode) {
    size_t count = *length;
    for (size_t i = 0; i < count; i++) {
        if (buf[i] == mode)
            return;
    }
    buf[count] = mode;
    *length = count + 1;
}

static jintArray com_softwinner_utils_HdmiCtsUtils_getHdmiSupportMode(
        JNIEnv *env, jclass clazz)
{
    jint buff[64];

    struct edid_info edidinfo;
    read_edid_info(&edidinfo);

    jsize length = 0;
    for (int i = 0; i < edidinfo.total_ycbcr420_vic; i++) {
        int mode = edidinfo.ycbcr420_vic[i].sunxi_disp_mode;
        if (mode != 0xff) {
            update_mode_arrary(buff, &length, mode);
        }
    }
    for (int i = 0; i < edidinfo.total_vic; i++) {
        int mode = edidinfo.supported_vic[i].sunxi_disp_mode;
        if (mode != 0xff) {
            update_mode_arrary(buff, &length, mode);
        }
    }

    if (edidinfo.present_3d) {
        buff[length] = 0x17; /* DISP_TV_MOD_1080P_24HZ_3D_FP */
        ++length;
    }

    for (int i = 0; i < length; i++)
        ALOGE("mode %d", buff[i]);

    jintArray outarray = (jintArray)env->NewGlobalRef(env->NewIntArray(length));
    if (!outarray) {
        ALOGE("error on NewIntArray");
        return NULL;
    }

    env->SetIntArrayRegion(outarray, 0, length, buff);
//  env->DeleteLocalRef(outarray);
    return outarray;
}

inline int getIntFromJavaObject(JNIEnv *env, jobject from, const char *name)
{
    int result = 0;
    jclass clsref = env->GetObjectClass(from);

    jfieldID fieldid = env->GetFieldID(clsref, name, "I");
    if (fieldid == NULL) {
        ALOGE("GetFieldID: can't find '%s'", name);
        goto errout__;
    }
    result = env->GetIntField(from, fieldid);
errout__:
    env->DeleteLocalRef(clsref);
    return result;
}

#define __GET_INT_VALUE(_env, _from, _outpter, _fieldname)  \
do {                                                        \
    _outpter->_fieldname =                                  \
        getIntFromJavaObject(_env, _from, #_fieldname);     \
} while (0)

static void getHdmiOutputConfigFromJavaObject(
        JNIEnv *env, jobject from, struct HdmiOutputConfig *out)
{
    __GET_INT_VALUE(env, from, out, deviceType);
    __GET_INT_VALUE(env, from, out, hdmiMode);
    __GET_INT_VALUE(env, from, out, colorSpace);
    __GET_INT_VALUE(env, from, out, colorDepth);
    __GET_INT_VALUE(env, from, out, colorimetry);
    __GET_INT_VALUE(env, from, out, valueRange);
    __GET_INT_VALUE(env, from, out, scanMode);
    __GET_INT_VALUE(env, from, out, hdrMode);
    __GET_INT_VALUE(env, from, out, aspectRatio);
#ifdef DEBUG
    ALOGD("HdmiOutputConfig from java object:");
    ALOGD("   deviceType: %d", out->deviceType);
    ALOGD("     hdmiMode: %d", out->hdmiMode);
    ALOGD("   colorSpace: %d", out->colorSpace);
    ALOGD("   colorDepth: %d", out->colorDepth);
    ALOGD("  colorimetry: %d", out->colorimetry);
    ALOGD("   valueRange: %d", out->valueRange);
    ALOGD("     scanMode: %d", out->scanMode);
    ALOGD("      hdrMode: %d", out->hdrMode);
    ALOGD("  aspectRatio: %d", out->aspectRatio);
#endif
}

static jint com_softwinner_utils_HdmiCtsUtils_setHdmiOutputConfig(
        JNIEnv *env, jclass clazz, jobject obj)
{
    struct HdmiOutputConfig config;
    getHdmiOutputConfigFromJavaObject(env, obj, &config);
    displayHalSetOutputConfig(&config);
    return 0;
}

static jobject com_softwinner_utils_HdmiCtsUtils_getHdmiOutputConfig(
        JNIEnv *env, jclass clazz)
{
    struct HdmiOutputConfig config;
    displayHalGetOutputConfig(&config);
    return generateHdimOutputConfigObject(env, &config);
}

static jobject com_softwinner_utils_HdmiCtsUtils_getConstraintByHdmiMode(
        JNIEnv *env, jclass clazz, jint mode)
{
    struct HdmiCapability capability;
    struct edid_info edidinfo;
    int hdmimode = mode;
    read_edid_info(&edidinfo);
    displayHalGetConstraintByHdmiMode(mode, &edidinfo, &capability);
    return generateHdimCapabilityObject(env, &capability);
}

#define HDMI_HDCP_ENABLE_PATH "/sys/class/hdmi/hdmi/attr/hdcp_enable"
static jint com_softwinner_utils_HdmiCtsUtils_getHdcpState(
        JNIEnv *env, jclass clazz)
{
    char buf[64];
    int count = 0;
    int retval = 0;

    int fd = open(HDMI_HDCP_ENABLE_PATH, O_RDONLY);
    if (fd < 0) {
        ALOGE("Can't open %s, errno = %d\n", HDMI_HDCP_ENABLE_PATH, errno);
        return -1;
    }
    count = read(fd, buf, 64);
    close(fd);

    if (count > 0) {
        retval = strtoul(buf, 0, 0);
        ALOGE("hdcp_enable = %s %d\n", buf, retval);
    }
    return retval ? 1 : 0;
}

static jint com_softwinner_utils_HdmiCtsUtils_setHdcpState(
        JNIEnv *env, jclass clazz, jint enable)
{
    char buf[64];
    sprintf(buf, "%s", enable ? "1" : "0");
    property_set("hdmi_cts.hdcp_enable", buf);
    ALOGE("set hdcp enable %s", buf);
    return -1;
}

#define HDMI_HPD_MASK_PATH "/sys/class/hdmi/hdmi/attr/hpd_mask"
static jint com_softwinner_utils_HdmiCtsUtils_getHotplugReportEnabled(
        JNIEnv *env, jclass clazz)
{
    char buf[64];
    int count = 0;
    int retval = 0;

    int fd = open(HDMI_HPD_MASK_PATH, O_RDONLY);
    if (fd < 0) {
        ALOGE("Can't open %s, errno = %d\n", HDMI_HPD_MASK_PATH, errno);
        return -1;
    }
    count = read(fd, buf, 64);
    close(fd);

    if (count > 0) {
        retval = strtoul(buf, 0, 0);
        ALOGE("hpd_mask = %s %d\n", buf, retval);
    }
    return (retval == 0x1000) ? 0 : 1;
}

static jint com_softwinner_utils_HdmiCtsUtils_setHotplugReportEnabled(
        JNIEnv *env, jclass clazz, jint enable)
{
#if 0
    char buf[64];
    int len;

    int fd = open(HDMI_HPD_MASK_PATH, O_WRONLY);
    if (fd < 0) {
        ALOGE("Can't open %s, errno = %d\n", HDMI_HPD_MASK_PATH, errno);
        return -1;
    }
    len = sprintf(buf, "%s", enable ? "0" : "0x1000");
    write(fd, buf, len);
    close(fd);
#else
    char buf[64];
    sprintf(buf, "%s", enable ? "0" : "0x1000");
    property_set("hdmi_cts.hpdreport", buf);
#endif
    return 0;
}

/*
 * cec_local_standby_mode = 0 --> send cec standby msg on suspend
 * cec_local_standby_mode = 1 --> NOT send cec standby msg on suspend
 */
#define HDMI_CEC_STANDBY_PATH "/sys/class/hdmi/hdmi/attr/cec_local_standby_mode"
static jint com_softwinner_utils_HdmiCtsUtils_getCecStandbyEnabled(
        JNIEnv *env, jclass clazz)
{
    char buf[64];
    int count = 0;
    int retval = 0;

    int fd = open(HDMI_CEC_STANDBY_PATH, O_RDONLY);
    if (fd < 0) {
        ALOGE("Can't open %s, errno = %d\n", HDMI_CEC_STANDBY_PATH, errno);
        return -1;
    }
    count = read(fd, buf, 64);
    close(fd);

    if (count > 0) {
        retval = strtoul(buf, 0, 0);
        ALOGE("cec standby = %s %d\n", buf, retval);
    }
    return (retval == 0) ? 1 : 0;
}

static jint com_softwinner_utils_HdmiCtsUtils_setCecStandbyEnabled(
        JNIEnv *env, jclass clazz, jint enable)
{
    char buf[64];
    sprintf(buf, "%s", enable ? "0" : "1");
    property_set("hdmi_cts.cecstandby", buf);
    return 0;
}

static jint com_softwinner_utils_HdmiCtsUtils_getOutputType(
        JNIEnv *env, jclass clazz)
{
    //TODO:
    return 4;
}


static const JNINativeMethod g_methods[] = {
    { "getHdmiCapability",
            "()Lcom/softwinner/utils/HdmiCapability;",
            (void *)com_softwinner_utils_HdmiCtsUtils_getHdmiCapability
    },
    { "getHdmiSupportMode",
            "()[I",
            (void *)com_softwinner_utils_HdmiCtsUtils_getHdmiSupportMode
    },
    { "setHdmiOutputConfig",
            "(Lcom/softwinner/utils/HdmiOutputConfig;)I",
            (void *)com_softwinner_utils_HdmiCtsUtils_setHdmiOutputConfig
    },
    { "getHdmiOutputConfig",
            "()Lcom/softwinner/utils/HdmiOutputConfig;",
            (void *)com_softwinner_utils_HdmiCtsUtils_getHdmiOutputConfig
    },
    { "getConstraintByHdmiMode",
            "(I)Lcom/softwinner/utils/HdmiCapability;",
            (void *)com_softwinner_utils_HdmiCtsUtils_getConstraintByHdmiMode
    },
    { "getHdcpState",
            "()I",
            (void *)com_softwinner_utils_HdmiCtsUtils_getHdcpState
    },
    { "setHdcpState",
            "(I)I",
            (void *)com_softwinner_utils_HdmiCtsUtils_setHdcpState
    },
    { "getHotplugReportEnabled",
            "()I",
            (void *)com_softwinner_utils_HdmiCtsUtils_getHotplugReportEnabled
    },
    { "setHotplugReportEnabled",
            "(I)I",
            (void *)com_softwinner_utils_HdmiCtsUtils_setHotplugReportEnabled
    },
    { "getCecStandbyEnabled",
            "()I",
            (void *)com_softwinner_utils_HdmiCtsUtils_getCecStandbyEnabled
    },
    { "setCecStandbyEnabled",
            "(I)I",
            (void *)com_softwinner_utils_HdmiCtsUtils_setCecStandbyEnabled
    },
    { "getOutputType",
            "()I",
            (void *)com_softwinner_utils_HdmiCtsUtils_getOutputType
    },
};

static int register_com_sample_hdmicts_utils_SampleUtils(JNIEnv* env)
{
    jclass clazz;
    clazz = env->FindClass("com/softwinner/utils/HdmiCtsUtils");
    if (clazz == NULL)
        return JNI_FALSE;
    if (env->RegisterNatives(clazz, g_methods, NELEM(g_methods)) < 0)
        return JNI_FALSE;
    return JNI_TRUE;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
     JNIEnv* env = NULL;

     jint Ret = -1;
     if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
         goto bail;
     if (!register_com_sample_hdmicts_utils_SampleUtils(env))
         goto bail;

     Ret = JNI_VERSION_1_4;
 bail:
     return Ret;
}
