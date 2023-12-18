
#include <gtest/gtest.h>

#include <errno.h>
#include <sys/mman.h>

#include "Layer.h"
#include "private_handle.h"
#include "IonBuffer.h"
#include "HardwareRotator.h"
#include "RotatorDevice.h"
#include "utils.h"
#include "Rect.h"

#include <hardware/hwcomposer_defs.h>

using namespace sunxi;
#define ALIGN(x,a) (((x) + (a) - 1L) & ~((a) - 1L))
#define GTEST_COUT std::cerr << "[ INFO     ] "

#define PixelDataEncode(x) (x & 0xfe)

class RotateTest: public testing::Test
{
public:
    virtual void SetUp() override {
        mSourceLayer = std::make_shared<Layer>();
        mBufferHandle = (private_handle_t *)malloc(sizeof(private_handle_t));
        mRotater = createHardwareRotator();
    }

    virtual void TearDown() override {
        free(mBufferHandle);
    }

    void initBuffer(int w, int h, int align, int format, int transform) {

        int computeSize = mRotater->computeBufferSize(w, h, align, format);
        int sourceBufferSize = ALIGN(computeSize, 4096);

        GTEST_COUT << "raw   wxh: " << w << "x" << h << " size: " << sourceBufferSize << "(" << computeSize << ")" << std::endl;

        mSourceBuf = std::make_unique<IonBuffer>(sourceBufferSize, GRALLOC_USAGE_HW_2D);
        mBufferHandle->share_fd = mSourceBuf->getPrivateBuffer()->handle;
        mBufferHandle->width  = w;
        mBufferHandle->height = h;
        mBufferHandle->aw_byte_align[0] = align;
        mBufferHandle->aw_byte_align[1] = align / 2;
        mBufferHandle->aw_byte_align[2] = align / 2;
        mBufferHandle->format = format;
        mSourceLayer->setLayerBuffer((buffer_handle_t)mBufferHandle, -1);
        mSourceLayer->setLayerTransform(transform /*HWC_TRANSFORM_ROT_270 */);

        int alignWidth  = 0;
        int alignHeight = 0;
        int outAlign    = align;

        mRotater->getAlignedWidthAndHeight(mSourceLayer, &alignWidth, &alignHeight, &outAlign);
        computeSize = mRotater->computeBufferSize(alignWidth, alignHeight, outAlign, format);
        int outputBuffersize = ALIGN(computeSize, 4096);

        GTEST_COUT << "align wxh: " << alignWidth << "x" << alignHeight << " size: " << outputBuffersize << "(" << computeSize << ")" << std::endl;

        mRotatedBuf = std::make_unique<IonBuffer>(outputBuffersize, GRALLOC_USAGE_HW_2D);
        mOutputBuffer = std::make_unique<OutputBuffer>();
        mOutputBuffer->buffer = mRotatedBuf->getPrivateBuffer();
        mOutputBuffer->acquireFence = sunxi::uniquefd(-1);

        if (transform & HAL_TRANSFORM_ROT_90) {
            mOutputBuffer->width  = alignHeight;
            mOutputBuffer->height = alignWidth;
        } else {
            mOutputBuffer->width  = alignWidth;
            mOutputBuffer->height = alignHeight;
        }
        mOutputBuffer->align  = outAlign;
        mOutputBuffer->format = format;

        writePattern(mSourceBuf.get(), w, h, nullptr, format);
    }

    void writePattern(const IonBuffer* buf, int w, int h, int* aligns, int format);
    int checkPattern();

public:
    std::shared_ptr<Layer> mSourceLayer;
    private_handle_t* mBufferHandle;
    std::unique_ptr<IonBuffer> mSourceBuf;

    std::unique_ptr<OutputBuffer> mOutputBuffer;
    std::unique_ptr<IonBuffer> mRotatedBuf;

    std::unique_ptr<RotatorDevice> mRotater;
};

class IonBufferMapper {
public:
    IonBufferMapper(const IonBuffer *buf) {
        const PrivateBuffer_t* pbuf = buf->getPrivateBuffer();
        mBufferHandle = sunxi::uniquefd(dup(pbuf->handle));
        void* vaddr = mmap(NULL, pbuf->size,
                PROT_READ|PROT_WRITE,
                MAP_SHARED, mBufferHandle.get(), 0);
        if (vaddr == MAP_FAILED) {
            GTEST_COUT << "mmap failed: " << strerror(errno) << std::endl;
        } else {
            mOffset = vaddr;
            mSize = pbuf->size;
        }
    }

    ~IonBufferMapper() {
        if (mOffset != nullptr) {
            munmap(mOffset, mSize);
            mOffset = nullptr;
            mSize = 0;
        }
    }

    void* getBufferOffset() const { return mOffset; }

private:
    sunxi::uniquefd mBufferHandle;
    uint32_t mSize = 0;
    void* mOffset = nullptr;
};

class PixelPicker {
public:
    typedef struct Pixel {
        union {
            struct { char r, g, b; };
            struct { char y, u, v; };
        };
        char a;
    } Pixel_t;

    PixelPicker(char **offset, int format, int* stride) {
        mOffset[0] = offset[0];
        mOffset[1] = offset[1];
        mOffset[2] = offset[2];

        mStride[0] = stride[0];
        mStride[1] = stride[1];
        mStride[2] = stride[2];

        mPixelFormat = format;
    }

    Pixel_t get(int x, int y) {
        switch (mPixelFormat) {
            case HAL_PIXEL_FORMAT_YV12: return yv12Get(x, y);
            default: return Pixel_t();
        }
    }
    void set(int x, int y, const Pixel_t& pixel) {
        switch (mPixelFormat) {
            case HAL_PIXEL_FORMAT_YV12: yv12Set(x, y, pixel); break;
            default: break;
        }
    }

    Pixel_t yv12Get(int x, int y) {
        Pixel_t ret;
        ret.y = *(mOffset[0] + (mStride[0] * y + x));
        int uvx = x >> 1;
        int uvy = y >> 1;
        ret.u = *(mOffset[1] + (mStride[1] * uvy + uvx));
        ret.v = *(mOffset[2] + (mStride[2] * uvy + uvx));
        return ret;
    }

    void yv12Set(int x, int y, const Pixel_t& pixel) {
        int uvx = x >> 1;
        int uvy = y >> 1;
        char *py = (mOffset[0] + (mStride[0] * y + x));
        char *pu = (mOffset[1] + (mStride[1] * uvy + uvx));
        char *pv = (mOffset[2] + (mStride[2] * uvy + uvx));

        *py = pixel.y;
        *pu = pixel.u;
        *pv = pixel.v;
    }

private:
    char *mOffset[3] = {nullptr};
    int mStride[3] = {0};
    int mPixelFormat;
};

void RotateTest::writePattern(const IonBuffer* buf, int w, int h, int* /*aligns*/, int format)
{
    IonBufferMapper mapper(buf);
    char *offset = (char *)mapper.getBufferOffset();
    if (offset == nullptr) {
        GTEST_COUT << "buffer offset is 0, writePattern exit" << std::endl;
        return;
    }

    int stride[3];
    stride[0] = w;
    stride[1] = stride[2] = (w >> 1);

    char* yuvOffset[3];
    yuvOffset[0] = offset;
    yuvOffset[1] = offset + stride[0] * h;
    yuvOffset[2] = yuvOffset[1] + stride[1] * (h >> 1);

    PixelPicker picker(yuvOffset, format, stride);

    for (int i = 0; i < w ; i++) {
        char v = PixelDataEncode(i);
        //GTEST_COUT << "Write: " << (int)v << std::endl;
        PixelPicker::Pixel_t pixel;
        pixel.y = pixel.u = pixel.v = v;
        picker.set(i, 0, pixel);
    }
}

int RotateTest::checkPattern()
{
    IonBufferMapper mapper(mRotatedBuf.get());
    char *offset = (char *)mapper.getBufferOffset();
    if (offset == nullptr) {
        GTEST_COUT << "buffer offset is 0, checkPattern exit" << std::endl;
        return -1;
    }

    int stride[3];
    stride[0] = mOutputBuffer->width;
    stride[1] = stride[2] = ALIGN(((mOutputBuffer->width) >> 1), mOutputBuffer->align);

    char* yuvOffset[3];
    yuvOffset[0] = offset;
    yuvOffset[1] = offset + stride[0] * mOutputBuffer->height;
    yuvOffset[2] = yuvOffset[1] + stride[1] * (mOutputBuffer->height >> 1);

    PixelPicker picker(yuvOffset, mBufferHandle->format, stride);

    int srcW = mBufferHandle->width;
    hwc_transform_t transform = static_cast<hwc_transform_t>(mSourceLayer->transform());

    int error = 0;
    for (int i = 0; i < srcW; i++) {
        int px = i;
        int py = 0;
        char v = PixelDataEncode(i);
        if ((int32_t)transform == HAL_TRANSFORM_ROT_90) {
            std::swap(px, py);
            px = mOutputBuffer->width - px - 1;
        }
        PixelPicker::Pixel_t pixel = picker.get(px, py);
        //GTEST_COUT << (int)pixel.y << ", " << (int)pixel.u << ", " << (int)pixel.v << std::endl;
        if (pixel.y != v || pixel.u != v || pixel.v != v)
            error++;
    }
    return error;
}

TEST_F(RotateTest, rotate176x144align16_270)
{
    initBuffer(176, 144, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate192x160align16_270)
{
    initBuffer(192, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), true);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate192x144align16_270)
{
    initBuffer(192, 144, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), true);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate176x160align16_270)
{
    initBuffer(176, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate208x160align16_270)
{
    initBuffer(208, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate144x160align16_270)
{
    initBuffer(144, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate160x160align16_270)
{
    initBuffer(160, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate112x160align16_270)
{
    initBuffer(112, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate240x160align16_270)
{
    initBuffer(240, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate128x160align16_270)
{
    initBuffer(128, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), true);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate304x160align16_270)
{
    initBuffer(304, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate368x160align16_270)
{
    initBuffer(368, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate48x160align16_270)
{
    initBuffer(48, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate32x160align16_90)
{
    initBuffer(32, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_90);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), true);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
    EXPECT_EQ(checkPattern(), 0);
}

TEST_F(RotateTest, rotate32x160align16_270)
{
    initBuffer(32, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
    EXPECT_EQ(checkPattern(), 0);
}

TEST_F(RotateTest, rotate160x176align16_270)
{
    initBuffer(160, 176, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate176x160align16_0)
{
    initBuffer(176, 160, 16, HAL_PIXEL_FORMAT_YV12, 0);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), true);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate176x160align16_180)
{
    initBuffer(176, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_180);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate176x160align16_90)
{
    initBuffer(176, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_90);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), true);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
    EXPECT_EQ(checkPattern(), 0);
}

TEST_F(RotateTest, rotate160x182align16_270)
{
    initBuffer(160, 182, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_270);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate176x160align16_flipH)
{
    initBuffer(176, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_FLIP_H);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
    //EXPECT_EQ(checkPattern(), 0);
}

TEST_F(RotateTest, rotate176x160align16_flipV)
{
    initBuffer(176, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_FLIP_V);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), true);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
    //EXPECT_EQ(checkPattern(), 0);
}

TEST_F(RotateTest, rotate176x160align16_flipH90)
{
    initBuffer(176, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_FLIP_H_ROT_90);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), false);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
    //EXPECT_EQ(checkPattern(), 0);
}

TEST_F(RotateTest, rotate176x160align16_flipV90)
{
    initBuffer(176, 160, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_FLIP_V_ROT_90);
    EXPECT_EQ(mRotater->capableHook(mSourceLayer), true);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
    //EXPECT_EQ(checkPattern(), 0);
}

TEST_F(RotateTest, rotate160x176align16_0)
{
    initBuffer(160, 176, 16, HAL_PIXEL_FORMAT_YV12, 0);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate160x176align16_180)
{
    initBuffer(160, 176, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_180);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
}

TEST_F(RotateTest, rotate160x176align16_90)
{
    initBuffer(160, 176, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_ROT_90);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
    EXPECT_EQ(checkPattern(), 0);
}

TEST_F(RotateTest, rotate160x176align16_flipH)
{
    initBuffer(160, 176, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_FLIP_H);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
    //EXPECT_EQ(checkPattern(), 0);
}

TEST_F(RotateTest, rotate160x176align16_flipV)
{
    initBuffer(160, 176, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_FLIP_V);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
    //EXPECT_EQ(checkPattern(), 0);
}

TEST_F(RotateTest, rotate160x176align16_flipH90)
{
    initBuffer(160, 176, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_FLIP_H_ROT_90);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
    //EXPECT_EQ(checkPattern(), 0);
}

TEST_F(RotateTest, rotate160x176align16_flipV90)
{
    initBuffer(160, 176, 16, HAL_PIXEL_FORMAT_YV12, HWC_TRANSFORM_FLIP_V_ROT_90);
    std::unique_ptr<RotatorDevice::Frame> frame =
        mRotater->createFrame(mSourceLayer, mOutputBuffer.get());
    EXPECT_EQ(mRotater->rotateOneFrame(frame.get()), 0);
    //EXPECT_EQ(checkPattern(), 0);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

