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

package android.mediav2.cts;

import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import androidx.test.filters.LargeTest;
import androidx.test.rule.ActivityTestRule;

import org.junit.Assume;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.io.IOException;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import static org.junit.Assert.assertTrue;

@RunWith(Parameterized.class)
public class CodecDecoderSurfaceTest extends CodecDecoderTestBase {
    private static final String LOG_TAG = CodecDecoderSurfaceTest.class.getSimpleName();

    private final String mReconfigFile;

    public CodecDecoderSurfaceTest(String decoder, String mime, String testFile,
            String reconfigFile) {
        super(decoder, mime, testFile);
        mReconfigFile = reconfigFile;
    }

    void dequeueOutput(int bufferIndex, MediaCodec.BufferInfo info) {
        if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
            mSawOutputEOS = true;
        }
        if (ENABLE_LOGS) {
            Log.v(LOG_TAG, "output: id: " + bufferIndex + " flags: " + info.flags + " size: " +
                    info.size + " timestamp: " + info.presentationTimeUs);
        }
        if (info.size > 0 && (info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) == 0) {
            mOutputBuff.saveOutPTS(info.presentationTimeUs);
            mOutputCount++;
        }
        mCodec.releaseOutputBuffer(bufferIndex, mSurface != null);
    }

    private void decodeAndSavePts(String file, String decoder, long pts, int mode, int frameLimit)
            throws IOException, InterruptedException {
        Surface sf = mSurface;
        mSurface = null; // for reference, decode in non-surface mode
        mOutputBuff = new OutputManager();
        mCodec = MediaCodec.createByCodecName(decoder);
        MediaFormat format = setUpSource(file);
        configureCodec(format, false, true, false);
        mCodec.start();
        mExtractor.seekTo(pts, mode);
        doWork(frameLimit);
        queueEOS();
        waitForAllOutputs();
        mCodec.stop();
        mCodec.release();
        mExtractor.release();
        mSurface = sf; // restore surface
    }

    @Rule
    public ActivityTestRule<CodecTestActivity> mActivityRule =
            new ActivityTestRule<>(CodecTestActivity.class);

    @Parameterized.Parameters(name = "{index}({0}_{1})")
    public static Collection<Object[]> input() {
        final boolean isEncoder = false;
        final boolean needAudio = false;
        final boolean needVideo = true;
        final List<Object[]> exhaustiveArgsList = Arrays.asList(new Object[][]{
                {MediaFormat.MIMETYPE_VIDEO_MPEG2, "bbb_340x280_768kbps_30fps_mpeg2.mp4",
                        "bbb_520x390_1mbps_30fps_mpeg2.mp4"},
                {MediaFormat.MIMETYPE_VIDEO_MPEG2,
                        "bbb_512x288_30fps_1mbps_mpeg2_interlaced_nob_2fields.mp4",
                        "bbb_520x390_1mbps_30fps_mpeg2.mp4"},
                {MediaFormat.MIMETYPE_VIDEO_MPEG2,
                        "bbb_512x288_30fps_1mbps_mpeg2_interlaced_nob_1field.ts",
                        "bbb_520x390_1mbps_30fps_mpeg2.mp4"},
                {MediaFormat.MIMETYPE_VIDEO_AVC, "bbb_340x280_768kbps_30fps_avc.mp4",
                        "bbb_520x390_1mbps_30fps_avc.mp4"},
                {MediaFormat.MIMETYPE_VIDEO_AVC, "bbb_360x640_768kbps_30fps_avc.mp4",
                        "bbb_520x390_1mbps_30fps_avc.mp4"},
                {MediaFormat.MIMETYPE_VIDEO_AVC, "bbb_160x1024_1500kbps_30fps_avc.mp4",
                        "bbb_520x390_1mbps_30fps_avc.mp4"},
                {MediaFormat.MIMETYPE_VIDEO_AVC, "bbb_1280x120_1500kbps_30fps_avc.mp4",
                        "bbb_340x280_768kbps_30fps_avc.mp4"},
                {MediaFormat.MIMETYPE_VIDEO_HEVC, "bbb_520x390_1mbps_30fps_hevc.mp4",
                        "bbb_340x280_768kbps_30fps_hevc.mp4"},
                {MediaFormat.MIMETYPE_VIDEO_MPEG4, "bbb_128x96_64kbps_12fps_mpeg4.mp4",
                        "bbb_176x144_192kbps_15fps_mpeg4.mp4"},
                {MediaFormat.MIMETYPE_VIDEO_H263, "bbb_176x144_128kbps_15fps_h263.3gp",
                        "bbb_176x144_192kbps_10fps_h263.3gp"},
                {MediaFormat.MIMETYPE_VIDEO_VP8, "bbb_340x280_768kbps_30fps_vp8.webm",
                        "bbb_520x390_1mbps_30fps_vp8.webm"},
                {MediaFormat.MIMETYPE_VIDEO_VP9, "bbb_340x280_768kbps_30fps_vp9.webm",
                        "bbb_520x390_1mbps_30fps_vp9.webm"},
                {MediaFormat.MIMETYPE_VIDEO_VP9,
                        "bbb_340x280_768kbps_30fps_split_non_display_frame_vp9.webm",
                        "bbb_520x390_1mbps_30fps_split_non_display_frame_vp9.webm"},
                {MediaFormat.MIMETYPE_VIDEO_AV1, "bbb_340x280_768kbps_30fps_av1.mp4",
                        "bbb_520x390_1mbps_30fps_av1.mp4"},
                {MediaFormat.MIMETYPE_VIDEO_AV1,
                        "bikes_qcif_color_bt2020_smpte2086Hlg_bt2020Ncl_fr_av1.mp4",
                        "bbb_520x390_1mbps_30fps_av1.mp4"},
        });
        return prepareParamList(exhaustiveArgsList, isEncoder, needAudio, needVideo, true);
    }

    /**
     * Tests decoder for codec is in sync and async mode with surface.
     * In these scenarios, Timestamp and it's ordering is verified.
     */
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testSimpleDecodeToSurface() throws IOException, InterruptedException {
        boolean[] boolStates = {true, false};
        OutputManager ref;
        OutputManager test = new OutputManager();
        final long pts = 0;
        final int mode = MediaExtractor.SEEK_TO_CLOSEST_SYNC;
        CodecTestActivity activity = mActivityRule.getActivity();
        setUpSurface(activity);
        {
            decodeAndSavePts(mTestFile, mCodecName, pts, mode, Integer.MAX_VALUE);
            ref = mOutputBuff;
            // TODO: Timestamps for deinterlaced content are under review. (E.g. can decoders
            // produce multiple progressive frames?) For now, do not verify timestamps.
            if (!mIsInterlaced) {
                assertTrue("input pts list and output pts list are not identical",
                        ref.isOutPtsListIdenticalToInpPtsList(false));
            }
            MediaFormat format = setUpSource(mTestFile);
            mCodec = MediaCodec.createByCodecName(mCodecName);
            activity.setScreenParams(getWidth(format), getHeight(format), true);
            for (boolean isAsync : boolStates) {
                String log = String.format("codec: %s, file: %s, mode: %s:: ", mCodecName,
                        mTestFile, (isAsync ? "async" : "sync"));
                mOutputBuff = test;
                mOutputBuff.reset();
                mExtractor.seekTo(pts, mode);
                configureCodec(format, isAsync, true, false);
                mCodec.start();
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                // TODO: Timestamps for deinterlaced content are under review. (E.g. can decoders
                // produce multiple progressive frames?) For now, do not verify timestamps.
                if (mIsInterlaced) {
                    assertTrue(log + "decoder output is flaky", ref.equalsInterlaced(test));
                } else {
                    assertTrue(log + "decoder output is flaky", ref.equals(test));
                }
            }
            mCodec.release();
            mExtractor.release();
        }
        tearDownSurface();
    }

    /**
     * Tests flush when codec is in sync and async mode with surface. In these scenarios,
     * Timestamp and the ordering is verified.
     */
    @Ignore("TODO(b/147576107)")
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testFlush() throws IOException, InterruptedException {
        MediaFormat format = setUpSource(mTestFile);
        mExtractor.release();
        mCsdBuffers.clear();
        for (int i = 0; ; i++) {
            String csdKey = "csd-" + i;
            if (format.containsKey(csdKey)) {
                mCsdBuffers.add(format.getByteBuffer(csdKey));
            } else break;
        }
        final long pts = 500000;
        final int mode = MediaExtractor.SEEK_TO_CLOSEST_SYNC;
        boolean[] boolStates = {true, false};
        OutputManager test = new OutputManager();
        CodecTestActivity activity = mActivityRule.getActivity();
        setUpSurface(activity);
        {
            decodeAndSavePts(mTestFile, mCodecName, pts, mode, Integer.MAX_VALUE);
            OutputManager ref = mOutputBuff;
            // TODO: Timestamps for deinterlaced content are under review. (E.g. can decoders
            // produce multiple progressive frames?) For now, do not verify timestamps.
            if (!mIsInterlaced) {
                assertTrue("input pts list and output pts list are not identical",
                        ref.isOutPtsListIdenticalToInpPtsList(false));
            }
            mOutputBuff = test;
            setUpSource(mTestFile);
            mCodec = MediaCodec.createByCodecName(mCodecName);
            activity.setScreenParams(getWidth(format), getHeight(format), false);
            for (boolean isAsync : boolStates) {
                String log = String.format("decoder: %s, input file: %s, mode: %s:: ", mCodecName,
                        mTestFile, (isAsync ? "async" : "sync"));
                mExtractor.seekTo(0, mode);
                configureCodec(format, isAsync, true, false);
                mCodec.start();

                /* test flush in running state before queuing input */
                flushCodec();
                if (mIsCodecInAsyncMode) mCodec.start();
                queueCodecConfig(); /* flushed codec too soon after start, resubmit csd */

                doWork(1);
                flushCodec();
                if (mIsCodecInAsyncMode) mCodec.start();
                queueCodecConfig(); /* flushed codec too soon after start, resubmit csd */

                mExtractor.seekTo(0, mode);
                test.reset();
                doWork(23);
                assertTrue(log + " pts is not strictly increasing",
                        test.isPtsStrictlyIncreasing(mPrevOutputPts));

                /* test flush in running state */
                flushCodec();
                if (mIsCodecInAsyncMode) mCodec.start();
                test.reset();
                mExtractor.seekTo(pts, mode);
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                assertTrue(log + "decoder output is flaky", ref.equals(test));

                /* test flush in eos state */
                flushCodec();
                if (mIsCodecInAsyncMode) mCodec.start();
                test.reset();
                mExtractor.seekTo(pts, mode);
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                assertTrue(log + "decoder output is flaky", ref.equals(test));
            }
            mCodec.release();
            mExtractor.release();
        }
        tearDownSurface();
    }

    /**
     * Tests reconfigure when codec is in sync and async mode with surface. In these scenarios,
     * Timestamp and the ordering is verified.
     */
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testReconfigure() throws IOException, InterruptedException {
        Assume.assumeTrue("Test needs Android 11", IS_AT_LEAST_R);

        MediaFormat format = setUpSource(mTestFile);
        mExtractor.release();
        MediaFormat newFormat = setUpSource(mReconfigFile);
        mExtractor.release();
        final long pts = 500000;
        final int mode = MediaExtractor.SEEK_TO_CLOSEST_SYNC;
        boolean[] boolStates = {true, false};
        OutputManager test = new OutputManager();
        CodecTestActivity activity = mActivityRule.getActivity();
        setUpSurface(activity);
        {
            decodeAndSavePts(mTestFile, mCodecName, pts, mode, Integer.MAX_VALUE);
            OutputManager ref = mOutputBuff;
            if (!mIsInterlaced) {
                assertTrue("input pts list and reference pts list are not identical",
                        ref.isOutPtsListIdenticalToInpPtsList(false));
            }
            decodeAndSavePts(mReconfigFile, mCodecName, pts, mode, Integer.MAX_VALUE);
            OutputManager configRef = mOutputBuff;
            // TODO: Timestamps for deinterlaced content are under review. (E.g. can decoders
            // produce multiple progressive frames?) For now, do not verify timestamps.
            if (!mIsInterlaced) {
                assertTrue("input pts list and reconfig ref output pts list are not identical",
                        configRef.isOutPtsListIdenticalToInpPtsList(false));
            }
            mOutputBuff = test;
            mCodec = MediaCodec.createByCodecName(mCodecName);
            activity.setScreenParams(getWidth(format), getHeight(format), false);
            for (boolean isAsync : boolStates) {
                setUpSource(mTestFile);
                String log = String.format("decoder: %s, input file: %s, mode: %s:: ", mCodecName,
                        mTestFile, (isAsync ? "async" : "sync"));
                mExtractor.seekTo(0, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                configureCodec(format, isAsync, true, false);

                /* test reconfigure in stopped state */
                reConfigureCodec(format, !isAsync, false, false);
                mCodec.start();

                /* test reconfigure in running state before queuing input */
                reConfigureCodec(format, !isAsync, false, false);
                mCodec.start();
                doWork(23);

                /* test reconfigure codec in running state */
                reConfigureCodec(format, isAsync, true, false);
                mCodec.start();
                test.reset();
                mExtractor.seekTo(pts, mode);
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                assertTrue(log + "decoder output is flaky", ref.equals(test));

                /* test reconfigure codec at eos state */
                reConfigureCodec(format, !isAsync, false, false);
                mCodec.start();
                test.reset();
                mExtractor.seekTo(pts, mode);
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                assertTrue(log + "decoder output is flaky", ref.equals(test));
                mExtractor.release();

                /* test reconfigure codec for new file */
                setUpSource(mReconfigFile);
                log = String.format("decoder: %s, input file: %s, mode: %s:: ", mCodecName,
                        mReconfigFile, (isAsync ? "async" : "sync"));
                activity.setScreenParams(getWidth(newFormat), getHeight(newFormat), true);
                reConfigureCodec(newFormat, isAsync, false, false);
                mCodec.start();
                test.reset();
                mExtractor.seekTo(pts, mode);
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                assertTrue(log + "decoder output is flaky", configRef.equals(test));
                mExtractor.release();
            }
            mCodec.release();
        }
        tearDownSurface();
    }

    private native boolean nativeTestSimpleDecode(String decoder, Surface surface, String mime,
            String testFile, String refFile, float rmsError, long checksum);

    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testSimpleDecodeToSurfaceNative() throws IOException, InterruptedException {
        MediaFormat format = setUpSource(mTestFile);
        mExtractor.release();
        CodecTestActivity activity = mActivityRule.getActivity();
        setUpSurface(activity);
        activity.setScreenParams(getWidth(format), getHeight(format), false);
        {
            assertTrue(nativeTestSimpleDecode(mCodecName, mSurface, mMime, mInpPrefix + mTestFile,
                    mInpPrefix + mReconfigFile, -1.0f, 0L));
        }
        tearDownSurface();
    }

    private native boolean nativeTestFlush(String decoder, Surface surface, String mime,
            String testFile);

    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testFlushNative() throws IOException, InterruptedException {
        MediaFormat format = setUpSource(mTestFile);
        mExtractor.release();
        CodecTestActivity activity = mActivityRule.getActivity();
        setUpSurface(activity);
        activity.setScreenParams(getWidth(format), getHeight(format), true);
        {
            assertTrue(nativeTestFlush(mCodecName, mSurface, mMime, mInpPrefix + mTestFile));
        }
        tearDownSurface();
    }
}
