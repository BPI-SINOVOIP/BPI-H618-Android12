/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SUNXI_HWC_SCREEN_TRANSFORM_H_
#define SUNXI_HWC_SCREEN_TRANSFORM_H_

#include <algorithm>
#include <cmath>
#include <string>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <math.h>
#include <unistd.h>

#include "clz.h"
#include "Rect.h"

namespace details {

class Vec3 {
public:
    explicit Vec3() { v[0] = v[1] = v[2] = 0.0f; }
    Vec3(float x, float y, float z) {
        v[0] = x;
        v[1] = y;
        v[2] = z;
    }
    Vec3(const Vec3&) = default;
    ~Vec3() = default;
    Vec3& operator = (const Vec3&) = default;

    inline float& operator[](size_t i) {
        assert(i < 3);
        return v[i];
    }

    inline float const& operator[](size_t i) const {
        assert(i < 3);
        return v[i];
    }

private:
    float v[3];
};

class Matrix {
public:
    Matrix() { reset(); }

    // reset to identity matrix
    void reset() {
        for (int i = 0; i < 3; i++) {
            Vec3& v = m[i];
            for (int j = 0; j < 3; j++)
                v[j] = (i == j) ? 1.0f : 0.0f;
        }
    }

    void setTranslation(float tx, float ty) {
        m[2][0] = tx;
        m[2][1] = ty;
        m[2][2] = 1.0f;
    }

    void setScale(float sx, float sy) {
        m[0][0] = sx;
        m[1][1] = sy;
    }

    Vec3 transform(const Vec3& v) const {
        Vec3 r;
        r[0] = m[0][0]*v[0] + m[1][0]*v[1] + m[2][0]*v[2];
        r[1] = m[0][1]*v[0] + m[1][1]*v[1] + m[2][1]*v[2];
        r[2] = m[0][2]*v[0] + m[1][2]*v[1] + m[2][2]*v[2];
        return r;
    }

    FloatRect transform(const Rect& bounds) const {
        Vec3 lt(bounds.left,  bounds.top,    1.0f);
        Vec3 rt(bounds.right, bounds.top,    1.0f);
        Vec3 lb(bounds.left,  bounds.bottom, 1.0f);
        Vec3 rb(bounds.right, bounds.bottom, 1.0f);

        lt = transform(lt);
        rt = transform(rt);
        lb = transform(lb);
        rb = transform(rb);

        FloatRect r;
        r.left   = sunxi::min(lt[0], rt[0], lb[0], rb[0]);
        r.top    = sunxi::min(lt[1], rt[1], lb[1], rb[1]);
        r.right  = sunxi::max(lt[0], rt[0], lb[0], rb[0]);
        r.bottom = sunxi::max(lt[1], rt[1], lb[1], rb[1]);
        return r;
    }

    FloatRect transform(const hwc_rect_t& bounds) const {
        Rect r = bounds;
        return transform(r);
    }

private:
    Vec3 m[3];
};
} // namespace details

typedef details::Matrix Matrix;

namespace sunxi {

// Transform display frame from framebuffer to screen coordinate
class ScreenTransform {
public:
    void setFramebufferSize(int width, int height) {
        if (mFramebufferBounds.width() != width
                || mFramebufferBounds.height() != height) {
            mFramebufferBounds = Rect(width, height);
            mDirty = true;
        }
    }
    void setScreenSize(int width, int height) {
        if (mScreenBounds.width() != width
                || mScreenBounds.height() != height) {
            mScreenBounds = Rect(width, height);
            mDirty = true;
        }
    }
    void setScale(float sx, float sy) {
        if (mScaleX != sx || mScaleY != sy) {
            mScaleX = sx;
            mScaleY = sy;
            mDirty = true;
        }
    }

    void update() {
        if (mDirty) {
            mDirty = false;
            updateTransformMatrix();
        }
    }

    Rect transform(const Rect& bounds) const {
        Rect result;
        FloatRect rect = mTransform.transform(bounds);
        result.left   = std::max(0, (int)std::floor(rect.left));
        result.top    = std::max(0, (int)std::floor(rect.top ));
        result.right  = std::min(mScreenBounds.width(),  (int)std::ceil(rect.right ));
        result.bottom = std::min(mScreenBounds.height(), (int)std::ceil(rect.bottom));
        return result;
    }

    const Rect& getFramebufferSize() const { return mFramebufferBounds; }
    const Rect& getScreenSize() const { return mScreenBounds; }
    void getScale(float *x, float *y) const {
        *x = mScaleX;
        *y = mScaleX;
    }

    std::string toString() {
        std::ostringstream debugstr;
        debugstr << "FrameBufferBounds[" << mFramebufferBounds.width() << ", " << mFramebufferBounds.height() << "] "
                 << "ScreenBounds[" << mScreenBounds.width() << ", " << mScreenBounds.height() << "] "
                 << "Scale[" << mScaleX << ", " << mScaleY << "]" << std::endl;
        return debugstr.str();
    }

#define DEFAULT_W 1280
#define DEFAULT_H 720

    ScreenTransform()
        : mDirty(true),
          mFramebufferBounds(DEFAULT_W, DEFAULT_H),
          mScreenBounds(DEFAULT_W, DEFAULT_H),
          mScaleX(1.0f),
          mScaleY(1.0f) { }

private:
    void updateTransformMatrix() {
        float px = mScreenBounds.width();
        float py = mScreenBounds.height();
        float fx = mFramebufferBounds.width();
        float fy = mFramebufferBounds.height();

        float sx = (px / fx) * mScaleX;
        float sy = (py / fy) * mScaleY;

        mTransform = Matrix();
        mTransform.setScale(sx, sy);

        float tx = mScreenBounds.width()  * (1.0f - mScaleX) / 2.0f;
        float ty = mScreenBounds.height() * (1.0f - mScaleY) / 2.0f;
        mTransform.setTranslation(tx, ty);
    }

    bool mDirty;
    Rect mFramebufferBounds;
    Rect mScreenBounds;
    float mScaleX;  // scale factor at x direction
    float mScaleY;  // scale factor at y direction

    Matrix mTransform;
};

} // namespace sunxi

#endif
