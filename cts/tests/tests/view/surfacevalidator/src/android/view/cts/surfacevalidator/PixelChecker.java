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
package android.view.cts.surfacevalidator;

import android.graphics.Rect;
import android.media.Image;
import android.os.Trace;

import java.nio.ByteBuffer;

public abstract class PixelChecker {
    private int mMatchingPixelCount = 0;
    private PixelColor mPixelColor;

    private static final int PIXEL_STRIDE = 4;

    public PixelChecker() {
        mPixelColor = new PixelColor();
    }

    public PixelChecker(int color) {
        mPixelColor = new PixelColor(color);
    }

    int getNumMatchingPixels(PixelColor expectedColor, Image.Plane plane, Rect boundsToCheck) {
        int numMatchingPixels = 0;
        ByteBuffer buffer = plane.getBuffer();
        int rowStride = plane.getRowStride();
        final int bytesWidth = boundsToCheck.width() * PIXEL_STRIDE;
        byte[] scanline = new byte[bytesWidth];
        for (int row = boundsToCheck.top; row < boundsToCheck.bottom; row++) {
            buffer.position(rowStride * row + boundsToCheck.left * PIXEL_STRIDE);
            buffer.get(scanline, 0, scanline.length);
            for (int i = 0; i < bytesWidth; i += PIXEL_STRIDE) {
                // Format is RGBA_8888 not ARGB_8888
                if (matchesColor(expectedColor, scanline, i)) {
                    numMatchingPixels++;
                }
            }
        }
        return numMatchingPixels;
    }

    boolean matchesColor(PixelColor expectedColor, byte[] scanline, int offset) {
        final int red = scanline[offset + 0] & 0xFF;
        final int green = scanline[offset + 1] & 0xFF;
        final int blue = scanline[offset + 2] & 0xFF;
        final int alpha = scanline[offset + 3] & 0xFF;

        return alpha <= expectedColor.mMaxAlpha
                && alpha >= expectedColor.mMinAlpha
                && red <= expectedColor.mMaxRed
                && red >= expectedColor.mMinRed
                && green <= expectedColor.mMaxGreen
                && green >= expectedColor.mMinGreen
                && blue <= expectedColor.mMaxBlue
                && blue >= expectedColor.mMinBlue;
    }


    public boolean validatePlane(Image.Plane plane, long frameNumber,
            Rect boundsToCheck, int width, int height) {
        Trace.beginSection("compare and sum");
        mMatchingPixelCount = getNumMatchingPixels(mPixelColor, plane, boundsToCheck);
        Trace.endSection();

        return checkPixels(mMatchingPixelCount, width, height);
    }

    public String getLastError() {
        return "pixel count = " + mMatchingPixelCount + ")";
    }

    public abstract boolean checkPixels(int matchingPixelCount, int width, int height);
}
