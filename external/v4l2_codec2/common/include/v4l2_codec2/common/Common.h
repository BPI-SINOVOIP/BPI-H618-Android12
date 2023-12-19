// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_V4L2_CODEC2_COMMON_COMMON_H
#define ANDROID_V4L2_CODEC2_COMMON_COMMON_H

#include <inttypes.h>

#include <optional>
#include <string>
#include <vector>

#include <ui/Rect.h>
#include <ui/Size.h>

#include <v4l2_codec2/common/VideoPixelFormat.h>

namespace android {

// The stride, offset and size of a video frame plane.
struct VideoFramePlane {
    uint32_t mStride = 0;
    size_t mOffset = 0;
    size_t mSize = 0;
};

// A video frame's layout, containing pixel format, size and layout of individual planes.
struct VideoFrameLayout {
    VideoPixelFormat mFormat = VideoPixelFormat::UNKNOWN;
    android::ui::Size mCodedSize;
    std::vector<VideoFramePlane> mPlanes;
    bool mMultiPlanar = false;
};

// Check whether |rect1| completely contains |rect2|.
bool contains(const Rect& rect1, const Rect& rect2);

// Convert the specified |rect| to a string.
std::string toString(const Rect& rect);

// Get the area encapsulated by the |size|. Returns nullopt if multiplication causes overflow.
std::optional<int> getArea(const ui::Size& size);

// Check whether the specified |size| is empty
bool isEmpty(const ui::Size& size);

// Convert the specified |size| to a string.
std::string toString(const ui::Size& size);

}  // namespace android

#endif  // ANDROID_V4L2_CODEC2_COMMON_COMMON_H
