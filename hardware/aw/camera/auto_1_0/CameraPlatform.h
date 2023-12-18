#ifndef __HAL_CAMERA_PLATFORM_H__
#define __HAL_CAMERA_PLATFORM_H__

#include <memory/memoryAdapter.h>
#include <memory/sc_interface.h>
#include GPU_PUBLIC_INCLUDE
#include <HardwareAPI.h> //VideoNativeHandleMetadata strust dependencies

#ifdef __A80__
#define __PLATFORM_A80__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN

//#define __OPEN_FACEDECTION__
//#define __OPEN_SMILEDECTION__
//#define __OPEN_BLINKDECTION__
//#define __OPEN_SMARTDECTION__

#define __CEDARX_FRAMEWORK_2__

#define USE_ION_MEM_ALLOCATOR
#endif

#ifdef __A83__
#define __PLATFORM_A83__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN ALIGN_4K

#define __CEDARX_FRAMEWORK_2__

//#define __OPEN_FACEDECTION__
//#define __OPEN_SMILEDECTION__
//#define __OPEN_SMARTDECTION__
//#define __OPEN_BLINKDECTION__


#define USE_ION_MEM_ALLOCATOR
#define WATI_FACEDETECT
#define USE_SUNXI_CAMERA_H
#endif

#ifdef __T5__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN ALIGN_16B

#define __CEDARX_FRAMEWORK_2__
#define ENABLE_CAMERA_SOURCE  1

//#define USE_DEINTERLACE_HW
//#define __OPEN_FACEDECTION__
//#define __OPEN_SMILEDECTION__
//#define __OPEN_SMARTDECTION__
//#define __OPEN_BLINKDECTION__


#define USE_ION_MEM_ALLOCATOR
//#define WATI_FACEDETECT
//#define USE_SUNXI_CAMERA_H
#define USE_CSI_VIN_DRIVER
#endif

#ifdef __T7__
#define __PLATFORM_T7__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN ALIGN_16B

#define __CEDARX_FRAMEWORK_2__
#define ENABLE_CAMERA_SOURCE  1

#define USE_DEINTERLACE_HW
//#define __OPEN_FACEDECTION__
//#define __OPEN_SMILEDECTION__
//#define __OPEN_SMARTDECTION__
//#define __OPEN_BLINKDECTION__


#define USE_ION_MEM_ALLOCATOR
//#define WATI_FACEDETECT
//#define USE_SUNXI_CAMERA_H
#define USE_CSI_VIN_DRIVER
#endif

#ifdef __A64__
#define __PLATFORM_A64__
#define BUFFER_PHY_OFFSET 0
#define GPU_BUFFER_ALIGN

#define __CEDARX_FRAMEWORK_2__

//#define __OPEN_FACEDECTION__
//#define __OPEN_SMILEDECTION__
//#define __OPEN_SMARTDECTION__
//#define __OPEN_BLINKDECTION__


#define USE_ION_MEM_ALLOCATOR
//#define WATI_FACEDETECT
#define USE_SUNXI_CAMERA_H
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
