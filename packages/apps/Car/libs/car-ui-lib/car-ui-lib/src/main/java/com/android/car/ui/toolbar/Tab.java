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

import android.graphics.drawable.Drawable;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.List;
import java.util.function.Consumer;

/**
 * This is a data class the represents a tab in the toolbar. They can be added to the
 * toolbar via {@link ToolbarController#setTabs(List)}.
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public final class Tab {
    @Nullable
    private final String mText;
    @Nullable
    private final Drawable mIcon;
    @Nullable
    private final Consumer<Tab> mSelectedListener;

    private Tab(@NonNull Builder builder) {
        mText = builder.mText;
        mIcon = builder.mIcon;
        mSelectedListener = builder.mSelectedListener;
    }

    /** Gets the tab's text */
    @Nullable
    public String getText() {
        return mText;
    }

    /** Gets the tab's icon */
    @Nullable
    public Drawable getIcon() {
        return mIcon;
    }

    /** Gets the listener to call when the tab is selected */
    @Nullable
    public Consumer<Tab> getSelectedListener() {
        return mSelectedListener;
    }

    /** Creates a new {@link Builder} */
    public static Builder builder() {
        return new Builder();
    }

    /** Builder for {@link Tab} */
    public static class Builder {
        @Nullable
        private String mText = null;
        @Nullable
        private Drawable mIcon = null;
        @Nullable
        private Consumer<Tab> mSelectedListener = null;

        private Builder() {
        }

        /** Sets the tab's text */
        public Builder setText(String text) {
            mText = text;
            return this;
        }

        /** Sets the tab's icon */
        public Builder setIcon(Drawable icon) {
            mIcon = icon;
            return this;
        }

        /** Sets a listener that is called when the tab is selected */
        public Builder setSelectedListener(Consumer<Tab> callback) {
            mSelectedListener = callback;
            return this;
        }

        /** Builds the final {@link Tab} */
        public Tab build() {
            return new Tab(this);
        }
    }
}
