#pragma once

#include <string>

#include <android/log.h>
#include <log/log.h>

namespace aw {
namespace hardware {
namespace keymaster {

class AWKeymasterLogger {
  public:
    AWKeymasterLogger(std::string logTag);
    ~AWKeymasterLogger();
    void enable_kmsg(int enable);
    void log(int prio, const char* fmt, ...) __attribute__((__format__(printf, 3, 4)));
    void log(const char* fmt, ...) __attribute__((__format__(printf, 2, 3)));
    void dumphex(const void* data, int len);

  private:
    int kmsg_fd;
    int kmsgEnable;
    std::string logTag;
};

}
}
}
