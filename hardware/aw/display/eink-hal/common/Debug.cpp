
#include "Debug.h"

/* static */ Debug Debug::gDebugInstance;

Debug::Debug()
    : mDebugFlags(0x01), mVerboseLog(false)
{ }

void Debug::setDebugTag(int32_t mask, bool verbose)
{
    mDebugFlags = mask | 0x01;
    mVerboseLog = verbose;
}

void Debug::error(const char *format, ...)
{
    va_list list;
    va_start(list, format);
    __android_log_vprint(ANDROID_LOG_ERROR, LOG_TAG, format, list);
    va_end(list);
}

void Debug::warning(const char *format, ...)
{
    va_list list;
    va_start(list, format);
    __android_log_vprint(ANDROID_LOG_WARN, LOG_TAG, format, list);
    va_end(list);
}

void Debug::info(const char *format, ...)
{
    va_list list;
    va_start(list, format);
    __android_log_vprint(ANDROID_LOG_INFO, LOG_TAG, format, list);
    va_end(list);
}

void Debug::debug(const char *format, ...)
{
    va_list list;
    va_start(list, format);
    __android_log_vprint(ANDROID_LOG_DEBUG, LOG_TAG, format, list);
    va_end(list);
}

void Debug::verbose(const char *format, ...)
{
    va_list list;
    va_start(list, format);
    __android_log_vprint(ANDROID_LOG_VERBOSE, LOG_TAG, format, list);
    va_end(list);
}

void Debug::beginTrace(const char *function_name, const char *custom_string)
{
    if (CC_UNLIKELY(atrace_is_tag_enabled(ATRACE_TAG))) {
        char name[PATH_MAX] = {0};
        snprintf(name, sizeof(name), "%s::%s", function_name, custom_string);
        atrace_begin(ATRACE_TAG, name);
    }
}

void Debug::endTrace() {
    if (CC_UNLIKELY(atrace_is_tag_enabled(ATRACE_TAG)))
        atrace_end(ATRACE_TAG);
}

void Debug::traceInt(const char *name, int32_t value) {
    if (CC_UNLIKELY(atrace_is_tag_enabled(ATRACE_TAG)))
        atrace_int(ATRACE_TAG, name, value);
}

// dump buffer request handle
DumpBufferRequest DumpBufferRequest::sInstance;

DumpBufferRequest::DumpBufferRequest()
    : mTarget(0), mRequestCount(0), mLayerMask(0) { }

void DumpBufferRequest::updateRequest(int type, int count, int layerMask)
{
    mTarget = type;
    mRequestCount = count;
    mLayerMask = layerMask;
}

bool DumpBufferRequest::requestDump(int type, int layerMask)
{
    if (type != mTarget || mRequestCount <= 0) {
        return false;
    }

    if (type == eHardwareRotateBuffer) {
        mRequestCount--;
        return true;
    } else {
        if ((layerMask & mLayerMask) != 0) {
            mRequestCount--;
            return true;
        }
    }
    return false;
}

