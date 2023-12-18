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

package com.android.managedprovisioning.provisioning;

import android.annotation.DrawableRes;
import android.annotation.RawRes;
import android.annotation.StringRes;

/**
 * A wrapper describing the contents of an education screen.
 */
final class TransitionScreenWrapper {
    public final @StringRes int header;
    public final @StringRes int description;
    public final @RawRes int drawable;
    public final @StringRes int subHeaderTitle;
    public final @StringRes int subHeader;
    public final @DrawableRes int subHeaderIcon;
    public final boolean shouldLoop;
    public final @StringRes int secondarySubHeaderTitle;
    public final @StringRes int secondarySubHeader;
    public final @DrawableRes int secondarySubHeaderIcon;

    TransitionScreenWrapper(@StringRes int header, @RawRes int drawable) {
        this(header, /* description= */ 0, drawable, /* shouldLoop */ true);
    }

    TransitionScreenWrapper(@StringRes int header, @StringRes int description,
            @RawRes int drawable, boolean shouldLoop) {
        this(header, /* description= */ description, drawable, 0, 0, 0, shouldLoop, 0, 0, 0);
    }

    private TransitionScreenWrapper(int header, int description, int drawable, int subHeaderTitle,
            int subHeader, int subHeaderIcon, boolean shouldLoop,
            int secondarySubHeaderTitle, int secondarySubHeader, int secondarySubHeaderIcon) {
        this.header = header;
        this.description = description;
        this.drawable = drawable;
        this.subHeaderTitle = subHeaderTitle;
        this.subHeader = subHeader;
        this.subHeaderIcon = subHeaderIcon;
        this.shouldLoop = shouldLoop;
        this.secondarySubHeaderTitle = secondarySubHeaderTitle;
        this.secondarySubHeader = secondarySubHeader;
        this.secondarySubHeaderIcon = secondarySubHeaderIcon;

        validateFields();
    }

    private void validateFields() {
        final boolean isItemProvided =
                subHeader != 0
                        || subHeaderIcon != 0
                        || subHeaderTitle != 0
                        || secondarySubHeader != 0
                        || secondarySubHeaderIcon != 0
                        || secondarySubHeaderTitle != 0;
        if (isItemProvided && drawable != 0) {
            throw new IllegalArgumentException(
                    "Cannot show items and animation at the same time.");
        }
        if (header == 0) {
            throw new IllegalArgumentException("Header resource id must be a positive number.");
        }
    }

    public static final class Builder {
        @StringRes int mHeader;
        @StringRes int mDescription;
        @RawRes int mDrawable;
        @StringRes private int mSubHeaderTitle;
        @StringRes int mSubHeader;
        @DrawableRes int mSubHeaderIcon;
        boolean mShouldLoop;
        @StringRes int mSecondarySubHeaderTitle;
        @StringRes int mSecondarySubHeader;
        @DrawableRes int mSecondarySubHeaderIcon;

        public Builder setHeader(int header) {
            mHeader = header;
            return this;
        }

        public Builder setDescription(int description) {
            mDescription = description;
            return this;
        }

        public Builder setAnimation(int drawable) {
            mDrawable = drawable;
            return this;
        }

        public Builder setSubHeaderTitle(int subHeaderTitle) {
            mSubHeaderTitle = subHeaderTitle;
            return this;
        }

        public Builder setSubHeader(int subHeader) {
            mSubHeader = subHeader;
            return this;
        }

        public Builder setSubHeaderIcon(int subHeaderIcon) {
            mSubHeaderIcon = subHeaderIcon;
            return this;
        }

        public Builder setShouldLoop(boolean shouldLoop) {
            mShouldLoop = shouldLoop;
            return this;
        }

        public Builder setSecondarySubHeaderTitle(int secondarySubHeaderTitle) {
            mSecondarySubHeaderTitle = secondarySubHeaderTitle;
            return this;
        }

        public Builder setSecondarySubHeader(int secondarySubHeader) {
            mSecondarySubHeader = secondarySubHeader;
            return this;
        }

        public Builder setSecondarySubHeaderIcon(int secondarySubHeaderIcon) {
            mSecondarySubHeaderIcon = secondarySubHeaderIcon;
            return this;
        }

        public TransitionScreenWrapper build() {
            return new TransitionScreenWrapper(mHeader, mDescription, mDrawable, mSubHeaderTitle,
                    mSubHeader, mSubHeaderIcon, mShouldLoop, mSecondarySubHeaderTitle,
                    mSecondarySubHeader, mSecondarySubHeaderIcon);
        }
    }
}
