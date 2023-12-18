#define LOG_NDEBUG 0
#define LOG_TAG "handwritten-libs"

#include "ion_alloc.h"
#include "CommonDefs.h"

int ion_fd = -1;

void ion_init(){
	if((ion_fd = open(ION_DEV_NAME, O_RDWR)) < 0) {
		PR_ERR("[ion] err: open %s dev failed\n",ION_DEV_NAME);
		return;
	}
	PR_INFO("[ion]: ion_fd %d\n", ion_fd);
}

void ion_dump(){
	struct ion_custom_data custom_data;
	int ret = -1;
	if( ion_fd < 0 ){
		return ;
	}
	custom_data.cmd = ION_IOC_SUNXI_DUMP;
	custom_data.arg = 0;
	ret = ioctl(ion_fd, ION_IOC_CUSTOM, &custom_data);
	if(ret) {
		PR_ERR("[ion]: ION_IOC_CUSTOM err, ret %d\n",ret);
	}
}

int ion_memory_request(struct buffer_info_t* buffer_info)
{
	struct ion_allocation_data alloc_data;
	//struct ion_handle_data handle_data;
	struct ion_fd_data fd_data;
	int ret = -1;
	//void *user_addr;
	//unsigned int y_pitch, u_pitch;
	//unsigned int y_size, u_size;

	/* alloc buffer */
	alloc_data.len = buffer_info->mem_size;
	alloc_data.align = ION_ALLOC_ALIGN;
	alloc_data.heap_id_mask = ION_HEAP_SYSTEM_MASK;
	alloc_data.flags = 0;//ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
	PR_INFO("[ion]: ION_HEAP_TYPE %x, alloc_data.heap_id_mask 0x%x\n",
					ION_HEAP_SYSTEM_MASK, alloc_data.heap_id_mask);
	ret = ioctl(ion_fd, ION_IOC_ALLOC, &alloc_data);
	if(ret) {
		PR_ERR("[ion]: ION_IOC_ALLOC err, ret %d, handle %d\n",ret, alloc_data.handle);
		goto out;
	}
	PR_INFO("[ion]: ION_IOC_ALLOC succes, handle %d\n",alloc_data.handle);

	/* map dma buffer fd */
	fd_data.handle = alloc_data.handle;
	ret = ioctl(ion_fd, ION_IOC_MAP, &fd_data);
	if(ret) {
		PR_ERR("[ion]: ION_IOC_MAP err, ret %d, dmabuf fd 0x%08x\n",ret,
				(unsigned int)fd_data.fd);
		goto out;
	}
	PR_INFO("[ion]: ION_IOC_MAP success, get dmabuf fd 0x%08x\n",(unsigned int)fd_data.fd);

	buffer_info->fd = fd_data.fd;
	memcpy(&(buffer_info->alloc_data),&alloc_data,sizeof(ion_allocation_data));

out:
	return ret;
}

int ion_memory_release(struct buffer_info_t* buffer_info)
{
	struct ion_handle_data handle_data;
	int ret = -1;
	if( ion_fd < 0 ){
		return -1;
	}
	PR_INFO("[ion]: ion_fd %d\n", ion_fd);

	PR_INFO("[ion]: close dmabuf fd %d\n", buffer_info->fd);
	close(buffer_info->fd);

	/* free buffer */
	handle_data.handle = buffer_info->alloc_data.handle;
	ret = ioctl(ion_fd, ION_IOC_FREE, &handle_data);
	if(ret) {
		PR_ERR("[ion]: ION_IOC_FREE err, ret %d\n",ret);
		return -1;
	}
	PR_INFO("[ion]: handle %d ION_IOC_FREE done\n",handle_data.handle);
	/* close ion device fd */
	return ret;
}

void ion_deinit(){
	if(ion_fd){
		close(ion_fd);
	}
}

