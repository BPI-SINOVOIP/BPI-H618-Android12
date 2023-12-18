#ifndef DTV_ION_UTIL_H
#define DTV_ION_UTIL_H

int dtv_ion_open(void);
int dtv_ion_close(void);
int dtv_ion_AllocFd(size_t len, int *dma_buf_fd);
int dtv_ion_FreeFd(int dma_buf_fd);
void dtv_ion_CloseFd(int dma_buf_fd);
int dtv_ion_mmap(int buf_fd, size_t len, unsigned char **pVirAddr);
int dtv_ion_munmap(size_t len, unsigned char *pVirAddr);
int dtv_ion_get_phyAddr(int dma_buf_fd, unsigned long *pAddr);
int dtv_ion_free_phyAddr(int dma_buf_fd);
void dtv_ion_flush_cache(void *startAddr, int size);

int dtv_ion_testsample(void);

#endif