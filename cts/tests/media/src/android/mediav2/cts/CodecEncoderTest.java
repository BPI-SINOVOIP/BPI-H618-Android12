/*
 * Copyright (C) 2019 The Android Open Source Project
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
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Bundle;
import android.util.Log;

import androidx.test.filters.LargeTest;
import androidx.test.filters.SmallTest;

import org.junit.Assume;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

/**
 * Validate encode functionality of listed encoder components
 *
 * The test aims to test all encoders advertised in MediaCodecList. Hence we are not using
 * MediaCodecList#findEncoderForFormat to create codec. Further, it can so happen that the
 * test clip chosen is not supported by component (codecCapabilities.isFormatSupported()
 * fails), then it is better to remove the format but not skip testing the component. The idea
 * of these tests are not to cover CDD requirements but to test components and their plugins
 */
@RunWith(Parameterized.class)
public class CodecEncoderTest extends CodecEncoderTestBase {
    private static final String LOG_TAG = CodecEncoderTest.class.getSimpleName();
    private int mNumSyncFramesReceived;
    private ArrayList<Integer> mSyncFramesPos;

    public CodecEncoderTest(String encoder, String mime, int[] bitrates, int[] encoderInfo1,
            int[] encoderInfo2) {
        super(encoder, mime, bitrates, encoderInfo1, encoderInfo2);
        mSyncFramesPos = new ArrayList<>();
    }

    @Override
    void resetContext(boolean isAsync, boolean signalEOSWithLastFrame) {
        super.resetContext(isAsync, signalEOSWithLastFrame);
        mNumSyncFramesReceived = 0;
        mSyncFramesPos.clear();
    }

    @Override
    void flushCodec() {
        super.flushCodec();
        mNumSyncFramesReceived = 0;
        mSyncFramesPos.clear();
    }

    void dequeueOutput(int bufferIndex, MediaCodec.BufferInfo info) {
        if (info.size > 0 && (info.flags & MediaCodec.BUFFER_FLAG_KEY_FRAME) != 0) {
            mNumSyncFramesReceived += 1;
            mSyncFramesPos.add(mOutputCount);
        }
        super.dequeueOutput(bufferIndex, info);
    }

    private void forceSyncFrame() {
        final Bundle syncFrame = new Bundle();
        syncFrame.putInt(MediaCodec.PARAMETER_KEY_REQUEST_SYNC_FRAME, 0);
        if (ENABLE_LOGS) {
            Log.v(LOG_TAG, "requesting key frame");
        }
        mCodec.setParameters(syncFrame);
    }

    private void updateBitrate(int bitrate) {
        final Bundle bitrateUpdate = new Bundle();
        bitrateUpdate.putInt(MediaCodec.PARAMETER_KEY_VIDEO_BITRATE, bitrate);
        if (ENABLE_LOGS) {
            Log.v(LOG_TAG, "requesting bitrate to be changed to " + bitrate);
        }
        mCodec.setParameters(bitrateUpdate);
    }

    @Parameterized.Parameters(name = "{index}({0}_{1})")
    public static Collection<Object[]> input() {
        final boolean isEncoder = true;
        final boolean needAudio = true;
        final boolean needVideo = true;
        final List<Object[]> exhaustiveArgsList = Arrays.asList(new Object[][]{
                // Audio - CodecMime, arrays of bit-rates, sample rates, channel counts
                {MediaFormat.MIMETYPE_AUDIO_AAC, new int[]{64000, 128000}, new int[]{8000, 11025,
                        22050, 44100, 48000}, new int[]{1, 2}},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new int[]{6600, 8850, 12650, 14250, 15850,
                        18250, 19850, 23050, 23850}, new int[]{16000}, new int[]{1}},
                {MediaFormat.MIMETYPE_AUDIO_AMR_NB, new int[]{4750, 5150, 5900, 6700, 7400, 7950,
                        10200, 12200}, new int[]{8000}, new int[]{1}},
                {MediaFormat.MIMETYPE_AUDIO_AMR_WB, new int[]{6600, 8850, 12650, 14250, 15850,
                        18250, 19850, 23050, 23850}, new int[]{16000}, new int[]{1}},
                /* TODO(169310292) */
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new int[]{/* 0, 1, 2, */ 3, 4, 5, 6, 7, 8},
                        new int[]{8000, 48000, 96000, 192000}, new int[]{1, 2}},

                // Video - CodecMime, arrays of bit-rates, height, width
                {MediaFormat.MIMETYPE_VIDEO_H263, new int[]{32000, 64000}, new int[]{176},
                        new int[]{144}},
                {MediaFormat.MIMETYPE_VIDEO_MPEG4, new int[]{32000, 64000}, new int[]{176},
                        new int[]{144}},
                {MediaFormat.MIMETYPE_VIDEO_AVC, new int[]{256000, 512000}, new int[]{176, 352,
                        352, 480}, new int[]{144, 240, 288, 360}},
                {MediaFormat.MIMETYPE_VIDEO_HEVC, new int[]{256000, 512000}, new int[]{176, 352,
                        352, 480}, new int[]{144, 240, 288, 360}},
                {MediaFormat.MIMETYPE_VIDEO_VP8, new int[]{256000, 512000}, new int[]{176, 352,
                        352, 480}, new int[]{144, 240, 288, 360}},
                {MediaFormat.MIMETYPE_VIDEO_VP9, new int[]{256000, 512000}, new int[]{176, 352,
                        352, 480}, new int[]{144, 240, 288, 360}},
                {MediaFormat.MIMETYPE_VIDEO_AV1, new int[]{256000, 512000}, new int[]{176, 352,
                        352, 480}, new int[]{144, 240, 288, 360}},
        });
        return prepareParamList(exhaustiveArgsList, isEncoder, needAudio, needVideo, true);
    }

    /**
     * Tests encoder for combinations:
     * 1. Codec Sync Mode, Signal Eos with Last frame
     * 2. Codec Sync Mode, Signal Eos Separately
     * 3. Codec Async Mode, Signal Eos with Last frame
     * 4. Codec Async Mode, Signal Eos Separately
     * In all these scenarios, Timestamp ordering is verified. The output has to be
     * consistent (not flaky) in all runs.
     */
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testSimpleEncode() throws IOException, InterruptedException {
        setUpParams(Integer.MAX_VALUE);
        boolean[] boolStates = {true, false};
        setUpSource(mInputFile);
        OutputManager ref = new OutputManager();
        OutputManager test = new OutputManager();
        {
            mCodec = MediaCodec.createByCodecName(mCodecName);
            assertTrue("codec name act/got: " + mCodec.getName() + '/' + mCodecName,
                    mCodec.getName().equals(mCodecName));
            assertTrue("error! codec canonical name is null",
                    mCodec.getCanonicalName() != null && !mCodec.getCanonicalName().isEmpty());
            /* TODO(b/149027258) */
            if (true) mSaveToMem = false;
            else mSaveToMem = true;
            for (MediaFormat format : mFormats) {
                int loopCounter = 0;
                if (mIsAudio) {
                    mSampleRate = format.getInteger(MediaFormat.KEY_SAMPLE_RATE);
                    mChannels = format.getInteger(MediaFormat.KEY_CHANNEL_COUNT);
                } else {
                    mWidth = format.getInteger(MediaFormat.KEY_WIDTH);
                    mHeight = format.getInteger(MediaFormat.KEY_HEIGHT);
                }
                for (boolean eosType : boolStates) {
                    for (boolean isAsync : boolStates) {
                        String log = String.format(
                                "format: %s \n codec: %s, file: %s, mode: %s, eos type: %s:: ",
                                format, mCodecName, mInputFile, (isAsync ? "async" : "sync"),
                                (eosType ? "eos with last frame" : "eos separate"));
                        mOutputBuff = loopCounter == 0 ? ref : test;
                        mOutputBuff.reset();
                        mInfoList.clear();
                        validateMetrics(mCodecName);
                        configureCodec(format, isAsync, eosType, true);
                        mCodec.start();
                        doWork(Integer.MAX_VALUE);
                        queueEOS();
                        waitForAllOutputs();
                        validateMetrics(mCodecName, format);
                        /* TODO(b/147348711) */
                        if (false) mCodec.stop();
                        else mCodec.reset();
                        assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                        assertTrue(log + "no input sent", 0 != mInputCount);
                        assertTrue(log + "output received", 0 != mOutputCount);
                        if (!mIsAudio) {
                            assertTrue(
                                    log + "input count != output count, act/exp: " + mOutputCount +
                                            " / " + mInputCount, mInputCount == mOutputCount);
                        }
                        if (loopCounter != 0) {
                            assertTrue(log + "encoder output is flaky", ref.equals(test));
                        } else {
                            if (mIsAudio) {
                                assertTrue(log + " pts is not strictly increasing",
                                        ref.isPtsStrictlyIncreasing(mPrevOutputPts));
                            } else {
                                assertTrue(
                                        log + " input pts list and output pts list are not identical",
                                        ref.isOutPtsListIdenticalToInpPtsList((mMaxBFrames != 0)));
                            }
                        }
                        loopCounter++;
                    }
                }
            }
            mCodec.release();
        }
    }

    private boolean isCodecLossless(String mime) {
        return mime.equals(MediaFormat.MIMETYPE_AUDIO_FLAC) ||
                mime.equals(MediaFormat.MIMETYPE_AUDIO_RAW);
    }

    /**
     * Identity test for encoder
     */
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testLosslessEncodeDecode() throws IOException, InterruptedException {
        Assume.assumeTrue(isCodecLossless(mMime));
        setUpParams(Integer.MAX_VALUE);
        setUpSource(mInputFile);
        mOutputBuff = new OutputManager();
        {
            mCodec = MediaCodec.createByCodecName(mCodecName);
            mSaveToMem = true;
            for (MediaFormat format : mFormats) {
                if (mIsAudio) {
                    mSampleRate = format.getInteger(MediaFormat.KEY_SAMPLE_RATE);
                    mChannels = format.getInteger(MediaFormat.KEY_CHANNEL_COUNT);
                } else {
                    mWidth = format.getInteger(MediaFormat.KEY_WIDTH);
                    mHeight = format.getInteger(MediaFormat.KEY_HEIGHT);
                }
                String log = String.format("format: %s \n codec: %s, file: %s :: ", format,
                        mCodecName, mInputFile);
                mOutputBuff.reset();
                mInfoList.clear();
                configureCodec(format, true, true, true);
                mCodec.start();
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + "unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "no output received", 0 != mOutputCount);
                if (!mIsAudio) {
                    assertTrue(
                            log + "input count != output count, act/exp: " + mOutputCount +
                                    " / " + mInputCount, mInputCount == mOutputCount);
                }
                if (mIsAudio) {
                    assertTrue(log + " pts is not strictly increasing",
                            mOutputBuff.isPtsStrictlyIncreasing(mPrevOutputPts));
                } else {
                    assertTrue(
                            log + " input pts list and output pts list are not identical",
                            mOutputBuff.isOutPtsListIdenticalToInpPtsList((mMaxBFrames != 0)));
                }
                ArrayList<String> listOfDecoders = selectCodecs(mMime, null, null, false);
                assertFalse("no suitable codecs found for mime: " + mMime,
                        listOfDecoders.isEmpty());
                for (String decoder : listOfDecoders) {
                    ByteBuffer out = decodeElementaryStream(decoder, format,
                            mOutputBuff.getBuffer(), mInfoList);
                    if (!out.equals(ByteBuffer.wrap(mInputData))) {
                        fail(log + "identity test failed");
                    }
                }
            }
            mCodec.release();
        }
    }

    private native boolean nativeTestSimpleEncode(String encoder, String file, String mime,
            int[] list0, int[] list1, int[] list2, int colorFormat);

    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testSimpleEncodeNative() throws IOException {
        int colorFormat = -1;
        {
            if (!mIsAudio) {
                colorFormat = findByteBufferColorFormat(mCodecName, mMime);
                assertTrue("no valid color formats received", colorFormat != -1);
            }
            assertTrue(nativeTestSimpleEncode(mCodecName, mInpPrefix + mInputFile, mMime, mBitrates,
                    mEncParamList1, mEncParamList2, colorFormat));
        }
    }

    /**
     * Tests flush when codec is in sync and async mode. In these scenarios, Timestamp
     * ordering is verified. The output has to be consistent (not flaky) in all runs
     */
    @Ignore("TODO(b/147576107, b/148652492, b/148651699)")
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testFlush() throws IOException, InterruptedException {
        setUpParams(1);
        setUpSource(mInputFile);
        boolean[] boolStates = {true, false};
        mOutputBuff = new OutputManager();
        {
            MediaFormat inpFormat = mFormats.get(0);
            if (mIsAudio) {
                mSampleRate = inpFormat.getInteger(MediaFormat.KEY_SAMPLE_RATE);
                mChannels = inpFormat.getInteger(MediaFormat.KEY_CHANNEL_COUNT);
            } else {
                mWidth = inpFormat.getInteger(MediaFormat.KEY_WIDTH);
                mHeight = inpFormat.getInteger(MediaFormat.KEY_HEIGHT);
            }
            mCodec = MediaCodec.createByCodecName(mCodecName);
            for (boolean isAsync : boolStates) {
                String log = String.format("encoder: %s, input file: %s, mode: %s:: ", mCodecName,
                        mInputFile, (isAsync ? "async" : "sync"));
                configureCodec(inpFormat, isAsync, true, true);
                mCodec.start();

                /* test flush in running state before queuing input */
                flushCodec();
                mOutputBuff.reset();
                mInfoList.clear();
                if (mIsCodecInAsyncMode) mCodec.start();
                doWork(23);
                assertTrue(log + " pts is not strictly increasing",
                        mOutputBuff.isPtsStrictlyIncreasing(mPrevOutputPts));
                boolean checkMetrics = (mOutputCount != 0);

                /* test flush in running state */
                flushCodec();
                mOutputBuff.reset();
                mInfoList.clear();
                if (mIsCodecInAsyncMode) mCodec.start();
                if (checkMetrics) validateMetrics(mCodecName, inpFormat);
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                if (!mIsAudio) {
                    assertTrue(log + "input count != output count, act/exp: " + mOutputCount +
                            " / " + mInputCount, mInputCount == mOutputCount);
                    assertTrue(
                            log + " input pts list and output pts list are not identical",
                            mOutputBuff.isOutPtsListIdenticalToInpPtsList((mMaxBFrames != 0)));
                } else {
                    assertTrue(log + " pts is not strictly increasing",
                            mOutputBuff.isPtsStrictlyIncreasing(mPrevOutputPts));
                }

                /* test flush in eos state */
                flushCodec();
                mOutputBuff.reset();
                mInfoList.clear();
                if (mIsCodecInAsyncMode) mCodec.start();
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                if (!mIsAudio) {
                    assertTrue(log + "input count != output count, act/exp: " + mOutputCount +
                            " / " + mInputCount, mInputCount == mOutputCount);
                    assertTrue(
                            log + " input pts list and output pts list are not identical",
                            mOutputBuff.isOutPtsListIdenticalToInpPtsList((mMaxBFrames != 0)));
                } else {
                    assertTrue(log + " pts is not strictly increasing",
                            mOutputBuff.isPtsStrictlyIncreasing(mPrevOutputPts));
                }
            }
            mCodec.release();
        }
    }

    private native boolean nativeTestFlush(String encoder, String file, String mime,
            int[] list0, int[] list1, int[] list2, int colorFormat);

    @Ignore("TODO(b/147576107, b/148652492, b/148651699)")
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testFlushNative() throws IOException {
        int colorFormat = -1;
        {
            if (!mIsAudio) {
                colorFormat = findByteBufferColorFormat(mCodecName, mMime);
                assertTrue("no valid color formats received", colorFormat != -1);
            }
            assertTrue(nativeTestFlush(mCodecName, mInpPrefix + mInputFile, mMime, mBitrates,
                    mEncParamList1, mEncParamList2, colorFormat));
        }
    }

    /**
     * Tests reconfigure when codec is in sync and async mode. In these
     * scenarios, Timestamp ordering is verified. The output has to be consistent (not flaky)
     * in all runs
     */
    @Ignore("TODO(b/148523403)")
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testReconfigure() throws IOException, InterruptedException {
        setUpParams(2);
        setUpSource(mInputFile);
        boolean[] boolStates = {true, false};
        OutputManager test = new OutputManager();
        {
            boolean saveToMem = false; /* TODO(b/149027258) */
            OutputManager configRef = null;
            if (mFormats.size() > 1) {
                MediaFormat format = mFormats.get(1);
                encodeToMemory(mInputFile, mCodecName, Integer.MAX_VALUE, format, saveToMem);
                configRef = mOutputBuff;
                if (mIsAudio) {
                    assertTrue("config reference output pts is not strictly increasing",
                            configRef.isPtsStrictlyIncreasing(mPrevOutputPts));
                } else {
                    assertTrue("input pts list and reconfig ref output pts list are not identical",
                            configRef.isOutPtsListIdenticalToInpPtsList((mMaxBFrames != 0)));
                }
            }
            MediaFormat format = mFormats.get(0);
            encodeToMemory(mInputFile, mCodecName, Integer.MAX_VALUE, format, saveToMem);
            OutputManager ref = mOutputBuff;
            if (mIsAudio) {
                assertTrue("reference output pts is not strictly increasing",
                        ref.isPtsStrictlyIncreasing(mPrevOutputPts));
            } else {
                assertTrue("input pts list and ref output pts list are not identical",
                        ref.isOutPtsListIdenticalToInpPtsList((mMaxBFrames != 0)));
            }
            mOutputBuff = test;
            mCodec = MediaCodec.createByCodecName(mCodecName);
            for (boolean isAsync : boolStates) {
                String log = String.format("encoder: %s, input file: %s, mode: %s:: ", mCodecName,
                        mInputFile, (isAsync ? "async" : "sync"));
                configureCodec(format, isAsync, true, true);

                /* test reconfigure in stopped state */
                reConfigureCodec(format, !isAsync, false, true);
                mCodec.start();

                /* test reconfigure in running state before queuing input */
                reConfigureCodec(format, !isAsync, false, true);
                mCodec.start();
                doWork(23);

                if (mOutputCount != 0) validateMetrics(mCodecName, format);

                /* test reconfigure codec in running state */
                reConfigureCodec(format, isAsync, true, true);
                mCodec.start();
                mSaveToMem = saveToMem;
                test.reset();
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                if (!mIsAudio) {
                    assertTrue(log + "input count != output count, act/exp: " + mOutputCount +
                            " / " + mInputCount, mInputCount == mOutputCount);
                }
                assertTrue(log + "encoder output is flaky", ref.equals(test));

                /* test reconfigure codec at eos state */
                reConfigureCodec(format, !isAsync, false, true);
                mCodec.start();
                test.reset();
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                if (!mIsAudio) {
                    assertTrue(log + "input count != output count, act/exp: " + mOutputCount +
                            " / " + mInputCount, mInputCount == mOutputCount);
                }
                assertTrue(log + "encoder output is flaky", ref.equals(test));

                /* test reconfigure codec for new format */
                if (mFormats.size() > 1) {
                    reConfigureCodec(mFormats.get(1), isAsync, false, true);
                    mCodec.start();
                    test.reset();
                    doWork(Integer.MAX_VALUE);
                    queueEOS();
                    waitForAllOutputs();
                    /* TODO(b/147348711) */
                    if (false) mCodec.stop();
                    else mCodec.reset();
                    assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                    assertTrue(log + "no input sent", 0 != mInputCount);
                    assertTrue(log + "output received", 0 != mOutputCount);
                    if (!mIsAudio) {
                        assertTrue(log + "input count != output count, act/exp: " + mOutputCount +
                                " / " + mInputCount, mInputCount == mOutputCount);
                    }
                    assertTrue(log + "encoder output is flaky", configRef.equals(test));
                }
                mSaveToMem = false;
            }
            mCodec.release();
        }
    }

    private native boolean nativeTestReconfigure(String encoder, String file, String mime,
            int[] list0, int[] list1, int[] list2, int colorFormat);

    @Ignore("TODO(b/147348711, b/149981033)")
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testReconfigureNative() throws IOException {
        int colorFormat = -1;
        {
            if (!mIsAudio) {
                colorFormat = findByteBufferColorFormat(mCodecName, mMime);
                assertTrue("no valid color formats received", colorFormat != -1);
            }
            assertTrue(nativeTestReconfigure(mCodecName, mInpPrefix + mInputFile, mMime, mBitrates,
                    mEncParamList1, mEncParamList2, colorFormat));
        }
    }

    /**
     * Tests encoder for only EOS frame
     */
    @SmallTest
    @Test(timeout = PER_TEST_TIMEOUT_SMALL_TEST_MS)
    public void testOnlyEos() throws IOException, InterruptedException {
        setUpParams(1);
        boolean[] boolStates = {true, false};
        OutputManager ref = new OutputManager();
        OutputManager test = new OutputManager();
        {
            mCodec = MediaCodec.createByCodecName(mCodecName);
            /* TODO(b/149027258) */
            if (true) mSaveToMem = false;
            else mSaveToMem = true;
            int loopCounter = 0;
            for (boolean isAsync : boolStates) {
                String log = String.format("encoder: %s, input file: %s, mode: %s:: ", mCodecName,
                        mInputFile, (isAsync ? "async" : "sync"));
                configureCodec(mFormats.get(0), isAsync, false, true);
                mOutputBuff = loopCounter == 0 ? ref : test;
                mOutputBuff.reset();
                mInfoList.clear();
                mCodec.start();
                queueEOS();
                waitForAllOutputs();
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                if (loopCounter != 0) {
                    assertTrue(log + "encoder output is flaky", ref.equals(test));
                } else {
                    if (mIsAudio) {
                        assertTrue(log + " pts is not strictly increasing",
                                ref.isPtsStrictlyIncreasing(mPrevOutputPts));
                    } else {
                        assertTrue(
                                log + " input pts list and output pts list are not identical",
                                ref.isOutPtsListIdenticalToInpPtsList((mMaxBFrames != 0)));
                    }
                }
                loopCounter++;
            }
            mCodec.release();
        }
    }

    private native boolean nativeTestOnlyEos(String encoder, String mime, int[] list0, int[] list1,
            int[] list2, int colorFormat);

    @SmallTest
    @Test(timeout = PER_TEST_TIMEOUT_SMALL_TEST_MS)
    public void testOnlyEosNative() throws IOException {
        int colorFormat = -1;
        {
            if (!mIsAudio) {
                colorFormat = findByteBufferColorFormat(mCodecName, mMime);
                assertTrue("no valid color formats received", colorFormat != -1);
            }
            assertTrue(nativeTestOnlyEos(mCodecName, mMime, mBitrates, mEncParamList1,
                    mEncParamList2, colorFormat));
        }
    }

    /**
     * Test set parameters : force key frame
     */
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testSetForceSyncFrame() throws IOException, InterruptedException {
        Assume.assumeTrue(!mIsAudio);
        // Maximum allowed key frame interval variation from the target value.
        final int MAX_KEYFRAME_INTERVAL_VARIATION = 3;
        setUpParams(1);
        boolean[] boolStates = {true, false};
        setUpSource(mInputFile);
        MediaFormat format = mFormats.get(0);
        format.removeKey(MediaFormat.KEY_I_FRAME_INTERVAL);
        format.setFloat(MediaFormat.KEY_I_FRAME_INTERVAL, 500.f);
        mWidth = format.getInteger(MediaFormat.KEY_WIDTH);
        mHeight = format.getInteger(MediaFormat.KEY_HEIGHT);
        final int KEY_FRAME_INTERVAL = 2; // force key frame every 2 seconds.
        final int KEY_FRAME_POS = mFrameRate * KEY_FRAME_INTERVAL;
        final int NUM_KEY_FRAME_REQUESTS = 7;
        mOutputBuff = new OutputManager();
        {
            mCodec = MediaCodec.createByCodecName(mCodecName);
            for (boolean isAsync : boolStates) {
                String log = String.format(
                        "format: %s \n codec: %s, file: %s, mode: %s:: ", format, mCodecName,
                        mInputFile, (isAsync ? "async" : "sync"));
                mOutputBuff.reset();
                mInfoList.clear();
                configureCodec(format, isAsync, false, true);
                mCodec.start();
                for (int i = 0; i < NUM_KEY_FRAME_REQUESTS; i++) {
                    doWork(KEY_FRAME_POS);
                    assertTrue(!mSawInputEOS);
                    forceSyncFrame();
                    mNumBytesSubmitted = 0;
                }
                queueEOS();
                waitForAllOutputs();
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                assertTrue(log + "input count != output count, act/exp: " + mOutputCount + " / " +
                        mInputCount, mInputCount == mOutputCount);
                assertTrue(log + " input pts list and output pts list are not identical",
                        mOutputBuff.isOutPtsListIdenticalToInpPtsList((mMaxBFrames != 0)));
                assertTrue(log + "sync frames exp/act: " + NUM_KEY_FRAME_REQUESTS + " / " +
                        mNumSyncFramesReceived, mNumSyncFramesReceived >= NUM_KEY_FRAME_REQUESTS);
                for (int i = 0, expPos = 0, index = 0; i < NUM_KEY_FRAME_REQUESTS; i++) {
                    int j = index;
                    for (; j < mSyncFramesPos.size(); j++) {
                        // Check key frame intervals:
                        // key frame position should not be greater than target value + 3
                        // key frame position should not be less than target value - 3
                        if (Math.abs(expPos - mSyncFramesPos.get(j)) <=
                                MAX_KEYFRAME_INTERVAL_VARIATION) {
                            index = j;
                            break;
                        }
                    }
                    if (j == mSyncFramesPos.size()) {
                        Log.w(LOG_TAG, "requested key frame at frame index " + expPos +
                                " none found near by");
                    }
                    expPos += KEY_FRAME_POS;
                }
            }
            mCodec.release();
        }
    }

    private native boolean nativeTestSetForceSyncFrame(String encoder, String file, String mime,
            int[] list0, int[] list1, int[] list2, int colorFormat);

    @Ignore("TODO(b/) = test sometimes timesout")
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testSetForceSyncFrameNative() throws IOException {
        Assume.assumeTrue(!mIsAudio);
        int colorFormat = -1;
        {
            if (!mIsAudio) {
                colorFormat = findByteBufferColorFormat(mCodecName, mMime);
                assertTrue("no valid color formats received", colorFormat != -1);
            }
            assertTrue(nativeTestSetForceSyncFrame(mCodecName, mInpPrefix + mInputFile, mMime,
                    mBitrates, mEncParamList1, mEncParamList2, colorFormat));
        }
    }

    /**
     * Test set parameters : change bitrate dynamically
     */
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testAdaptiveBitRate() throws IOException, InterruptedException {
        Assume.assumeTrue(!mIsAudio);
        setUpParams(1);
        boolean[] boolStates = {true, false};
        setUpSource(mInputFile);
        MediaFormat format = mFormats.get(0);
        mWidth = format.getInteger(MediaFormat.KEY_WIDTH);
        mHeight = format.getInteger(MediaFormat.KEY_HEIGHT);
        final int ADAPTIVE_BR_INTERVAL = 3; // change br every 3 seconds.
        final int ADAPTIVE_BR_DUR_FRM = mFrameRate * ADAPTIVE_BR_INTERVAL;
        final int BR_CHANGE_REQUESTS = 7;
        mOutputBuff = new OutputManager();
        mSaveToMem = true;
        {
            /* TODO(b/147574800) */
            if (mCodecName.equals("c2.android.hevc.encoder")) return;
            mCodec = MediaCodec.createByCodecName(mCodecName);
            format.removeKey(MediaFormat.KEY_BITRATE_MODE);
            MediaCodecInfo.EncoderCapabilities cap =
                    mCodec.getCodecInfo().getCapabilitiesForType(mMime).getEncoderCapabilities();
            if (cap.isBitrateModeSupported(MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_CBR)) {
                format.setInteger(MediaFormat.KEY_BITRATE_MODE,
                        MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_CBR);
            } else {
                format.setInteger(MediaFormat.KEY_BITRATE_MODE,
                        MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_VBR);
            }
            for (boolean isAsync : boolStates) {
                String log = String.format(
                        "format: %s \n codec: %s, file: %s, mode: %s:: ", format, mCodecName,
                        mInputFile, (isAsync ? "async" : "sync"));
                mOutputBuff.reset();
                mInfoList.clear();
                configureCodec(format, isAsync, false, true);
                mCodec.start();
                int expOutSize = 0;
                int bitrate = format.getInteger(MediaFormat.KEY_BIT_RATE);
                for (int i = 0; i < BR_CHANGE_REQUESTS; i++) {
                    doWork(ADAPTIVE_BR_DUR_FRM);
                    assertTrue(!mSawInputEOS);
                    expOutSize += ADAPTIVE_BR_INTERVAL * bitrate;
                    if ((i & 1) == 1) bitrate *= 2;
                    else bitrate /= 2;
                    updateBitrate(bitrate);
                    mNumBytesSubmitted = 0;
                }
                queueEOS();
                waitForAllOutputs();
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                assertTrue(log + "input count != output count, act/exp: " + mOutputCount + " / " +
                        mInputCount, mInputCount == mOutputCount);
                assertTrue(log + " input pts list and output pts list are not identical",
                        mOutputBuff.isOutPtsListIdenticalToInpPtsList((mMaxBFrames != 0)));
                /* TODO: validate output br with sliding window constraints Sec 5.2 cdd */
                int outSize = mOutputBuff.getOutStreamSize() * 8;
                float brDev = Math.abs(expOutSize - outSize) * 100.0f / expOutSize;
                if (ENABLE_LOGS) {
                    Log.d(LOG_TAG, log + "relative br error is " + brDev + '%');
                }
                if (brDev > 50) {
                    fail(log + "relative br error is too large " + brDev + '%');
                }
            }
            mCodec.release();
        }
    }

    private native boolean nativeTestAdaptiveBitRate(String encoder, String file, String mime,
            int[] list0, int[] list1, int[] list2, int colorFormat);

    @Ignore("TODO(b/) = test sometimes timesout")
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testAdaptiveBitRateNative() throws IOException {
        Assume.assumeTrue(!mIsAudio);
        int colorFormat = -1;
        {
            /* TODO(b/147574800) */
            if (mCodecName.equals("c2.android.hevc.encoder")) return;
            if (!mIsAudio) {
                colorFormat = findByteBufferColorFormat(mCodecName, mMime);
                assertTrue("no valid color formats received", colorFormat != -1);
            }
            assertTrue(nativeTestAdaptiveBitRate(mCodecName, mInpPrefix + mInputFile, mMime,
                    mBitrates, mEncParamList1, mEncParamList2, colorFormat));
        }
    }
}
