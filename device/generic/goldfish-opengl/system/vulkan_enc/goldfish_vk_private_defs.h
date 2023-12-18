// Copyright (C) 2018 The Android Open Source Project
// Copyright (C) 2018 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include <vulkan/vulkan.h>

#ifdef __cplusplus
#include <algorithm>
extern "C" {
#endif

#define VK_ANDROID_native_buffer 1
#define VK_ANDROID_NATIVE_BUFFER_EXTENSION_NUMBER 11

/* NOTE ON VK_ANDROID_NATIVE_BUFFER_SPEC_VERSION 6
 *
 * This version of the extension transitions from gralloc0 to gralloc1 usage
 * flags (int -> 2x uint64_t). The WSI implementation will temporarily continue
 * to fill out deprecated fields in VkNativeBufferANDROID, and will call the
 * deprecated vkGetSwapchainGrallocUsageANDROID if the new
 * vkGetSwapchainGrallocUsage2ANDROID is not supported. This transitionary
 * backwards-compatibility support is temporary, and will likely be removed in
 * (along with all gralloc0 support) in a future release.
 */
#define VK_ANDROID_NATIVE_BUFFER_SPEC_VERSION     7
#define VK_ANDROID_NATIVE_BUFFER_EXTENSION_NAME   "VK_ANDROID_native_buffer"

#define VK_ANDROID_NATIVE_BUFFER_ENUM(type,id)    ((type)(1000000000 + (1000 * (VK_ANDROID_NATIVE_BUFFER_EXTENSION_NUMBER - 1)) + (id)))
#define VK_STRUCTURE_TYPE_NATIVE_BUFFER_ANDROID   VK_ANDROID_NATIVE_BUFFER_ENUM(VkStructureType, 0)
#define VK_STRUCTURE_TYPE_SWAPCHAIN_IMAGE_CREATE_INFO_ANDROID VK_ANDROID_NATIVE_BUFFER_ENUM(VkStructureType, 1)
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENTATION_PROPERTIES_ANDROID VK_ANDROID_NATIVE_BUFFER_ENUM(VkStructureType, 2)

typedef enum VkSwapchainImageUsageFlagBitsANDROID {
    VK_SWAPCHAIN_IMAGE_USAGE_SHARED_BIT_ANDROID = 0x00000001,
    VK_SWAPCHAIN_IMAGE_USAGE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} VkSwapchainImageUsageFlagBitsANDROID;
typedef VkFlags VkSwapchainImageUsageFlagsANDROID;

typedef struct {
    VkStructureType             sType; // must be VK_STRUCTURE_TYPE_NATIVE_BUFFER_ANDROID
    const void*                 pNext;

    // Buffer handle and stride returned from gralloc alloc()
    const uint32_t*             handle;
    int                         stride;

    // Gralloc format and usage requested when the buffer was allocated.
    int                         format;
    int                         usage; // DEPRECATED in SPEC_VERSION 6
    // -- Added in SPEC_VERSION 6 --
    uint64_t                consumer;
    uint64_t                producer;
} VkNativeBufferANDROID;

typedef struct {
    VkStructureType                        sType; // must be VK_STRUCTURE_TYPE_SWAPCHAIN_IMAGE_CREATE_INFO_ANDROID
    const void*                            pNext;

    VkSwapchainImageUsageFlagsANDROID      usage;
} VkSwapchainImageCreateInfoANDROID;

typedef struct {
    VkStructureType                        sType; // must be VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENTATION_PROPERTIES_ANDROID
    const void*                            pNext;

    VkBool32                               sharedImage;
} VkPhysicalDevicePresentationPropertiesANDROID;

// -- DEPRECATED in SPEC_VERSION 6 --
typedef VkResult (VKAPI_PTR *PFN_vkGetSwapchainGrallocUsageANDROID)(VkDevice device, VkFormat format, VkImageUsageFlags imageUsage, int* grallocUsage);
// -- ADDED in SPEC_VERSION 6 --
typedef VkResult (VKAPI_PTR *PFN_vkGetSwapchainGrallocUsage2ANDROID)(VkDevice device, VkFormat format, VkImageUsageFlags imageUsage, VkSwapchainImageUsageFlagsANDROID swapchainImageUsage, uint64_t* grallocConsumerUsage, uint64_t* grallocProducerUsage);
typedef VkResult (VKAPI_PTR *PFN_vkAcquireImageANDROID)(VkDevice device, VkImage image, int nativeFenceFd, VkSemaphore semaphore, VkFence fence);
typedef VkResult (VKAPI_PTR *PFN_vkQueueSignalReleaseImageANDROID)(VkQueue queue, uint32_t waitSemaphoreCount, const VkSemaphore* pWaitSemaphores, VkImage image, int* pNativeFenceFd);

typedef VkResult (VKAPI_PTR *PFN_vkMapMemoryIntoAddressSpaceGOOGLE)(VkDevice device, VkDeviceMemory memory, uint64_t* pAddress);

#define VK_GOOGLE_gfxstream 1
#define VK_GOOGLE_GFXSTREAM_EXTENSION_NUMBER 386

#define VK_GOOGLE_GFXSTREAM_ENUM(type,id)    ((type)(1000000000 + (1000 * (VK_GOOGLE_GFXSTREAM_EXTENSION_NUMBER - 1)) + (id)))
#define VK_STRUCTURE_TYPE_IMPORT_COLOR_BUFFER_GOOGLE   VK_GOOGLE_GFXSTREAM_ENUM(VkStructureType, 0)
#define VK_STRUCTURE_TYPE_IMPORT_PHYSICAL_ADDRESS_GOOGLE   VK_GOOGLE_GFXSTREAM_ENUM(VkStructureType, 1)
#define VK_STRUCTURE_TYPE_IMPORT_BUFFER_GOOGLE   VK_GOOGLE_GFXSTREAM_ENUM(VkStructureType, 2)

typedef struct {
    VkStructureType sType; // must be VK_STRUCTURE_TYPE_IMPORT_COLOR_BUFFER_GOOGLE
    const void* pNext;
    uint32_t colorBuffer;
} VkImportColorBufferGOOGLE;

typedef struct {
    VkStructureType sType; // must be VK_STRUCTURE_TYPE_IMPORT_PHYSICAL_ADDRESS_GOOGLE
    const void* pNext;
    uint64_t physicalAddress;
    VkDeviceSize size;
    VkFormat format;
    VkImageTiling tiling;
    uint32_t tilingParameter;
} VkImportPhysicalAddressGOOGLE;

typedef struct {
    VkStructureType sType; // must be VK_STRUCTURE_TYPE_IMPORT_BUFFER_GOOGLE
    const void* pNext;
    uint32_t buffer;
} VkImportBufferGOOGLE;

typedef VkResult (VKAPI_PTR *PFN_vkRegisterImageColorBufferGOOGLE)(VkDevice device, VkImage image, uint32_t colorBuffer);
typedef VkResult (VKAPI_PTR *PFN_vkRegisterBufferColorBufferGOOGLE)(VkDevice device, VkBuffer image, uint32_t colorBuffer);

typedef VkResult (VKAPI_PTR *PFN_vkGetMemoryHostAddressInfoGOOGLE)(VkDevice device, VkDeviceMemory memory, uint64_t* pAddress, uint64_t* pSize);

typedef VkResult (VKAPI_PTR *PFN_vkFreeMemorySyncGOOGLE)(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks* pAllocationCallbacks);

#define VK_ANDROID_external_memory_android_hardware_buffer 1
struct AHardwareBuffer;
struct VkAndroidHardwareBufferPropertiesANDROID;
struct VkMemoryGetAndroidHardwareBufferInfoANDROID;

#ifdef __Fuchsia__

typedef struct VkAndroidHardwareBufferUsageANDROID {
    VkStructureType    sType;
    void*              pNext;
    uint64_t           androidHardwareBufferUsage;
} VkAndroidHardwareBufferUsageANDROID;

typedef struct VkAndroidHardwareBufferPropertiesANDROID {
    VkStructureType    sType;
    void*              pNext;
    VkDeviceSize       allocationSize;
    uint32_t           memoryTypeBits;
} VkAndroidHardwareBufferPropertiesANDROID;

typedef struct VkAndroidHardwareBufferFormatPropertiesANDROID {
    VkStructureType                  sType;
    void*                            pNext;
    VkFormat                         format;
    uint64_t                         externalFormat;
    VkFormatFeatureFlags             formatFeatures;
    VkComponentMapping               samplerYcbcrConversionComponents;
    VkSamplerYcbcrModelConversion    suggestedYcbcrModel;
    VkSamplerYcbcrRange              suggestedYcbcrRange;
    VkChromaLocation                 suggestedXChromaOffset;
    VkChromaLocation                 suggestedYChromaOffset;
} VkAndroidHardwareBufferFormatPropertiesANDROID;

typedef struct VkImportAndroidHardwareBufferInfoANDROID {
    VkStructureType            sType;
    const void*                pNext;
    struct AHardwareBuffer*    buffer;
} VkImportAndroidHardwareBufferInfoANDROID;

typedef struct VkMemoryGetAndroidHardwareBufferInfoANDROID {
    VkStructureType    sType;
    const void*        pNext;
    VkDeviceMemory     memory;
} VkMemoryGetAndroidHardwareBufferInfoANDROID;

typedef struct VkExternalFormatANDROID {
    VkStructureType    sType;
    void*              pNext;
    uint64_t           externalFormat;
} VkExternalFormatANDROID;


typedef VkResult (VKAPI_PTR *PFN_vkGetAndroidHardwareBufferPropertiesANDROID)(VkDevice device, const struct AHardwareBuffer* buffer, VkAndroidHardwareBufferPropertiesANDROID* pProperties);
typedef VkResult (VKAPI_PTR *PFN_vkGetMemoryAndroidHardwareBufferANDROID)(VkDevice device, const VkMemoryGetAndroidHardwareBufferInfoANDROID* pInfo, struct AHardwareBuffer** pBuffer);

#ifndef VK_NO_PROTOTYPES
VKAPI_ATTR VkResult VKAPI_CALL vkGetAndroidHardwareBufferPropertiesANDROID(
    VkDevice                                    device,
    const struct AHardwareBuffer*               buffer,
    VkAndroidHardwareBufferPropertiesANDROID*   pProperties);

VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryAndroidHardwareBufferANDROID(
    VkDevice                                    device,
    const VkMemoryGetAndroidHardwareBufferInfoANDROID* pInfo,
    struct AHardwareBuffer**                    pBuffer);
#endif

/**
 * Buffer pixel formats.
 */
enum AHardwareBuffer_Format {
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_R8G8B8A8_UNORM
     *   OpenGL ES: GL_RGBA8
     */
    AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM           = 1,
    /**
     * 32 bits per pixel, 8 bits per channel format where alpha values are
     * ignored (always opaque).
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_R8G8B8A8_UNORM
     *   OpenGL ES: GL_RGB8
     */
    AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM           = 2,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_R8G8B8_UNORM
     *   OpenGL ES: GL_RGB8
     */
    AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM             = 3,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_R5G6B5_UNORM_PACK16
     *   OpenGL ES: GL_RGB565
     */
    AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM             = 4,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_R16G16B16A16_SFLOAT
     *   OpenGL ES: GL_RGBA16F
     */
    AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT       = 0x16,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_A2B10G10R10_UNORM_PACK32
     *   OpenGL ES: GL_RGB10_A2
     */
    AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM        = 0x2b,
    /**
     * Opaque binary blob format.
     * Must have height 1 and one layer, with width equal to the buffer
     * size in bytes. Corresponds to Vulkan buffers and OpenGL buffer
     * objects. Can be bound to the latter using GL_EXT_external_buffer.
     */
    AHARDWAREBUFFER_FORMAT_BLOB                     = 0x21,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_D16_UNORM
     *   OpenGL ES: GL_DEPTH_COMPONENT16
     */
    AHARDWAREBUFFER_FORMAT_D16_UNORM                = 0x30,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_X8_D24_UNORM_PACK32
     *   OpenGL ES: GL_DEPTH_COMPONENT24
     */
    AHARDWAREBUFFER_FORMAT_D24_UNORM                = 0x31,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_D24_UNORM_S8_UINT
     *   OpenGL ES: GL_DEPTH24_STENCIL8
     */
    AHARDWAREBUFFER_FORMAT_D24_UNORM_S8_UINT        = 0x32,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_D32_SFLOAT
     *   OpenGL ES: GL_DEPTH_COMPONENT32F
     */
    AHARDWAREBUFFER_FORMAT_D32_FLOAT                = 0x33,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_D32_SFLOAT_S8_UINT
     *   OpenGL ES: GL_DEPTH32F_STENCIL8
     */
    AHARDWAREBUFFER_FORMAT_D32_FLOAT_S8_UINT        = 0x34,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_S8_UINT
     *   OpenGL ES: GL_STENCIL_INDEX8
     */
    AHARDWAREBUFFER_FORMAT_S8_UINT                  = 0x35,
    /**
     * YUV 420 888 format.
     * Must have an even width and height. Can be accessed in OpenGL
     * shaders through an external sampler. Does not support mip-maps
     * cube-maps or multi-layered textures.
     */
    AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420             = 0x23,
};
/**
 * Buffer usage flags, specifying how the buffer will be accessed.
 */
enum AHardwareBuffer_UsageFlags {
    /// The buffer will never be locked for direct CPU reads using the
    /// AHardwareBuffer_lock() function. Note that reading the buffer
    /// using OpenGL or Vulkan functions or memory mappings is still
    /// allowed.
    AHARDWAREBUFFER_USAGE_CPU_READ_NEVER        = 0UL,
    /// The buffer will sometimes be locked for direct CPU reads using
    /// the AHardwareBuffer_lock() function. Note that reading the
    /// buffer using OpenGL or Vulkan functions or memory mappings
    /// does not require the presence of this flag.
    AHARDWAREBUFFER_USAGE_CPU_READ_RARELY       = 2UL,
    /// The buffer will often be locked for direct CPU reads using
    /// the AHardwareBuffer_lock() function. Note that reading the
    /// buffer using OpenGL or Vulkan functions or memory mappings
    /// does not require the presence of this flag.
    AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN        = 3UL,
    /// CPU read value mask.
    AHARDWAREBUFFER_USAGE_CPU_READ_MASK         = 0xFUL,
    /// The buffer will never be locked for direct CPU writes using the
    /// AHardwareBuffer_lock() function. Note that writing the buffer
    /// using OpenGL or Vulkan functions or memory mappings is still
    /// allowed.
    AHARDWAREBUFFER_USAGE_CPU_WRITE_NEVER       = 0UL << 4,
    /// The buffer will sometimes be locked for direct CPU writes using
    /// the AHardwareBuffer_lock() function. Note that writing the
    /// buffer using OpenGL or Vulkan functions or memory mappings
    /// does not require the presence of this flag.
    AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY      = 2UL << 4,
    /// The buffer will often be locked for direct CPU writes using
    /// the AHardwareBuffer_lock() function. Note that writing the
    /// buffer using OpenGL or Vulkan functions or memory mappings
    /// does not require the presence of this flag.
    AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN       = 3UL << 4,
    /// CPU write value mask.
    AHARDWAREBUFFER_USAGE_CPU_WRITE_MASK        = 0xFUL << 4,
    /// The buffer will be read from by the GPU as a texture.
    AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE      = 1UL << 8,
    /**
     * The buffer will be written to by the GPU as a framebuffer
     * attachment.
     *
     * Note that the name of this flag is somewhat misleading: it does
     * not imply that the buffer contains a color format. A buffer with
     * depth or stencil format that will be used as a framebuffer
     * attachment should also have this flag.
     */
    AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT       = 1UL << 9,
    /**
     * The buffer is protected from direct CPU access or being read by
     * non-secure hardware, such as video encoders.
     *
     * This flag is incompatible with CPU read and write flags. It is
     * mainly used when handling DRM video. Refer to the EGL extension
     * EGL_EXT_protected_content and GL extension
     * GL_EXT_protected_textures for more information on how these
     * buffers are expected to behave.
     */
    AHARDWAREBUFFER_USAGE_PROTECTED_CONTENT      = 1UL << 14,
    /// The buffer will be read by a hardware video encoder.
    AHARDWAREBUFFER_USAGE_VIDEO_ENCODE           = 1UL << 16,
    /**
     * The buffer will be used for direct writes from sensors.
     * When this flag is present, the format must be AHARDWAREBUFFER_FORMAT_BLOB.
     */
    AHARDWAREBUFFER_USAGE_SENSOR_DIRECT_DATA     = 1UL << 23,
    /**
     * The buffer will be used as a shader storage or uniform buffer object.
     * When this flag is present, the format must be AHARDWAREBUFFER_FORMAT_BLOB.
     */
    AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER        = 1UL << 24,
    /**
     * The buffer will be used as a cube map texture.
     * When this flag is present, the buffer must have a layer count
     * that is a multiple of 6. Note that buffers with this flag must be
     * bound to OpenGL textures using the extension
     * GL_EXT_EGL_image_storage instead of GL_KHR_EGL_image.
     */
    AHARDWAREBUFFER_USAGE_GPU_CUBE_MAP               = 1UL << 25,
    /**
     * The buffer contains a complete mipmap hierarchy.
     * Note that buffers with this flag must be bound to OpenGL textures using
     * the extension GL_EXT_EGL_image_storage instead of GL_KHR_EGL_image.
     */
    AHARDWAREBUFFER_USAGE_GPU_MIPMAP_COMPLETE        = 1UL << 26,
    AHARDWAREBUFFER_USAGE_VENDOR_0  = 1ULL << 28,
    AHARDWAREBUFFER_USAGE_VENDOR_1  = 1ULL << 29,
    AHARDWAREBUFFER_USAGE_VENDOR_2  = 1ULL << 30,
    AHARDWAREBUFFER_USAGE_VENDOR_3  = 1ULL << 31,
    AHARDWAREBUFFER_USAGE_VENDOR_4  = 1ULL << 48,
    AHARDWAREBUFFER_USAGE_VENDOR_5  = 1ULL << 49,
    AHARDWAREBUFFER_USAGE_VENDOR_6  = 1ULL << 50,
    AHARDWAREBUFFER_USAGE_VENDOR_7  = 1ULL << 51,
    AHARDWAREBUFFER_USAGE_VENDOR_8  = 1ULL << 52,
    AHARDWAREBUFFER_USAGE_VENDOR_9  = 1ULL << 53,
    AHARDWAREBUFFER_USAGE_VENDOR_10 = 1ULL << 54,
    AHARDWAREBUFFER_USAGE_VENDOR_11 = 1ULL << 55,
    AHARDWAREBUFFER_USAGE_VENDOR_12 = 1ULL << 56,
    AHARDWAREBUFFER_USAGE_VENDOR_13 = 1ULL << 57,
    AHARDWAREBUFFER_USAGE_VENDOR_14 = 1ULL << 58,
    AHARDWAREBUFFER_USAGE_VENDOR_15 = 1ULL << 59,
    AHARDWAREBUFFER_USAGE_VENDOR_16 = 1ULL << 60,
    AHARDWAREBUFFER_USAGE_VENDOR_17 = 1ULL << 61,
    AHARDWAREBUFFER_USAGE_VENDOR_18 = 1ULL << 62,
    AHARDWAREBUFFER_USAGE_VENDOR_19 = 1ULL << 63,
};
/**
 * Buffer description. Used for allocating new buffers and querying
 * parameters of existing ones.
 */
typedef struct AHardwareBuffer_Desc {
    uint32_t    width;      ///< Width in pixels.
    uint32_t    height;     ///< Height in pixels.
    /**
     * Number of images in an image array. AHardwareBuffers with one
     * layer correspond to regular 2D textures. AHardwareBuffers with
     * more than layer correspond to texture arrays. If the layer count
     * is a multiple of 6 and the usage flag
     * AHARDWAREBUFFER_USAGE_GPU_CUBE_MAP is present, the buffer is
     * a cube map or a cube map array.
     */
    uint32_t    layers;
    uint32_t    format;     ///< One of AHardwareBuffer_Format.
    uint64_t    usage;      ///< Combination of AHardwareBuffer_UsageFlags.
    uint32_t    stride;     ///< Row stride in pixels, ignored for AHardwareBuffer_allocate()
    uint32_t    rfu0;       ///< Initialize to zero, reserved for future use.
    uint64_t    rfu1;       ///< Initialize to zero, reserved for future use.
} AHardwareBuffer_Desc;

#endif // __Fuchsia__

#define VK_GOOGLE_sized_descriptor_update_template 1

typedef void (VKAPI_PTR *PFN_vkUpdateDescriptorSetWithTemplateSizedGOOGLE)(
    VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    uint32_t imageInfoCount,
    uint32_t bufferInfoCount,
    uint32_t bufferViewCount,
    const uint32_t* pImageInfoEntryIndices,
    const uint32_t* pBufferInfoEntryIndices,
    const uint32_t* pBufferViewEntryIndices,
    const VkDescriptorImageInfo* pImageInfos,
    const VkDescriptorBufferInfo* pBufferInfos,
    const VkBufferView* pBufferViews);

#define VK_GOOGLE_async_command_buffers 1

typedef void (VKAPI_PTR *PFN_vkBeginCommandBufferAsyncGOOGLE)(
    VkCommandBuffer commandBuffer,
    const VkCommandBufferBeginInfo* pBeginInfo);
typedef void (VKAPI_PTR *PFN_vkEndCommandBufferAsyncGOOGLE)(
    VkCommandBuffer commandBuffer);
typedef void (VKAPI_PTR *PFN_vkResetCommandBufferAsyncGOOGLE)(
    VkCommandBuffer commandBuffer,
    VkCommandBufferResetFlags flags);
typedef void (VKAPI_PTR *PFN_vkCommandBufferHostSyncGOOGLE)(
    VkCommandBuffer commandBuffer,
    uint32_t needHostSync,
    uint32_t sequenceNumber);

typedef void (VKAPI_PTR *PFN_vkCreateImageWithRequirementsGOOGLE)(
    VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage, VkMemoryRequirements* pMemoryRequirements);

#ifndef VK_FUCHSIA_buffer_collection
#define VK_FUCHSIA_buffer_collection 1
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkBufferCollectionFUCHSIA)

#define VK_FUCHSIA_BUFFER_COLLECTION_SPEC_VERSION 1
#define VK_FUCHSIA_BUFFER_COLLECTION_EXTENSION_NAME "VK_FUCHSIA_buffer_collection"

typedef struct VkBufferCollectionCreateInfoFUCHSIA {
    VkStructureType    sType;
    const void*        pNext;
    uint32_t           collectionToken;
} VkBufferCollectionCreateInfoFUCHSIA;

typedef struct VkImportMemoryBufferCollectionFUCHSIA {
    VkStructureType              sType;
    const void*                  pNext;
    VkBufferCollectionFUCHSIA    collection;
    uint32_t                     index;
} VkImportMemoryBufferCollectionFUCHSIA;

typedef struct VkBufferCollectionImageCreateInfoFUCHSIA {
    VkStructureType              sType;
    const void*                  pNext;
    VkBufferCollectionFUCHSIA    collection;
    uint32_t                     index;
} VkBufferCollectionImageCreateInfoFUCHSIA;

typedef struct VkBufferCollectionBufferCreateInfoFUCHSIA {
    VkStructureType              sType;
    const void*                  pNext;
    VkBufferCollectionFUCHSIA    collection;
    uint32_t                     index;
} VkBufferCollectionBufferCreateInfoFUCHSIA;

typedef struct VkBufferCollectionPropertiesFUCHSIA {
    VkStructureType    sType;
    void*              pNext;
    uint32_t           memoryTypeBits;
    uint32_t           count;
} VkBufferCollectionPropertiesFUCHSIA;

#define VK_STRUCTURE_TYPE_IMPORT_MEMORY_BUFFER_COLLECTION_FUCHSIA       \
    ((VkStructureType)1001004004)
#define VK_STRUCTURE_TYPE_BUFFER_COLLECTION_IMAGE_CREATE_INFO_FUCHSIA \
    ((VkStructureType)1001004005)
#define VK_STRUCTURE_TYPE_BUFFER_COLLECTION_BUFFER_CREATE_INFO_FUCHSIA \
    ((VkStructureType)1001004008)
#endif  // VK_FUCHSIA_buffer_collection

#ifndef VK_FUCHSIA_external_memory
#define VK_FUCHSIA_external_memory 1
#define VK_FUCHSIA_EXTERNAL_MEMORY_SPEC_VERSION 1
#define VK_FUCHSIA_EXTERNAL_MEMORY_EXTENSION_NAME "VK_FUCHSIA_external_memory"

typedef struct VkBufferConstraintsInfoFUCHSIA {
    VkStructureType sType;
    const void* pNext;
    const VkBufferCreateInfo* pBufferCreateInfo;
    VkFormatFeatureFlags requiredFormatFeatures;
    uint32_t minCount;
} VkBufferConstraintsInfoFUCHSIA;

typedef struct VkImportMemoryZirconHandleInfoFUCHSIA {
    VkStructureType                       sType;
    const void*                           pNext;
    VkExternalMemoryHandleTypeFlagBits    handleType;
    uint32_t                              handle;
} VkImportMemoryZirconHandleInfoFUCHSIA;

typedef struct VkMemoryZirconHandlePropertiesFUCHSIA {
    VkStructureType    sType;
    void*              pNext;
    uint32_t           memoryTypeBits;
} VkMemoryZirconHandlePropertiesFUCHSIA;

typedef struct VkMemoryGetZirconHandleInfoFUCHSIA {
    VkStructureType                       sType;
    const void*                           pNext;
    VkDeviceMemory                        memory;
    VkExternalMemoryHandleTypeFlagBits    handleType;
} VkMemoryGetZirconHandleInfoFUCHSIA;

#define VK_STRUCTURE_TYPE_BUFFER_COLLECTION_BUFFER_CREATE_INFO_FUCHSIA \
    ((VkStructureType)1001004008)

#if VK_HEADER_VERSION < 174
#define VK_STRUCTURE_TYPE_IMPORT_MEMORY_ZIRCON_HANDLE_INFO_FUCHSIA \
    ((VkStructureType)1000364000)
#define VK_EXTERNAL_MEMORY_HANDLE_TYPE_ZIRCON_VMO_BIT_FUCHSIA \
    ((VkExternalMemoryHandleTypeFlagBits)0x00000800)
#endif

// Deprecated
#define VK_STRUCTURE_TYPE_TEMP_IMPORT_MEMORY_ZIRCON_HANDLE_INFO_FUCHSIA \
    ((VkStructureType)1001005000)
#define VK_EXTERNAL_MEMORY_HANDLE_TYPE_TEMP_ZIRCON_VMO_BIT_FUCHSIA \
    ((VkExternalMemoryHandleTypeFlagBits)0x00100000)

#else // VK_FUCHSIA_external_memory

// For backward compatibility
#if VK_HEADER_VERSION >= 174
#define VK_STRUCTURE_TYPE_TEMP_IMPORT_MEMORY_ZIRCON_HANDLE_INFO_FUCHSIA \
    ((VkStructureType)1001005000)
#define VK_EXTERNAL_MEMORY_HANDLE_TYPE_TEMP_ZIRCON_VMO_BIT_FUCHSIA \
    ((VkExternalMemoryHandleTypeFlagBits)0x00100000)
#endif  // VK_HEADER_VERSION >= 174

// For forward compatibility
#ifndef VK_STRUCTURE_TYPE_IMPORT_MEMORY_ZIRCON_HANDLE_INFO_FUCHSIA
#define VK_STRUCTURE_TYPE_IMPORT_MEMORY_ZIRCON_HANDLE_INFO_FUCHSIA ((VkStructureType)1000364000)
#endif  // VK_STRUCTURE_TYPE_IMPORT_MEMORY_ZIRCON_HANDLE_INFO_FUCHSIA

// For forward compatibility
#ifndef VK_EXTERNAL_MEMORY_HANDLE_TYPE_ZIRCON_VMO_BIT_FUCHSIA
#define VK_EXTERNAL_MEMORY_HANDLE_TYPE_ZIRCON_VMO_BIT_FUCHSIA \
    ((VkExternalMemoryHandleTypeFlagBits)0x00000800)
#endif  // VK_EXTERNAL_MEMORY_HANDLE_TYPE_ZIRCON_VMO_BIT_FUCHSIA

#endif  // VK_FUCHSIA_external_memory


#ifndef VK_FUCHSIA_external_semaphore
#define VK_FUCHSIA_external_semaphore 1
#define VK_FUCHSIA_EXTERNAL_SEMAPHORE_SPEC_VERSION 1
#define VK_FUCHSIA_EXTERNAL_SEMAPHORE_EXTENSION_NAME "VK_FUCHSIA_external_semaphore"

typedef struct VkImportSemaphoreZirconHandleInfoFUCHSIA {
    VkStructureType                          sType;
    const void*                              pNext;
    VkSemaphore                              semaphore;
    VkSemaphoreImportFlags                   flags;
    VkExternalSemaphoreHandleTypeFlagBits    handleType;
#if VK_HEADER_VERSION < 174
    uint32_t                                 handle;
#else // VK_HEADER_VERSION >= 174
    uint32_t                                 zirconHandle;
#endif // VK_HEADER_VERSION < 174
} VkImportSemaphoreZirconHandleInfoFUCHSIA;

typedef struct VkSemaphoreGetZirconHandleInfoFUCHSIA {
    VkStructureType                          sType;
    const void*                              pNext;
    VkSemaphore                              semaphore;
    VkExternalSemaphoreHandleTypeFlagBits    handleType;
} VkSemaphoreGetZirconHandleInfoFUCHSIA;

#if VK_HEADER_VERSION < 174
#define VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_ZIRCON_EVENT_BIT_FUCHSIA \
    ((VkExternalMemoryHandleTypeFlagBits)0x00000080)
#endif

// Deprecated
#define VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TEMP_ZIRCON_EVENT_BIT_FUCHSIA \
    ((VkExternalSemaphoreHandleTypeFlagBits)0x00100000)

#else // VK_FUCHSIA_external_semaphore

// For backward compatibility
#if VK_HEADER_VERSION >= 174
#define VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TEMP_ZIRCON_EVENT_BIT_FUCHSIA \
    ((VkExternalSemaphoreHandleTypeFlagBits)0x00100000)
#endif  // VK_HEADER_VERSION >= 174

// For forward compatibility
#ifndef VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TEMP_ZIRCON_EVENT_BIT_FUCHSIA
#define VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_ZIRCON_EVENT_BIT_FUCHSIA \
    ((VkExternalMemoryHandleTypeFlagBits)0x00000080)
#endif  // VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TEMP_ZIRCON_EVENT_BIT_FUCHSIA

#endif  // VK_FUCHSIA_external_semaphore


// VulkanStream features
#define VULKAN_STREAM_FEATURE_NULL_OPTIONAL_STRINGS_BIT (1 << 0)
#define VULKAN_STREAM_FEATURE_IGNORED_HANDLES_BIT (1 << 1)
#define VULKAN_STREAM_FEATURE_SHADER_FLOAT16_INT8_BIT (1 << 2)
#define VULKAN_STREAM_FEATURE_QUEUE_SUBMIT_WITH_COMMANDS_BIT (1 << 3)

#define VK_YCBCR_CONVERSION_DO_NOTHING ((VkSamplerYcbcrConversion)0x1111111111111111)

// Stuff we advertised but didn't define the structs for it yet because
// we also needed to update our vulkan headers and xml

#ifndef VK_VERSION_1_2

typedef struct VkPhysicalDeviceShaderFloat16Int8Features {
    VkStructureType    sType;
    void*              pNext;
    VkBool32           shaderFloat16;
    VkBool32           shaderInt8;
} VkPhysicalDeviceShaderFloat16Int8Features;


#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES \
    ((VkStructureType)1000082000)

#endif

#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR \
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES

#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR \
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES

#ifndef VK_KHR_shader_float16_int8

#define VK_KHR_shader_float16_int8 1
#define VK_KHR_SHADER_FLOAT16_INT8_SPEC_VERSION 1
#define VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME "VK_KHR_shader_float16_int8"
typedef VkPhysicalDeviceShaderFloat16Int8Features VkPhysicalDeviceShaderFloat16Int8FeaturesKHR;
typedef VkPhysicalDeviceShaderFloat16Int8Features VkPhysicalDeviceFloat16Int8FeaturesKHR;

#endif

#define VK_GOOGLE_gfxstream 1

typedef void (VKAPI_PTR *PFN_vkQueueHostSyncGOOGLE)(
    VkQueue queue, uint32_t needHostSync, uint32_t sequenceNumber);
typedef void (VKAPI_PTR *PFN_vkQueueSubmitAsyncGOOGLE)(
    VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence);
typedef void (VKAPI_PTR *PFN_vkQueueWaitIdleAsyncGOOGLE)(VkQueue queue);
typedef void (VKAPI_PTR *PFN_vkQueueBindSparseAsyncGOOGLE)(
    VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo* pBindInfo, VkFence fence);

typedef VkResult (VKAPI_PTR *PFN_vkGetLinearImageLayoutGOOGLE)(VkDevice device, VkFormat format, VkDeviceSize* pOffset, VkDeviceSize* pRowPitchAlignment);

typedef void (VKAPI_PTR *PFN_vkQueueFlushCommandsGOOGLE)(VkQueue queue, VkDeviceSize dataSize, const void* pData);
typedef void (VKAPI_PTR *PFN_vkQueueCommitDescriptorSetUpdatesGOOGLE)(VkQueue queue, uint32_t descriptorPoolCount, const VkDescriptorPool* pDescriptorPools, uint32_t descriptorSetCount, const VkDescriptorSetLayout* pDescriptorSetLayouts, const uint64_t* pDescriptorSetPoolIds, const uint32_t* pDescriptorSetWhichPool, const uint32_t* pDescriptorSetPendingAllocation, const uint32_t* pDescriptorWriteStartingIndices, uint32_t pendingDescriptorWriteCount, const VkWriteDescriptorSet* pPendingDescriptorWrites);

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus

template<class T, typename F>
bool arrayany(const T* arr, uint32_t begin, uint32_t end, const F& func) {
    const T* e = arr + end;
    return std::find_if(arr + begin, e, func) != e;
}

#define DEFINE_ALIAS_FUNCTION(ORIGINAL_FN, ALIAS_FN) \
template <typename... Args> \
inline auto ALIAS_FN(Args&&... args) -> decltype(ORIGINAL_FN(std::forward<Args>(args)...)) { \
  return ORIGINAL_FN(std::forward<Args>(args)...); \
}

#endif
