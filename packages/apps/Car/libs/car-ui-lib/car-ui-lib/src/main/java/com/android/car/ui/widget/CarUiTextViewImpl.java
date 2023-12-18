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

package com.android.car.ui.widget;

import static java.util.Objects.requireNonNull;

import android.content.Context;
import android.text.Layout;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.OneShotPreDrawListener;

import com.android.car.ui.CarUiText;

import java.util.Collections;
import java.util.List;
import java.util.Scanner;

/**
 * Extension of {@link TextView} that supports {@link CarUiText}.
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public final class CarUiTextViewImpl extends CarUiTextView {

    @NonNull
    private List<CarUiText> mText = Collections.emptyList();
    private OneShotPreDrawListener mOneShotPreDrawListener;

    public CarUiTextViewImpl(Context context) {
        super(context);
    }

    public CarUiTextViewImpl(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public CarUiTextViewImpl(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public CarUiTextViewImpl(Context context, @Nullable AttributeSet attrs, int defStyleAttr,
                             int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    /**
     * Set text to display.
     *
     * @param textList list of text to display. Each {@link CarUiText} in the list will be rendered
     *                 on a new line, separated by a line break
     */
    @Override
    public void setText(@NonNull List<CarUiText> textList) {
        mText = requireNonNull(textList);
        if (mOneShotPreDrawListener == null) {
            mOneShotPreDrawListener = OneShotPreDrawListener.add(this, this::updateText);
        }
        setText(CarUiText.combineMultiLine(textList));
    }

    /**
     * Set text to display.
     */
    @Override
    public void setText(@NonNull CarUiText text) {
        mText = Collections.singletonList(requireNonNull(text));
        if (mOneShotPreDrawListener == null) {
            mOneShotPreDrawListener = OneShotPreDrawListener.add(this, this::updateText);
        }
        setText(text.getPreferredText());
    }

    private void updateText() {
        requireNonNull(mText);
        if (getLayout() == null) {
            mOneShotPreDrawListener = OneShotPreDrawListener.add(this, this::updateText);
            return;
        }

        mOneShotPreDrawListener = null;

        // If all lines of text have no limits, the preferred text set at invocation of
        // setText(List<CarUiText>)/ setText(CarUiText) does not need updating
        if (mText.stream().allMatch(line ->
                line.getMaxLines() == Integer.MAX_VALUE
                        && line.getMaxChars() == Integer.MAX_VALUE)) {
            return;
        }

        // Update rendered text if preferred text is truncated
        SpannableStringBuilder builder = new SpannableStringBuilder();
        CharSequence delimiter = "";
        for (int i = 0; i < mText.size(); i++) {
            CarUiText line = mText.get(i);
            builder.append(delimiter).append(getBestVariant(line));
            delimiter = "\n";
        }

        setText(builder);
    }

    private CharSequence getBestVariant(CarUiText text) {
        if (text.getTextVariants().size() > 1) {
            for (CharSequence variant : text.getTextVariants()) {
                if (variant.length() <= text.getMaxChars() && TextUtils.equals(variant,
                        getTruncatedText(variant, text.getMaxLines()))) {
                    return variant;
                }
            }
        }

        // If no text variant can be rendered without truncation, use the preferred text
        return getTruncatedText(text.getPreferredText(), text.getMaxLines());
    }

    private CharSequence getTruncatedText(CharSequence text, int maxLines) {
        Layout layout = requireNonNull(getLayout());
        int maxWidth = layout.getWidth();

        if (maxLines == 1) {
            return TextUtils.ellipsize(text, getPaint(), maxWidth, TextUtils.TruncateAt.END);
        }

        int lineCount = 0;
        int index = 0;
        int lastLineStart = 0;
        int length = text.length();
        boolean isTruncationComplete = false;

        while (!isTruncationComplete) {
            lastLineStart = index;
            // Measure the text, stopping early if the measured width exceeds textView width
            index += getPaint().breakText(text, index, length, true, maxWidth, null);

            // Break early if manual line break is present
            int lineBreak = TextUtils.indexOf(text, "\n", lastLineStart, index);
            if (lineBreak != -1) {
                index = Math.min(index, lineBreak + 1);
            }

            lineCount++;
            // Hitting maxLine limit or reaching the end of the CharSequence means truncation is
            // complete
            if (lineCount == maxLines || index > length - 1) {
                isTruncationComplete = true;
            }

            // Account for word wrapping by removing partial words at end of line by moving index
            // back to last whitespace character
            if (!isTruncationComplete && !Character.isWhitespace(text.charAt(index))) {
                int offset = 0;
                while (!Character.isWhitespace(text.charAt(index - offset - 1))) {
                    offset++;
                    // partial word reaches to the start of line, so it must be kept
                    if (index - offset == lastLineStart || offset >= index) {
                        offset = 0;
                        break;
                    }
                }

                index -= offset;
            }
        }

        SpannableStringBuilder builder = new SpannableStringBuilder();
        // Get text up until the last line
        builder.append(text.subSequence(0, lastLineStart));

        Scanner scanner = new Scanner(text.subSequence(lastLineStart, length).toString());
        if (scanner.hasNextLine()) {
            String lastLine = scanner.nextLine();
            // Add truncation ellipsis to last line if required
            builder.append(
                    TextUtils.ellipsize(lastLine, getPaint(), maxWidth, TextUtils.TruncateAt.END));
            if (text instanceof Spanned) {
                TextUtils.copySpansFrom(
                        (Spanned) text, 0, builder.length(), Object.class, builder, 0);
            }
        }

        return builder;
    }
}
