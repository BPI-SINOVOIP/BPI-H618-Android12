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

package com.android.car.ui.sharedlibrary.oemapis.toolbar;

import android.graphics.drawable.Drawable;

/** Interface representing a toolbar tab */
public final class TabOEMV1 {
    private final String mTitle;
    private final Drawable mIcon;
    private final Runnable mOnSelectedListener;
    private final boolean mTinted;

    private TabOEMV1(Builder builder) {
        mTitle = builder.mTitle;
        mIcon = builder.mIcon;
        mOnSelectedListener = builder.mOnSelectedListener;
        mTinted = builder.mTinted;
    }

    /** Constructs a new {@link Builder} to build a tab with */
    public static Builder builder() {
        return new Builder();
    }

    /** Gets the title of the tab */
    public String getTitle() {
        return mTitle;
    }

    /** Gets the icon of the tab. The icon may be tinted to match the theme of the toolbar */
    public Drawable getIcon() {
        return mIcon;
    }

    /** Gets the function to call when the tab is selected */
    public Runnable getOnSelectedListener() {
        return mOnSelectedListener;
    }

    /**
     * Returns if the icon should be tinted to match the style of the toolbar.
     * Most of the time this will be true. If not, then the original colors of the drawable
     * should be shown.
     */
    public boolean isTinted() {
        return mTinted;
    }

    /** Builder for {@link TabOEMV1} */
    public static class Builder {
        private String mTitle = null;
        private Drawable mIcon = null;
        private Runnable mOnSelectedListener = null;
        private boolean mTinted = true;

        private Builder() {
        }

        /** Sets the tab's text */
        public Builder setTitle(String title) {
            mTitle = title;
            return this;
        }

        /** Sets the tab's icon */
        public Builder setIcon(Drawable icon) {
            mIcon = icon;
            return this;
        }

        /** Sets a listener that is called when the tab is selected */
        public Builder setOnSelectedListener(Runnable callback) {
            mOnSelectedListener = callback;
            return this;
        }

        /** See {@link TabOEMV1#isTinted} */
        public Builder setTinted(boolean tinted) {
            mTinted = tinted;
            return this;
        }

        /** Builds the final {@link TabOEMV1} */
        public TabOEMV1 build() {
            return new TabOEMV1(this);
        }
    }
}
