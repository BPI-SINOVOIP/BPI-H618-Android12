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

import static com.google.common.truth.Truth.assertThat;

import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.MediaExtractor;
import android.media.metrics.LogSessionId;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import com.google.common.truth.Truth;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/** Test class used by host-side tests to trigger {@link MediaExtractor} media metric events. */
@RunWith(AndroidJUnit4.class)
public class MediaExtractorDeviceSideTest {

    static {
        System.loadLibrary("CtsMediaExtractorHostTestAppJni");
    }

    private static final String SAMPLE_PATH = "raw/small_sample.mp4";
    private AssetManager mAssetManager;

    @Before
    public void setUp() {
        mAssetManager = InstrumentationRegistry.getInstrumentation().getContext().getAssets();
    }

    @Test
    public void testEntryPointSdk() throws Exception {
        MediaExtractor mediaExtractor = new MediaExtractor();
        AssetManager assetManager =
                InstrumentationRegistry.getInstrumentation().getContext().getAssets();
        try (AssetFileDescriptor fileDescriptor = assetManager.openFd(SAMPLE_PATH)) {
            mediaExtractor.setDataSource(fileDescriptor);
        }
        mediaExtractor.release();
    }

    @Test
    public void testEntryPointNdkNoJvm() {
        extractUsingNdkMediaExtractor(mAssetManager, SAMPLE_PATH, /* withAttachedJvm= */ false);
    }

    @Test
    public void testEntryPointNdkWithJvm() {
        extractUsingNdkMediaExtractor(mAssetManager, SAMPLE_PATH, /* withAttachedJvm= */ true);
    }

    @Test
    public void testLogSessionId() throws Exception {
        MediaExtractor mediaExtractor = new MediaExtractor();
        AssetManager assetManager =
                InstrumentationRegistry.getInstrumentation().getContext().getAssets();
        try (AssetFileDescriptor fileDescriptor = assetManager.openFd(SAMPLE_PATH)) {
            mediaExtractor.setDataSource(fileDescriptor);
            assertThat(mediaExtractor.getLogSessionId())
                    .isEqualTo(LogSessionId.LOG_SESSION_ID_NONE);
            mediaExtractor.setLogSessionId(new LogSessionId("FakeLogSessionId"));
            assertThat(mediaExtractor.getLogSessionId().getStringId())
                    .isEqualTo("FakeLogSessionId");
        } finally {
            mediaExtractor.release();
        }
    }

    private native void extractUsingNdkMediaExtractor(
            AssetManager assetManager, String assetPath, boolean withAttachedJvm);
}
