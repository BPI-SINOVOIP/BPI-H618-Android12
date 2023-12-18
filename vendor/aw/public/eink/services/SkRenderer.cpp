#define LOG_NDEBUG 0
#define LOG_TAG "handwritten-libs"

//#include <system/graphics.h>
//#include <memory.h>

#include "ui/PixelFormat.h"

#include "SkBitmap.h"
#include "SkRenderer.h"
#include "SkImageInfo.h"
#include "SkPath.h"
#include "SkImage.h"
#include "SkStream.h"
#include "SkEncodedImageFormat.h"
#include "SkImageEncoder.h"
#include <android-base/logging.h>
#include <error.h>

namespace android
{

SkRenderer::SkRenderer(SkCanvas* argb, SkCanvas* gray8, SkCanvas* pathCanvas, SkBitmap* pathBitmap) :
        mArgbCanvas(argb),
        mGray8Canvas(gray8),
        mPathCanvas(pathCanvas),
        mPathBitmap(pathBitmap) {
    //init paint for hand write
    mPaint.setColor(SK_ColorBLACK);
    mPaint.setStrokeWidth(DEFAULT_STROKE_WIDTH);
    mPaint.setStyle(SkPaint::kStroke_Style);
    mPaint.setStrokeCap(SkPaint::kRound_Cap);
    mPaint.setStrokeJoin(SkPaint::kRound_Join);
    mPaint.setAntiAlias(true);

    //init rubber paint
    mRubberPaint.setColor(SK_ColorTRANSPARENT);
    mRubberPaint.setStrokeWidth(DEFAULT_RUBBER_WIDTH);
    mRubberPaint.setStyle(SkPaint::kStroke_Style);
    mRubberPaint.setBlendMode(SkBlendMode::kSrc);
    //use round
    mRubberPaint.setStrokeCap(SkPaint::kRound_Cap);
    mRubberPaint.setStrokeJoin(SkPaint::kRound_Join);
    //use square
    //mRubberPaint.setStrokeCap(SkPaint::kSquare_Cap);
    //mRubberPaint.setStrokeJoin(SkPaint::kBevel_Join);

    //mArgbCanvas->clear(SK_AlphaTRANSPARENT);
    mArgbCanvas->clear(SK_ColorWHITE);
    mGray8Canvas->clear(SK_ColorWHITE); // SK_ColorTRANSPARENT
    //have loaded last path, can't clear
    //mPathCanvas->clear(SK_AlphaTRANSPARENT);

    int w = mPathBitmap->width();
    int h = mPathBitmap->height();
    ssize_t bpr = mPathBitmap->rowBytes();
    LOG(DEBUG) << "path bitmap w=" <<w<<" h="<< h << " bpr="<<bpr;
    SkImageInfo imgInfo = SkImageInfo::Make(w, h,
        kN32_SkColorType, kPremul_SkAlphaType, nullptr);

    /* Because some backgrounds are transparent and become
     * black after blending, need to draw once to turn into
     * opaque.
     */
    LOG(DEBUG) << "create bg bitmap";
    mBgBitmap = new SkBitmap();
    mBgBitmap->setInfo(imgInfo, bpr);
    mBgBitmap->allocPixels(imgInfo, bpr);
    mBgCanvas = new SkCanvas(*mBgBitmap);
    mBgCanvas->clear(SK_ColorWHITE);

    /* Because the path bitmap is transparent and 32bit, use
     * drawBitmapRect api to blend and draw into argbBitmap
     * (possibly 16bit) or gray8Bitmap (8bit) become black,
     * need to blend and draw into blendBitmap (32bit) first.
     */
    LOG(DEBUG) << "create argb blend canvas";
    mBlendBitmap = new SkBitmap();
    mBlendBitmap->setInfo(imgInfo, bpr);
    mBlendBitmap->allocPixels(imgInfo, bpr);
    mBlendCanvas = new SkCanvas(*mBlendBitmap);

    //maybe without setBackground from app
    drawBackground();
}

SkRenderer::~SkRenderer() {
    savePic(LAST_PATH_FILE, mPathBitmap);

    if (mBgCanvas)
        delete mBgCanvas;

    if (mBgBitmap) {
        mBgBitmap->reset();
        delete mBgBitmap;
    }
    if (mBlendCanvas)
        delete mBlendCanvas;

    if (mBlendBitmap) {
        mBlendBitmap->reset();
        delete mBlendBitmap;
    }
}

void SkRenderer::setStrokeWidth(int width) {
    if(width >= 0){
        mPaint.setStrokeWidth(width);
    }
}

void SkRenderer::setRubberWidth(int width) {
    if(width >= 0){
        mRubberPaint.setStrokeWidth(width);
    }
}

void SkRenderer::drawRect(float l, float t, float r, float b) {
    mArgbCanvas->drawRect({l, t, r, b}, mPaint);
    mGray8Canvas->drawRect({l, t, r, b}, mPaint);
    mPathCanvas->drawRect({l, t, r, b}, mPaint);
}

void SkRenderer::drawPath(SkPath path, SkRect& r) {
    mArgbCanvas->drawPath(path, mPaint);
    mGray8Canvas->drawPath(path, mPaint);
    mPathCanvas->drawPath(path, mPaint);
    int width = mPaint.getStrokeWidth() / 2 + 1;
    r = path.getBounds().makeOutset(width, width);
}

void SkRenderer::drawRubberPath(SkPath path, SkRect& r) {
    mPathCanvas->drawPath(path, mRubberPaint);
    int width = mRubberPaint.getStrokeWidth() / 2 + 1;
    r = path.getBounds().makeOutset(width, width);
}

void SkRenderer::redrawRubberRegion(Rect dirty_bounds, int left, int top) {
    SkRect rect = SkRect::MakeLTRB(dirty_bounds.left - left,
            dirty_bounds.top - top,
            dirty_bounds.right - left,
            dirty_bounds.bottom - top);

    SkPaint paint_src_over, paint_src;
    paint_src_over.setBlendMode(SkBlendMode::kSrcOver);
    paint_src.setBlendMode(SkBlendMode::kSrc);
    mBlendCanvas->drawImageRect(mBgBitmap->asImage(), rect, rect, SkSamplingOptions(), &paint_src, SkCanvas::kStrict_SrcRectConstraint);
    mBlendCanvas->drawImageRect(mPathBitmap->asImage(), rect, rect, SkSamplingOptions(), &paint_src_over, SkCanvas::kStrict_SrcRectConstraint);
    mArgbCanvas->drawImageRect(mBlendBitmap->asImage(), rect, rect, SkSamplingOptions(), &paint_src, SkCanvas::kStrict_SrcRectConstraint);

    mGray8Canvas->drawImageRect(mBlendBitmap->asImage(), rect, rect, SkSamplingOptions(), &paint_src, SkCanvas::kStrict_SrcRectConstraint);
}

void SkRenderer::clearCanvas() {
    mArgbCanvas->clear(SK_ColorWHITE);
    mGray8Canvas->clear(SK_ColorWHITE);

    mPathCanvas->clear(SK_AlphaTRANSPARENT);
    drawBackground();
}

void SkRenderer::setBackground(SkBitmap* bgBitmap) {
    if (bgBitmap != nullptr) {
        mBgCanvas->clear(SK_ColorWHITE);

        // TODO degrees is ok, fix tx, ty for rotate
        int degrees = 90;
        int tx = 96;
        int ty = 1404;
        mBgCanvas->save();
        mBgCanvas->translate(tx, ty);
        mBgCanvas->rotate(-degrees);
        //mBgCanvas->drawBitmap(*bgBitmap, 0 , 0, &mBgPaint);
        mBgCanvas->drawImage(bgBitmap->asImage(), 0, 0, SkSamplingOptions(), &mBgPaint);
        mBgCanvas->restore();
        //savePic("/data/system/bg2.png", mBgBitmap);
        delete bgBitmap;
    } else {
        mBgCanvas->clear(SK_ColorWHITE);
    }

    drawBackground();
}

void SkRenderer::drawBackground() {
    mArgbCanvas->clear(SK_ColorWHITE);
    mGray8Canvas->clear(SK_ColorWHITE);

    if (mBgBitmap != nullptr) {
        mArgbCanvas->drawImage(mBgBitmap->asImage(), 0, 0, SkSamplingOptions(), &mBgPaint);
        mArgbCanvas->drawImage(mPathBitmap->asImage(), 0, 0, SkSamplingOptions(), &mBgPaint);

        mGray8Canvas->drawImage(mBgBitmap->asImage(), 0, 0, SkSamplingOptions(), &mBgPaint);
        mGray8Canvas->drawImage(mPathBitmap->asImage(), 0, 0, SkSamplingOptions(), &mBgPaint);
    }
}

bool SkRenderer::savePic(const char* path, SkBitmap *bitmap) {
    if (!bitmap) {
        return false;
    }

    SkFILEWStream stream(path);
    bool ret = SkEncodeImage(&stream, *bitmap, SkEncodedImageFormat::kPNG, /* Quality ranges from 0..100 */ 100);
    if (!ret) {
        LOG(ERROR) << "savePic " << path << "failed " << strerror(errno);
    } else {
        LOG(INFO) << "savePic " << path << " success";
    }
    return  ret;
}

}
