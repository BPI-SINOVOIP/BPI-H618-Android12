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
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import androidx.test.filters.LargeTest;
import androidx.test.filters.SmallTest;

import org.junit.Assume;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * Validate decode functionality of listed decoder components
 *
 * The test aims to test all decoders advertised in MediaCodecList. Hence we are not using
 * MediaCodecList#findDecoderForFormat to create codec. Further, it can so happen that the
 * test clip chosen is not supported by component (codecCapabilities.isFormatSupported()
 * fails), then it is better to replace the clip but not skip testing the component. The idea
 * of these tests are not to cover CDD requirements but to test components and their plugins
 */
@RunWith(Parameterized.class)
public class CodecDecoderTest extends CodecDecoderTestBase {
    private static final String LOG_TAG = CodecDecoderTest.class.getSimpleName();
    private static final float RMS_ERROR_TOLERANCE = 1.05f;        // 5%

    private final String mRefFile;
    private final String mReconfigFile;
    private final float mRmsError;
    private final long mRefCRC;

    public CodecDecoderTest(String decoder, String mime, String testFile, String refFile,
            String reconfigFile, float rmsError, long refCRC) {
        super(decoder, mime, testFile);
        mRefFile = refFile;
        mReconfigFile = reconfigFile;
        mRmsError = rmsError;
        mRefCRC = refCRC;
    }

    static short[] setUpAudioReference(String file) throws IOException {
        File refFile = new File(file);
        short[] refData;
        try (FileInputStream refStream = new FileInputStream(refFile)) {
            FileChannel fileChannel = refStream.getChannel();
            int length = (int) refFile.length();
            ByteBuffer refBuffer = ByteBuffer.allocate(length);
            refBuffer.order(ByteOrder.LITTLE_ENDIAN);
            fileChannel.read(refBuffer);
            refData = new short[length / 2];
            refBuffer.position(0);
            for (int i = 0; i < length / 2; i++) {
                refData[i] = refBuffer.getShort();
            }
        }
        return refData;
    }

    private ArrayList<MediaCodec.BufferInfo> createSubFrames(ByteBuffer buffer, int sfCount) {
        int size = (int) mExtractor.getSampleSize();
        if (size < 0) return null;
        mExtractor.readSampleData(buffer, 0);
        long pts = mExtractor.getSampleTime();
        int flags = mExtractor.getSampleFlags();
        if (size < sfCount) sfCount = size;
        ArrayList<MediaCodec.BufferInfo> list = new ArrayList<>();
        int offset = 0;
        for (int i = 0; i < sfCount; i++) {
            MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
            info.offset = offset;
            info.presentationTimeUs = pts;
            info.flags = 0;
            if ((flags & MediaExtractor.SAMPLE_FLAG_SYNC) != 0) {
                info.flags |= MediaCodec.BUFFER_FLAG_KEY_FRAME;
            }
            if ((flags & MediaExtractor.SAMPLE_FLAG_PARTIAL_FRAME) != 0) {
                info.flags |= MediaCodec.BUFFER_FLAG_PARTIAL_FRAME;
            }
            if (i != sfCount - 1) {
                info.size = size / sfCount;
                info.flags |= MediaExtractor.SAMPLE_FLAG_PARTIAL_FRAME;
            } else {
                info.size = size - offset;
            }
            list.add(info);
            offset += info.size;
        }
        return list;
    }

    @Parameterized.Parameters(name = "{index}({0}_{1})")
    public static Collection<Object[]> input() {
        final boolean isEncoder = false;
        final boolean needAudio = true;
        final boolean needVideo = true;
        // mime, testClip, referenceClip, reconfigureTestClip, refRmsError, refCRC32
        final List<Object[]> exhaustiveArgsList = Arrays.asList(new Object[][]{
                {MediaFormat.MIMETYPE_AUDIO_MPEG, "bbb_1ch_8kHz_lame_cbr.mp3",
                        "bbb_1ch_8kHz_s16le.raw", "bbb_2ch_44kHz_lame_vbr.mp3", 91.022f, -1L},
                {MediaFormat.MIMETYPE_AUDIO_MPEG, "bbb_2ch_44kHz_lame_cbr.mp3",
                        "bbb_2ch_44kHz_s16le.raw", "bbb_1ch_16kHz_lame_vbr.mp3", 103.60f, -1L},
                {MediaFormat.MIMETYPE_AUDIO_AMR_WB, "bbb_1ch_16kHz_16kbps_amrwb.3gp",
                        "bbb_1ch_16kHz_s16le.raw", "bbb_1ch_16kHz_23kbps_amrwb.3gp", 2393.598f,
                        -1L},
                {MediaFormat.MIMETYPE_AUDIO_AMR_NB, "bbb_1ch_8kHz_10kbps_amrnb.3gp",
                        "bbb_1ch_8kHz_s16le.raw", "bbb_1ch_8kHz_8kbps_amrnb.3gp", -1.0f, -1L},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, "bbb_1ch_16kHz_flac.mka",
                        "bbb_1ch_16kHz_s16le.raw", "bbb_2ch_44kHz_flac.mka", 0.0f, -1L},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, "bbb_2ch_44kHz_flac.mka",
                        "bbb_2ch_44kHz_s16le.raw", "bbb_1ch_16kHz_flac.mka", 0.0f, -1L},
                {MediaFormat.MIMETYPE_AUDIO_RAW, "bbb_1ch_16kHz.wav", "bbb_1ch_16kHz_s16le.raw",
                        "bbb_2ch_44kHz.wav", 0.0f, -1L},
                {MediaFormat.MIMETYPE_AUDIO_RAW, "bbb_2ch_44kHz.wav", "bbb_2ch_44kHz_s16le.raw",
                        "bbb_1ch_16kHz.wav", 0.0f, -1L},
                {MediaFormat.MIMETYPE_AUDIO_G711_ALAW, "bbb_1ch_8kHz_alaw.wav",
                        "bbb_1ch_8kHz_s16le.raw", "bbb_2ch_8kHz_alaw.wav", 23.08678f, -1L},
                {MediaFormat.MIMETYPE_AUDIO_G711_MLAW, "bbb_1ch_8kHz_mulaw.wav",
                        "bbb_1ch_8kHz_s16le.raw", "bbb_2ch_8kHz_mulaw.wav", 24.4131f, -1L},
                {MediaFormat.MIMETYPE_AUDIO_MSGSM, "bbb_1ch_8kHz_gsm.wav",
                        "bbb_1ch_8kHz_s16le.raw", "bbb_1ch_8kHz_gsm.wav", 946.02698f, -1L},
                {MediaFormat.MIMETYPE_AUDIO_VORBIS, "bbb_1ch_16kHz_vorbis.mka",
                        "bbb_1ch_8kHz_s16le.raw", "bbb_2ch_44kHz_vorbis.mka", -1.0f, -1L},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, "bbb_2ch_48kHz_opus.mka",
                        "bbb_2ch_48kHz_s16le.raw", "bbb_1ch_48kHz_opus.mka", -1.0f, -1L},
                {MediaFormat.MIMETYPE_AUDIO_AAC, "bbb_1ch_16kHz_aac.mp4",
                        "bbb_1ch_16kHz_s16le.raw", "bbb_2ch_44kHz_aac.mp4", -1.0f, -1L},
                {MediaFormat.MIMETYPE_VIDEO_MPEG2, "bbb_340x280_768kbps_30fps_mpeg2.mp4", null,
                        "bbb_520x390_1mbps_30fps_mpeg2.mp4", -1.0f, -1L},
                {MediaFormat.MIMETYPE_VIDEO_AVC, "bbb_340x280_768kbps_30fps_avc.mp4", null,
                        "bbb_520x390_1mbps_30fps_avc.mp4", -1.0f, 1746312400L},
                {MediaFormat.MIMETYPE_VIDEO_HEVC, "bbb_520x390_1mbps_30fps_hevc.mp4", null,
                        "bbb_340x280_768kbps_30fps_hevc.mp4", -1.0f, 3061322606L},
                {MediaFormat.MIMETYPE_VIDEO_MPEG4, "bbb_128x96_64kbps_12fps_mpeg4.mp4",
                        null, "bbb_176x144_192kbps_15fps_mpeg4.mp4", -1.0f, -1L},
                {MediaFormat.MIMETYPE_VIDEO_H263, "bbb_176x144_128kbps_15fps_h263.3gp",
                        null, "bbb_176x144_192kbps_10fps_h263.3gp", -1.0f, -1L},
                {MediaFormat.MIMETYPE_VIDEO_VP8, "bbb_340x280_768kbps_30fps_vp8.webm", null,
                        "bbb_520x390_1mbps_30fps_vp8.webm", -1.0f, 2030620796L},
                {MediaFormat.MIMETYPE_VIDEO_VP9, "bbb_340x280_768kbps_30fps_vp9.webm", null,
                        "bbb_520x390_1mbps_30fps_vp9.webm", -1.0f, 4122701060L},
                {MediaFormat.MIMETYPE_VIDEO_AV1, "bbb_340x280_768kbps_30fps_av1.mp4", null,
                        "bbb_520x390_1mbps_30fps_av1.mp4", -1.0f, 400672933L},
        });
        return prepareParamList(exhaustiveArgsList, isEncoder, needAudio, needVideo, true);
    }

    private native boolean nativeTestSimpleDecode(String decoder, Surface surface, String mime,
            String testFile, String refFile, float rmsError, long checksum);

    static void verify(OutputManager outBuff, String refFile, float rmsError, long refCRC)
            throws IOException {
        if (rmsError >= 0) {
            short[] refData = setUpAudioReference(mInpPrefix + refFile);
            float currError = outBuff.getRmsError(refData);
            float errMargin = rmsError * RMS_ERROR_TOLERANCE;
            assertTrue(String.format("%s rms error too high ref/exp/got %f/%f/%f", refFile,
                    rmsError, errMargin, currError), currError <= errMargin);
        } else if (refCRC >= 0) {
            assertEquals("checksum mismatch", refCRC, outBuff.getCheckSumImage());
        }
    }

    /**
     * Tests decoder for combinations:
     * 1. Codec Sync Mode, Signal Eos with Last frame
     * 2. Codec Sync Mode, Signal Eos Separately
     * 3. Codec Async Mode, Signal Eos with Last frame
     * 4. Codec Async Mode, Signal Eos Separately
     * In all these scenarios, Timestamp ordering is verified, For audio the Rms of output has to be
     * within the allowed tolerance. The output has to be consistent (not flaky) in all runs.
     */
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testSimpleDecode() throws IOException, InterruptedException {
        MediaFormat format = setUpSource(mTestFile);
        boolean[] boolStates = {true, false};
        mSaveToMem = true;
        OutputManager ref = new OutputManager();
        OutputManager test = new OutputManager();
        {
            mCodec = MediaCodec.createByCodecName(mCodecName);
            assertTrue("codec name act/got: " + mCodec.getName() + '/' + mCodecName,
                    mCodec.getName().equals(mCodecName));
            assertTrue("error! codec canonical name is null",
                    mCodec.getCanonicalName() != null && !mCodec.getCanonicalName().isEmpty());
            validateMetrics(mCodecName);
            int loopCounter = 0;
            for (boolean eosType : boolStates) {
                for (boolean isAsync : boolStates) {
                    boolean validateFormat = true;
                    String log = String.format("codec: %s, file: %s, mode: %s, eos type: %s:: ",
                            mCodecName, mTestFile, (isAsync ? "async" : "sync"),
                            (eosType ? "eos with last frame" : "eos separate"));
                    mOutputBuff = loopCounter == 0 ? ref : test;
                    mOutputBuff.reset();
                    mExtractor.seekTo(0, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                    configureCodec(format, isAsync, eosType, false);
                    MediaFormat defFormat = mCodec.getOutputFormat();
                    if (isFormatSimilar(format, defFormat)) {
                        if (ENABLE_LOGS) {
                            Log.d("Input format is same as default for format for %s", mCodecName);
                        }
                        validateFormat = false;
                    }
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
                    if (loopCounter != 0) {
                        assertTrue(log + "decoder output is flaky", ref.equals(test));
                    } else {
                        if (mIsAudio) {
                            assertTrue(log + " pts is not strictly increasing",
                                    ref.isPtsStrictlyIncreasing(mPrevOutputPts));
                        } else {
                            assertTrue(
                                    log + " input pts list and output pts list are not identical",
                                    ref.isOutPtsListIdenticalToInpPtsList(false));
                        }
                    }
                    if (validateFormat) {
                        assertTrue(log + "not received format change",
                                mIsCodecInAsyncMode ? mAsyncHandle.hasOutputFormatChanged() :
                                        mSignalledOutFormatChanged);
                        assertTrue(log + "configured format and output format are not similar",
                                isFormatSimilar(format,
                                        mIsCodecInAsyncMode ? mAsyncHandle.getOutputFormat() :
                                                mOutFormat));
                    }
                    loopCounter++;
                }
            }
            mCodec.release();
            if (mSaveToMem) verify(mOutputBuff, mRefFile, mRmsError, mRefCRC);
            assertTrue(nativeTestSimpleDecode(mCodecName, null, mMime, mInpPrefix + mTestFile,
                    mInpPrefix + mRefFile, mRmsError, ref.getCheckSumBuffer()));
        }
        mExtractor.release();
    }

    /**
     * Tests flush when codec is in sync and async mode. In these scenarios, Timestamp
     * ordering is verified. The output has to be consistent (not flaky) in all runs
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
        {
            decodeToMemory(mTestFile, mCodecName, pts, mode, Integer.MAX_VALUE);
            OutputManager ref = mOutputBuff;
            if (mIsAudio) {
                assertTrue("reference output pts is not strictly increasing",
                        ref.isPtsStrictlyIncreasing(mPrevOutputPts));
            } else {
                assertTrue("input pts list and output pts list are not identical",
                        ref.isOutPtsListIdenticalToInpPtsList(false));
            }
            mOutputBuff = test;
            setUpSource(mTestFile);
            mCodec = MediaCodec.createByCodecName(mCodecName);
            for (boolean isAsync : boolStates) {
                String log = String.format("decoder: %s, input file: %s, mode: %s:: ", mCodecName,
                        mTestFile, (isAsync ? "async" : "sync"));
                mExtractor.seekTo(0, mode);
                configureCodec(format, isAsync, true, false);
                MediaFormat defFormat = mCodec.getOutputFormat();
                boolean validateFormat = true;
                if (isFormatSimilar(format, defFormat)) {
                    if (ENABLE_LOGS) {
                        Log.d("Input format is same as default for format for %s", mCodecName);
                    }
                    validateFormat = false;
                }
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

                boolean checkMetrics = (mOutputCount != 0);

                /* test flush in running state */
                flushCodec();
                if (checkMetrics) validateMetrics(mCodecName, format);
                if (mIsCodecInAsyncMode) mCodec.start();
                mSaveToMem = true;
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
                if (validateFormat) {
                    assertTrue(log + "not received format change",
                            mIsCodecInAsyncMode ? mAsyncHandle.hasOutputFormatChanged() :
                                    mSignalledOutFormatChanged);
                    assertTrue(log + "configured format and output format are not similar",
                            isFormatSimilar(format,
                                    mIsCodecInAsyncMode ? mAsyncHandle.getOutputFormat() :
                                            mOutFormat));
                }
                mSaveToMem = false;
            }
            mCodec.release();
            mExtractor.release();
        }
    }

    private native boolean nativeTestFlush(String decoder, Surface surface, String mime,
            String testFile);

    @Ignore("TODO(b/147576107)")
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testFlushNative() {
        {
            assertTrue(nativeTestFlush(mCodecName, null, mMime, mInpPrefix + mTestFile));
        }
    }

    /**
     * Tests reconfigure when codec is in sync and async mode. In these scenarios, Timestamp
     * ordering is verified. The output has to be consistent (not flaky) in all runs
     */
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testReconfigure() throws IOException, InterruptedException {
        Assume.assumeTrue("Test needs Android 11", IS_AT_LEAST_R);

        MediaFormat format = setUpSource(mTestFile);
        mExtractor.release();
        MediaFormat newFormat = setUpSource(mReconfigFile);
        mExtractor.release();
        final long startTs = 0;
        final long seekTs = 500000;
        final int mode = MediaExtractor.SEEK_TO_CLOSEST_SYNC;
        boolean[] boolStates = {true, false};
        OutputManager test = new OutputManager();
        {
            decodeToMemory(mTestFile, mCodecName, startTs, mode, Integer.MAX_VALUE);
            OutputManager ref = mOutputBuff;
            decodeToMemory(mReconfigFile, mCodecName, seekTs, mode, Integer.MAX_VALUE);
            OutputManager configRef = mOutputBuff;
            if (mIsAudio) {
                assertTrue("reference output pts is not strictly increasing",
                        ref.isPtsStrictlyIncreasing(mPrevOutputPts));
                assertTrue("config reference output pts is not strictly increasing",
                        configRef.isPtsStrictlyIncreasing(mPrevOutputPts));
            } else {
                assertTrue("input pts list and reference pts list are not identical",
                        ref.isOutPtsListIdenticalToInpPtsList(false));
                assertTrue("input pts list and reconfig ref output pts list are not identical",
                        ref.isOutPtsListIdenticalToInpPtsList(false));
            }
            mOutputBuff = test;
            mCodec = MediaCodec.createByCodecName(mCodecName);
            for (boolean isAsync : boolStates) {
                setUpSource(mTestFile);
                String log = String.format("decoder: %s, input file: %s, mode: %s:: ", mCodecName,
                        mTestFile, (isAsync ? "async" : "sync"));
                mExtractor.seekTo(startTs, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                configureCodec(format, isAsync, true, false);
                MediaFormat defFormat = mCodec.getOutputFormat();
                boolean validateFormat = true;
                if (isFormatSimilar(format, defFormat)) {
                    if (ENABLE_LOGS) {
                        Log.d("Input format is same as default for format for %s", mCodecName);
                    }
                    validateFormat = false;
                }

                /* test reconfigure in stopped state */
                reConfigureCodec(format, !isAsync, false, false);
                mCodec.start();

                /* test reconfigure in running state before queuing input */
                reConfigureCodec(format, !isAsync, false, false);
                mCodec.start();
                doWork(23);

                if (mOutputCount != 0) {
                    if (validateFormat) {
                        assertTrue(log + "not received format change",
                                mIsCodecInAsyncMode ? mAsyncHandle.hasOutputFormatChanged() :
                                        mSignalledOutFormatChanged);
                        assertTrue(log + "configured format and output format are not similar",
                                isFormatSimilar(format,
                                        mIsCodecInAsyncMode ? mAsyncHandle.getOutputFormat() :
                                                mOutFormat));
                    }
                    validateMetrics(mCodecName, format);
                }

                /* test reconfigure codec in running state */
                reConfigureCodec(format, isAsync, true, false);
                mCodec.start();
                mSaveToMem = true;
                test.reset();
                mExtractor.seekTo(startTs, mode);
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                if (mSaveToMem) verify(mOutputBuff, mRefFile, mRmsError, mRefCRC);
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                assertTrue(log + "decoder output is flaky", ref.equals(test));
                if (validateFormat) {
                    assertTrue(log + "not received format change",
                            mIsCodecInAsyncMode ? mAsyncHandle.hasOutputFormatChanged() :
                                    mSignalledOutFormatChanged);
                    assertTrue(log + "configured format and output format are not similar",
                            isFormatSimilar(format,
                                    mIsCodecInAsyncMode ? mAsyncHandle.getOutputFormat() :
                                            mOutFormat));
                }

                /* test reconfigure codec at eos state */
                reConfigureCodec(format, !isAsync, false, false);
                mCodec.start();
                test.reset();
                mExtractor.seekTo(startTs, mode);
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                if (mSaveToMem) verify(mOutputBuff, mRefFile, mRmsError, mRefCRC);
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                assertTrue(log + "decoder output is flaky", ref.equals(test));
                if (validateFormat) {
                    assertTrue(log + "not received format change",
                            mIsCodecInAsyncMode ? mAsyncHandle.hasOutputFormatChanged() :
                                    mSignalledOutFormatChanged);
                    assertTrue(log + "configured format and output format are not similar",
                            isFormatSimilar(format,
                                    mIsCodecInAsyncMode ? mAsyncHandle.getOutputFormat() :
                                            mOutFormat));
                }
                mExtractor.release();

                /* test reconfigure codec for new file */
                setUpSource(mReconfigFile);
                log = String.format("decoder: %s, input file: %s, mode: %s:: ", mCodecName,
                        mReconfigFile, (isAsync ? "async" : "sync"));
                reConfigureCodec(newFormat, isAsync, false, false);
                if (isFormatSimilar(newFormat, defFormat)) {
                    if (ENABLE_LOGS) {
                        Log.d("Input format is same as default for format for %s", mCodecName);
                    }
                    validateFormat = false;
                }
                mCodec.start();
                test.reset();
                mExtractor.seekTo(seekTs, mode);
                doWork(Integer.MAX_VALUE);
                queueEOS();
                waitForAllOutputs();
                validateMetrics(mCodecName, newFormat);
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                assertTrue(log + "decoder output is flaky", configRef.equals(test));
                if (validateFormat) {
                    assertTrue(log + "not received format change",
                            mIsCodecInAsyncMode ? mAsyncHandle.hasOutputFormatChanged() :
                                    mSignalledOutFormatChanged);
                    assertTrue(log + "configured format and output format are not similar",
                            isFormatSimilar(newFormat,
                                    mIsCodecInAsyncMode ? mAsyncHandle.getOutputFormat() :
                                            mOutFormat));
                }
                mSaveToMem = false;
                mExtractor.release();
            }
            mCodec.release();
        }
    }

    /**
     * Tests decoder for only EOS frame
     */
    @SmallTest
    @Test(timeout = PER_TEST_TIMEOUT_SMALL_TEST_MS)
    public void testOnlyEos() throws IOException, InterruptedException {
        MediaFormat format = setUpSource(mTestFile);
        boolean[] boolStates = {true, false};
        OutputManager ref = new OutputManager();
        OutputManager test = new OutputManager();
        mSaveToMem = true;
        {
            mCodec = MediaCodec.createByCodecName(mCodecName);
            int loopCounter = 0;
            for (boolean isAsync : boolStates) {
                String log = String.format("decoder: %s, input file: %s, mode: %s:: ", mCodecName,
                        mTestFile, (isAsync ? "async" : "sync"));
                configureCodec(format, isAsync, false, false);
                mOutputBuff = loopCounter == 0 ? ref : test;
                mOutputBuff.reset();
                mCodec.start();
                queueEOS();
                waitForAllOutputs();
                mCodec.stop();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                if (loopCounter != 0) {
                    assertTrue(log + "decoder output is flaky", ref.equals(test));
                } else {
                    if (mIsAudio) {
                        assertTrue(log + " pts is not strictly increasing",
                                ref.isPtsStrictlyIncreasing(mPrevOutputPts));
                    } else {
                        assertTrue(
                                log + " input pts list and output pts list are not identical",
                                ref.isOutPtsListIdenticalToInpPtsList(false));
                    }
                }
                loopCounter++;
            }
            mCodec.release();
        }
        mExtractor.release();
    }

    private native boolean nativeTestOnlyEos(String decoder, String mime, String testFile);

    @SmallTest
    @Test
    public void testOnlyEosNative() {
        {
            assertTrue(nativeTestOnlyEos(mCodecName, mMime, mInpPrefix + mTestFile));
        }
    }

    /**
     * Test Decoder by Queuing CSD separately
     */
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testSimpleDecodeQueueCSD() throws IOException, InterruptedException {
        MediaFormat format = setUpSource(mTestFile);
        if (!hasCSD(format)) {
            mExtractor.release();
            return;
        }
        ArrayList<MediaFormat> formats = new ArrayList<>();
        formats.add(format);
        formats.add(new MediaFormat(format));
        for (int i = 0; ; i++) {
            String csdKey = "csd-" + i;
            if (format.containsKey(csdKey)) {
                mCsdBuffers.add(format.getByteBuffer(csdKey).duplicate());
                format.removeKey(csdKey);
            } else break;
        }
        boolean[] boolStates = {true, false};
        mSaveToMem = true;
        OutputManager ref = new OutputManager();
        OutputManager test = new OutputManager();
        {
            mCodec = MediaCodec.createByCodecName(mCodecName);
            int loopCounter = 0;
            for (int i = 0; i < formats.size(); i++) {
                MediaFormat fmt = formats.get(i);
                for (boolean eosMode : boolStates) {
                    for (boolean isAsync : boolStates) {
                        boolean validateFormat = true;
                        String log = String.format("codec: %s, file: %s, mode: %s, eos type: %s:: ",
                                mCodecName, mTestFile, (isAsync ? "async" : "sync"),
                                (eosMode ? "eos with last frame" : "eos separate"));
                        mOutputBuff = loopCounter == 0 ? ref : test;
                        mOutputBuff.reset();
                        mExtractor.seekTo(0, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                        configureCodec(fmt, isAsync, eosMode, false);
                        MediaFormat defFormat = mCodec.getOutputFormat();
                        if (isFormatSimilar(defFormat, format)) {
                            if (ENABLE_LOGS) {
                                Log.d("Input format is same as default for format for %s",
                                        mCodecName);
                            }
                            validateFormat = false;
                        }
                        mCodec.start();
                        if (i == 0) queueCodecConfig();
                        doWork(Integer.MAX_VALUE);
                        queueEOS();
                        waitForAllOutputs();
                        validateMetrics(mCodecName);
                        /* TODO(b/147348711) */
                        if (false) mCodec.stop();
                        else mCodec.reset();
                        assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                        assertTrue(log + "no input sent", 0 != mInputCount);
                        assertTrue(log + "output received", 0 != mOutputCount);
                        if (loopCounter != 0) {
                            assertTrue(log + "decoder output is flaky", ref.equals(test));
                        } else {
                            if (mIsAudio) {
                                assertTrue(log + " pts is not strictly increasing",
                                        ref.isPtsStrictlyIncreasing(mPrevOutputPts));
                            } else {
                                assertTrue(
                                        log + " input pts list and output pts list are not identical",
                                        ref.isOutPtsListIdenticalToInpPtsList(false));
                            }
                        }
                        if (validateFormat) {
                            assertTrue(log + "not received format change",
                                    mIsCodecInAsyncMode ? mAsyncHandle.hasOutputFormatChanged() :
                                            mSignalledOutFormatChanged);
                            assertTrue(log + "configured format and output format are not similar",
                                    isFormatSimilar(format,
                                            mIsCodecInAsyncMode ? mAsyncHandle.getOutputFormat() :
                                                    mOutFormat));
                        }
                        loopCounter++;
                    }
                }
            }
            mCodec.release();
        }
        mExtractor.release();
    }

    private native boolean nativeTestSimpleDecodeQueueCSD(String decoder, String mime,
            String testFile);

    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testSimpleDecodeQueueCSDNative() throws IOException {
        MediaFormat format = setUpSource(mTestFile);
        if (!hasCSD(format)) {
            mExtractor.release();
            return;
        }
        {
            assertTrue(nativeTestSimpleDecodeQueueCSD(mCodecName, mMime, mInpPrefix + mTestFile));
        }
        mExtractor.release();
    }

    /**
     * Test decoder for partial frame
     */
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testDecodePartialFrame() throws IOException, InterruptedException {
        Assume.assumeTrue(isFeatureSupported(mCodecName, mMime,
                MediaCodecInfo.CodecCapabilities.FEATURE_PartialFrame));
        MediaFormat format = setUpSource(mTestFile);
        boolean[] boolStates = {true, false};
        int frameLimit = 10;
        ByteBuffer buffer = ByteBuffer.allocate(4 * 1024 * 1024);
        OutputManager test = new OutputManager();
        {
            decodeToMemory(mTestFile, mCodecName, 0, MediaExtractor.SEEK_TO_CLOSEST_SYNC,
                    frameLimit);
            mCodec = MediaCodec.createByCodecName(mCodecName);
            OutputManager ref = mOutputBuff;
            if (mIsAudio) {
                assertTrue("reference output pts is not strictly increasing",
                        ref.isPtsStrictlyIncreasing(mPrevOutputPts));
            } else {
                assertTrue("input pts list and output pts list are not identical",
                        ref.isOutPtsListIdenticalToInpPtsList(false));
            }
            mSaveToMem = true;
            mOutputBuff = test;
            for (boolean isAsync : boolStates) {
                String log = String.format("decoder: %s, input file: %s, mode: %s:: ", mCodecName,
                        mTestFile, (isAsync ? "async" : "sync"));
                mExtractor.seekTo(0, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                test.reset();
                configureCodec(format, isAsync, true, false);
                mCodec.start();
                doWork(frameLimit - 1);
                ArrayList<MediaCodec.BufferInfo> list = createSubFrames(buffer, 4);
                assertTrue("no sub frames in list received for " + mTestFile,
                        list != null && list.size() > 0);
                doWork(buffer, list);
                queueEOS();
                waitForAllOutputs();
                /* TODO(b/147348711) */
                if (false) mCodec.stop();
                else mCodec.reset();
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                assertTrue(log + "decoder output is not consistent with ref", ref.equals(test));
            }
            mCodec.release();
        }
        mExtractor.release();
    }
}
