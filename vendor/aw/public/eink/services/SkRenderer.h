#pragma once

#include <ui/Region.h>
#include <utils/RefBase.h>

#include "SkCanvas.h"
#include "SkPaint.h"
#include "SkPath.h"
#include "SkRect.h"
#include "CommonDefs.h"

#define DEFAULT_STROKE_WIDTH 6
#define DEFAULT_RUBBER_WIDTH 40

namespace android {
class SkRenderer : public virtual RefBase {
public:
    SkRenderer(SkCanvas* argb, SkCanvas* gray8, SkCanvas* pathCanvas, SkBitmap* pathBitmap);
    ~SkRenderer();

    void setStrokeWidth(int width);
    void setRubberWidth(int width);
    void setBackground(SkBitmap* bgBitmap);
    void drawRect(float l, float t, float r, float b);
    void drawPath(SkPath path, SkRect& r);
    void drawRubberPath(SkPath path, SkRect& r);
    void redrawRubberRegion(Rect dirty_bounds, int left, int top);
    void clearCanvas();
    void drawBackground();
    bool savePic(const char* path, SkBitmap *bitmap);
private:
    SkCanvas* mArgbCanvas;
    SkCanvas* mGray8Canvas;
    SkCanvas* mBgCanvas;
    SkBitmap* mBgBitmap;
    SkBitmap* mBlendBitmap;
    SkCanvas* mBlendCanvas;
    SkCanvas* mPathCanvas;
    SkBitmap* mPathBitmap;
    SkPaint mPaint;
    SkPaint mRubberPaint;
    SkPaint mBgPaint;
};

}
