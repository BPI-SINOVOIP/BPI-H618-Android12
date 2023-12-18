/*
 * Copyright (C) 2021 The Android Open Source Project
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

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.function.Consumer;

/**
 * Do not use, this class is only public so that it can be accessed from a different classloader.
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public class DeprecatedTabWrapper {
    private final TabLayout.Tab mTab;
    private final Consumer<Tab> mOnSelectedListener;
    private String mTitle = "";
    private Drawable mIcon = null;

    public DeprecatedTabWrapper(
            @NonNull Context context,
            @NonNull TabLayout.Tab tab,
            @Nullable Runnable updateListener,
            @Nullable Consumer<Tab> onSelectedListener) {
        mTab = tab;
        mOnSelectedListener = onSelectedListener;

        TextViewListener textViewListener = new TextViewListener(context);
        textViewListener.setTextListener(str -> {
            mTitle = str == null ? "" : str.toString();
            if (updateListener != null) {
                updateListener.run();
            }
        });
        ImageViewListener imageViewListener = new ImageViewListener(context);
        imageViewListener.setImageDrawableListener(icon -> {
            mIcon = icon;
            if (updateListener != null) {
                updateListener.run();
            }
        });

        tab.bindIcon(imageViewListener);
        tab.bindText(textViewListener);
    }

    /**
     * Gets the underlying {@link TabLayout.Tab}
     */
    public TabLayout.Tab getDeprecatedTab() {
        return mTab;
    }

    /**
     * Converts the underlying {@link TabLayout.Tab} into a {@link Tab} and returns it.
     */
    public Tab getModernTab() {
        return Tab.builder()
                .setText(mTitle)
                .setIcon(mIcon)
                .setSelectedListener(mOnSelectedListener)
                .build();
    }
}
