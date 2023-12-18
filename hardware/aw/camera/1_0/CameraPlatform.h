#ifndef __HAL_CAMERA_PLATFORM_H__
#define __HAL_CAMERA_PLATFORM_H__

//#include <memory/memoryAdapter.h>
//#include <memory/sc_interface.h>
#include <errno.h>
#include GPU_PUBLIC_INCLUDE
#include <HardwareAPI.h> //VideoNativeHandleMetadata strust dependencies

#if defined __A80__
#define __PLATFORM_A80__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN
#define NB_BUFFER 6

//#define __OPEN_FACEDECTION__
//#define __OPEN_SMILEDECTION__
//#define __OPEN_BLINKDECTION__
//#define __OPEN_APPERCEIVEPEOPLE__

#define __CEDARX_FRAMEWORK_2__

#define USE_ION_MEM_ALLOCATOR

#elif defined __A83__
#define __PLATFORM_A83__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN ALIGN_4K
#define NB_BUFFER 6

#define __CEDARX_FRAMEWORK_2__

//#define __OPEN_FACEDECTION__
//#define __OPEN_SMILEDECTION__
//#define __OPEN_SMARTDECTION__
//#define __OPEN_BLINKDECTION__


#define USE_ION_MEM_ALLOCATOR
#define WATI_FACEDETECT
#define USE_SUNXI_CAMERA_H

#elif defined __A33__
#define __PLATFORM_A33__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN ALIGN_4K
#define NB_BUFFER 6

#define __CEDARX_FRAMEWORK_2__

//#define __OPEN_FACEDECTION__
//#define __OPEN_SMILEDECTION__
//#define __OPEN_SMARTDECTION__
//#define __OPEN_BLINKDECTION__


#define USE_ION_MEM_ALLOCATOR
//#define WATI_FACEDETECT
#define USE_SUNXI_CAMERA_H

#elif defined __A64__
#define __PLATFORM_A64__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN
#define NB_BUFFER 6

#define __CEDARX_FRAMEWORK_2__

//#define __OPEN_FACEDECTION__
//#define __OPEN_SMILEDECTION__
//#define __OPEN_SMARTDECTION__
//#define __OPEN_BLINKDECTION__


#define USE_ION_MEM_ALLOCATOR

//#define WATI_FACEDETECT
#define USE_SUNXI_CAMERA_H

#elif defined __A63__
#define __PLATFORM_A63__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN
#define NB_BUFFER 6

#define __CEDARX_FRAMEWORK_2__

//#define __OPEN_FACEDECTION__
//#define __OPEN_SMILEDECTION__
//#define __OPEN_SMARTDECTION__
//#define __OPEN_BLINKDECTION__


#define USE_ION_MEM_ALLOCATOR
//#define WATI_FACEDETECT
//#define USE_CSI_VIN_DRIVER
//If use uvc camera ,open this one
//#define USE_SUNXI_CAMERA_H

#elif defined __A50__
#define __PLATFORM_A50__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN
#define NB_BUFFER 10

#define __CEDARX_FRAMEWORK_2__

//#define __OPEN_FACEDECTION__
//#define __OPEN_SMILEDECTION__
//#define __OPEN_SMARTDECTION__
//#define __OPEN_BLINKDECTION__


#define USE_ION_MEM_ALLOCATOR

//#define WATI_FACEDETECT
#define USE_CSI_VIN_DRIVER
#define USE_ISP
#define PICTURE_DISCARD_FRAME_NUM 3

#elif defined __H6__
#define __PLATFORM_H6__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN
//Avoid gralloc module allocate buffer bug: out of memery
#define NB_BUFFER 6

#define __CEDARX_FRAMEWORK_2__


#define USE_ION_MEM_ALLOCATOR

//#define WATI_FACEDETECT
//#define USE_CSI_VIN_DRIVER
#define USE_SUNXI_CAMERA_H

#elif defined __T5__
#define __PLATFORM_T5__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN
//Avoid gralloc module allocate buffer bug: out of memery
#define NB_BUFFER 6

#define __CEDARX_FRAMEWORK_2__

#define USE_ION_MEM_ALLOCATOR

//#define WATI_FACEDETECT
#define USE_CSI_VIN_DRIVER
//#define USE_ISP
#define PICTURE_DISCARD_FRAME_NUM 5

#elif defined __T7__
#define __PLATFORM_T7__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN
//Avoid gralloc module allocate buffer bug: out of memery
#define NB_BUFFER 10

#define __CEDARX_FRAMEWORK_2__


#define USE_ION_MEM_ALLOCATOR

//#define WATI_FACEDETECT
#define USE_CSI_VIN_DRIVER
#define USE_ISP
#define PICTURE_DISCARD_FRAME_NUM 3

#elif defined __T8__
#define __PLATFORM_T8__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN
//Avoid gralloc module allocate buffer bug: out of memery
#define NB_BUFFER 6

#define __CEDARX_FRAMEWORK_2__

#define USE_ION_MEM_ALLOCATOR
#define USE_SUNXI_CAMERA_H
//#define WATI_FACEDETECT
//#define USE_CSI_VIN_DRIVER
//#define USE_ISP
#define PICTURE_DISCARD_FRAME_NUM 3

#elif defined __H3__
#define __PLATFORM_H3__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN
//Avoid gralloc module allocate buffer bug: out of memery
#define NB_BUFFER 3
//#define USE_DEINTERLACE_HW


#define __CEDARX_FRAMEWORK_2__

#define USE_SHARE_BUFFER
#define USE_ION_MEM_ALLOCATOR
#define USE_SUNXI_CAMERA_H
//#define WATI_FACEDETECT
//#define USE_CSI_VIN_DRIVER
//#define USE_ISP
#define PICTURE_DISCARD_FRAME_NUM 3

#elif defined __H616__
#define __PLATFORM_H616__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN
//Avoid gralloc module allocate buffer bug: out of memery
#define NB_BUFFER 3

#define __CEDARX_FRAMEWORK_2__

#define USE_SHARE_BUFFER
#define USE_ION_MEM_ALLOCATOR

//#define WATI_FACEDETECT
//#define USE_CSI_VIN_DRIVER
#define USE_SUNXI_CAMERA_H
#define PICTURE_DISCARD_FRAME_NUM 3

#elif defined __T3__
#define __PLATFORM_T3__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN
//Avoid gralloc module allocate buffer bug: out of memery
#define NB_BUFFER 8
#define USE_DEINTERLACE_HW


#define __CEDARX_FRAMEWORK_2__

#define USE_SHARE_BUFFER
#define USE_ION_MEM_ALLOCATOR
#define USE_SUNXI_CAMERA_H
//#define WATI_FACEDETECT
//#define USE_CSI_VIN_DRIVER
//#define USE_ISP
#define PICTURE_DISCARD_FRAME_NUM 3

#else

#define __PLATFORM_UNIVERSAL__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN
//Avoid gralloc module allocate buffer bug: out of memery
#define NB_BUFFER 8
//#define USE_DEINTERLACE_HW
#define __CEDARX_FRAMEWORK_2__
#define USE_SHARE_BUFFER
#define USE_ION_MEM_ALLOCATOR
#define USE_SUNXI_CAMERA_H
//#define WATI_FACEDETECT
//#define USE_CSI_VIN_DRIVER
//#define USE_ISP
#define PICTURE_DISCARD_FRAME_NUM 3

#endif

#ifdef USE_ION_MEM_ALLOCATOR
extern "C" int ion_alloc_open();
extern "C" int ion_alloc_close();
extern "C" void* ion_alloc_palloc(int size);
extern "C" void ion_alloc_pfree(void * pbuf);
extern "C" void* ion_alloc_vir2phy_cpu(void * pbuf);
extern "C" void* ion_alloc_phy2vir_cpu(void * pbuf);
extern "C" void ion_alloc_flush_cache(void* startAddr, int size);
extern "C" void ion_flush_cache_all();


//extern struct ScMemOpsS* MemCamAdapterGetOpsS();
//struct ScMemOpsS* mMemOpsS = MemAdapterGetOpsS();
/*#define camera_phy_alloc_open()      mMemOpsS->open()
#define camera_phy_alloc_close()     mMemOpsS->close()
#define camera_phy_alloc_alloc(x)    mMemOpsS->palloc(x)
#define camera_phy_alloc_free(x)     mMemOpsS->pfree(x)
#define camera_phy_alloc_vir2phy(x)  mMemOpsS->cpu_get_phyaddr(x)
#define camera_phy_alloc_phy2vir(x)  mMemOpsS->cpu_get_viraddr(x)
#define camera_phy_flush_cache(x,y)  mMemOpsS->flush_cache(x,y)*/
//#define camera_phy_flush_cache_all() ion_flush_cache_all()

#elif USE_SUNXI_MEM_ALLOCATOR
extern "C" int sunxi_alloc_open();
extern "C" int sunxi_alloc_close();
extern "C" int sunxi_alloc_alloc(int size);
extern "C" void sunxi_alloc_free(void * pbuf);
extern "C" int sunxi_alloc_vir2phy(void * pbuf);
extern "C" int sunxi_alloc_phy2vir(void * pbuf);
extern "C" void sunxi_flush_cache(void * startAddr, int size);
extern "C" void sunxi_flush_cache_all();

#define camera_phy_alloc_open        sunxi_alloc_open()
#define camera_phy_alloc_close       sunxi_alloc_close()
#define camera_phy_alloc_alloc(x)    sunxi_alloc_alloc(x)
#define camera_phy_alloc_free(x)     sunxi_alloc_free(x)
#define camera_phy_alloc_vir2phy(x)  sunxi_alloc_vir2phy(x)
#define camera_phy_alloc_phy2vir(x)  int sunxi_alloc_phy2vir(x);
#define camera_phy_flush_cache(x,y)  sunxi_flush_cache(x,y);
#define camera_phy_flush_cache_all() sunxi_flush_cache_all()
#endif

#endif
