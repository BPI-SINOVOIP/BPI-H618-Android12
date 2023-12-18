#pragma once

#ifndef ION_ALLOC__H
#define ION_ALLOC__H

#include <ion/ion.h>

namespace android {
#define ION_DEV_NAME    "/dev/ion"

#define SZ_64M  0x04000000
#define SZ_4M   0x00400000
#define SZ_1M   0x00100000
#define SZ_64K  0x00010000
#define SZ_4K   0x00001000
#define ION_ALLOC_SIZE      (SZ_4M + SZ_1M - SZ_64K)
#define ION_ALLOC_ALIGN     (SZ_4K)

struct buffer_info_t
{
    int fd;
    void *virt_addr;
    int mem_size;
    struct ion_allocation_data alloc_data;
};

class IonAlloc {

public:
    IonAlloc();
    ~IonAlloc();

    static void ion_init();
    static void ion_deinit();

    static int ion_memory_request(struct buffer_info_t* buffer_info);
    static int ion_memory_release(struct buffer_info_t* buffer_info);

private:

};

};//namespace android

#endif /* ION_ALLOC__H */
