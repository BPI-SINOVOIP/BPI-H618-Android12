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
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.opengl.GLES20;
import android.util.Log;
import android.util.Pair;
import android.view.Surface;

import androidx.test.filters.LargeTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import javax.microedition.khronos.opengles.GL10;

import static org.junit.Assert.assertTrue;

@RunWith(Parameterized.class)
public class EncodeDecodeAccuracyTest extends CodecDecoderTestBase {
    private final String LOG_TAG = EncodeDecodeAccuracyTest.class.getSimpleName();
    // The bitrates are configured to large values. The content has zero motion, so in-time the
    // qp of the encoded clips shall drop down to < 10. Further the color bands are aligned to 2,
    // so from downsampling rgb24 to yuv420p, even if bilinear filters are used as opposed to
    // skipping samples, we may not see large color loss. Hence allowable tolerance is kept to 8.
    // until QP stabilizes, the tolerance is set at 10.

    // TODO (b/193192195) Initial delta values used in the tests were 7 and 5, but were increased
    // to 10 and 8, as tests on emulator showed a higher delta. So the allowed delta values are
    // increased for now
    private final int TRANSIENT_STATE_COLOR_DELTA = 10;
    private final int STEADY_STATE_COLOR_DELTA = 8;
    private final int[][] mColorBars = new int[][]{
            {66, 133, 244},
            {219, 68, 55},
            {244, 180, 0},
            {15, 157, 88},
            {186, 85, 211},
            {119, 136, 153},
            {225, 179, 120},
            {224, 204, 151},
            {236, 121, 154},
            {159, 2, 81},
            {120, 194, 87}
    };
    private final int FRAMES_TO_ENCODE = 30;
    private final int STEADY_STATE_FRAME_INDEX = 20;

    private final String mCompName;
    private final int mWidth;
    private final int mHeight;
    private final int mFrameRate;
    private final int mBitRate;
    private final int mRange;
    private final int mStandard;
    private final int mTransferCurve;

    private final CodecAsyncHandler mAsyncHandleEncoder;
    private MediaCodec mEncoder;
    private Surface mInpSurface;
    private EGLWindowSurface mEGLWindowInpSurface;
    private OutputSurface mEGLWindowOutSurface;
    private boolean mSawInputEOSEnc;
    private boolean mSawOutputEOSEnc;
    private int mLatency;
    private boolean mReviseLatency;
    private int mInputCountEnc;
    private int mOutputCountEnc;
    private boolean mSaveToMemEnc;
    private OutputManager mOutputBuffEnc;
    ArrayList<MediaCodec.BufferInfo> mInfoListEnc;

    private final int mColorBarWidth;
    private final int xOffset;
    private final int yOffset = 64;
    private int mLargestColorDelta;
    private int mBadFrames;

    private final boolean mMuxOutput = false;
    private MediaMuxer mMuxer;
    private int mTrackID = -1;

    public EncodeDecodeAccuracyTest(String encoder, String mime, int width, int height,
            int frameRate, int bitrate, int range, int standard, int transfer) {
        super(null, mime, null);
        mCompName = encoder;
        mMime = mime;
        mWidth = width;
        mHeight = height;
        mFrameRate = frameRate;
        mBitRate = bitrate;
        mRange = range;
        mStandard = standard;
        mTransferCurve = transfer;
        mAsyncHandleEncoder = new CodecAsyncHandler();
        mLatency = 0;
        mReviseLatency = false;
        mInfoListEnc = new ArrayList<>();
        int barWidth = (mWidth + mColorBars.length - 1) / mColorBars.length;
        // aligning color bands to 2 is done because, during chroma subsampling of rgb24 to yuv420p,
        // simple skipping alternate samples or bilinear filter would have same effect.
        mColorBarWidth = (barWidth % 2 == 0) ? barWidth : barWidth + 1;
        assertTrue(mColorBarWidth >= 16);
        xOffset = mColorBarWidth >> 2;
    }

    @Parameterized.Parameters(name = "{index}({0}_{1}_{6}_{7}_{8})")
    public static Collection<Object[]> input() {
        final boolean isEncoder = true;
        final boolean needAudio = false;
        final boolean needVideo = true;
        final List<Object[]> baseArgsList = Arrays.asList(new Object[][]{
                // "video/*", width, height, framerate, bitrate, range, standard, transfer
                {720, 480, 30, 3000000, MediaFormat.COLOR_RANGE_LIMITED,
                        MediaFormat.COLOR_STANDARD_BT601_NTSC,
                        MediaFormat.COLOR_TRANSFER_SDR_VIDEO},
                {720, 576, 30, 3000000, MediaFormat.COLOR_RANGE_LIMITED,
                        MediaFormat.COLOR_STANDARD_BT601_PAL, MediaFormat.COLOR_TRANSFER_SDR_VIDEO},
                // TODO (b/186511593)
                /*
                {1280, 720, 30, 3000000, MediaFormat.COLOR_RANGE_LIMITED,
                        MediaFormat.COLOR_STANDARD_BT709, MediaFormat.COLOR_TRANSFER_SDR_VIDEO},
                {720, 480, 30, 3000000, MediaFormat.COLOR_RANGE_FULL,
                        MediaFormat.COLOR_STANDARD_BT601_NTSC,
                        MediaFormat.COLOR_TRANSFER_SDR_VIDEO},
                {720, 576, 30, 3000000, MediaFormat.COLOR_RANGE_FULL,
                        MediaFormat.COLOR_STANDARD_BT601_PAL, MediaFormat.COLOR_TRANSFER_SDR_VIDEO},
                {1280, 720, 30, 3000000, MediaFormat.COLOR_RANGE_FULL,
                        MediaFormat.COLOR_STANDARD_BT709, MediaFormat.COLOR_TRANSFER_SDR_VIDEO},

                {720, 480, 30, 3000000, MediaFormat.COLOR_RANGE_LIMITED,
                        MediaFormat.COLOR_STANDARD_BT709,
                        MediaFormat.COLOR_TRANSFER_SDR_VIDEO},
                {720, 576, 30, 3000000, MediaFormat.COLOR_RANGE_LIMITED,
                        MediaFormat.COLOR_STANDARD_BT709, MediaFormat.COLOR_TRANSFER_SDR_VIDEO},
                {1280, 720, 30, 3000000, MediaFormat.COLOR_RANGE_LIMITED,
                        MediaFormat.COLOR_STANDARD_BT601_PAL, MediaFormat.COLOR_TRANSFER_SDR_VIDEO},
                {720, 480, 30, 3000000, MediaFormat.COLOR_RANGE_FULL,
                        MediaFormat.COLOR_STANDARD_BT709,
                        MediaFormat.COLOR_TRANSFER_SDR_VIDEO},
                {720, 576, 30, 3000000, MediaFormat.COLOR_RANGE_FULL,
                        MediaFormat.COLOR_STANDARD_BT709, MediaFormat.COLOR_TRANSFER_SDR_VIDEO},
                {1280, 720, 30, 3000000, MediaFormat.COLOR_RANGE_FULL,
                        MediaFormat.COLOR_STANDARD_BT601_PAL, MediaFormat.COLOR_TRANSFER_SDR_VIDEO},
                 */
        });
        // Note: although vp8 and vp9 donot contain fields to signal color aspects properly, this
        // information can be muxed in to containers of mkv and mp4. So even those clips
        // should pass these tests
        final String[] mimes = {MediaFormat.MIMETYPE_VIDEO_AVC, MediaFormat.MIMETYPE_VIDEO_HEVC,
                MediaFormat.MIMETYPE_VIDEO_VP8, MediaFormat.MIMETYPE_VIDEO_VP9,
                MediaFormat.MIMETYPE_VIDEO_AV1};
        final List<Object[]> exhaustiveArgsList = new ArrayList<>();
        for (String mime : mimes) {
            for (Object[] obj : baseArgsList) {
                exhaustiveArgsList .add(new Object[]{mime, obj[0], obj[1], obj[2], obj[3], obj[4],
                        obj[5], obj[6]});
            }
        }
        return CodecTestBase.prepareParamList(exhaustiveArgsList, isEncoder, needAudio, needVideo,
                false);
    }

    private void resetContext(boolean isAsync) {
        super.resetContext(isAsync, false);
        mAsyncHandleEncoder.resetContext();
        mSawInputEOSEnc = false;
        mSawOutputEOSEnc = false;
        mLatency = 0;
        mReviseLatency = false;
        mInputCountEnc = 0;
        mOutputCountEnc = 0;
        mInfoListEnc.clear();
        mLargestColorDelta = 0;
        mBadFrames = 0;
    }

    private void configureCodec(MediaFormat encFormat, boolean isAsync) {
        resetContext(isAsync);
        mAsyncHandleEncoder.setCallBack(mEncoder, isAsync);
        mEncoder.configure(encFormat, null, MediaCodec.CONFIGURE_FLAG_ENCODE, null);
        if (mEncoder.getInputFormat().containsKey(MediaFormat.KEY_LATENCY)) {
            mReviseLatency = true;
            mLatency = mEncoder.getInputFormat().getInteger(MediaFormat.KEY_LATENCY);
        }
        mInpSurface = mEncoder.createInputSurface();
        assertTrue("Surface is not valid", mInpSurface.isValid());
        mEGLWindowInpSurface = new EGLWindowSurface(mInpSurface);
        if (ENABLE_LOGS) {
            Log.v(LOG_TAG, "codec configured");
        }
    }

    private void dequeueEncoderOutput(int bufferIndex, MediaCodec.BufferInfo info) {
        if (ENABLE_LOGS) {
            Log.v(LOG_TAG, "encoder output: id: " + bufferIndex + " flags: " + info.flags +
                    " size: " + info.size + " timestamp: " + info.presentationTimeUs);
        }
        if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
            mSawOutputEOSEnc = true;
        }
        if (info.size > 0) {
            if (mSaveToMemEnc) {
                ByteBuffer buf = mEncoder.getOutputBuffer(bufferIndex);
                MediaCodec.BufferInfo copy = new MediaCodec.BufferInfo();
                copy.set(mOutputBuffEnc.getOutStreamSize(), info.size, info.presentationTimeUs,
                        info.flags);
                mInfoListEnc.add(copy);
                mOutputBuffEnc.saveToMemory(buf, info);
                if (mMuxer != null) {
                    if (mTrackID == -1) {
                        mTrackID = mMuxer.addTrack(mEncoder.getOutputFormat());
                        mMuxer.start();
                    }
                    mMuxer.writeSampleData(mTrackID, buf, info);
                }
            }
            if ((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) == 0) {
                mOutputBuffEnc.saveOutPTS(info.presentationTimeUs);
                mOutputCountEnc++;
            }
        }
        mEncoder.releaseOutputBuffer(bufferIndex, false);
    }

    private void tryEncoderOutput(long timeOutUs) throws InterruptedException {
        if (mIsCodecInAsyncMode) {
            if (!mAsyncHandleEncoder.hasSeenError() && !mSawOutputEOSEnc) {
                int retry = 0;
                while (mReviseLatency) {
                    if (mAsyncHandleEncoder.hasOutputFormatChanged()) {
                        mReviseLatency = false;
                        int actualLatency = mAsyncHandleEncoder.getOutputFormat()
                                .getInteger(MediaFormat.KEY_LATENCY, mLatency);
                        if (mLatency < actualLatency) {
                            mLatency = actualLatency;
                            return;
                        }
                    } else {
                        if (retry > CodecTestBase.RETRY_LIMIT) {
                            throw new InterruptedException("did not receive output format " +
                                    "changed for encoder after " + CodecTestBase.Q_DEQ_TIMEOUT_US *
                                    CodecTestBase.RETRY_LIMIT + " us");
                        }
                        Thread.sleep(CodecTestBase.Q_DEQ_TIMEOUT_US / 1000);
                        retry++;
                    }
                }
                Pair<Integer, MediaCodec.BufferInfo> element = mAsyncHandleEncoder.getOutput();
                if (element != null) {
                    dequeueEncoderOutput(element.first, element.second);
                }
            }
        } else {
            MediaCodec.BufferInfo outInfo = new MediaCodec.BufferInfo();
            if (!mSawOutputEOSEnc) {
                int outputBufferId = mEncoder.dequeueOutputBuffer(outInfo, timeOutUs);
                if (outputBufferId >= 0) {
                    dequeueEncoderOutput(outputBufferId, outInfo);
                } else if (outputBufferId == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    mLatency = mEncoder.getOutputFormat()
                            .getInteger(MediaFormat.KEY_LATENCY, mLatency);
                }
            }
        }
    }

    private void waitForAllEncoderOutputs() throws InterruptedException {
        if (mIsCodecInAsyncMode) {
            while (!mAsyncHandleEncoder.hasSeenError() && !mSawOutputEOSEnc) {
                tryEncoderOutput(CodecTestBase.Q_DEQ_TIMEOUT_US);
            }
        } else {
            while (!mSawOutputEOSEnc) {
                tryEncoderOutput(CodecTestBase.Q_DEQ_TIMEOUT_US);
            }
        }
    }

    private void queueEOSEnc() {
        if (!mAsyncHandleEncoder.hasSeenError() && !mSawInputEOSEnc) {
            mEncoder.signalEndOfInputStream();
            mSawInputEOSEnc = true;
            if (ENABLE_LOGS) Log.d(LOG_TAG, "signalled end of stream");
        }
    }

    private void doWorkEnc(int frameLimit) throws InterruptedException {
        while (!mAsyncHandleEncoder.hasSeenError() && !mSawInputEOSEnc &&
                mInputCountEnc < frameLimit) {
            if (mInputCountEnc - mOutputCountEnc > mLatency) {
                tryEncoderOutput(CodecTestBase.Q_DEQ_TIMEOUT_US);
            }
            mEGLWindowInpSurface.makeCurrent();
            generateSurfaceFrame();
            mEGLWindowInpSurface
                    .setPresentationTime(computePresentationTime(mInputCountEnc) * 1000);
            if (ENABLE_LOGS) Log.d(LOG_TAG, "inputSurface swapBuffers");
            mEGLWindowInpSurface.swapBuffers();
            mInputCountEnc++;
        }
    }

    private MediaFormat setUpEncoderFormat() {
        MediaFormat format = new MediaFormat();
        format.setString(MediaFormat.KEY_MIME, mMime);
        format.setInteger(MediaFormat.KEY_WIDTH, mWidth);
        format.setInteger(MediaFormat.KEY_HEIGHT, mHeight);
        format.setInteger(MediaFormat.KEY_FRAME_RATE, mFrameRate);
        format.setInteger(MediaFormat.KEY_BIT_RATE, mBitRate);
        format.setFloat(MediaFormat.KEY_I_FRAME_INTERVAL, 1.0f);
        format.setInteger(MediaFormat.KEY_COLOR_FORMAT,
                MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
        format.setInteger(MediaFormat.KEY_COLOR_RANGE, mRange);
        format.setInteger(MediaFormat.KEY_COLOR_STANDARD, mStandard);
        format.setInteger(MediaFormat.KEY_COLOR_TRANSFER, mTransferCurve);
        return format;
    }

    private long computePresentationTime(int frameIndex) {
        return frameIndex * 1000000 / mFrameRate;
    }

    private void generateSurfaceFrame() {
        GLES20.glViewport(0, 0, mWidth, mHeight);
        GLES20.glEnable(GLES20.GL_SCISSOR_TEST);
        for (int i = 0; i < mColorBars.length; i++) {
            int startX = i * mColorBarWidth;
            int endX = Math.min(startX + mColorBarWidth, mWidth);
            GLES20.glScissor(startX, 0, endX, mHeight);
            GLES20.glClearColor(mColorBars[i][0] / 255.0f, mColorBars[i][1] / 255.0f,
                    mColorBars[i][2] / 255.0f, 1.0f);
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        }
    }

    boolean isColorClose(int actual, int expected) {
        int delta = Math.abs(actual - expected);
        if (delta > mLargestColorDelta) mLargestColorDelta = delta;
        return (delta <= (mOutputCount >= STEADY_STATE_FRAME_INDEX ? STEADY_STATE_COLOR_DELTA :
                TRANSIENT_STATE_COLOR_DELTA));
    }

    private boolean checkSurfaceFrame(int frameIndex) {
        ByteBuffer pixelBuf = ByteBuffer.allocateDirect(4);
        boolean frameFailed = false;
        for (int i = 0; i < mColorBars.length; i++) {
            int x = mColorBarWidth * i + xOffset;
            int y = yOffset;
            GLES20.glReadPixels(x, y, 1, 1, GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, pixelBuf);
            int r = pixelBuf.get(0) & 0xff;
            int g = pixelBuf.get(1) & 0xff;
            int b = pixelBuf.get(2) & 0xff;
            if (!(isColorClose(r, mColorBars[i][0]) && isColorClose(g, mColorBars[i][1]) &&
                    isColorClose(b, mColorBars[i][2]))) {
                Log.w(LOG_TAG, "Bad frame " + frameIndex + " (rect={" + x + " " + y + "} :rgb=" +
                        r + "," + g + "," + b + " vs. expected " + mColorBars[i][0] +
                        "," + mColorBars[i][1] + "," + mColorBars[i][2] + ")");
                frameFailed = true;
            }
        }
        return frameFailed;
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
        if (info.size > 0) {
            mEGLWindowOutSurface.awaitNewImage();
            mEGLWindowOutSurface.drawImage();
            if (checkSurfaceFrame(mOutputCount - 1)) mBadFrames++;
        }
    }

    private void decodeElementaryStream(MediaFormat format)
            throws IOException, InterruptedException {
        mEGLWindowOutSurface = new OutputSurface(mWidth, mHeight);
        mSurface = mEGLWindowOutSurface.getSurface();
        ArrayList<MediaFormat> formats = new ArrayList<>();
        formats.add(format);
        ArrayList<String> listOfDecoders =
                CodecDecoderTestBase.selectCodecs(mMime, formats, null, false);
        assertTrue("no suitable codecs found for : " + format.toString(),
                !listOfDecoders.isEmpty());
        for (String decoder : listOfDecoders) {
            mCodec = MediaCodec.createByCodecName(decoder);
            configureCodec(format, true, true, false);
            mOutputBuff = new OutputManager();
            mCodec.start();
            doWork(mOutputBuffEnc.getBuffer(), mInfoListEnc);
            queueEOS();
            waitForAllOutputs();
            mCodec.stop();
            mCodec.release();
        }
        mEGLWindowOutSurface.release();
    }

    /**
     * Current test encodes RGB frames at high bitrates (this is to ensure very minor quantization
     * losses). The Color Aspects information is passed in the bitstream or container format. The
     * decoder is expected to produce the color information accurately
     */
    @LargeTest
    @Test(timeout = CodecTestBase.PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testEncodeDecodeAccuracyRGB() throws IOException, InterruptedException {
        ArrayList<MediaFormat> formats = new ArrayList<>();
        formats.add(setUpEncoderFormat());
        final boolean isAsync = true;
        mSaveToMemEnc = true;
        mOutputBuffEnc = new OutputManager();
        String tmpPath;
        {
            if (mMuxOutput) {
                int muxerFormat;
                if (mMime.equals(MediaFormat.MIMETYPE_VIDEO_VP8) ||
                        mMime.equals(MediaFormat.MIMETYPE_VIDEO_VP9)) {
                    muxerFormat = MediaMuxer.OutputFormat.MUXER_OUTPUT_WEBM;
                    tmpPath = File.createTempFile("tmp", ".webm").getAbsolutePath();
                } else {
                    muxerFormat = MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4;
                    tmpPath = File.createTempFile("tmp", ".mp4").getAbsolutePath();
                }
                mMuxer = new MediaMuxer(tmpPath, muxerFormat);
            }
            mEncoder = MediaCodec.createByCodecName(mCompName);
            mOutputBuffEnc.reset();
            mInfoListEnc.clear();
            configureCodec(formats.get(0), isAsync);
            mEncoder.start();
            doWorkEnc(FRAMES_TO_ENCODE);
            queueEOSEnc();
            waitForAllEncoderOutputs();
            if (mMuxOutput) {
                if (mTrackID != -1) {
                    mMuxer.stop();
                    mTrackID = -1;
                }
                if (mMuxer != null) {
                    mMuxer.release();
                    mMuxer = null;
                }
            }
            MediaFormat outputFormat = mEncoder.getOutputFormat();
            assertTrue(outputFormat.containsKey(MediaFormat.KEY_COLOR_RANGE));
            assertTrue(outputFormat.containsKey(MediaFormat.KEY_COLOR_STANDARD));
            assertTrue(outputFormat.containsKey(MediaFormat.KEY_COLOR_TRANSFER));
            if (mMime.equals(MediaFormat.MIMETYPE_VIDEO_AVC) ||
                    mMime.equals(MediaFormat.MIMETYPE_VIDEO_HEVC)) {
                outputFormat.removeKey(MediaFormat.KEY_COLOR_RANGE);
                outputFormat.removeKey(MediaFormat.KEY_COLOR_STANDARD);
                outputFormat.removeKey(MediaFormat.KEY_COLOR_TRANSFER);
            }
            mEGLWindowInpSurface.release();
            mInpSurface.release();
            mInpSurface = null;
            mEncoder.reset();
            mEncoder.release();
            decodeElementaryStream(outputFormat);
            assertTrue("output pts list is not Strictly increasing",
                    mOutputBuffEnc.isPtsStrictlyIncreasing(-1));
            assertTrue("input pts list and output pts list are not identical",
                    mOutputBuff.isOutPtsListIdenticalToInpPtsList(false));
            assertTrue("mOutputCnt != mInputCnt", mOutputCount == FRAMES_TO_ENCODE);
            assertTrue("color difference exceeds allowed tolerance in " + mBadFrames + " out of " +
                    FRAMES_TO_ENCODE + " frames", 0 == mBadFrames);
            if (mMuxOutput) new File(tmpPath).delete();
        }
    }
}

