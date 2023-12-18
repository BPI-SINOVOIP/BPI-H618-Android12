#define LOG_NDEBUG 0
#define LOG_TAG "IonAlloc"

#include <android-base/logging.h>
#include <error.h>

#include "CommonDefs.h"
#include "IonAlloc.h"

namespace android {
static int sIonFd = -1;

IonAlloc::IonAlloc() {
}

IonAlloc::~IonAlloc() {
}

void IonAlloc::ion_init(){
    PR_INFO("ion_init");

    if ((sIonFd = ion_open()) < 0) {
        PR_ERR("[ion] err: open %s dev failed, err %s\n", ION_DEV_NAME,  strerror(errno));
        return;
    }
    PR_INFO("[ion]: ion_fd %d\n", sIonFd);
}

void IonAlloc::ion_deinit(){
    PR_INFO("ion_deinit");

    if(sIonFd){
        ion_close(sIonFd);
    }
}

int IonAlloc::ion_memory_request(struct buffer_info_t* buffer_info)
{
    PR_INFO("ion_memory_request");

    struct ion_allocation_data alloc_data;
    int ret = -1;
    //int handle = -1;
    int fd = -1;

    alloc_data.len = buffer_info->mem_size;
    alloc_data.align = ION_ALLOC_ALIGN;
    alloc_data.heap_id_mask = ION_HEAP_SYSTEM_MASK;
    alloc_data.flags = 0;//ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;

    ret = ion_alloc_fd(sIonFd, buffer_info->mem_size,
        ION_ALLOC_ALIGN, ION_HEAP_SYSTEM_MASK, 0, &fd);
    if (ret != 0) {
        PR_ERR("ion alloacte system buffer failed, mask=0x%08x, ret=%d",
            ION_HEAP_SYSTEM_MASK, ret);
        goto out;
    }
    PR_INFO("[ion]: ion_alloc_fd succes, fd %d\n", fd);

    //import buffer : linux 5.4 not support
    /*
    ret = ion_import(mIonFd, fd, &handle);
    if (ret !=0) {
        PR_ERR("ion_import failed,  ret=%d err=%s", ret, strerror(errno));
        goto out;
    }
    alloc_data.handle = handle;
    */
    buffer_info->fd = fd;

    memcpy(&(buffer_info->alloc_data),&alloc_data,sizeof(ion_allocation_data));
out:
    return ret;
}

int IonAlloc::ion_memory_release(struct buffer_info_t* buffer_info)
{
    PR_INFO("ion_memory_release Ionfd=%d", sIonFd);

    //int handle = -1;
    int ret = -1;
    if( sIonFd < 0 ){
        return -1;
    }
    PR_INFO("[ion]: close buf fd=%d\n", buffer_info->fd);
    close(buffer_info->fd);
    buffer_info->fd = 0;

    //free buffer : linux 5.4 not need to ion_free
    /*
    handle = buffer_info->alloc_data.handle;
    ret = ion_free(mIonFd, handle);
    if(ret) {
        PR_ERR("[ion]: ION_IOC_FREE err, ret %d\n",ret);
        return -1;
    }
    PR_INFO("[ion]: handle %d ION_IOC_FREE done\n", handle);
    */
    return ret;
}
};

