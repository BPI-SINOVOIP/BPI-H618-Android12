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

package com.android.car.messenger.impl.datamodels.util;

import static java.lang.Math.min;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Shader.TileMode;
import android.text.TextUtils;

import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.messenger.R;
import com.android.car.messenger.core.ui.shared.LetterTileDrawable;

import java.util.List;

/**
 * Avatar Utils for generating conversation and contact avatars
 *
 * <p>For historical context, AvatarUtil is derived from Android Messages implementation of group
 * avatars particularly from these sources:
 *
 * <p>AvatarGroupRequestDescriptor#generateDestRectArray:
 * packages/apps/Messaging/src/com/android/messaging/datamodel/media/AvatarGroupRequestDescriptor
 *
 * <p>CompositeImageRequest#loadMediaInternal:
 * packages/apps/Messaging/src/com/android/messaging/datamodel/media/CompositeImageRequest
 *
 * <p>ImageUtils#drawBitmapWithCircleOnCanvas:
 * packages/apps/Messaging/src/com/android/messaging/util/ImageUtils.java
 *
 * <p>Current implementation is close to reference. However, future iterations can diverge.
 */
public final class AvatarUtil {

    private AvatarUtil() {}

    private static final class GroupAvatarConfigs {
        int mWidth;
        int mHeight;
        int mMaximumGroupSize;
        int mBackgroundColor;
        int mStrokeColor;
        boolean mFillBackground;
    }

    /**
     * Supports creating a group avatar: a minimum of 1 avatar and a maximum of four avatars are
     * supported. Any avatars beyond the 4th index is ignored.
     */
    @Nullable
    public static Bitmap createGroupAvatar(
            @NonNull Context context, @Nullable List<Bitmap> participantsIcon) {
        if (participantsIcon == null || participantsIcon.isEmpty()) {
            return null;
        }

        GroupAvatarConfigs groupAvatarConfigs = new GroupAvatarConfigs();
        Resources resources = context.getResources();
        int size = resources.getDimensionPixelSize(R.dimen.conversation_avatar_width);
        groupAvatarConfigs.mWidth = size;
        groupAvatarConfigs.mHeight = size;
        groupAvatarConfigs.mMaximumGroupSize =
                resources.getInteger(R.integer.group_avatar_max_group_size);
        groupAvatarConfigs.mBackgroundColor =
                resources.getColor(R.color.group_avatar_background_color, context.getTheme());
        groupAvatarConfigs.mStrokeColor =
                resources.getColor(R.color.group_avatar_stroke_color, context.getTheme());
        groupAvatarConfigs.mFillBackground =
                context.getResources().getBoolean(R.bool.group_avatar_fill_background);

        if (participantsIcon.size() == 1 || groupAvatarConfigs.mMaximumGroupSize == 1) {
            return participantsIcon.get(0);
        }

        return createGroupAvatarBitmap(participantsIcon, groupAvatarConfigs);
    }

    /**
     * Resolves person avatar to either the provided bitmap clipped into a circle or a letter
     * drawable
     */
    @Nullable
    public static Bitmap resolvePersonAvatar(
            @NonNull Context context, @Nullable Bitmap bitmap, @Nullable CharSequence name) {
        if (bitmap != null) {
            return AvatarUtil.createClippedCircle(bitmap);
        } else {
            return createLetterTile(context, name);
        }
    }

    /**
     * Create a {@link Bitmap} for the given name
     *
     * @param name will decide the color for the drawable. If null, a default color will be used.
     */
    @Nullable
    private static Bitmap createLetterTile(@NonNull Context context, @Nullable CharSequence name) {
        if (TextUtils.isEmpty(name)) {
            return null;
        }
        char firstInitial = name.charAt(0);
        String letters = Character.isLetter(firstInitial) ? Character.toString(firstInitial) : null;
        LetterTileDrawable drawable =
                new LetterTileDrawable(context.getResources(), letters, name.toString());
        int size = context.getResources().getDimensionPixelSize(R.dimen.conversation_avatar_width);
        return drawable.toBitmap(size);
    }

    /** Returns a circle-clipped bitmap */
    @NonNull
    private static Bitmap createClippedCircle(Bitmap bitmap) {
        final int width = bitmap.getWidth();
        final int height = bitmap.getHeight();
        final Bitmap outputBitmap = Bitmap.createBitmap(width, height, Config.ARGB_8888);

        final Path path = new Path();
        path.addCircle(
                (float) (width / 2),
                (float) (height / 2),
                (float) min(width, (height / 2)),
                Path.Direction.CCW);

        final Canvas canvas = new Canvas(outputBitmap);
        canvas.clipPath(path);
        canvas.drawBitmap(bitmap, 0, 0, null);
        return outputBitmap;
    }

    /** Creates a group avatar bitmap */
    @NonNull
    private static Bitmap createGroupAvatarBitmap(
            @NonNull List<Bitmap> participantsIcon, GroupAvatarConfigs groupAvatarConfigs) {
        int width = groupAvatarConfigs.mWidth;
        int height = groupAvatarConfigs.mHeight;
        Bitmap bitmap = createOrReuseBitmap(width, height, Color.TRANSPARENT);
        Canvas canvas = new Canvas(bitmap);
        RectF[] rect =
                generateDestRectArray(
                        width,
                        height,
                        /* cropToCircle= */ true,
                        min(participantsIcon.size(), groupAvatarConfigs.mMaximumGroupSize));

        for (int i = 0; i < rect.length; i++) {
            RectF avatarDestOnGroup = rect[i];
            // Draw the bitmap into a smaller size with a circle mask.
            Bitmap resourceBitmap = participantsIcon.get(i);
            RectF resourceRect =
                    new RectF(
                            /* left= */ 0,
                            /* top= */ 0,
                            resourceBitmap.getWidth(),
                            resourceBitmap.getHeight());
            Bitmap smallCircleBitmap =
                    createOrReuseBitmap(
                            Math.round(avatarDestOnGroup.width()),
                            Math.round(avatarDestOnGroup.height()),
                            Color.TRANSPARENT);
            RectF smallCircleRect =
                    new RectF(
                            /* left= */ 0,
                            /* top= */ 0,
                            smallCircleBitmap.getWidth(),
                            smallCircleBitmap.getHeight());
            Canvas smallCircleCanvas = new Canvas(smallCircleBitmap);
            drawBitmapWithCircleOnCanvas(
                    resourceBitmap,
                    smallCircleCanvas,
                    resourceRect,
                    smallCircleRect,
                    groupAvatarConfigs.mFillBackground,
                    groupAvatarConfigs.mBackgroundColor,
                    groupAvatarConfigs.mStrokeColor);
            Matrix matrix = new Matrix();
            matrix.setRectToRect(smallCircleRect, avatarDestOnGroup, Matrix.ScaleToFit.FILL);
            canvas.drawBitmap(smallCircleBitmap, matrix, new Paint(Paint.ANTI_ALIAS_FLAG));
        }

        return bitmap;
    }

    /**
     * Given the source bitmap and a canvas, draws the bitmap through a circular mask. Only draws a
     * circle with diameter equal to the destination width.
     *
     * @param bitmap The source bitmap to draw.
     * @param canvas The canvas to draw it on.
     * @param source The source bound of the bitmap.
     * @param dest The destination bound on the canvas.
     * @param fillBackground when set, fill the circle with backgroundColor
     * @param strokeColor draw a border outside the circle with strokeColor
     */
    private static void drawBitmapWithCircleOnCanvas(
            @NonNull Bitmap bitmap,
            @NonNull Canvas canvas,
            @NonNull RectF source,
            @NonNull RectF dest,
            boolean fillBackground,
            int backgroundColor,
            int strokeColor) {
        // Draw bitmap through shader first.
        final BitmapShader shader = new BitmapShader(bitmap, TileMode.CLAMP, TileMode.CLAMP);
        final Matrix matrix = new Matrix();

        // Fit bitmap to bounds.
        matrix.setRectToRect(source, dest, Matrix.ScaleToFit.CENTER);

        shader.setLocalMatrix(matrix);
        Paint bitmapPaint = new Paint();

        bitmapPaint.setAntiAlias(true);
        if (fillBackground) {
            bitmapPaint.setColor(backgroundColor);
            canvas.drawCircle(dest.centerX(), dest.centerX(), dest.width() / 2f, bitmapPaint);
        }

        bitmapPaint.setShader(shader);
        canvas.drawCircle(dest.centerX(), dest.centerX(), dest.width() / 2f, bitmapPaint);
        bitmapPaint.setShader(null);

        if (strokeColor != Color.TRANSPARENT) {
            final Paint stroke = new Paint();
            stroke.setAntiAlias(true);
            stroke.setColor(strokeColor);
            stroke.setStyle(Paint.Style.STROKE);
            final float strokeWidth = 6f;
            stroke.setStrokeWidth(strokeWidth);
            canvas.drawCircle(
                    dest.centerX(),
                    dest.centerX(),
                    /* radius= */ dest.width() / 2f - stroke.getStrokeWidth() / 2f,
                    stroke);
        }
    }

    @NonNull
    private static Bitmap createOrReuseBitmap(int width, int height, @ColorInt int background) {
        Bitmap bitmap =
                Bitmap.createBitmap(width, height, /* Bitmap.Config= */ Bitmap.Config.ARGB_8888);
        bitmap.eraseColor(background);
        return bitmap;
    }

    /**
     * Generates an array of {@link RectF} which represents where each of the individual avatar
     * should be located in the final group avatar image. The location of each avatar depends on the
     * size of the group and the size of the overall group avatar size. If we're cropping to a
     * circle, inset the rects so the circle surrounds all the mini-avatars.
     */
    public static RectF[] generateDestRectArray(
            int desiredWidth, int desiredHeight, boolean cropToCircle, int groupSize) {
        float halfWidth = desiredWidth / 2F;
        float halfHeight = desiredHeight / 2F;

        // If we're cropping to a circle, calculate an inset so that all the mini-avatars will fit
        // inside the circle.
        float inset =
                cropToCircle ? (float) ((Math.hypot(halfWidth, halfHeight) - halfWidth) / 2f) : 0F;
        RectF[] destArray = new RectF[groupSize];
        switch (groupSize) {
            case 2:
                /*
                 * +-------+
                 * | 0 |   |
                 * +-------+
                 * |   | 1 |
                 * +-------+         *
                 * We want two circles which touches in the center. To get this we know that
                 * the diagonal
                 * of the overall group avatar is squareRoot(2) * w We also know that the two
                 * circles
                 * touches the at the center of the overall group avatar and the distance from
                 * the center of
                 * the circle to the corner of the group avatar is radius * squareRoot(2).
                 * Therefore, the
                 * following emerges.
                 *
                 * w * squareRoot(2) = 2 (radius + radius * squareRoot(2)) Solving for radius
                 * we get: d =
                 * 2 * radius = ( squareRoot(2) / (squareRoot(2) + 1)) * w d = (2 - squareRoot(2)
                 * ) * w
                 */
                float diameter = (float) ((2 - Math.sqrt(2)) * ((float) desiredWidth - inset));
                destArray[0] = new RectF(inset, inset, diameter, diameter);
                destArray[1] =
                        new RectF(
                                /* left= */ (float) desiredWidth - diameter,
                                /* top= */ (float) desiredHeight - diameter,
                                /* right= */ (float) desiredWidth - inset,
                                /* bottom= */ (float) desiredHeight - inset);
                break;
            case 3:
                /*
                 * +-------+
                 * | | 0 | |
                 * +-------+
                 * | 1 | 2 |
                 * +-------+
                 *   i0
                 *   |\
                 * a | \ c
                 *   --- i2
                 *    b
                 *
                 * a = radius * squareRoot(3) due to the triangle being a 30-60-90 right
                 * triangle. b =
                 * radius of circle c = 2 * radius of circle
                 *
                 * All three of the images are circles and therefore image zero will not touch
                 * image one
                 * or image two. Move image zero down so it touches image one and image two. This
                 * can be
                 * done by keeping image zero in the center and moving it down slightly. The
                 * amount to move
                 * down can be calculated by solving a right triangle. We know that the center x
                 * of image
                 * two to the center x of image zero is the radius of the circle, this is the
                 * length of edge
                 * b. Also we know that the distance from image zero to image two's center is 2 *
                 * radius,
                 * edge c. From this we know that the distance from center y of image two to
                 * center y of
                 * image one, edge a, is equal to radius * squareRoot(3) due to this triangle
                 * being a
                 * 30-60-90 right triangle.
                 */
                float quarterWidth = (float) desiredWidth / 4F;
                float threeQuarterWidth = 3 * quarterWidth;
                float radius = cropToCircle ? (halfHeight - inset) / 2 : (float) desiredHeight / 4F;
                float imageTwoCenterY = (float) desiredHeight - radius;
                float lengthOfEdgeA = (float) (radius * Math.sqrt(3));
                float imageZeroCenterY = imageTwoCenterY - lengthOfEdgeA;
                float imageZeroTop = imageZeroCenterY - radius - 2 * inset;
                float imageZeroBottom = imageZeroCenterY + radius - 2 * inset;
                destArray[0] =
                        new RectF(
                                quarterWidth, imageZeroTop,
                                threeQuarterWidth, imageZeroBottom);
                destArray[1] =
                        new RectF(
                                inset,
                                /* top= */ halfHeight - inset,
                                halfWidth,
                                /* bottom= */ (float) desiredHeight - 2 * inset);
                destArray[2] =
                        new RectF(
                                halfWidth,
                                /* top= */ halfHeight - inset,
                                /* right= */ (float) desiredWidth - inset,
                                /* bottom= */ (float) desiredHeight - 2 * inset);
                break;
            default:
                /*
                 * +-------+
                 * | 0 | 1 |
                 * +-------+
                 * | 2 | 3 |
                 * +-------+
                 */
                destArray[0] = new RectF(inset, inset, halfWidth, halfHeight);
                destArray[1] =
                        new RectF(
                                halfWidth,
                                inset,
                                /* right= */ (float) desiredWidth - inset,
                                halfHeight);
                destArray[2] =
                        new RectF(
                                inset,
                                halfHeight,
                                halfWidth,
                                /* bottom= */ (float) desiredHeight - inset);
                destArray[3] =
                        new RectF(
                                halfWidth,
                                halfHeight,
                                /* right= */ (float) desiredWidth - inset,
                                /* bottom= */ (float) desiredHeight - inset);
                break;
        }
        return destArray;
    }
}
