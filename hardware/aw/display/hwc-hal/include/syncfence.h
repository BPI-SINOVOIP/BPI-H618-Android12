
#ifndef SUNXI_SYNC_FENCE_H
#define SUNXI_SYNC_FENCE_H

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>

/*
 * Android libsync has not export some sw_sync_* functions which need by hwc,
 * make a libsync copy as libsync_aw to satisfy hwc.
 */

#if defined(SUNXI_SYNCFENCE_ENABLED)

struct syncfence_create_data {
    __u32   value;
    char    name[32];
    __s32   fence;
};

#define SYNCFENCE_IOC_MAGIC 'Z'
#define SYNCFENCE_IOC_CREATE_FENCE \
    _IOWR(SYNCFENCE_IOC_MAGIC, 0, struct syncfence_create_data)
#define SYNCFENCE_IOC_INC _IOW(SYNCFENCE_IOC_MAGIC, 1, __u32)

inline int fence_timeline_create(void) {
    return open("/dev/syncfence", O_RDWR);
}

inline int fence_timeline_inc(int fd, unsigned count) {
    __u32 arg = count;
    return ioctl(fd, SYNCFENCE_IOC_INC, &arg);
}

inline int fence_create(int fd, const char *name, unsigned value) {
    struct syncfence_create_data data;
    int err;

    data.value = value;
    strlcpy(data.name, name, sizeof(data.name));

    err = ioctl(fd, SYNCFENCE_IOC_CREATE_FENCE, &data);
    if (err < 0)
        return err;

    return data.fence;
}

#else
struct sw_sync_create_fence_data {
  __u32 value;
  char name[32];
  __s32 fence;
};

#define SW_SYNC_IOC_MAGIC 'W'
#define SW_SYNC_IOC_CREATE_FENCE _IOWR(SW_SYNC_IOC_MAGIC, 0, struct sw_sync_create_fence_data)
#define SW_SYNC_IOC_INC _IOW(SW_SYNC_IOC_MAGIC, 1, __u32)

inline int fence_timeline_create(void) {
    int ret;
    ret = open("/sys/kernel/debug/sync/sw_sync", O_RDWR);
    if (ret < 0)
        ret = open("/dev/sw_sync", O_RDWR);

    return ret;
}

inline int fence_timeline_inc(int fd, unsigned count) {
    __u32 arg = count;
    return ioctl(fd, SW_SYNC_IOC_INC, &arg);
}

inline int fence_create(int fd, const char *name, unsigned value) {
    struct sw_sync_create_fence_data data;
    int err;

    data.value = value;
    strlcpy(data.name, name, sizeof(data.name));

    err = ioctl(fd, SW_SYNC_IOC_CREATE_FENCE, &data);
    if (err < 0)
        return err;

    return data.fence;
}
#endif

#include <sync/sync.h>

enum Status {
    Invalid,     // Fence is invalid
    Unsignaled,  // Fence is valid but has not yet signaled
    Signaled,    // Fence is valid and has signaled
};

inline int get_fence_status(int fd) {
    if (fd == -1) {
        return Signaled;
    }
    if (sync_wait(fd, 0) < 0) {
        return (errno == ETIME) ? Unsignaled : Invalid;
    }
    return Signaled;
}

inline int fence_merge(const char* name, int fd1, int fd2) {
    if (fd1 >= 0 && fd2 >= 0) {
        return sync_merge(name, fd1, fd2);
    } else if (fd1 >= 0) {
        return sync_merge(name, fd1, fd1);
    } else if (fd2 >= 0) {
        return sync_merge(name, fd2, fd2);
    }
    return -1;
}

#endif
