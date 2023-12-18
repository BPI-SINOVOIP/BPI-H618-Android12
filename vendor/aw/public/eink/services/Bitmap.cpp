#define LOG_TAG "Bitmap"
#include "Bitmap.h"

//#include "GraphicBuffer.h"
#include "SkBitmap.h"
#include "SkPixelRef.h"
#include "SkImageEncoder.h"
#include "SkImageInfo.h"
#include "SkColor.h"
#include "SkColorSpace.h"
#include "SkStream.h"

#include <binder/Parcel.h>
#include <binder/ParcelFileDescriptor.h>
#include <hwui/Paint.h>
#include <hwui/Bitmap.h>
#include <utils/Color.h>

#include <string.h>
#include <memory>
#include <string>

#include <sys/mman.h>

using ::android::os::ParcelFileDescriptor;

#define DEBUG_PARCEL 0
#define ASHMEM_BITMAP_MIN_SIZE (128 * (1 << 10))

///////////////////////////////////////////////////////////////////////////////

// This is the maximum possible size because the SkColorSpace must be
// representable (and therefore serializable) using a matrix and numerical
// transfer function.  If we allow more color space representations in the
// framework, we may need to update this maximum size.
static constexpr uint32_t kMaxColorSpaceSerializedBytes = 80;

namespace android {
namespace graphics {

status_t Bitmap::readFromParcel(const Parcel* p) {
    ALOGD("Bitmap::readFromParcel\n");
    if (p == nullptr) return BAD_VALUE;

    const bool        isMutable = p->readInt32() != 0;
    const SkColorType colorType = (SkColorType)p->readInt32();
    const SkAlphaType alphaType = (SkAlphaType)p->readInt32();
    const uint32_t    colorSpaceSize = p->readUint32();
    ALOGD("isMutable=%d, colorSpaceSize=%d\n", isMutable, colorSpaceSize);
    sk_sp<SkColorSpace> colorSpace;
    if (colorSpaceSize > 0) {
        if (colorSpaceSize > kMaxColorSpaceSerializedBytes) {
            ALOGD("Bitmap_createFromParcel: Serialized SkColorSpace is larger than expected: "
                    "%d bytes\n", colorSpaceSize);
        }

        const void* data = p->readInplace(colorSpaceSize);
        if (data) {
            colorSpace = SkColorSpace::Deserialize(data, colorSpaceSize);
        } else {
            ALOGD("Bitmap_createFromParcel: Unable to read serialized SkColorSpace data\n");
        }
    }
    const int         width = p->readInt32();
    const int         height = p->readInt32();
    const int         rowBytes = p->readInt32();
    const int         density = p->readInt32();
    ALOGD("width=%d, height=%d, rowBytes=%d, density=%d\n", width, height, rowBytes, density);

    if (kN32_SkColorType != colorType &&
            kRGBA_F16_SkColorType != colorType &&
            kRGB_565_SkColorType != colorType &&
            kARGB_4444_SkColorType != colorType &&
            kAlpha_8_SkColorType != colorType) {
        SkDebugf("Bitmap_createFromParcel unknown colortype: %d\n", colorType);
        return BAD_VALUE;
    }

    auto imageInfo = SkImageInfo::Make(width, height, colorType, alphaType, colorSpace);
    size_t allocationSize = 0;
    if (!SkBitmap().setInfo(imageInfo, rowBytes)) {
        ALOGE("Received bad SkImageInfo");
        return BAD_VALUE;
    }
    if (!::android::Bitmap::computeAllocationSize(rowBytes, height, &allocationSize)) {
        ALOGE("Received bad bitmap size: width=%d, height=%d, rowBytes=%d, density=%d", width, height, rowBytes, density);
        return BAD_VALUE;
    }

    const int type = p->readInt32();
    ALOGD("type=%d\n", type);
    if (type == 0) { // IN_PLACE
        const int32_t    size = p->readInt32();
        const void* data = p->readInplace(colorSpaceSize);
        nativeBitmap = ::android::Bitmap::allocateHeapBitmap(allocationSize, imageInfo, rowBytes);
        if (nativeBitmap) {
            memcpy(nativeBitmap->pixels(), data, size);
            ALOGD("nativeBitmap IN_PLACE OK\n");
            return OK;
        }
    } else if (type == 1) { // ASHMEM
        const int32_t    size = p->readInt32();
        std::optional<ParcelFileDescriptor> parcelFd;

        status_t status = p->readParcelable(&parcelFd);
        if (status == 0 && parcelFd) {
            android::base::unique_fd fd(parcelFd->release().release());
            int flags = PROT_READ;
            if (isMutable) {
                flags |= PROT_WRITE;
            }
            void* addr = mmap(nullptr, size, flags, MAP_SHARED, fd.get(), 0);
            if (addr == MAP_FAILED) {
                const int err = errno;
                ALOGW("mmap failed, error %d (%s)", err, strerror(err));
                return BAD_VALUE;
            }
            nativeBitmap = ::android::Bitmap::createFrom(
                    imageInfo, rowBytes, fd.release(), addr, size, !isMutable);
            ALOGD("nativeBitmap ASHMEM OK\n");
            return OK;
        }
    } else {
        return BAD_VALUE;
    }
    ALOGD("nativeBitmap FAIL\n");
    return BAD_VALUE;
}

status_t Bitmap::writeToParcel(Parcel* p) const {
    ALOGD("Bitmap::writeToParcel\n");
    if (p == nullptr) return BAD_VALUE;
    return BAD_VALUE;
}


} // namespace graphics
} // namespace android