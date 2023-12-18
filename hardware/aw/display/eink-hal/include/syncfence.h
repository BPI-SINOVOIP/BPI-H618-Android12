
#ifndef SUNXI_SYNC_FENCE_H
#define SUNXI_SYNC_FENCE_H

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>

struct sw_sync_create_fence_data {
  __u32 value;
  char name[32];
  __s32 fence;
};

#define SUNXI_SW_SYNC_IOC_MAGIC 'S'
#define SUNXI_SW_SYNC_IOC_CREATE_FENCE _IOWR(SUNXI_SW_SYNC_IOC_MAGIC, 0, struct sw_sync_create_fence_data)
#define SUNXI_SW_SYNC_IOC_INC _IOW(SUNXI_SW_SYNC_IOC_MAGIC, 1, __u32)

inline int fence_timeline_create(void) {
    int ret;
	ret = open("/dev/sw_sync", O_RDWR);

    return ret;
}

inline int fence_timeline_inc(int fd, unsigned count) {
    __u32 arg = count;
    return ioctl(fd, SUNXI_SW_SYNC_IOC_INC, &arg);
}

inline int fence_create(int fd, const char *name, unsigned value) {
    struct sw_sync_create_fence_data data;
    int err;

    data.value = value;
    strlcpy(data.name, name, sizeof(data.name));

    err = ioctl(fd, SUNXI_SW_SYNC_IOC_CREATE_FENCE, &data);
    if (err < 0)
        return err;

    return data.fence;
}

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

inline int fence_wait(int fd, int timeout)
{
    if (fd == -1)
        return Signaled;

	if (sync_wait(fd, timeout) < 0) {
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
