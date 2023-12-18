/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef ANDROID_GRAPHICS_BITMAP_H
#define ANDROID_GRAPHICS_BITMAP_H

#include <hwui/Bitmap.h>
#include <utils/Errors.h>
#include <utils/String16.h>

#include <binder/Parcelable.h>

namespace android {

namespace graphics {

/**
 * A simple holder for an SkBitmap, to match the managed-side
 * android.graphics.Bitmap parcelable behavior.
 *
 * This implements android/graphics/Bitmap.aidl
 *
 */
class Bitmap : public Parcelable {
  public:

    sk_sp<android::Bitmap> nativeBitmap;

    virtual status_t writeToParcel(Parcel* p) const override;
    virtual status_t readFromParcel(const Parcel* p) override;

};

} // namespace graphics
} // namespace android

#endif  // ANDROID_GRAPHICS_BITMAP_H
