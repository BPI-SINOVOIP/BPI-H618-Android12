
#ifndef _SUNXI_DEBUG_H_
#define _SUNXI_DEBUG_H_

#define ATRACE_TAG (ATRACE_TAG_GRAPHICS | ATRACE_TAG_HAL)

#include <bitset>

#include <android/log.h>
#include <cutils/trace.h>
#include <utils/Trace.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "sunxihwc"

// This enum represents different modules/logical unit tags
// that a log message may be associated with.
// Client may use this to filter messages for dynamic logging.
enum DebugTag {
    kTagNone,         // Debug log is not tagged. This type of logs should always be printed.
    kTagStrategy,     // Debug log is tagged for strategy decisions.
    kTagDriverConfig, // Debug log is tagged for driver config.
    kTagDeviceSwitch, // Debug log is tagged for device switching.
    kTagVSync,        // Debug log is tagged for VsyncThread.
    kTagScaler,       // Debug log is tagged for scaler detect.
    kTagFrameRate,    // Debug log is tagged for FrameRate output.
    kTagRotate,       // Debug log is tagged for hardware rotator debug.
    kTagPerf,         // Debug log is tagged for perf debug.
};

#define DLOG(method, format, ...) \
    Debug::get().method("%s: " format, __FUNCTION__, ##__VA_ARGS__)

#define DLOG_IF(tag, method, format, ...)        \
    if (CC_UNLIKELY(Debug::get().enable(tag))) { \
        DLOG(method, format, ##__VA_ARGS__);     \
    }

#define DEBUG_ENABLE(tag) Debug::get().enable(tag)

#define DLOGE_IF(tag, format, ...) DLOG_IF(tag, error,   format, ##__VA_ARGS__)
#define DLOGW_IF(tag, format, ...) DLOG_IF(tag, warning, format, ##__VA_ARGS__)
#define DLOGI_IF(tag, format, ...) DLOG_IF(tag, info,    format, ##__VA_ARGS__)
#define DLOGD_IF(tag, format, ...) DLOG_IF(tag, debug,   format, ##__VA_ARGS__)
#define DLOGV_IF(tag, format, ...) DLOG_IF(tag, verbose, format, ##__VA_ARGS__)

#define DLOGE(format, ...) DLOG(error,   format, ##__VA_ARGS__)
#define DLOGD(format, ...) DLOG(debug,   format, ##__VA_ARGS__)
#define DLOGW(format, ...) DLOG(warning, format, ##__VA_ARGS__)
#define DLOGI(format, ...) DLOG(info,    format, ##__VA_ARGS__)
#define DLOGV(format, ...) DLOG(verbose, format, ##__VA_ARGS__)

#define DTRACE_INT(name, value) Debug::get().traceInt(name, value)
#define DTRACE_BEGIN(custom_string) Debug::get().beginTrace(__FUNCTION__, custom_string)
#define DTRACE_END() Debug::get().endTrace()
#define DTRACE_SCOPED(custom_string) ScopeTracer scope_tracer(__FUNCTION__, custom_string)
#define DTRACE_FUNC() ScopeTracer scope_tracer(__FUNCTION__, "")

class Debug {
private:
    static Debug gDebugInstance;
    std::bitset<32> mDebugFlags;
    bool mVerboseLog;

    Debug();

public:
    static inline Debug& get() { return gDebugInstance; }

    inline bool enable(DebugTag tag) const { return mDebugFlags[tag]; }
    void setDebugTag(int32_t mask, bool verbose);

    void   error(const char *format, ...);
    void warning(const char *format, ...);
    void    info(const char *format, ...);
    void   debug(const char *format, ...);
    void verbose(const char *format, ...);

    void traceInt(const char* name, int32_t value);
    void beginTrace(const char *function_name, const char *custom_string);
    void endTrace();
};

class ScopeTracer {
public:
    ScopeTracer(const char *function_name, const char *custom_string) {
        Debug::get().beginTrace(function_name, custom_string);
    }
    ~ScopeTracer() { Debug::get().endTrace(); }
};

class DumpBufferRequest {
public:
    static inline DumpBufferRequest& get() { return sInstance; }

    enum {
        eInputBuffer  = 0,
        eOutputBuffer = 1,
        eHardwareRotateBuffer = 2,
    };
    void updateRequest(int type, int count, int layerMask = 0);
    bool requestDump(int type, int layerMask = 0);

private:
    DumpBufferRequest();

    int mTarget;
    int mRequestCount;
    int mLayerMask;
    static DumpBufferRequest sInstance;
};

#endif

