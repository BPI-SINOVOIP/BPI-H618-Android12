#define LOG_NDEBUG 0
#define LOG_TAG "Handwriting"

#include <android-base/logging.h>
#include <gui/SurfaceComposerClient.h>
#include <ui/DisplayMode.h>
#include <ui/DisplayState.h>
#include <ui/PixelFormat.h>
#include <ui/Rotation.h>
#include <SkImageGenerator.h>
#include <SkImageInfo.h>
#include <cutils/trace.h>
#include <error.h>

#include "CommonDefs.h"
#include "DisplayConfigHal.h"
#include "Handwriting.h"
#include "SkRenderer.h"

namespace android {
void Handwriting:: allocBuffer(int w, int h, int bpp, buffer_info_t& outBuffer) {
    outBuffer.mem_size = w * h * bpp;

    outBuffer.fd = -1;
    IonAlloc::ion_memory_request(&outBuffer);
    outBuffer.virt_addr = mmap(nullptr, outBuffer.mem_size,
            PROT_READ | PROT_WRITE, MAP_SHARED, outBuffer.fd, 0);
    if (outBuffer.virt_addr == MAP_FAILED) {
        LOG(ERROR) << "mmap err: " <<  strerror(errno) << ", addr: " << outBuffer.virt_addr;
        IonAlloc::ion_memory_release(&outBuffer);
        return;
    }
    LOG(DEBUG) << "alloc buffer success, addr: " << outBuffer.virt_addr << ", fd: " << outBuffer.fd;
}

void Handwriting::freeBuffer(buffer_info_t& buffer) {
    if (nullptr != buffer.virt_addr) {
        munmap(buffer.virt_addr, buffer.mem_size);
        IonAlloc::ion_memory_release(&buffer);
        buffer.virt_addr = nullptr;
    }
    LOG(DEBUG) << "free buffer done";
}

void Handwriting::createGray8Canvas() {
    // gray8 bpp as: 1
    allocBuffer(mBufferWidth, mBufferHeight, 1, mBufferInfo);
    SkImageInfo imgInfo = SkImageInfo::Make(mBufferWidth, mBufferHeight,
            kGray_8_SkColorType, kOpaque_SkAlphaType, nullptr);
    mBitmap = new SkBitmap();
    mBitmap->setInfo(imgInfo, mBufferWidth); // TODO align stride insteadof width
    mBitmap->setPixels(mBufferInfo.virt_addr);
    mCanvas = new SkCanvas(*mBitmap);
}

void Handwriting::createPathCanvas() {
    //need alpha, can't 565 color
    LOG(DEBUG) << "create path argb canvas";
    //buffer_info_t bufferInfo;
    //allocBuffer(mBufferWidth, mBufferHeight, 4, bufferInfo);
    SkImageInfo imgInfo = SkImageInfo::Make(mBufferWidth, mBufferHeight,
        kN32_SkColorType, kPremul_SkAlphaType, nullptr);
    mPathBitmap = new SkBitmap();
    mPathBitmap->allocPixels(imgInfo, mBufferWidth * 4);
    //mPathBitmap->setInfo(imgInfo, mBufferWidth*4);
    //mPathBitmap->setPixels(bufferInfo.virt_addr); //other way to init Pixel
    mPathCanvas = new SkCanvas(*mPathBitmap);
}

void Handwriting::loadLastPath() {
    LOG(DEBUG) << "start loadLastPath";
    mPathCanvas->clear(SK_AlphaTRANSPARENT);

    sk_sp<SkData> data = SkData::MakeFromFileName(LAST_PATH_FILE);
    std::unique_ptr<SkImageGenerator> gen(SkImageGenerator::MakeFromEncoded(std::move(data)));
    if (gen) {
        SkPaint paint;
        SkBitmap *lastBitmap = new SkBitmap();
        bool allocRet = lastBitmap->tryAllocPixels(gen->getInfo());
        if (allocRet) {
            bool ret = gen->getPixels(gen->getInfo().makeColorSpace(nullptr), lastBitmap->getPixels(), lastBitmap->rowBytes());
            if (ret) {
                mPathCanvas->drawImage(lastBitmap->asImage(), 0, 0, SkSamplingOptions(), &paint);
                LOG(DEBUG) << "finish loadLastPath";
            }

            if (mDebugBitmap) {
                char path[] = "/data/eink/loadpen.png";
                SkFILEWStream stream(path);
                bool ret2 = SkEncodeImage(&stream, *lastBitmap, SkEncodedImageFormat::kPNG, 100);
                if (!ret2) {
                    LOG(ERROR) << "savePic " << path << " failed : " << strerror(errno);
                } else {
                    LOG(ERROR) << "savePic " << path << " success";
                }
            }
            lastBitmap->reset();
        }
    } else {
        LOG(ERROR) << "loadLastPath create image failed";
    }

    if (mDebugBitmap) {
        char path[] = "/data/eink/newpen.png";
        SkFILEWStream stream(path);
        bool ret2 = SkEncodeImage(&stream, *mPathBitmap, SkEncodedImageFormat::kPNG, 100);
        if (!ret2) {
            LOG(ERROR) << "savePic " << path << " failed : " << strerror(errno);
        } else {
            LOG(ERROR) << "savePic " << path << " success";
        }
    }
}

Handwriting::Handwriting(sp<Surface> surface, int x, int y) : mSurface(surface) {
    IonAlloc::ion_init();
    sp<IBinder> token = SurfaceComposerClient::getInternalDisplayToken();
    if (token == nullptr) {
        LOG(ERROR) << "failed to getInternalDisplayToken";
    }
    LOG(DEBUG) << "Location: " << x << ", " << y;
    int dWidth = 0;
    int dHeight = 0;
    ui::DisplayMode displayMode;
    const status_t error = SurfaceComposerClient::getActiveDisplayMode(token, &displayMode);
    if (error != NO_ERROR) {
        LOG(ERROR) << "Can't get active display mode.";
    } else {
        dWidth = displayMode.resolution.getWidth();
        dHeight = displayMode.resolution.getHeight();
        // TODO: eink display hardware always landscape, but system display is portrait.
        // due g2d is not available in non-64bit align buffer(1404x1872 is non-64bit align) before F version IC(current is E).
        // so rotation is handle by framework, hwc can't use g2d to handle it.
        // always allocate landscape buffer
        if (dWidth < dHeight) {
            mBufferWidth = dHeight;
            mBufferHeight = dWidth;
        } else {
            mBufferWidth = dWidth;
            mBufferHeight = dHeight;
        }
    }
    ui::DisplayState displayState;
    const status_t status = SurfaceComposerClient::getDisplayState(token, &displayState);
    if (status != NO_ERROR) {
        LOG(ERROR) << "failed to getDisplayState";
    }
    // Display size: 1872x1404, orientation:3
    LOG(DEBUG) << "Display size: " << dWidth << "x" << dHeight << ", orientation:" << (int)displayState.orientation;
    // Buffer size: 1872x1404
    LOG(DEBUG) << "Buffer size: " << mBufferWidth << "x" << mBufferHeight;

    createGray8Canvas();

    // create argb canvas
    ANativeWindow_Buffer info;
    surface->lock(&info, nullptr);
    // format:4 wxh:1404x1711 stride:1408
    LOG(DEBUG) << "format:" << info.format << " wxh:" << info.width << "x" << info.height << " stride:" << info.stride;

    surface->unlockAndPost();
    int pixelBytes = 1;
    SkColorType colorType = kUnknown_SkColorType;
    SkAlphaType alphaType = kOpaque_SkAlphaType;
    switch (info.format) {
        case WINDOW_FORMAT_RGBA_8888:
            colorType = kN32_SkColorType;
            alphaType = kPremul_SkAlphaType;
            pixelBytes = 4;
            break;
        case WINDOW_FORMAT_RGBX_8888:
            colorType = kN32_SkColorType;
            alphaType = kOpaque_SkAlphaType;
            pixelBytes = 4;
            break;
        case AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT:
            colorType = kRGBA_F16_SkColorType;
            alphaType = kPremul_SkAlphaType;
            pixelBytes = 2;
            break;
        case WINDOW_FORMAT_RGB_565:
            colorType = kRGB_565_SkColorType;
            alphaType = kOpaque_SkAlphaType;
            pixelBytes = 2;
            break;
        default:
            break;
    }
    SkImageInfo argbInfo = SkImageInfo::Make(info.width, info.height,
            colorType, alphaType);
    mArgbBitmap = new SkBitmap();
    //mArgbBitmap->setInfo(argbInfo, info.stride);
    mArgbBitmap->allocPixels(argbInfo, info.stride * pixelBytes);
    mArgbCanvas = new SkCanvas(*mArgbBitmap);

    int degree = (360 - (int)displayState.orientation * 90) % 360;
    int tx, ty = 0;
    switch (displayState.orientation) {
        case ui::Rotation::Rotation0:
            tx = x;
            ty = y;
            mDrawRect.left = x;
            mDrawRect.top = y;
            mDrawRect.right = x + info.width;
            mDrawRect.bottom = y + info.height;
            break;
        case ui::Rotation::Rotation90:
            tx = dWidth - y;
            ty = x;
            mDrawRect.left = dWidth - y - info.height;
            mDrawRect.top = x;
            mDrawRect.right = dWidth - y;
            mDrawRect.bottom = x + info.width;
            break;
        case ui::Rotation::Rotation180:
            tx = dWidth - x;
            ty = dHeight - y;
            mDrawRect.left = dWidth - x - info.width;
            mDrawRect.top = dHeight - y - info.height;
            mDrawRect.right = dWidth - x;
            mDrawRect.bottom = dHeight - y;
            break;
        case ui::Rotation::Rotation270:
            tx = y;
            ty = dHeight-x;
            mDrawRect.left = y;
            mDrawRect.top = dHeight - x - info.width;
            mDrawRect.right = y + info.height;
            mDrawRect.bottom = dHeight - x;
            break;
    }
    //LOG(DEBUG) << "rotate: " << degree << ", tx: " << tx << ", ty: " << ty;
    mArgbCanvas->rotate(degree);
    mArgbCanvas->translate(-tx, -ty);

    createPathCanvas();
    loadLastPath();

    mEventQueue = new EventQueue();
    mEventHandler = new EventHandler(mEventQueue);
    mSurfaceUpdater = new SurfaceUpdater();
    LOG(DEBUG) << "setBuffer " << mBufferInfo.fd;
    DisplayConfigHal::getInstance()->setBuffer(mBufferInfo.fd);
}

Handwriting::~Handwriting() {
    if (mDebugBitmap) {
        mSkRenderer->savePic("/data/eink/gray8.png", mBitmap);
        mSkRenderer->savePic("/data/eink/argb.png", mArgbBitmap);
    }

    mEventHandler = nullptr;
    mDrawThread = nullptr;
    mTestThread = nullptr;
    mSurfaceUpdater = nullptr;

    mSkRenderer = nullptr;

    mBitmap->setPixels(nullptr);
    delete mCanvas;
    delete mBitmap;

    delete mArgbCanvas;
    if (mArgbBitmap) {
        mArgbBitmap->reset();
        delete mArgbBitmap;
    }

    if (mPathCanvas)
        delete mPathCanvas;
    if (mPathBitmap) {
        mPathBitmap->reset();
        delete mPathBitmap;
    }

    freeBuffer(mBufferInfo);
    IonAlloc::ion_deinit();

    mSurface = nullptr;
}

void Handwriting::start() {
    if (mDrawThread == nullptr) {
        mSkRenderer = new SkRenderer(mArgbCanvas, mCanvas, mPathCanvas, mPathBitmap);
        if (mDebugBitmap) {
            mSkRenderer->savePic("/data/eink/gray8-1.png", mBitmap);
            mSkRenderer->savePic("/data/eink/argb-1.png", mArgbBitmap);
        }

        mDrawThread = new DrawThread(mEventQueue, mSkRenderer, mDrawRect);
        mDrawThread->setListener(mSurfaceUpdater);
    }

    mSurfaceUpdater->start(mSurface, mArgbBitmap);
    mDrawThread->start();
    mEventHandler->start();
}

void Handwriting::stop() {
    mEventQueue->enqueue(EVENT_TYPE_STOP);
    if (mDrawThread != nullptr) {
        mDrawThread->stop();
        mDrawThread = nullptr;
    }

    if (mEventHandler != nullptr) {
        mEventHandler->stop();
        mEventHandler = nullptr;
    }

    if (mTestThread != nullptr) {
        mTestThread->stop();
        mTestThread = nullptr;
    }

    if (mSurfaceUpdater != nullptr) {
        mSurfaceUpdater->stop();
        mSurfaceUpdater = nullptr;
    }

    mEventQueue = nullptr;
    LOG(DEBUG) << "Handwriting stop end";
}

void Handwriting::setWindowFocus(bool winFocus) {
    if (mWindowFocus != winFocus) {
        mWindowFocus = winFocus;

        mEventQueue->enqueue(EVENT_TYPE_FOCUS, &mWindowFocus);
        if (mWindowFocus && mBufferInfo.fd > 0) {
            //HWC eink-hal will close fd when exit handwritten mode.
            DisplayConfigHal::getInstance()->setBuffer(mBufferInfo.fd);
        }
    }
}

void Handwriting::clear() {
    mEventQueue->enqueue(EVENT_TYPE_CLEAR);
}

void Handwriting::refresh() {
    mEventQueue->enqueue(EVENT_TYPE_REFRESH);
}

void Handwriting::setBackground(SkBitmap* bgBitmap) {
    mEventQueue->enqueue(EVENT_TYPE_BACKGROUND, bgBitmap);
}

bool Handwriting::save(const ::std::string& path, bool withBackground) {
    if (mSkRenderer == nullptr) {
        LOG(ERROR) << "save failed for not init";
        return false;
    }

    if (withBackground) {
        return mSkRenderer->savePic(path.c_str(), mArgbBitmap);
    } else {
        return mSkRenderer->savePic(path.c_str(), mPathBitmap);
    }
}

int Handwriting::autoTest(bool start, int interval, int pressureMin, int pressureMax, int step, int stepInterval) {
    if (start) {
        if (mTestThread != nullptr) {
            return RET_FAIL;
        }
        mTestThread = new AutoTestThread(mEventQueue, mDrawRect, interval, pressureMin, pressureMax, step, stepInterval);
        mTestThread->start();
    } else {
        if (mTestThread != nullptr) {
            mTestThread->stop();
            mTestThread = nullptr;
        }
    }
    return RET_OK;
}

int Handwriting::autoFixLineTest(int mode, int xstep, int ystep, int interval) {
    if (mode > 0) {
        if (mTestThread != nullptr) {
            return RET_FAIL;
        }
        mTestThread = new AutoTestThread(mEventQueue, mDrawRect, mode, xstep, ystep, interval);
        mTestThread->start();
    } else {
        if (mTestThread != nullptr) {
            mTestThread->stop();
            mTestThread = nullptr;
        }
    }
    return RET_OK;
}

}; // namespace android
