/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.car.messenger.core.ui.shared;

import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.R;

/**
 * A drawable that encapsulates all the functionality needed to display a letter tile to represent a
 * contact image.
 */
@SuppressWarnings("StaticAssignmentInConstructor")
public class LetterTileDrawable extends Drawable {
    /** Letter tile */
    @NonNull private static int[] sColors;

    private static int sDefaultColor;
    private static int sTileFontColor;
    private static float sLetterToTileRatio;
    @NonNull private static Drawable sDefaultPersonAvatar;
    @NonNull private static Drawable sDefaultBusinessAvatar;
    @NonNull private static Drawable sDefaultVoicemailAvatar;

    /** Reusable components to avoid new allocations */
    @NonNull private static final Paint sPaint = new Paint();

    @NonNull private static final Rect sRect = new Rect();

    /** Contact type constants */
    public static final int TYPE_PERSON = 1;

    public static final int TYPE_BUSINESS = 2;
    public static final int TYPE_VOICEMAIL = 3;
    public static final int TYPE_DEFAULT = TYPE_PERSON;

    @NonNull private final Paint mPaint;

    @Nullable private String mLetters;
    private int mColor;
    private int mContactType = TYPE_DEFAULT;
    private float mScale = 1.0f;
    private float mOffset = 0.0f;
    private boolean mIsCircle = false;

    /** A custom Drawable that draws letters on a colored background. */
    // The use pattern for this constructor is:
    // create LTD, setContactDetails(), and setIsCircular(true) if needed.
    public LetterTileDrawable(@NonNull final Resources res) {
        this(res, null, null);
    }

    /** A custom Drawable that draws letters on a colored background. */
    // This constructor allows passing the letters and identifier directly. There is no need to
    // call setContactDetails() again. setIsCircular(true) needs to be called separately if needed.
    public LetterTileDrawable(
            @NonNull final Resources res, @Nullable String letters, @Nullable String identifier) {
        mPaint = new Paint();
        mPaint.setFilterBitmap(true);
        mPaint.setDither(true);
        setScale(0.7f);

        if (sColors == null) {
            sDefaultColor = res.getColor(R.color.letter_tile_default_color, null /* theme */);
            TypedArray ta = res.obtainTypedArray(R.array.letter_tile_colors);
            if (ta.length() == 0) {
                // TODO(b/26518438). Looks like robolectric shadow doesn't currently support
                // obtainTypedArray and always returns length 0 array, which will make some code
                // below that does a division by length of sColors choke. Workaround by creating
                // an array of length 1.
                sColors = new int[] {sDefaultColor};

            } else {
                sColors = new int[ta.length()];
                for (int i = ta.length() - 1; i >= 0; i--) {
                    sColors[i] = ta.getColor(i, sDefaultColor);
                }
                ta.recycle();
            }

            sTileFontColor = res.getColor(R.color.letter_tile_font_color, null /* theme */);
            sLetterToTileRatio = res.getFraction(R.fraction.letter_to_tile_ratio, 1, 1);
            // TODO: get images for business and voicemail
            sDefaultPersonAvatar = res.getDrawable(R.drawable.ic_person, null /* theme */);
            sDefaultBusinessAvatar = res.getDrawable(R.drawable.ic_person, null /* theme */);
            sDefaultVoicemailAvatar = res.getDrawable(R.drawable.ic_person, null /* theme */);
            sPaint.setTypeface(
                    Typeface.create(
                            res.getString(R.string.config_letter_tile_font_family),
                            res.getInteger(R.integer.config_letter_tile_text_style)));
            sPaint.setTextAlign(Align.CENTER);
            sPaint.setAntiAlias(true);
        }

        setContactDetails(letters, identifier);
    }

    @Override
    public void draw(@NonNull final Canvas canvas) {
        final Rect bounds = getBounds();
        if (!isVisible() || bounds.isEmpty()) {
            return;
        }
        // Draw letter tile.
        drawLetterTile(canvas);
    }

    /**
     * Draw the drawable onto the canvas at the current bounds taking into account the current
     * scale.
     */
    private void drawDrawableOnCanvas(final Drawable drawable, @NonNull final Canvas canvas) {
        // The drawable should be drawn in the middle of the canvas without changing its width to
        // height ratio.
        final Rect destRect = copyBounds();

        // Crop the destination bounds into a square, scaled and offset as appropriate
        final int halfLength = (int) (mScale * Math.min(destRect.width(), destRect.height()) / 2);

        destRect.set(
                destRect.centerX() - halfLength,
                (int) (destRect.centerY() - halfLength + mOffset * destRect.height()),
                destRect.centerX() + halfLength,
                (int) (destRect.centerY() + halfLength + mOffset * destRect.height()));

        drawable.setAlpha(mPaint.getAlpha());
        drawable.setColorFilter(sTileFontColor, PorterDuff.Mode.SRC_IN);
        drawable.setBounds(destRect);
        drawable.draw(canvas);
    }

    private void drawLetterTile(@NonNull final Canvas canvas) {
        // Draw background color.
        sPaint.setColor(mColor);

        sPaint.setAlpha(mPaint.getAlpha());
        final Rect bounds = getBounds();
        final int minDimension = Math.min(bounds.width(), bounds.height());

        if (mIsCircle) {
            canvas.drawCircle(bounds.centerX(), bounds.centerY(), minDimension / 2, sPaint);
        } else {
            canvas.drawRect(bounds, sPaint);
        }

        if (!TextUtils.isEmpty(mLetters)) {
            // Scale text by canvas bounds and user selected scaling factor
            sPaint.setTextSize(mScale * sLetterToTileRatio * minDimension);
            // sPaint.setTextSize(sTileLetterFontSize);
            sPaint.getTextBounds(mLetters, 0, mLetters.length(), sRect);
            sPaint.setColor(sTileFontColor);

            // Draw the letter in the canvas, vertically shifted up or down by the user-defined
            // offset
            canvas.drawText(
                    mLetters,
                    0,
                    mLetters.length(),
                    bounds.centerX(),
                    bounds.centerY() + mOffset * bounds.height() + sRect.height() / 2,
                    sPaint);
        } else {
            // Draw the default image if there is no letter/digit to be drawn
            final Drawable drawable = getDrawablepForContactType(mContactType);
            drawDrawableOnCanvas(drawable, canvas);
        }
    }

    public int getColor() {
        return mColor;
    }

    /** Returns a deterministic color based on the provided contact identifier string. */
    private int pickColor(@Nullable final String identifier) {
        if (TextUtils.isEmpty(identifier) || mContactType == TYPE_VOICEMAIL) {
            return sDefaultColor;
        }
        // String.hashCode() implementation is not supposed to change across java versions, so
        // this should guarantee the same email address always maps to the same color.
        // The email should already have been normalized by the ContactRequest.
        final int color = Math.abs(identifier.hashCode()) % sColors.length;
        return sColors[color];
    }

    @NonNull
    private static Drawable getDrawablepForContactType(int contactType) {
        switch (contactType) {
            case TYPE_BUSINESS:
                return sDefaultBusinessAvatar;
            case TYPE_VOICEMAIL:
                return sDefaultVoicemailAvatar;
            case TYPE_PERSON:
            default:
                return sDefaultPersonAvatar;
        }
    }

    @Override
    public void setAlpha(final int alpha) {
        mPaint.setAlpha(alpha);
    }

    @Override
    public void setColorFilter(@NonNull final ColorFilter cf) {
        mPaint.setColorFilter(cf);
    }

    @Override
    public int getOpacity() {
        return PixelFormat.OPAQUE;
    }

    /**
     * Scale the drawn letter tile to a ratio of its default size
     *
     * @param scale The ratio the letter tile should be scaled to as a percentage of its default
     *     size, from a scale of 0 to 2.0f. The default is 1.0f.
     */
    public void setScale(float scale) {
        mScale = scale;
    }

    /**
     * Assigns the vertical offset of the position of the letter tile to the ContactDrawable
     *
     * @param offset The provided offset must be within the range of -0.5f to 0.5f. If set to -0.5f,
     *     the letter will be shifted upwards by 0.5 times the height of the canvas it is being
     *     drawn on, which means it will be drawn with the center of the letter starting at the top
     *     edge of the canvas. If set to 0.5f, the letter will be shifted downwards by 0.5 times the
     *     height of the canvas it is being drawn on, which means it will be drawn with the center
     *     of the letter starting at the bottom edge of the canvas. The default is 0.0f.
     */
    public void setOffset(float offset) {
        mOffset = offset;
    }

    /**
     * Sets the details.
     *
     * @param letters The letters need to be drawn
     * @param identifier decides the color for the drawable.
     */
    public void setContactDetails(@Nullable String letters, @Nullable String identifier) {
        mLetters = letters;
        mColor = pickColor(identifier);
    }

    public void setContactType(int contactType) {
        mContactType = contactType;
    }

    public void setIsCircular(boolean isCircle) {
        mIsCircle = isCircle;
    }

    /**
     * Convert the drawable to a bitmap.
     *
     * @param size The target size of the bitmap.
     * @return A bitmap representation of the drawable.
     */
    @NonNull
    public Bitmap toBitmap(int size) {
        Bitmap largeIcon = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(largeIcon);
        Rect bounds = getBounds();
        setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
        draw(canvas);
        setBounds(bounds);
        return largeIcon;
    }
}
