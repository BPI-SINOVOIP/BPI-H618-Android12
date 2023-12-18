#pragma once

#include <thread>

#include <system/graphics.h>
#include <SkCanvas.h>
#include <SkBitmap.h>

#include <gui/Surface.h>
#include <ui/Rect.h>
#include <ui/GraphicBuffer.h>
#include <linux/input.h>

#include "IonAlloc.h"
#include "EventHandler.h"
#include "SurfaceUpdater.h"
#include "DrawThread.h"
#include "events.h"
#include "AutoTestThread.h"

namespace android {

class Handwriting : public virtual RefBase {

public:

    Handwriting(sp<Surface> surface, int x, int y);
    ~Handwriting();

    void start();
    void stop();
    void setWindowFocus(bool winFocus);
    void clear();
    void refresh();
    void setBackground(SkBitmap* bgBitmap);
    bool save(const ::std::string& path, bool withBackground);
    int autoTest(bool start, int interval, int pressureMin, int pressureMax, int step = 0, int stepInterval = 0);
    int autoFixLineTest(int mode, int xstep, int ystep, int interval/*ms*/);

private:
    void allocBuffer(int w, int h, int bpp, buffer_info_t& outBuffer);
    void freeBuffer(buffer_info_t& buffer);
    void createGray8Canvas();
    void createPathCanvas();
    void loadLastPath();

    Rect mDrawRect;
    sp<SurfaceUpdater> mSurfaceUpdater;
    sp<EventHandler> mEventHandler;
    sp<EventQueue> mEventQueue;
    sp<SkRenderer> mSkRenderer;
    sp<DrawThread> mDrawThread;
    sp<Surface> mSurface;

    sp<AutoTestThread> mTestThread;

    int mBufferWidth;
    int mBufferHeight;

    bool mWindowFocus = false;

    buffer_info_t mBufferInfo;
    SkBitmap *mBitmap;
    SkCanvas *mCanvas;
    SkBitmap *mArgbBitmap;
    SkCanvas *mArgbCanvas;
    SkBitmap *mPathBitmap;
    SkCanvas *mPathCanvas;

    bool mDebugBitmap = false;
};

};//namespace android

