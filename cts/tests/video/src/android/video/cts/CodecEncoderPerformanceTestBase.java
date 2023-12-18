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

package android.video.cts;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

class CodecEncoderPerformanceTestBase extends CodecPerformanceTestBase {
    private static final String LOG_TAG = CodecEncoderPerformanceTest.class.getSimpleName();
    private static final Map<String, Float> transcodeAVCToTargetBitrateMap = new HashMap<>();

    final String mEncoderMime;
    final String mEncoderName;
    final int mBitrate;
    double mAchievedFps;

    private boolean mSawEncInputEOS = false;
    private boolean mSawEncOutputEOS = false;
    private int mEncOutputNum = 0;
    private MediaCodec mEncoder;
    private MediaFormat mEncoderFormat;

    // Suggested bitrate scaling factors for transcoding avc to target format.
    static {
        transcodeAVCToTargetBitrateMap.put(MediaFormat.MIMETYPE_VIDEO_VP8, 1.25f);
        transcodeAVCToTargetBitrateMap.put(MediaFormat.MIMETYPE_VIDEO_AVC, 1.0f);
        transcodeAVCToTargetBitrateMap.put(MediaFormat.MIMETYPE_VIDEO_VP9, 0.7f);
        transcodeAVCToTargetBitrateMap.put(MediaFormat.MIMETYPE_VIDEO_HEVC, 0.6f);
        transcodeAVCToTargetBitrateMap.put(MediaFormat.MIMETYPE_VIDEO_AV1, 0.4f);
    }

    public static float getBitrateScalingFactor(String mime) {
        return transcodeAVCToTargetBitrateMap.getOrDefault(mime, 1.5f);
    }

    public CodecEncoderPerformanceTestBase(String decoderName, String testFile, String encoderMime,
            String encoderName, int bitrate, int keyPriority, float scalingFactor) {
        super(decoderName, testFile, keyPriority, scalingFactor);
        mEncoderMime = encoderMime;
        mEncoderName = encoderName;
        mBitrate = bitrate;
    }

    static ArrayList<String> getMimesOfAvailableHardwareVideoEncoders() {
        MediaCodecList codecList = new MediaCodecList(MediaCodecList.REGULAR_CODECS);
        MediaCodecInfo[] codecInfos = codecList.getCodecInfos();
        ArrayList<String> listOfMimes = new ArrayList<>();
        for (MediaCodecInfo codecInfo : codecInfos) {
            if (!codecInfo.isEncoder() || !codecInfo.isHardwareAccelerated()) continue;
            String[] types = codecInfo.getSupportedTypes();
            for (String type : types) {
                if (type.startsWith("video/") && !listOfMimes.contains(type)) {
                    listOfMimes.add(type);
                }
            }
        }
        return listOfMimes;
    }

    public static MediaFormat setUpEncoderFormat(MediaFormat format, String mime, int bitrate) {
        MediaFormat fmt = new MediaFormat();
        fmt.setString(MediaFormat.KEY_MIME, mime);
        fmt.setInteger(MediaFormat.KEY_WIDTH, format.getInteger(MediaFormat.KEY_WIDTH));
        fmt.setInteger(MediaFormat.KEY_HEIGHT, format.getInteger(MediaFormat.KEY_HEIGHT));
        fmt.setInteger(MediaFormat.KEY_BIT_RATE, bitrate);
        fmt.setInteger(MediaFormat.KEY_FRAME_RATE,
                format.getInteger(MediaFormat.KEY_FRAME_RATE, 30));
        fmt.setFloat(MediaFormat.KEY_I_FRAME_INTERVAL, 1.0f);
        fmt.setInteger(MediaFormat.KEY_COLOR_FORMAT,
                MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
        return fmt;
    }

    private void dequeueEncoderOutput(int bufferIndex, MediaCodec.BufferInfo info) {
        if (info.size > 0 && (info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) == 0) {
            mEncOutputNum++;
        }
        if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
            mSawEncOutputEOS = true;
        }
        mEncoder.releaseOutputBuffer(bufferIndex, false);
    }

    private void setUpFormats(MediaFormat format) throws IOException {
        mDecoderFormat = new MediaFormat(format);
        mDecoderFormat.setInteger(MediaFormat.KEY_PRIORITY, mKeyPriority);
        mEncoderFormat = setUpEncoderFormat(mDecoderFormat, mEncoderMime, mBitrate);
        mEncoderFormat.setInteger(MediaFormat.KEY_PRIORITY, mKeyPriority);
        double maxOperatingRateDecoder = getMaxOperatingRate(mDecoderName, mDecoderMime);
        double maxOperatingRateEncoder = getMaxOperatingRate(mEncoderName, mEncoderMime);
        mOperatingRateExpected = Math.min(maxOperatingRateDecoder, maxOperatingRateEncoder);
        // As both decoder and encoder are running in concurrently, expected rate is halved
        mOperatingRateExpected /= 2.0;
        if (mMaxOpRateScalingFactor > 0.0f) {
            int operatingRateToSet = (int) (mOperatingRateExpected * mMaxOpRateScalingFactor);
            if (mMaxOpRateScalingFactor < 1.0f) {
                mOperatingRateExpected = operatingRateToSet;
            }
            mDecoderFormat.setInteger(MediaFormat.KEY_OPERATING_RATE, operatingRateToSet);
            mEncoderFormat.setInteger(MediaFormat.KEY_OPERATING_RATE, operatingRateToSet);
        } else if (mMaxOpRateScalingFactor < 0.0f) {
            mDecoderFormat.setInteger(MediaFormat.KEY_OPERATING_RATE, -1);
            mEncoderFormat.setInteger(MediaFormat.KEY_OPERATING_RATE, -1);
        }
        mEncoderFormat.setInteger(MediaFormat.KEY_COMPLEXITY,
                getEncoderMinComplexity(mEncoderName, mEncoderMime));
    }

    private void doWork() {
        while (!mSawEncOutputEOS) {
            if (!mSawDecInputEOS) {
                int inputBufIndex = mDecoder.dequeueInputBuffer(Q_DEQ_TIMEOUT_US);
                if (inputBufIndex >= 0) {
                    enqueueDecoderInput(inputBufIndex);
                }
            }
            if (!mSawDecOutputEOS) {
                MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
                int outputBufIndex = mDecoder.dequeueOutputBuffer(info, Q_DEQ_TIMEOUT_US);
                if (outputBufIndex >= 0) {
                    dequeueDecoderOutput(outputBufIndex, info, true);
                }
            }
            if (mSawDecOutputEOS && !mSawEncInputEOS) {
                mEncoder.signalEndOfInputStream();
                mSawEncInputEOS = true;
            }
            MediaCodec.BufferInfo outInfo = new MediaCodec.BufferInfo();
            int outputBufferId = mEncoder.dequeueOutputBuffer(outInfo, Q_DEQ_TIMEOUT_US);
            if (outputBufferId >= 0) {
                dequeueEncoderOutput(outputBufferId, outInfo);
            }
        }
    }

    public void encode() throws IOException {
        MediaFormat format = setUpDecoderInput();
        assertNotNull("Video track not present in " + mTestFile, format);

        if (EXCLUDE_ENCODER_MAX_RESOLUTION) {
            int maxFrameSize = getMaxFrameSize(mEncoderName, mEncoderMime);
            assumeTrue(mWidth + "x" + mHeight + " is skipped as it not less than half of " +
                    "maximum frame size: " + maxFrameSize + " supported by the encoder.",
                    mWidth * mHeight < maxFrameSize / 2);
        }

        setUpFormats(format);
        mDecoder = MediaCodec.createByCodecName(mDecoderName);
        mEncoder = MediaCodec.createByCodecName(mEncoderName);
        mEncoder.configure(mEncoderFormat, null, MediaCodec.CONFIGURE_FLAG_ENCODE, null);
        mSurface = mEncoder.createInputSurface();
        assertTrue("Surface created is null.", mSurface != null);
        assertTrue("Surface is not valid", mSurface.isValid());
        mDecoder.configure(mDecoderFormat, mSurface, null, 0);
        mDecoder.start();
        mEncoder.start();
        long start = System.currentTimeMillis();
        doWork();
        long finish = System.currentTimeMillis();
        mEncoder.stop();
        mSurface.release();
        mEncoder.release();
        mDecoder.stop();
        mDecoder.release();
        mEncoder = null;
        mDecoder = null;
        assertTrue("Encoder output count is zero", mEncOutputNum > 0);
        mAchievedFps = mEncOutputNum / ((finish - start) / 1000.0);
    }

    @Override
    protected void finalize() throws Throwable {
        if (mDecoder != null) {
            mDecoder.release();
            mDecoder = null;
        }
        if (mEncoder != null) {
            mEncoder.release();
            mEncoder = null;
        }
    }
}
