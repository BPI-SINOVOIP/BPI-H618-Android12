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

#ifndef SUNXI_HWC_RECT_H_
#define SUNXI_HWC_RECT_H_

#include <unistd.h>
#include <hardware/hwcomposer2.h>

/* Rectangular window area. */
class Rect: public hwc_rect {
public:

    inline Rect() {
        left = top = 0;
        right = bottom = -1;
    }

    inline Rect(int w, int h) {
        left = top = 0;
        right = w;
        bottom = h;
    }

    inline Rect(int l, int t, int r, int b) {
        left = l;
        top = t;
        right = r;
        bottom = b;
    }

    inline Rect(const hwc_rect& rhs) {
        left = rhs.left;
        top = rhs.top;
        right = rhs.right;
        bottom = rhs.bottom;
    }

    inline void clear() {
        left = top = right = bottom = 0;
    }

    inline int width() const {
        return (right - left);
    }

    inline int height() const {
        return (bottom - top);
    }

    // a valid rectangle has a non negative width and height
    inline bool isValid() const {
        return (width()>=0) && (height()>=0);
    }
};

typedef hwc_frect FloatRect;

#endif

