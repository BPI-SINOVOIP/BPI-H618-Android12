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
package com.android.car.ui.sharedlibrary.oemapis.recyclerview;

import android.text.SpannableString;

/**
 * The OEM interface for header list item.
 */
public final class HeaderListItemOEMV1 implements ListItemOEMV1 {
    private final SpannableString mTitle;
    private final SpannableString mBody;

    HeaderListItemOEMV1(Builder builder) {
        mTitle = builder.mTitle;
        mBody = builder.mBody;
    }

    /**
     * Returns the title text for the header.
     */
    public SpannableString getTitle() {
        return mTitle;
    }

    /**
     * Returns the body text for the header.
     */
    public SpannableString getBody() {
        return mBody;
    }

    /**
     * A builder of {@link HeaderListItemOEMV1}.
     */
    public static final class Builder {
        private final SpannableString mTitle;
        private SpannableString mBody;

        /**
         * Returns a new instance of a {@link Builder}.
         *
         * @param title text to display as title
         */
        public Builder(SpannableString title) {
            mTitle = title;
        }

        /**
         * Sets the body of the item.
         *
         * @param body text to display as body text.
         */
        public Builder setBody(SpannableString body) {
            mBody = body;
            return this;
        }

        /**
         * Returns a {@link HeaderListItemOEMV1} for this {@link ContentListItemOEMV1.Builder}.
         */
        public HeaderListItemOEMV1 build() {
            return new HeaderListItemOEMV1(this);
        }
    }
}
