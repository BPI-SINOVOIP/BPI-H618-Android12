/*
 * Copyright 2019 The Android Open Source Project
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
package com.android.car.ui;

import android.text.SpannableStringBuilder;

import androidx.annotation.NonNull;

import java.util.Collections;
import java.util.List;

/**
 * Text that includes rendering information.
 */
public class CarUiText {
    private final int mMaxLines;
    private final int mMaxChars;
    private final List<CharSequence> mVariants;

    /**
     * Convenience method that returns a single {@link CharSequence} that is a combination of the
     * preferred text of a list of {@link CarUiText}, separated by line breaks.
     */
    public static CharSequence combineMultiLine(@NonNull List<CarUiText> lines) {
        SpannableStringBuilder builder = new SpannableStringBuilder();
        CharSequence delimiter = "";
        for (CarUiText line : lines) {
            builder.append(delimiter)
                    .append(line.getPreferredText() == null ? " " : line.getPreferredText());
            delimiter = "\n";
        }
        return builder;
    }

    private CarUiText(Builder builder) {
        mVariants = builder.mVariants;
        mMaxChars = builder.mMaxChars;
        mMaxLines = builder.mMaxLines;
    }

    /**
     * Create a new {@link CarUiText}.
     *
     * @param text text to display
     *
     * @deprecated Use {@link Builder} to instantiate {@link CarUiText} instances
     */
    @Deprecated
    public CarUiText(@NonNull CharSequence text) {
        this(text, Integer.MAX_VALUE);
    }

    /**
     * Create a new {@link CarUiText}.
     *
     * @param text     text to display
     * @param maxLines the maximum number of lines the text should be displayed on when width
     *                 constraints force the text to be wrapped. Text that exceeds the maximum
     *                 number of lines is ellipsized
     *
     * @deprecated Use {@link Builder} to instantiate {@link CarUiText} instances
     */
    @Deprecated
    public CarUiText(@NonNull CharSequence text, int maxLines) {
        mVariants = Collections.singletonList(text);
        mMaxLines = maxLines;
        mMaxChars = Integer.MAX_VALUE;
    }

    /**
     * Create a new {@link CarUiText}.
     *
     * @param variants list of text variants. Variants provide alternative text to be used to avoid
     *                 truncation. Provide variants in order of preference.
     * @param maxLines the maximum number of lines the text should be displayed on when width
     *                 constraints force the text to be wrapped. Text that exceeds the maximum
     *                 number of lines is ellipsized
     *
     * @deprecated Use {@link Builder} to instantiate {@link CarUiText} instances
     */
    @Deprecated
    public CarUiText(@NonNull List<CharSequence> variants, int maxLines) {
        mVariants = variants;
        mMaxLines = maxLines;
        mMaxChars = Integer.MAX_VALUE;
    }

    /**
     * Create a new {@link CarUiText}.
     *
     * @param variants list of text variants. Variants provide alternative text to be used to avoid
     *                 truncation. Provide variants in order of preference.
     * @param maxLines the maximum number of lines the text should be displayed on when width
     *                 constraints force the text to be wrapped. Text that exceeds the maximum
     *                 number of lines is ellipsized
     * @param maxChars the maximum number of characters that should be displayed
     *
     * @deprecated Use {@link Builder} to instantiate {@link CarUiText} instances
     */
    @Deprecated
    public CarUiText(@NonNull List<CharSequence> variants, int maxLines, int maxChars) {
        mVariants = variants;
        mMaxLines = maxLines;
        mMaxChars = maxChars;
    }

    /**
     * Returns the maximum number of lines the text should be displayed on when width constraints
     * force the text to be wrapped
     */
    public int getMaxLines() {
        return mMaxLines;
    }

    /**
     * Returns the maximum number of characters that should be displayed for the text
     */
    public int getMaxChars() {
        return mMaxChars;
    }

    /**
     * Returns the list of text variants for this {@link CarUiText}.
     */
    public List<CharSequence> getTextVariants() {
        return mVariants;
    }

    /**
     * Returns the preferred text to render for this {@link CarUiText}.
     */
    public CharSequence getPreferredText() {
        return mVariants.get(0);
    }

    /**
     * A builder of {@link CarUiText}.
     */
    public static final class Builder {
        private int mMaxLines = Integer.MAX_VALUE;
        private int mMaxChars = Integer.MAX_VALUE;
        private final List<CharSequence> mVariants;

        /**
         * Returns a new instance of a {@link Builder}.
         *
         * @param text text to display
         */
        public Builder(CharSequence text) {
            this(Collections.singletonList(text));
        }

        /**
         * Returns a new instance of a {@link Builder}.
         *
         * @param variants list of text variants. Variants provide alternative text to be used to
         *                 avoid truncation. Provide variants in order of preference.
         */
        public Builder(@NonNull List<CharSequence> variants) {
            mVariants = variants;
        }

        /**
         * Sets the maximum number of characters that should be displayed.
         */
        public Builder setMaxChars(int chars) {
            mMaxChars = chars;
            return this;
        }

        /**
         * Sets the maximum number of lines the text should be displayed on when width constraints
         * force the text to be wrapped. Text that exceeds the maximum number of lines is
         * ellipsized.
         */
        public Builder setMaxLines(int lines) {
            mMaxLines = lines;
            return this;
        }

        /**
         * Returns a {@link CarUiText} for this {@link Builder}.
         */
        public CarUiText build() {
            return new CarUiText(this);
        }
    }
}
