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

package android.mediav2.cts;

import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;

import androidx.test.filters.LargeTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

/**
 * The following test validates that the decode can be paused
 */
@RunWith(Parameterized.class)
public class CodecDecoderPauseTest extends CodecDecoderTestBase {
    private static final String LOG_TAG = CodecDecoderPauseTest.class.getSimpleName();
    private final long PAUSE_TIME_MS = 10000;
    private final int NUM_FRAMES = 8;
    private final int mSupport;

    public CodecDecoderPauseTest(String decoder, String mime, String srcFile, int support) {
        super(decoder, mime, srcFile);
        mSupport = support;
    }

    @Parameterized.Parameters(name = "{index}({0}_{1})")
    public static Collection<Object[]> input() {
        final boolean isEncoder = false;
        final boolean needAudio = true;
        final boolean needVideo = true;
        // mime, source file, codecs required to support
        final List<Object[]> exhaustiveArgsList = Arrays.asList(new Object[][]{
                {MediaFormat.MIMETYPE_AUDIO_AAC, "bbb_2ch_48kHz_he_aac.mp4", CODEC_ALL},
                {MediaFormat.MIMETYPE_VIDEO_AVC, "bbb_cif_avc_delay16.mp4", CODEC_ALL},
                {MediaFormat.MIMETYPE_VIDEO_H263, "bbb_176x144_128kbps_15fps_h263.3gp", CODEC_ALL},
                {MediaFormat.MIMETYPE_VIDEO_HEVC, "bbb_cif_hevc_delay15.mp4", CODEC_ALL},
                {MediaFormat.MIMETYPE_VIDEO_MPEG2, "bbb_640x360_512kbps_30fps_mpeg2_2b.mp4", CODEC_ALL},
                {MediaFormat.MIMETYPE_VIDEO_MPEG4, "bbb_176x144_192kbps_15fps_mpeg4.mp4", CODEC_ALL},
                {MediaFormat.MIMETYPE_VIDEO_VP8, "bbb_640x360_512kbps_30fps_vp8.webm", CODEC_ALL},
                {MediaFormat.MIMETYPE_VIDEO_VP9, "bbb_cif_768kbps_30fps_vp9.mkv", CODEC_ALL},
        });
        return prepareParamList(exhaustiveArgsList, isEncoder, needAudio, needVideo, false);
    }

    /**
     * Test decodes and compares decoded output of two files.
     */
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testPause() throws IOException, InterruptedException {
        ArrayList<MediaFormat> formats = new ArrayList<>();
        formats.add(setUpSource(mTestFile));
        mExtractor.release();
        if (!areFormatsSupported(mCodecName, mMime, formats)) {
            if (mSupport == CODEC_ALL) {
                fail("format(s) not supported by component: " + mCodecName + " for mime : " +
                        mMime);
            }
            if (mSupport != CODEC_OPTIONAL && selectCodecs(mMime, formats, null, false).isEmpty()) {
                fail("format(s) not supported by any component for mime : " + mMime);
            }
            return;
        }
        final boolean isAsync = true;
        MediaFormat format = setUpSource(mTestFile);
        {
            mCodec = MediaCodec.createByCodecName(mCodecName);
            int loopCounter = 0;
            boolean[] boolStates = {true, false};
            OutputManager ref = new OutputManager();
            OutputManager test = new OutputManager();
            for (boolean enablePause : boolStates) {
                String log = String.format("decoder: %s, input file: %s, mode: %s:: ", mCodecName,
                        mTestFile, (isAsync ? "async" : "sync"));
                mExtractor.seekTo(0, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                configureCodec(format, isAsync, false, false);
                mOutputBuff = loopCounter == 0 ? ref : test;
                mOutputBuff.reset();
                mCodec.start();
                if (enablePause) {
                    doWork(NUM_FRAMES);
                    Thread.sleep(PAUSE_TIME_MS);
                }
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                if (loopCounter != 0) {
                    assertTrue(log + "decoder output is flaky", ref.equals(test));
                } else {
                    if (mIsAudio) {
                        assertTrue(log + " pts is not strictly increasing",
                                ref.isPtsStrictlyIncreasing(mPrevOutputPts));
                    } else {
                        assertTrue(log + " input pts list and output pts list are not identical",
                                ref.isOutPtsListIdenticalToInpPtsList(false));
                    }
                }
                loopCounter++;
            }
            mCodec.release();
        }
        mExtractor.release();
    }
}
