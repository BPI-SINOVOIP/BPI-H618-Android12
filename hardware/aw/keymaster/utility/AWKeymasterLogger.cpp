#include "include/AW_KM_utility/AWKeymasterLogger.h"

#include <android/log.h>
#include <log/log.h>
#include <errno.h>

namespace aw {
namespace hardware {
namespace keymaster {

AWKeymasterLogger::AWKeymasterLogger(std::string logTag) {
    this->logTag = logTag;
    kmsgEnable = 0;
    kmsg_fd = TEMP_FAILURE_RETRY(open("/dev/kmsg_debug", O_WRONLY));
    if (!kmsg_fd) {
        __android_log_print(ANDROID_LOG_WARN, logTag.data(),
                            "open kmsg_debug failed, console print will be disabled");
    }
}

AWKeymasterLogger::~AWKeymasterLogger() {
    if (kmsg_fd) close(kmsg_fd);
}

void AWKeymasterLogger::enable_kmsg(int enable) {
    kmsgEnable = enable;
}

#define LOG_BUF_SIZE 1024
void AWKeymasterLogger::log(int prio, const char* fmt, ...) {
    va_list ap;
    char buf[LOG_BUF_SIZE];

    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
    if (kmsg_fd && kmsgEnable) {
        std::string tmp_str(buf);
        tmp_str = logTag + " : " + tmp_str;
        write(kmsg_fd, tmp_str.data(), strlen(tmp_str.data()));
    }
    __android_log_write(prio, logTag.data(), buf);
    va_end(ap);
}

void AWKeymasterLogger::log(const char* fmt, ...) {
    va_list ap;
    char buf[LOG_BUF_SIZE];

    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
    std::string tmp_str(buf);
    if (kmsg_fd && kmsgEnable) {
        std::string tmp_str(buf);
        tmp_str = logTag + " : " + tmp_str;
        write(kmsg_fd, tmp_str.data(), strlen(tmp_str.data()));
    }
    __android_log_write(ANDROID_LOG_WARN, logTag.data(), buf);
    va_end(ap);
}

void AWKeymasterLogger::dumphex(const void* data, int size) {
    int i = 0;
    int line;

    char addr_prefix[32];
    char data_str[128];
    char tmp[sizeof(addr_prefix) + sizeof(data_str) + 1];
    int offset = 0;

    int noBuffInfo = 1;

    if (!noBuffInfo) log(ANDROID_LOG_DEBUG, "data buf=%p, size=%d\n", data, size);
    for (line = 0; offset < size; line++) {
        sprintf(&addr_prefix[0], "0x%08x:  ", line * 16);
        if (size - offset >= 16) {
            for (i = 0; i < 16; i++) {
                sprintf(&data_str[i * 3], "%02x ", ((uint8_t*)data)[offset + i]);
            }
            offset += 16;
        } else {
            for (i = 0; i < size - offset; i++) {
                sprintf(&data_str[i * 3], "%02x ", ((uint8_t*)data)[offset + i]);
            }
            offset = size;
        }
        if (noBuffInfo) {
            strcpy(tmp, data_str);
        } else {
            strcpy(tmp, addr_prefix);
            strcat(tmp, data_str);
        }
        log(ANDROID_LOG_DEBUG, "%s", tmp);
    }
}
}
}
}
