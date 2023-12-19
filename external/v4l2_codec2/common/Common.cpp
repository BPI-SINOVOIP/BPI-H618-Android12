// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <v4l2_codec2/common/Common.h>

#include <base/numerics/safe_math.h>

namespace android {

bool contains(const Rect& rect1, const Rect& rect2) {
    return (rect2.left >= rect1.left && rect2.right <= rect1.right && rect2.top >= rect1.top &&
            rect2.bottom <= rect1.bottom);
}

std::string toString(const Rect& rect) {
    return std::string("(") + std::to_string(rect.left) + "," + std::to_string(rect.top) + ") " +
           std::to_string(rect.width()) + "x" + std::to_string(rect.height());
}

std::optional<int> getArea(const ui::Size& size) {
    base::CheckedNumeric<int> checked_area = size.width;
    checked_area *= size.height;
    return checked_area.IsValid() ? std::optional<int>(checked_area.ValueOrDie()) : std::nullopt;
}

bool isEmpty(const ui::Size& size) {
    return !size.width || !size.height;
}

std::string toString(const ui::Size& size) {
    return std::to_string(size.width) + "x" + std::to_string(size.height);
}

}  // namespace android
