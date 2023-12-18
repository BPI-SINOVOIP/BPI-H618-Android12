/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.car.ui.toolbar;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.widget.ImageView;

import java.util.function.Consumer;

@SuppressWarnings("AndroidJdkLibsChecker")
public final class ImageViewListener extends ImageView {

    private Consumer<Drawable> mImageDrawableListener;

    public ImageViewListener(Context context) {
        super(context);
    }

    @Override
    public void setImageDrawable(Drawable drawable) {
        super.setImageDrawable(drawable);

        if (mImageDrawableListener != null) {
            mImageDrawableListener.accept(drawable);
        }
    }

    public void setImageDrawableListener(Consumer<Drawable> imageDrawableListener) {
        mImageDrawableListener = imageDrawableListener;
    }
}
