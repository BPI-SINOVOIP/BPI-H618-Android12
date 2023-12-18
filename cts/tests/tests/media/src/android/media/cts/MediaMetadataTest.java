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
package android.media.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.graphics.Bitmap;
import android.media.MediaMetadata;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests {@link MediaMetadata}.
 */
// TODO(b/168668505): Add tests for other methods.
@SmallTest
@RunWith(AndroidJUnit4.class)
@NonMediaMainlineTest
public class MediaMetadataTest {

    @Test
    public void getBitmapDimensionLimit_returnsIntegerMaxWhenNotSet() {
        MediaMetadata metadata = new MediaMetadata.Builder().build();
        assertEquals(Integer.MAX_VALUE, metadata.getBitmapDimensionLimit());
    }

    @Test
    public void builder_setBitmapDimensionLimit_bitmapsAreScaledDown() {
        // A large bitmap (64MB).
        final int originalWidth = 4096;
        final int originalHeight = 4096;
        Bitmap testBitmap = Bitmap.createBitmap(
                originalWidth, originalHeight, Bitmap.Config.ARGB_8888);

        final int testBitmapDimensionLimit = 16;

        MediaMetadata metadata = new MediaMetadata.Builder()
                .putBitmap(MediaMetadata.METADATA_KEY_ALBUM_ART, testBitmap)
                .setBitmapDimensionLimit(testBitmapDimensionLimit)
                .build();
        assertEquals(testBitmapDimensionLimit, metadata.getBitmapDimensionLimit());

        Bitmap scaledDownBitmap = metadata.getBitmap(MediaMetadata.METADATA_KEY_ALBUM_ART);
        assertNotNull(scaledDownBitmap);
        assertTrue(scaledDownBitmap.getWidth() <= testBitmapDimensionLimit);
        assertTrue(scaledDownBitmap.getHeight() <= testBitmapDimensionLimit);
    }

    @Test
    public void builder_setBitmapDimensionLimit_bitmapsAreNotScaledDown() {
        // A small bitmap.
        final int originalWidth = 16;
        final int originalHeight = 16;
        Bitmap testBitmap = Bitmap.createBitmap(
                originalWidth, originalHeight, Bitmap.Config.ARGB_8888);

        // The limit is larger than the width/height.
        final int testBitmapDimensionLimit = 256;

        MediaMetadata metadata = new MediaMetadata.Builder()
                .putBitmap(MediaMetadata.METADATA_KEY_ALBUM_ART, testBitmap)
                .setBitmapDimensionLimit(testBitmapDimensionLimit)
                .build();
        assertEquals(testBitmapDimensionLimit, metadata.getBitmapDimensionLimit());

        Bitmap notScaledDownBitmap = metadata.getBitmap(MediaMetadata.METADATA_KEY_ALBUM_ART);
        assertNotNull(notScaledDownBitmap);
        assertEquals(originalWidth, notScaledDownBitmap.getWidth());
        assertEquals(originalHeight, notScaledDownBitmap.getHeight());
    }

    @Test
    public void builder_setMaxBitmapDimensionLimit_unsetLimit() {
        final int testBitmapDimensionLimit = 256;
        MediaMetadata metadata = new MediaMetadata.Builder()
                .setBitmapDimensionLimit(testBitmapDimensionLimit)
                .build();
        assertEquals(testBitmapDimensionLimit, metadata.getBitmapDimensionLimit());

        // Using copy constructor, unset the limit by passing zero to the limit.
        MediaMetadata copiedMetadataWithLimitUnset = new MediaMetadata.Builder()
                .setBitmapDimensionLimit(Integer.MAX_VALUE)
                .build();
        assertEquals(Integer.MAX_VALUE, copiedMetadataWithLimitUnset.getBitmapDimensionLimit());
    }

}
