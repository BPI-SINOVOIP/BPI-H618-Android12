#include <algorithm>
#include <functional>
#include <limits>
#include <ostream>

#include <android/native_window.h>

#include <gui/ISurfaceComposer.h>
#include <gui/LayerState.h>

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <private/gui/ComposerService.h>

#include <ui/DisplayInfo.h>
#include <ui/Rect.h>
#include <utils/String8.h>

#include <math.h>
#include <math/vec3.h>

#include <stdlib.h>
#include "options.h"

using namespace android;

using Transaction = SurfaceComposerClient::Transaction;

int buffer_width_  = 720;
int buffer_height_ = 1280;

static params_t param_;

// Fill an RGBA_8888 formatted surface with a single color.
static void fillSurfaceRGBA8(const sp<SurfaceControl>& sc, uint8_t r, uint8_t g, uint8_t b,
                             bool unlock = true) {
    ANativeWindow_Buffer outBuffer;
    sp<Surface> s = sc->getSurface();
    s->lock(&outBuffer, nullptr);
    uint8_t* img = reinterpret_cast<uint8_t*>(outBuffer.bits);
    for (int y = 0; y < outBuffer.height; y++) {
        for (int x = 0; x < outBuffer.width; x++) {
            uint8_t* pixel = img + (4 * (y * outBuffer.stride + x));
            pixel[0] = r;
            pixel[1] = g;
            pixel[2] = b;
            pixel[3] = 255;
        }
    }
    if (unlock) {
        s->unlockAndPost();
    }
}

class ScalerTest {
public:
    void setup() {
        mComposerClient = new SurfaceComposerClient;

        sp<IBinder> display(
                SurfaceComposerClient::getInternalDisplayToken());

        DisplayInfo info;
        SurfaceComposerClient::getDisplayInfo(display, &info);

        //ssize_t displayWidth = info.w;
        //ssize_t displayHeight = info.h;

        // Background surface

#if 0
        mBGSurfaceControl =
            mComposerClient->createSurface(String8("BG Test Surface"), displayWidth,
                    displayHeight, PIXEL_FORMAT_RGBA_8888, 0);
        fillSurfaceRGBA8(mBGSurfaceControl, 63, 63, 195);
#endif

        // Foreground surface
        mFGSurfaceControl = mComposerClient->createSurface(
                String8("FG Test Surface"), param_.width, param_.height,
                PIXEL_FORMAT_RGBA_8888, 0);
        //fillSurfaceRGBA8(mFGSurfaceControl, 195, 63, 63);

        // Synchronization surface
        //mSyncSurfaceControl = mComposerClient->createSurface(String8("Sync Test Surface"), 1, 1,
        //        PIXEL_FORMAT_RGBA_8888, 0);
        //fillSurfaceRGBA8(mSyncSurfaceControl, 31, 31, 31);

        asTransaction([&](Transaction& t) {
                t.setDisplayLayerStack(display, 0);

                //t.setLayer(mBGSurfaceControl, INT32_MAX - 2).show(mBGSurfaceControl);

                t.setLayer(mFGSurfaceControl, INT32_MAX - 4)
                //.setTransformToDisplayInverse(mFGSurfaceControl, param_.transform ? true : false)
                .setTransform(mFGSurfaceControl, param_.transform)
                //.setMatrix(mFGSurfaceControl, 0.0f, 1.0f, -1.0f, 0.0f)
                //.setCrop(mFGSurfaceControl, Rect(displayWidth, displayHeight))
                //.setCrop(mFGSurfaceControl, Rect(0, 600, displayHeight, displayWidth+600))
                .setPosition(mFGSurfaceControl, param_.left, param_.top)
                //.setCrop(mFGSurfaceControl, Rect(buffer_width_, buffer_height_))
                .setOverrideScalingMode(mFGSurfaceControl, NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW)
                //.setPosition(mFGSurfaceControl, 0, displayHeight/2)
                //.setFrame(mFGSurfaceControl, Rect(display_frame_x_, display_frame_y_))
                .show(mFGSurfaceControl);

                //t.setLayer(mSyncSurfaceControl, INT32_MAX - 1)
                //.setPosition(mSyncSurfaceControl, displayWidth - 2, displayHeight - 2)
                //.show(mSyncSurfaceControl);
                });
    }

    void waitForPostedBuffers() {
        // Since the sync surface is in synchronous mode (i.e. double buffered)
        // posting three buffers to it should ensure that at least two
        // SurfaceFlinger::handlePageFlip calls have been made, which should
        // guaranteed that a buffer posted to another Surface has been retired.
        //fillSurfaceRGBA8(mSyncSurfaceControl, 31, 31, 31);
        //fillSurfaceRGBA8(mSyncSurfaceControl, 31, 31, 31);
        //fillSurfaceRGBA8(mSyncSurfaceControl, 31, 31, 31);

        sp<Surface> s = mFGSurfaceControl->getSurface();
        auto anw = static_cast<ANativeWindow*>(s.get());
        native_window_set_buffers_transform(anw, param_.transform);

        //const Rect  crop(0, 0, buffer_width_, buffer_height_);
        //const Rect frame(display_frame_x_, display_frame_y_);
        //Transaction().setCrop(mFGSurfaceControl, crop).setFrame(mFGSurfaceControl, frame).apply();
        //Transaction().setFrame(mFGSurfaceControl, frame).apply();
        Transaction().setMatrix(mFGSurfaceControl, param_.scale, 0.0f, 0.0f, param_.scale).apply();
        fillSurfaceRGBA8(mFGSurfaceControl, 195, 63, 63);
    }

    void asTransaction(const std::function<void(Transaction&)>& exec) {
        Transaction t;
        exec(t);
        t.apply(true);
    }

private:
    sp<SurfaceComposerClient> mComposerClient;
    sp<SurfaceControl> mBGSurfaceControl;
    sp<SurfaceControl> mFGSurfaceControl;
    sp<SurfaceControl> mSyncSurfaceControl;
};

int main(int argc, char** argv) {

    memset(&param_, 0, sizeof(param_));
    param_.width  = 1280;
    param_.height = 720;
    param_.scale = 0.625f;

    parse_options(argc, argv, &param_);

    printf("buffer %dx%d display position [%d, %d]\n",
            param_.width, param_.height,
            param_.left, param_.top);

    printf("layer scale    : %f\n", param_.scale);
    printf("layer transform: %d\n", param_.transform);

    ScalerTest test;
    test.setup();
    test.waitForPostedBuffers();

    while (true) {
        usleep(32000);
        test.waitForPostedBuffers();
    }
}

