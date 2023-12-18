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

import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.media.Image;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.os.Build;
import android.os.PersistableBundle;
import android.util.Log;
import android.util.Pair;
import android.view.Surface;

import androidx.annotation.NonNull;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Assert;
import org.junit.Before;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.zip.CRC32;

import com.android.compatibility.common.util.ApiLevelUtil;

import static android.media.MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface;
import static android.media.MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

class CodecAsyncHandler extends MediaCodec.Callback {
    private static final String LOG_TAG = CodecAsyncHandler.class.getSimpleName();
    private final Lock mLock = new ReentrantLock();
    private final Condition mCondition = mLock.newCondition();
    private final LinkedList<Pair<Integer, MediaCodec.BufferInfo>> mCbInputQueue;
    private final LinkedList<Pair<Integer, MediaCodec.BufferInfo>> mCbOutputQueue;
    private MediaFormat mOutFormat;
    private boolean mSignalledOutFormatChanged;
    private volatile boolean mSignalledError;

    CodecAsyncHandler() {
        mCbInputQueue = new LinkedList<>();
        mCbOutputQueue = new LinkedList<>();
        mSignalledError = false;
        mSignalledOutFormatChanged = false;
    }

    void clearQueues() {
        mLock.lock();
        mCbInputQueue.clear();
        mCbOutputQueue.clear();
        mLock.unlock();
    }

    void resetContext() {
        clearQueues();
        mOutFormat = null;
        mSignalledOutFormatChanged = false;
        mSignalledError = false;
    }

    @Override
    public void onInputBufferAvailable(@NonNull MediaCodec codec, int bufferIndex) {
        assertTrue(bufferIndex >= 0);
        mLock.lock();
        mCbInputQueue.add(new Pair<>(bufferIndex, (MediaCodec.BufferInfo) null));
        mCondition.signalAll();
        mLock.unlock();
    }

    @Override
    public void onOutputBufferAvailable(@NonNull MediaCodec codec, int bufferIndex,
            @NonNull MediaCodec.BufferInfo info) {
        assertTrue(bufferIndex >= 0);
        mLock.lock();
        mCbOutputQueue.add(new Pair<>(bufferIndex, info));
        mCondition.signalAll();
        mLock.unlock();
    }

    @Override
    public void onError(@NonNull MediaCodec codec, MediaCodec.CodecException e) {
        mLock.lock();
        mSignalledError = true;
        mCondition.signalAll();
        mLock.unlock();
        Log.e(LOG_TAG, "received media codec error : " + e.getMessage());
    }

    @Override
    public void onOutputFormatChanged(@NonNull MediaCodec codec, @NonNull MediaFormat format) {
        mOutFormat = format;
        mSignalledOutFormatChanged = true;
        Log.i(LOG_TAG, "Output format changed: " + format.toString());
    }

    void setCallBack(MediaCodec codec, boolean isCodecInAsyncMode) {
        if (isCodecInAsyncMode) {
            codec.setCallback(this);
        } else {
            codec.setCallback(null);
        }
    }

    Pair<Integer, MediaCodec.BufferInfo> getInput() throws InterruptedException {
        Pair<Integer, MediaCodec.BufferInfo> element = null;
        mLock.lock();
        while (!mSignalledError) {
            if (mCbInputQueue.isEmpty()) {
                mCondition.await();
            } else {
                element = mCbInputQueue.remove(0);
                break;
            }
        }
        mLock.unlock();
        return element;
    }

    Pair<Integer, MediaCodec.BufferInfo> getOutput() throws InterruptedException {
        Pair<Integer, MediaCodec.BufferInfo> element = null;
        mLock.lock();
        while (!mSignalledError) {
            if (mCbOutputQueue.isEmpty()) {
                mCondition.await();
            } else {
                element = mCbOutputQueue.remove(0);
                break;
            }
        }
        mLock.unlock();
        return element;
    }

    Pair<Integer, MediaCodec.BufferInfo> getWork() throws InterruptedException {
        Pair<Integer, MediaCodec.BufferInfo> element = null;
        mLock.lock();
        while (!mSignalledError) {
            if (mCbInputQueue.isEmpty() && mCbOutputQueue.isEmpty()) {
                mCondition.await();
            } else {
                if (!mCbOutputQueue.isEmpty()) {
                    element = mCbOutputQueue.remove(0);
                    break;
                }
                if (!mCbInputQueue.isEmpty()) {
                    element = mCbInputQueue.remove(0);
                    break;
                }
            }
        }
        mLock.unlock();
        return element;
    }

    boolean isInputQueueEmpty() {
        mLock.lock();
        boolean isEmpty = mCbInputQueue.isEmpty();
        mLock.unlock();
        return isEmpty;
    }

    boolean hasSeenError() {
        return mSignalledError;
    }

    boolean hasOutputFormatChanged() {
        return mSignalledOutFormatChanged;
    }

    MediaFormat getOutputFormat() {
        return mOutFormat;
    }
}

class OutputManager {
    private static final String LOG_TAG = OutputManager.class.getSimpleName();
    private byte[] memory;
    private int memIndex;
    private CRC32 mCrc32UsingImage;
    private CRC32 mCrc32UsingBuffer;
    private ArrayList<Long> inpPtsList;
    private ArrayList<Long> outPtsList;

    OutputManager() {
        memory = new byte[1024];
        memIndex = 0;
        mCrc32UsingImage = new CRC32();
        mCrc32UsingBuffer = new CRC32();
        inpPtsList = new ArrayList<>();
        outPtsList = new ArrayList<>();
    }

    void saveInPTS(long pts) {
        // Add only Unique timeStamp, discarding any duplicate frame / non-display frame
        if (!inpPtsList.contains(pts)) {
            inpPtsList.add(pts);
        }
    }

    void saveOutPTS(long pts) {
        outPtsList.add(pts);
    }

    boolean isPtsStrictlyIncreasing(long lastPts) {
        boolean res = true;
        for (int i = 0; i < outPtsList.size(); i++) {
            if (lastPts < outPtsList.get(i)) {
                lastPts = outPtsList.get(i);
            } else {
                Log.e(LOG_TAG, "Timestamp ordering check failed: last timestamp: " + lastPts +
                        " current timestamp:" + outPtsList.get(i));
                res = false;
                break;
            }
        }
        return res;
    }

    boolean isOutPtsListIdenticalToInpPtsList(boolean requireSorting) {
        boolean res;
        Collections.sort(inpPtsList);
        if (requireSorting) {
            Collections.sort(outPtsList);
        }
        if (outPtsList.size() != inpPtsList.size()) {
            Log.e(LOG_TAG, "input and output presentation timestamp list sizes are not identical" +
                    "exp/rec" + inpPtsList.size() + '/' + outPtsList.size());
            return false;
        } else {
            int count = 0;
            for (int i = 0; i < outPtsList.size(); i++) {
                if (!outPtsList.get(i).equals(inpPtsList.get(i))) {
                    count ++;
                    Log.e(LOG_TAG, "input output pts mismatch, exp/rec " + outPtsList.get(i) + '/' +
                            inpPtsList.get(i));
                    if (count == 20) {
                        Log.e(LOG_TAG, "stopping after 20 mismatches, ...");
                        break;
                    }
                }
            }
            res = (count == 0);
        }
        return res;
    }

    int getOutStreamSize() {
        return memIndex;
    }

    void checksum(ByteBuffer buf, int size) {
        checksum(buf, size, 0, 0, 0);
    }

    void checksum(ByteBuffer buf, int size, int width, int height, int stride) {
        int cap = buf.capacity();
        assertTrue("checksum() params are invalid: size = " + size + " cap = " + cap,
                size > 0 && size <= cap);
        if (buf.hasArray()) {
            if (width > 0 && height > 0 && stride > 0) {
                int offset = buf.position() + buf.arrayOffset();
                byte[] bb = new byte[width * height];
                for (int i = 0; i < height; ++i) {
                    System.arraycopy(buf.array(), offset, bb, i * width, width);
                    offset += stride;
                }
                mCrc32UsingBuffer.update(bb, 0, width * height);
            } else {
                mCrc32UsingBuffer.update(buf.array(), buf.position() + buf.arrayOffset(), size);
            }
        } else if (width > 0 && height > 0 && stride > 0) {
            // Checksum only the Y plane
            int pos = buf.position();
            int offset = pos;
            byte[] bb = new byte[width * height];
            for (int i = 0; i < height; ++i) {
                buf.position(offset);
                buf.get(bb, i * width, width);
                offset += stride;
            }
            mCrc32UsingBuffer.update(bb, 0, width * height);
            buf.position(pos);
        } else {
            int pos = buf.position();
            final int rdsize = Math.min(4096, size);
            byte[] bb = new byte[rdsize];
            int chk;
            for (int i = 0; i < size; i += chk) {
                chk = Math.min(rdsize, size - i);
                buf.get(bb, 0, chk);
                mCrc32UsingBuffer.update(bb, 0, chk);
            }
            buf.position(pos);
        }
    }

    void checksum(Image image) {
        int format = image.getFormat();
        assertEquals("unexpected image format", ImageFormat.YUV_420_888, format);

        Rect cropRect = image.getCropRect();
        int imageWidth = cropRect.width();
        int imageHeight = cropRect.height();
        assertTrue("unexpected image dimensions", imageWidth > 0 && imageHeight > 0);

        int imageLeft = cropRect.left;
        int imageTop = cropRect.top;
        Image.Plane[] planes = image.getPlanes();
        for (int i = 0; i < planes.length; ++i) {
            ByteBuffer buf = planes[i].getBuffer();
            int width, height, rowStride, pixelStride, x, y, left, top;
            rowStride = planes[i].getRowStride();
            pixelStride = planes[i].getPixelStride();
            if (i == 0) {
                width = imageWidth;
                height = imageHeight;
                left = imageLeft;
                top = imageTop;
            } else {
                width = imageWidth / 2;
                height = imageHeight / 2;
                left = imageLeft / 2;
                top = imageTop / 2;
            }
            int cropOffset = left + top * rowStride;
            // local contiguous pixel buffer
            byte[] bb = new byte[width * height];
            if (buf.hasArray()) {
                byte[] b = buf.array();
                int offs = buf.arrayOffset() + cropOffset;
                if (pixelStride == 1) {
                    for (y = 0; y < height; ++y) {
                        System.arraycopy(b, offs + y * rowStride, bb, y * width, width);
                    }
                } else {
                    // do it pixel-by-pixel
                    for (y = 0; y < height; ++y) {
                        int lineOffset = offs + y * rowStride;
                        for (x = 0; x < width; ++x) {
                            bb[y * width + x] = b[lineOffset + x * pixelStride];
                        }
                    }
                }
            } else { // almost always ends up here due to direct buffers
                int base = buf.position();
                int pos = base + cropOffset;
                if (pixelStride == 1) {
                    for (y = 0; y < height; ++y) {
                        buf.position(pos + y * rowStride);
                        buf.get(bb, y * width, width);
                    }
                } else {
                    // local line buffer
                    byte[] lb = new byte[rowStride];
                    // do it pixel-by-pixel
                    for (y = 0; y < height; ++y) {
                        buf.position(pos + y * rowStride);
                        // we're only guaranteed to have pixelStride * (width - 1) + 1 bytes
                        buf.get(lb, 0, pixelStride * (width - 1) + 1);
                        for (x = 0; x < width; ++x) {
                            bb[y * width + x] = lb[x * pixelStride];
                        }
                    }
                }
                buf.position(base);
            }
            mCrc32UsingImage.update(bb, 0, width * height);
        }
    }

    void saveToMemory(ByteBuffer buf, MediaCodec.BufferInfo info) {
        if (memIndex + info.size >= memory.length) {
            memory = Arrays.copyOf(memory, memIndex + info.size);
        }
        buf.position(info.offset);
        buf.get(memory, memIndex, info.size);
        memIndex += info.size;
    }

    void position(int index) {
        if (index < 0 || index >= memory.length) index = 0;
        memIndex = index;
    }

    ByteBuffer getBuffer() {
        return ByteBuffer.wrap(memory);
    }

    void reset() {
        position(0);
        mCrc32UsingImage.reset();
        mCrc32UsingBuffer.reset();
        inpPtsList.clear();
        outPtsList.clear();
    }

    float getRmsError(short[] refData) {
        long totalErrorSquared = 0;
        assertTrue(0 == (memIndex & 1));
        short[] shortData = new short[memIndex / 2];
        ByteBuffer.wrap(memory, 0, memIndex).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer()
                .get(shortData);
        if (refData.length != shortData.length) return Float.MAX_VALUE;
        for (int i = 0; i < shortData.length; i++) {
            int d = shortData[i] - refData[i];
            totalErrorSquared += d * d;
        }
        long avgErrorSquared = (totalErrorSquared / shortData.length);
        return (float) Math.sqrt(avgErrorSquared);
    }

    long getCheckSumImage() {
        return mCrc32UsingImage.getValue();
    }

    long getCheckSumBuffer() {
        return mCrc32UsingBuffer.getValue();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        OutputManager that = (OutputManager) o;
        // TODO: Timestamps for deinterlaced content are under review. (E.g. can decoders
        // produce multiple progressive frames?) For now, do not verify timestamps.
        boolean isEqual = this.equalsInterlaced(o);
        if (!outPtsList.equals(that.outPtsList)) {
            isEqual = false;
            Log.e(LOG_TAG, "ref and test presentation timestamp mismatch");
        }
        return isEqual;
    }

    public boolean equalsInterlaced(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        OutputManager that = (OutputManager) o;
        boolean isEqual = true;
        if (mCrc32UsingImage.getValue() != that.mCrc32UsingImage.getValue()) {
            isEqual = false;
            Log.e(LOG_TAG, "ref and test crc32 checksums calculated using image mismatch " +
                          mCrc32UsingImage.getValue() + '/' + that.mCrc32UsingImage.getValue());
        }
        if (mCrc32UsingBuffer.getValue() != that.mCrc32UsingBuffer.getValue()) {
            isEqual = false;
            Log.e(LOG_TAG, "ref and test crc32 checksums calculated using buffer mismatch " +
                          mCrc32UsingBuffer.getValue() + '/' + that.mCrc32UsingBuffer.getValue());
            if (memIndex == that.memIndex) {
                int count = 0;
                for (int i = 0; i < memIndex; i++) {
                    if (memory[i] != that.memory[i]) {
                        count++;
                        if (count < 20) {
                            Log.d(LOG_TAG, "sample at " + i + " exp/got:: " + memory[i] + '/' +
                                    that.memory[i]);
                        }
                    }
                }
                if (count != 0) {
                    Log.e(LOG_TAG, "ref and test o/p samples mismatch " + count);
                }
            } else {
                Log.e(LOG_TAG, "ref and test o/p sizes mismatch " + memIndex + '/' + that.memIndex);
            }
        }
        return isEqual;
    }
}

abstract class CodecTestBase {
    public static final boolean IS_AT_LEAST_R = ApiLevelUtil.isAtLeast(Build.VERSION_CODES.R);
    private static final String LOG_TAG = CodecTestBase.class.getSimpleName();

    static final String CODEC_PREFIX_KEY = "codec-prefix";
    static final String MIME_SEL_KEY = "mime-sel";
    static final Map<String, String> codecSelKeyMimeMap = new HashMap<>();
    static final boolean ENABLE_LOGS = false;
    static final int PER_TEST_TIMEOUT_LARGE_TEST_MS = 300000;
    static final int PER_TEST_TIMEOUT_SMALL_TEST_MS = 60000;
    static final int UNSPECIFIED = 0;
    static final int CODEC_ALL = 0; // All codecs should support
    static final int CODEC_ANY = 1; // Atleast one codec should support
    static final int CODEC_OPTIONAL = 2; // Codec support is optional
    // Maintain Timeouts in sync with their counterpart in NativeMediaCommon.h
    static final long Q_DEQ_TIMEOUT_US = 5000; // block at most 5ms while looking for io buffers
    static final int RETRY_LIMIT = 100; // max poll counter before test aborts and returns error
    static final String INVALID_CODEC = "unknown.codec_";
    static final String mInpPrefix = WorkDir.getMediaDirString();
    static final PackageManager pm =
            InstrumentationRegistry.getInstrumentation().getContext().getPackageManager();
    static String mimeSelKeys;
    static String codecPrefix;

    CodecAsyncHandler mAsyncHandle;
    boolean mIsCodecInAsyncMode;
    boolean mSawInputEOS;
    boolean mSawOutputEOS;
    boolean mSignalEOSWithLastFrame;
    int mInputCount;
    int mOutputCount;
    long mPrevOutputPts;
    boolean mSignalledOutFormatChanged;
    MediaFormat mOutFormat;
    boolean mIsAudio;

    boolean mSaveToMem;
    OutputManager mOutputBuff;

    String mCodecName;
    MediaCodec mCodec;
    Surface mSurface;

    static {
        System.loadLibrary("ctsmediav2codec_jni");

        codecSelKeyMimeMap.put("vp8", MediaFormat.MIMETYPE_VIDEO_VP8);
        codecSelKeyMimeMap.put("vp9", MediaFormat.MIMETYPE_VIDEO_VP9);
        codecSelKeyMimeMap.put("av1", MediaFormat.MIMETYPE_VIDEO_AV1);
        codecSelKeyMimeMap.put("avc", MediaFormat.MIMETYPE_VIDEO_AVC);
        codecSelKeyMimeMap.put("hevc", MediaFormat.MIMETYPE_VIDEO_HEVC);
        codecSelKeyMimeMap.put("mpeg4", MediaFormat.MIMETYPE_VIDEO_MPEG4);
        codecSelKeyMimeMap.put("h263", MediaFormat.MIMETYPE_VIDEO_H263);
        codecSelKeyMimeMap.put("mpeg2", MediaFormat.MIMETYPE_VIDEO_MPEG2);
        codecSelKeyMimeMap.put("vraw", MediaFormat.MIMETYPE_VIDEO_RAW);
        codecSelKeyMimeMap.put("amrnb", MediaFormat.MIMETYPE_AUDIO_AMR_NB);
        codecSelKeyMimeMap.put("amrwb", MediaFormat.MIMETYPE_AUDIO_AMR_WB);
        codecSelKeyMimeMap.put("mp3", MediaFormat.MIMETYPE_AUDIO_MPEG);
        codecSelKeyMimeMap.put("aac", MediaFormat.MIMETYPE_AUDIO_AAC);
        codecSelKeyMimeMap.put("vorbis", MediaFormat.MIMETYPE_AUDIO_VORBIS);
        codecSelKeyMimeMap.put("opus", MediaFormat.MIMETYPE_AUDIO_OPUS);
        codecSelKeyMimeMap.put("g711alaw", MediaFormat.MIMETYPE_AUDIO_G711_ALAW);
        codecSelKeyMimeMap.put("g711mlaw", MediaFormat.MIMETYPE_AUDIO_G711_MLAW);
        codecSelKeyMimeMap.put("araw", MediaFormat.MIMETYPE_AUDIO_RAW);
        codecSelKeyMimeMap.put("flac", MediaFormat.MIMETYPE_AUDIO_FLAC);
        codecSelKeyMimeMap.put("gsm", MediaFormat.MIMETYPE_AUDIO_MSGSM);

        android.os.Bundle args = InstrumentationRegistry.getArguments();
        mimeSelKeys = args.getString(MIME_SEL_KEY);
        codecPrefix = args.getString(CODEC_PREFIX_KEY);
    }

    static boolean isTv() {
        return pm.hasSystemFeature(PackageManager.FEATURE_LEANBACK);
    }

    static boolean hasMicrophone() {
        return pm.hasSystemFeature(PackageManager.FEATURE_MICROPHONE);
    }

    static boolean hasCamera() {
        return pm.hasSystemFeature(PackageManager.FEATURE_CAMERA_ANY);
    }

    static boolean isWatch() {
        return pm.hasSystemFeature(PackageManager.FEATURE_WATCH);
    }

    static boolean isAutomotive() {
        return pm.hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE);
    }

    static boolean isPc() {
        return pm.hasSystemFeature(PackageManager.FEATURE_PC);
    }

    static boolean hasAudioOutput() {
        return pm.hasSystemFeature(PackageManager.FEATURE_AUDIO_OUTPUT);
    }

    static boolean isHandheld() {
        // handheld nature is not exposed to package manager, for now
        // we check for touchscreen and NOT watch and NOT tv and NOT pc
        return pm.hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN) && !isWatch() && !isTv() &&
                !isAutomotive() && !isPc();
    }

    static boolean hasDecoder(String mime) {
        return CodecTestBase.selectCodecs(mime, null, null, false).size() != 0;
    }

    static boolean hasEncoder(String mime) {
        return CodecTestBase.selectCodecs(mime, null, null, true).size() != 0;
    }

    static boolean isFeatureSupported(String name, String mime, String feature) throws IOException {
        MediaCodec codec = MediaCodec.createByCodecName(name);
        MediaCodecInfo.CodecCapabilities codecCapabilities =
                codec.getCodecInfo().getCapabilitiesForType(mime);
        boolean isSupported = codecCapabilities.isFeatureSupported(feature);
        codec.release();
        return isSupported;
    }

    static boolean areFormatsSupported(String name, String mime, ArrayList<MediaFormat> formats)
            throws IOException {
        MediaCodec codec = MediaCodec.createByCodecName(name);
        MediaCodecInfo.CodecCapabilities codecCapabilities =
                codec.getCodecInfo().getCapabilitiesForType(mime);
        boolean isSupported = true;
        if (formats != null) {
            for (int i = 0; i < formats.size() && isSupported; i++) {
                isSupported = codecCapabilities.isFormatSupported(formats.get(i));
            }
        }
        codec.release();
        return isSupported;
    }

    static ArrayList<String> compileRequiredMimeList(boolean isEncoder, boolean needAudio,
            boolean needVideo) {
        Set<String> list = new HashSet<>();
        if (!isEncoder) {
            if (hasAudioOutput() && needAudio) {
                // sec 5.1.2
                list.add(MediaFormat.MIMETYPE_AUDIO_AAC);
                list.add(MediaFormat.MIMETYPE_AUDIO_FLAC);
                list.add(MediaFormat.MIMETYPE_AUDIO_MPEG);
                list.add(MediaFormat.MIMETYPE_AUDIO_VORBIS);
                list.add(MediaFormat.MIMETYPE_AUDIO_RAW);
                list.add(MediaFormat.MIMETYPE_AUDIO_OPUS);
            }
            if (isHandheld() || isTv() || isAutomotive()) {
                // sec 2.2.2, 2.3.2, 2.5.2
                if (needAudio) {
                    list.add(MediaFormat.MIMETYPE_AUDIO_AAC);
                }
                if (needVideo) {
                    list.add(MediaFormat.MIMETYPE_VIDEO_AVC);
                    list.add(MediaFormat.MIMETYPE_VIDEO_MPEG4);
                    list.add(MediaFormat.MIMETYPE_VIDEO_H263);
                    list.add(MediaFormat.MIMETYPE_VIDEO_VP8);
                    list.add(MediaFormat.MIMETYPE_VIDEO_VP9);
                }
            }
            if (isHandheld()) {
                // sec 2.2.2
                if (needAudio) {
                    list.add(MediaFormat.MIMETYPE_AUDIO_AMR_NB);
                    list.add(MediaFormat.MIMETYPE_AUDIO_AMR_WB);
                }
                if (needVideo) {
                    list.add(MediaFormat.MIMETYPE_VIDEO_HEVC);
                }
            }
            if (isTv() && needVideo) {
                // sec 2.3.2
                list.add(MediaFormat.MIMETYPE_VIDEO_HEVC);
                list.add(MediaFormat.MIMETYPE_VIDEO_MPEG2);
            }
        } else {
            if (hasMicrophone() && needAudio) {
                // sec 5.1.1
                // TODO(b/154423550)
                // list.add(MediaFormat.MIMETYPE_AUDIO_RAW);
                list.add(MediaFormat.MIMETYPE_AUDIO_FLAC);
                list.add(MediaFormat.MIMETYPE_AUDIO_OPUS);
            }
            if (isHandheld() || isTv() || isAutomotive()) {
                // sec 2.2.2, 2.3.2, 2.5.2
                if (needAudio) {
                    list.add(MediaFormat.MIMETYPE_AUDIO_AAC);
                }
                if (needVideo) {
                    list.add(MediaFormat.MIMETYPE_VIDEO_AVC);
                    list.add(MediaFormat.MIMETYPE_VIDEO_VP8);
                }
            }
            if (isHandheld() && needAudio) {
                // sec 2.2.2
                list.add(MediaFormat.MIMETYPE_AUDIO_AMR_NB);
                list.add(MediaFormat.MIMETYPE_AUDIO_AMR_WB);
            }
        }
        return new ArrayList<>(list);
    }

    static ArrayList<String> compileCompleteTestMimeList(boolean isEncoder, boolean needAudio,
            boolean needVideo) {
        ArrayList<String> mimes = new ArrayList<>();
        if (mimeSelKeys == null) {
            ArrayList<String> cddRequiredMimeList =
                    compileRequiredMimeList(isEncoder, needAudio, needVideo);
            MediaCodecList codecList = new MediaCodecList(MediaCodecList.REGULAR_CODECS);
            MediaCodecInfo[] codecInfos = codecList.getCodecInfos();
            for (MediaCodecInfo codecInfo : codecInfos) {
                if (codecInfo.isEncoder() != isEncoder) continue;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q && codecInfo.isAlias()) continue;
                String[] types = codecInfo.getSupportedTypes();
                for (String type : types) {
                    if (!needAudio && type.startsWith("audio/")) continue;
                    if (!needVideo && type.startsWith("video/")) continue;
                    if (!mimes.contains(type)) {
                        mimes.add(type);
                    }
                }
            }
            // TODO(b/154423708): add checks for video o/p port and display length >= 2.5"
            /* sec 5.2: device implementations include an embedded screen display with the
            diagonal length of at least 2.5inches or include a video output port or declare the
            support of a camera */
            if (isEncoder && hasCamera() && needVideo &&
                    !mimes.contains(MediaFormat.MIMETYPE_VIDEO_AVC) &&
                    !mimes.contains(MediaFormat.MIMETYPE_VIDEO_VP8)) {
                // Add required cdd mimes here so that respective codec tests fail.
                mimes.add(MediaFormat.MIMETYPE_VIDEO_AVC);
                mimes.add(MediaFormat.MIMETYPE_VIDEO_VP8);
                Log.e(LOG_TAG,"device must support at least one of VP8 or AVC video encoders");
            }
            for (String mime : cddRequiredMimeList) {
                if (!mimes.contains(mime)) {
                    // Add required cdd mimes here so that respective codec tests fail.
                    mimes.add(mime);
                    Log.e(LOG_TAG, "no codec found for mime " + mime + " as required by cdd");
                }
            }
        } else {
            for (Map.Entry<String, String> entry : codecSelKeyMimeMap.entrySet()) {
                String key = entry.getKey();
                String value = entry.getValue();
                if (mimeSelKeys.contains(key) && !mimes.contains(value)) mimes.add(value);
            }
        }
        return mimes;
    }

    static List<Object[]> prepareParamList(List<Object[]> exhaustiveArgsList, boolean isEncoder,
            boolean needAudio, boolean needVideo, boolean mustTestAllCodecs) {
        ArrayList<String> mimes = compileCompleteTestMimeList(isEncoder, needAudio, needVideo);
        ArrayList<String> cddRequiredMimeList =
                compileRequiredMimeList(isEncoder, needAudio, needVideo);
        final List<Object[]> argsList = new ArrayList<>();
        int argLength = exhaustiveArgsList.get(0).length;
        for (String mime : mimes) {
            ArrayList<String> totalListOfCodecs = selectCodecs(mime, null, null, isEncoder);
            ArrayList<String> listOfCodecs = new ArrayList<>();
            if (codecPrefix != null) {
                for (String codec : totalListOfCodecs) {
                    if (codec.startsWith(codecPrefix)) {
                        listOfCodecs.add(codec);
                    }
                }
            } else {
                listOfCodecs = totalListOfCodecs;
            }
            if (mustTestAllCodecs && listOfCodecs.size() == 0 && codecPrefix == null) {
                listOfCodecs.add(INVALID_CODEC + mime);
            }
            boolean miss = true;
            for (Object[] arg : exhaustiveArgsList) {
                if (mime.equals(arg[0])) {
                    for (String codec : listOfCodecs) {
                        Object[] arg_ = new Object[argLength + 1];
                        arg_[0] = codec;
                        System.arraycopy(arg, 0, arg_, 1, argLength);
                        argsList.add(arg_);
                    }
                    miss = false;
                }
            }
            if (miss && mustTestAllCodecs) {
                if (!cddRequiredMimeList.contains(mime)) {
                    Log.w(LOG_TAG, "no test vectors available for optional mime type " + mime);
                    continue;
                }
                for (String codec : listOfCodecs) {
                    Object[] arg_ = new Object[argLength + 1];
                    arg_[0] = codec;
                    arg_[1] = mime;
                    System.arraycopy(exhaustiveArgsList.get(0), 1, arg_, 2, argLength - 1);
                    argsList.add(arg_);
                }
            }
        }
        return argsList;
    }

    abstract void enqueueInput(int bufferIndex) throws IOException;

    abstract void dequeueOutput(int bufferIndex, MediaCodec.BufferInfo info);

    void configureCodec(MediaFormat format, boolean isAsync, boolean signalEOSWithLastFrame,
            boolean isEncoder) {
        resetContext(isAsync, signalEOSWithLastFrame);
        mAsyncHandle.setCallBack(mCodec, isAsync);
        // signalEOS flag has nothing to do with configure. We are using this flag to try all
        // available configure apis
        if (signalEOSWithLastFrame) {
            mCodec.configure(format, mSurface, null,
                    isEncoder ? MediaCodec.CONFIGURE_FLAG_ENCODE : 0);
        } else {
            mCodec.configure(format, mSurface, isEncoder ? MediaCodec.CONFIGURE_FLAG_ENCODE : 0,
                    null);
        }
        if (ENABLE_LOGS) {
            Log.v(LOG_TAG, "codec configured");
        }
    }

    void flushCodec() {
        mCodec.flush();
        // TODO(b/147576107): is it ok to clearQueues right away or wait for some signal
        mAsyncHandle.clearQueues();
        mSawInputEOS = false;
        mSawOutputEOS = false;
        mInputCount = 0;
        mOutputCount = 0;
        mPrevOutputPts = Long.MIN_VALUE;
        if (ENABLE_LOGS) {
            Log.v(LOG_TAG, "codec flushed");
        }
    }

    void reConfigureCodec(MediaFormat format, boolean isAsync, boolean signalEOSWithLastFrame,
            boolean isEncoder) {
        /* TODO(b/147348711) */
        if (false) mCodec.stop();
        else mCodec.reset();
        configureCodec(format, isAsync, signalEOSWithLastFrame, isEncoder);
    }

    void resetContext(boolean isAsync, boolean signalEOSWithLastFrame) {
        mAsyncHandle.resetContext();
        mIsCodecInAsyncMode = isAsync;
        mSawInputEOS = false;
        mSawOutputEOS = false;
        mSignalEOSWithLastFrame = signalEOSWithLastFrame;
        mInputCount = 0;
        mOutputCount = 0;
        mPrevOutputPts = Long.MIN_VALUE;
        mSignalledOutFormatChanged = false;
    }

    void enqueueEOS(int bufferIndex) {
        if (!mSawInputEOS) {
            mCodec.queueInputBuffer(bufferIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
            mSawInputEOS = true;
            if (ENABLE_LOGS) {
                Log.v(LOG_TAG, "Queued End of Stream");
            }
        }
    }

    void doWork(int frameLimit) throws InterruptedException, IOException {
        int frameCount = 0;
        if (mIsCodecInAsyncMode) {
            // dequeue output after inputEOS is expected to be done in waitForAllOutputs()
            while (!mAsyncHandle.hasSeenError() && !mSawInputEOS && frameCount < frameLimit) {
                Pair<Integer, MediaCodec.BufferInfo> element = mAsyncHandle.getWork();
                if (element != null) {
                    int bufferID = element.first;
                    MediaCodec.BufferInfo info = element.second;
                    if (info != null) {
                        // <id, info> corresponds to output callback. Handle it accordingly
                        dequeueOutput(bufferID, info);
                    } else {
                        // <id, null> corresponds to input callback. Handle it accordingly
                        enqueueInput(bufferID);
                        frameCount++;
                    }
                }
            }
        } else {
            MediaCodec.BufferInfo outInfo = new MediaCodec.BufferInfo();
            // dequeue output after inputEOS is expected to be done in waitForAllOutputs()
            while (!mSawInputEOS && frameCount < frameLimit) {
                int outputBufferId = mCodec.dequeueOutputBuffer(outInfo, Q_DEQ_TIMEOUT_US);
                if (outputBufferId >= 0) {
                    dequeueOutput(outputBufferId, outInfo);
                } else if (outputBufferId == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    mOutFormat = mCodec.getOutputFormat();
                    mSignalledOutFormatChanged = true;
                }
                int inputBufferId = mCodec.dequeueInputBuffer(Q_DEQ_TIMEOUT_US);
                if (inputBufferId != -1) {
                    enqueueInput(inputBufferId);
                    frameCount++;
                }
            }
        }
    }

    void queueEOS() throws InterruptedException {
        if (mIsCodecInAsyncMode) {
            while (!mAsyncHandle.hasSeenError() && !mSawInputEOS) {
                Pair<Integer, MediaCodec.BufferInfo> element = mAsyncHandle.getWork();
                if (element != null) {
                    int bufferID = element.first;
                    MediaCodec.BufferInfo info = element.second;
                    if (info != null) {
                        dequeueOutput(bufferID, info);
                    } else {
                        enqueueEOS(element.first);
                    }
                }
            }
        } else {
            MediaCodec.BufferInfo outInfo = new MediaCodec.BufferInfo();
            while (!mSawInputEOS) {
                int outputBufferId = mCodec.dequeueOutputBuffer(outInfo, Q_DEQ_TIMEOUT_US);
                if (outputBufferId >= 0) {
                    dequeueOutput(outputBufferId, outInfo);
                } else if (outputBufferId == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    mOutFormat = mCodec.getOutputFormat();
                    mSignalledOutFormatChanged = true;
                }
                int inputBufferId = mCodec.dequeueInputBuffer(Q_DEQ_TIMEOUT_US);
                if (inputBufferId != -1) {
                    enqueueEOS(inputBufferId);
                }
            }
        }
    }

    void waitForAllOutputs() throws InterruptedException {
        if (mIsCodecInAsyncMode) {
            while (!mAsyncHandle.hasSeenError() && !mSawOutputEOS) {
                Pair<Integer, MediaCodec.BufferInfo> element = mAsyncHandle.getOutput();
                if (element != null) {
                    dequeueOutput(element.first, element.second);
                }
            }
        } else {
            MediaCodec.BufferInfo outInfo = new MediaCodec.BufferInfo();
            while (!mSawOutputEOS) {
                int outputBufferId = mCodec.dequeueOutputBuffer(outInfo, Q_DEQ_TIMEOUT_US);
                if (outputBufferId >= 0) {
                    dequeueOutput(outputBufferId, outInfo);
                } else if (outputBufferId == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    mOutFormat = mCodec.getOutputFormat();
                    mSignalledOutFormatChanged = true;
                }
            }
        }
    }

    static ArrayList<String> selectCodecs(String mime, ArrayList<MediaFormat> formats,
            String[] features, boolean isEncoder) {
        MediaCodecList codecList = new MediaCodecList(MediaCodecList.REGULAR_CODECS);
        MediaCodecInfo[] codecInfos = codecList.getCodecInfos();
        ArrayList<String> listOfCodecs = new ArrayList<>();
        for (MediaCodecInfo codecInfo : codecInfos) {
            if (codecInfo.isEncoder() != isEncoder) continue;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q && codecInfo.isAlias()) continue;
            String[] types = codecInfo.getSupportedTypes();
            for (String type : types) {
                if (type.equalsIgnoreCase(mime)) {
                    boolean isOk = true;
                    MediaCodecInfo.CodecCapabilities codecCapabilities =
                            codecInfo.getCapabilitiesForType(type);
                    if (formats != null) {
                        for (MediaFormat format : formats) {
                            if (!codecCapabilities.isFormatSupported(format)) {
                                isOk = false;
                                break;
                            }
                        }
                    }
                    if (features != null) {
                        for (String feature : features) {
                            if (!codecCapabilities.isFeatureSupported(feature)) {
                                isOk = false;
                                break;
                            }
                        }
                    }
                    if (isOk) listOfCodecs.add(codecInfo.getName());
                }
            }
        }
        return listOfCodecs;
    }

    static int getWidth(MediaFormat format) {
        int width = format.getInteger(MediaFormat.KEY_WIDTH, -1);
        if (format.containsKey("crop-left") && format.containsKey("crop-right")) {
            width = format.getInteger("crop-right") + 1 - format.getInteger("crop-left");
        }
        return width;
    }

    static int getHeight(MediaFormat format) {
        int height = format.getInteger(MediaFormat.KEY_HEIGHT, -1);
        if (format.containsKey("crop-top") && format.containsKey("crop-bottom")) {
            height = format.getInteger("crop-bottom") + 1 - format.getInteger("crop-top");
        }
        return height;
    }

    boolean isFormatSimilar(MediaFormat inpFormat, MediaFormat outFormat) {
        if (inpFormat == null || outFormat == null) return false;
        String inpMime = inpFormat.getString(MediaFormat.KEY_MIME);
        String outMime = outFormat.getString(MediaFormat.KEY_MIME);
        // not comparing input and output mimes because for a codec, mime is raw on one side and
        // encoded type on the other
        if (outMime.startsWith("audio/")) {
            return inpFormat.getInteger(MediaFormat.KEY_CHANNEL_COUNT, -1) ==
                    outFormat.getInteger(MediaFormat.KEY_CHANNEL_COUNT, -2) &&
                    inpFormat.getInteger(MediaFormat.KEY_SAMPLE_RATE, -1) ==
                            outFormat.getInteger(MediaFormat.KEY_SAMPLE_RATE, -2) &&
                    inpMime.startsWith("audio/");
        } else if (outMime.startsWith("video/")) {
            return getWidth(inpFormat) == getWidth(outFormat) &&
                    getHeight(inpFormat) == getHeight(outFormat) && inpMime.startsWith("video/");
        }
        return true;
    }

    PersistableBundle validateMetrics(String codec) {
        PersistableBundle metrics = mCodec.getMetrics();
        assertTrue("metrics is null", metrics != null);
        assertTrue(metrics.getString(MediaCodec.MetricsConstants.CODEC).equals(codec));
        if (mIsAudio) {
            assertTrue(metrics.getString(MediaCodec.MetricsConstants.MODE)
                    .equals(MediaCodec.MetricsConstants.MODE_AUDIO));
        } else {
            assertTrue(metrics.getString(MediaCodec.MetricsConstants.MODE)
                    .equals(MediaCodec.MetricsConstants.MODE_VIDEO));
        }
        return metrics;
    }

    PersistableBundle validateMetrics(String codec, MediaFormat format) {
        PersistableBundle metrics = validateMetrics(codec);
        if (!mIsAudio) {
            assertTrue(metrics.getInt(MediaCodec.MetricsConstants.WIDTH) == getWidth(format));
            assertTrue(metrics.getInt(MediaCodec.MetricsConstants.HEIGHT) == getHeight(format));
        }
        assertTrue(metrics.getInt(MediaCodec.MetricsConstants.SECURE) == 0);
        return metrics;
    }

    void validateColorAspects(MediaFormat fmt, int range, int standard, int transfer) {
        int colorRange = fmt.getInteger(MediaFormat.KEY_COLOR_RANGE, UNSPECIFIED);
        int colorStandard = fmt.getInteger(MediaFormat.KEY_COLOR_STANDARD, UNSPECIFIED);
        int colorTransfer = fmt.getInteger(MediaFormat.KEY_COLOR_TRANSFER, UNSPECIFIED);
        if (range > UNSPECIFIED) {
            assertEquals("color range mismatch ", range, colorRange);
        }
        if (standard > UNSPECIFIED) {
            assertEquals("color standard mismatch ", standard, colorStandard);
        }
        if (transfer > UNSPECIFIED) {
            assertEquals("color transfer mismatch ", transfer, colorTransfer);
        }
    }

    public void setUpSurface(CodecTestActivity activity) throws InterruptedException {
        activity.waitTillSurfaceIsCreated();
        mSurface = activity.getSurface();
        assertTrue("Surface created is null.", mSurface != null);
        assertTrue("Surface created is invalid.", mSurface.isValid());
    }

    public void tearDownSurface() {
        if (mSurface != null) {
            mSurface.release();
            mSurface = null;
        }
    }

    @Before
    public void isCodecNameValid() {
        if (mCodecName != null && mCodecName.startsWith(INVALID_CODEC)) {
            fail("no valid component available for current test ");
        }
    }
}

class CodecDecoderTestBase extends CodecTestBase {
    private static final String LOG_TAG = CodecDecoderTestBase.class.getSimpleName();

    String mMime;
    String mTestFile;
    boolean mIsInterlaced;

    ArrayList<ByteBuffer> mCsdBuffers;
    private int mCurrCsdIdx;

    private ByteBuffer flatBuffer = ByteBuffer.allocate(4 * Integer.BYTES);

    MediaExtractor mExtractor;

    CodecDecoderTestBase(String codecName, String mime, String testFile) {
        mCodecName = codecName;
        mMime = mime;
        mTestFile = testFile;
        mAsyncHandle = new CodecAsyncHandler();
        mCsdBuffers = new ArrayList<>();
        mIsAudio = mMime.startsWith("audio/");
    }

    MediaFormat setUpSource(String srcFile) throws IOException {
        return setUpSource(mInpPrefix, srcFile);
    }

    MediaFormat setUpSource(String prefix, String srcFile) throws IOException {
        mExtractor = new MediaExtractor();
        mExtractor.setDataSource(prefix + srcFile);
        for (int trackID = 0; trackID < mExtractor.getTrackCount(); trackID++) {
            MediaFormat format = mExtractor.getTrackFormat(trackID);
            if (mMime.equalsIgnoreCase(format.getString(MediaFormat.KEY_MIME))) {
                mExtractor.selectTrack(trackID);
                if (!mIsAudio) {
                    if (mSurface == null) {
                        // COLOR_FormatYUV420Flexible must be supported by all components
                        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, COLOR_FormatYUV420Flexible);
                    } else {
                        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, COLOR_FormatSurface);
                    }
                }
                // TODO: determine this from the extractor format when it becomes exposed.
                mIsInterlaced = srcFile.contains("_interlaced_");
                return format;
            }
        }
        fail("No track with mime: " + mMime + " found in file: " + srcFile);
        return null;
    }

    boolean hasCSD(MediaFormat format) {
        return format.containsKey("csd-0");
    }

    void flattenBufferInfo(MediaCodec.BufferInfo info, boolean isAudio) {
        if (isAudio) {
            flatBuffer.putInt(info.size);
        }
        flatBuffer.putInt(info.flags & ~MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                .putLong(info.presentationTimeUs);
        flatBuffer.flip();
    }

    void enqueueCodecConfig(int bufferIndex) {
        ByteBuffer inputBuffer = mCodec.getInputBuffer(bufferIndex);
        ByteBuffer csdBuffer = mCsdBuffers.get(mCurrCsdIdx);
        inputBuffer.put((ByteBuffer) csdBuffer.rewind());
        mCodec.queueInputBuffer(bufferIndex, 0, csdBuffer.limit(), 0,
                MediaCodec.BUFFER_FLAG_CODEC_CONFIG);
        if (ENABLE_LOGS) {
            Log.v(LOG_TAG, "queued csd: id: " + bufferIndex + " size: " + csdBuffer.limit());
        }
    }

    void enqueueInput(int bufferIndex) {
        if (mExtractor.getSampleSize() < 0) {
            enqueueEOS(bufferIndex);
        } else {
            ByteBuffer inputBuffer = mCodec.getInputBuffer(bufferIndex);
            mExtractor.readSampleData(inputBuffer, 0);
            int size = (int) mExtractor.getSampleSize();
            long pts = mExtractor.getSampleTime();
            int extractorFlags = mExtractor.getSampleFlags();
            int codecFlags = 0;
            if ((extractorFlags & MediaExtractor.SAMPLE_FLAG_SYNC) != 0) {
                codecFlags |= MediaCodec.BUFFER_FLAG_KEY_FRAME;
            }
            if ((extractorFlags & MediaExtractor.SAMPLE_FLAG_PARTIAL_FRAME) != 0) {
                codecFlags |= MediaCodec.BUFFER_FLAG_PARTIAL_FRAME;
            }
            if (!mExtractor.advance() && mSignalEOSWithLastFrame) {
                codecFlags |= MediaCodec.BUFFER_FLAG_END_OF_STREAM;
                mSawInputEOS = true;
            }
            if (ENABLE_LOGS) {
                Log.v(LOG_TAG, "input: id: " + bufferIndex + " size: " + size + " pts: " + pts +
                        " flags: " + codecFlags);
            }
            mCodec.queueInputBuffer(bufferIndex, 0, size, pts, codecFlags);
            if (size > 0 && (codecFlags & (MediaCodec.BUFFER_FLAG_CODEC_CONFIG |
                    MediaCodec.BUFFER_FLAG_PARTIAL_FRAME)) == 0) {
                mOutputBuff.saveInPTS(pts);
                mInputCount++;
            }
        }
    }

    void enqueueInput(int bufferIndex, ByteBuffer buffer, MediaCodec.BufferInfo info) {
        ByteBuffer inputBuffer = mCodec.getInputBuffer(bufferIndex);
        buffer.position(info.offset);
        for (int i = 0; i < info.size; i++) {
            inputBuffer.put(buffer.get());
        }
        if (ENABLE_LOGS) {
            Log.v(LOG_TAG, "input: id: " + bufferIndex + " flags: " + info.flags + " size: " +
                    info.size + " timestamp: " + info.presentationTimeUs);
        }
        mCodec.queueInputBuffer(bufferIndex, 0, info.size, info.presentationTimeUs,
                info.flags);
        if (info.size > 0 && ((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) == 0) &&
                ((info.flags & MediaCodec.BUFFER_FLAG_PARTIAL_FRAME) == 0)) {
            mOutputBuff.saveInPTS(info.presentationTimeUs);
            mInputCount++;
        }
    }

    void dequeueOutput(int bufferIndex, MediaCodec.BufferInfo info) {
        if (info.size > 0 && mSaveToMem) {
            ByteBuffer buf = mCodec.getOutputBuffer(bufferIndex);
            flattenBufferInfo(info, mIsAudio);
            mOutputBuff.checksum(flatBuffer, flatBuffer.limit());
            if (mIsAudio) {
                mOutputBuff.checksum(buf, info.size);
                mOutputBuff.saveToMemory(buf, info);
            } else {
                // tests both getOutputImage and getOutputBuffer. Can do time division
                // multiplexing but lets allow it for now
                MediaFormat format = mCodec.getOutputFormat();
                int width = format.getInteger(MediaFormat.KEY_WIDTH);
                int height = format.getInteger(MediaFormat.KEY_HEIGHT);
                int stride = format.getInteger(MediaFormat.KEY_STRIDE);
                mOutputBuff.checksum(buf, info.size, width, height, stride);

                Image img = mCodec.getOutputImage(bufferIndex);
                assertTrue(img != null);
                mOutputBuff.checksum(img);
            }
        }
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
        mCodec.releaseOutputBuffer(bufferIndex, false);
    }

    void doWork(ByteBuffer buffer, ArrayList<MediaCodec.BufferInfo> list)
            throws InterruptedException {
        int frameCount = 0;
        if (mIsCodecInAsyncMode) {
            // output processing after queuing EOS is done in waitForAllOutputs()
            while (!mAsyncHandle.hasSeenError() && !mSawInputEOS && frameCount < list.size()) {
                Pair<Integer, MediaCodec.BufferInfo> element = mAsyncHandle.getWork();
                if (element != null) {
                    int bufferID = element.first;
                    MediaCodec.BufferInfo info = element.second;
                    if (info != null) {
                        dequeueOutput(bufferID, info);
                    } else {
                        enqueueInput(bufferID, buffer, list.get(frameCount));
                        frameCount++;
                    }
                }
            }
        } else {
            MediaCodec.BufferInfo outInfo = new MediaCodec.BufferInfo();
            // output processing after queuing EOS is done in waitForAllOutputs()
            while (!mSawInputEOS && frameCount < list.size()) {
                int outputBufferId = mCodec.dequeueOutputBuffer(outInfo, Q_DEQ_TIMEOUT_US);
                if (outputBufferId >= 0) {
                    dequeueOutput(outputBufferId, outInfo);
                } else if (outputBufferId == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    mOutFormat = mCodec.getOutputFormat();
                    mSignalledOutFormatChanged = true;
                }
                int inputBufferId = mCodec.dequeueInputBuffer(Q_DEQ_TIMEOUT_US);
                if (inputBufferId != -1) {
                    enqueueInput(inputBufferId, buffer, list.get(frameCount));
                    frameCount++;
                }
            }
        }
    }

    void queueCodecConfig() throws InterruptedException {
        if (mIsCodecInAsyncMode) {
            for (mCurrCsdIdx = 0; !mAsyncHandle.hasSeenError() && mCurrCsdIdx < mCsdBuffers.size();
                 mCurrCsdIdx++) {
                Pair<Integer, MediaCodec.BufferInfo> element = mAsyncHandle.getInput();
                if (element != null) {
                    enqueueCodecConfig(element.first);
                }
            }
        } else {
            for (mCurrCsdIdx = 0; mCurrCsdIdx < mCsdBuffers.size(); mCurrCsdIdx++) {
                enqueueCodecConfig(mCodec.dequeueInputBuffer(-1));
            }
        }
    }

    void decodeToMemory(String file, String decoder, long pts, int mode, int frameLimit)
            throws IOException, InterruptedException {
        mSaveToMem = true;
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
        mSaveToMem = false;
    }

    @Override
    PersistableBundle validateMetrics(String decoder, MediaFormat format) {
        PersistableBundle metrics = super.validateMetrics(decoder, format);
        assertTrue(metrics.getString(MediaCodec.MetricsConstants.MIME_TYPE).equals(mMime));
        assertTrue(metrics.getInt(MediaCodec.MetricsConstants.ENCODER) == 0);
        return metrics;
    }

    void validateColorAspects(String decoder, String parent, String name, int range, int standard,
            int transfer, boolean ignoreColorBox)
            throws IOException, InterruptedException {
        mOutputBuff = new OutputManager();
        MediaFormat format = setUpSource(parent, name);
        if (ignoreColorBox) {
            format.removeKey(MediaFormat.KEY_COLOR_RANGE);
            format.removeKey(MediaFormat.KEY_COLOR_STANDARD);
            format.removeKey(MediaFormat.KEY_COLOR_TRANSFER);
        }
        if (decoder == null) {
            MediaCodecList codecList = new MediaCodecList(MediaCodecList.REGULAR_CODECS);
            decoder = codecList.findDecoderForFormat(format);
        }
        mCodec = MediaCodec.createByCodecName(decoder);
        configureCodec(format, true, true, false);
        mCodec.start();
        doWork(1);
        queueEOS();
        waitForAllOutputs();
        validateColorAspects(mCodec.getOutputFormat(), range, standard, transfer);
        mCodec.stop();
        mCodec.release();
        mExtractor.release();
    }
}

class CodecEncoderTestBase extends CodecTestBase {
    private static final String LOG_TAG = CodecEncoderTestBase.class.getSimpleName();

    // files are in WorkDir.getMediaDirString();
    private static final String mInputAudioFile = "bbb_2ch_44kHz_s16le.raw";
    private static final String mInputVideoFile = "bbb_cif_yuv420p_30fps.yuv";
    private final int INP_FRM_WIDTH = 352;
    private final int INP_FRM_HEIGHT = 288;

    final String mMime;
    final int[] mBitrates;
    final int[] mEncParamList1;
    final int[] mEncParamList2;

    final String mInputFile;
    byte[] mInputData;
    int mNumBytesSubmitted;
    long mInputOffsetPts;

    ArrayList<MediaFormat> mFormats;
    ArrayList<MediaCodec.BufferInfo> mInfoList;

    int mWidth, mHeight;
    int mFrameRate;
    int mMaxBFrames;
    int mChannels;
    int mSampleRate;

    CodecEncoderTestBase(String encoder, String mime, int[] bitrates, int[] encoderInfo1,
            int[] encoderInfo2) {
        mMime = mime;
        mCodecName = encoder;
        mBitrates = bitrates;
        mEncParamList1 = encoderInfo1;
        mEncParamList2 = encoderInfo2;
        mFormats = new ArrayList<>();
        mInfoList = new ArrayList<>();
        mWidth = INP_FRM_WIDTH;
        mHeight = INP_FRM_HEIGHT;
        if (mime.equals(MediaFormat.MIMETYPE_VIDEO_MPEG4)) mFrameRate = 12;
        else if (mime.equals(MediaFormat.MIMETYPE_VIDEO_H263)) mFrameRate = 12;
        else mFrameRate = 30;
        mMaxBFrames = 0;
        mChannels = 1;
        mSampleRate = 8000;
        mAsyncHandle = new CodecAsyncHandler();
        mIsAudio = mMime.startsWith("audio/");
        mInputFile = mIsAudio ? mInputAudioFile : mInputVideoFile;
    }

    /**
     * Selects encoder input color format in byte buffer mode. As of now ndk tests support only
     * 420p, 420sp. COLOR_FormatYUV420Flexible although can represent any form of yuv, it doesn't
     * work in ndk due to lack of AMediaCodec_GetInputImage()
     */
    static int findByteBufferColorFormat(String encoder, String mime) throws IOException {
        MediaCodec codec = MediaCodec.createByCodecName(encoder);
        MediaCodecInfo.CodecCapabilities cap = codec.getCodecInfo().getCapabilitiesForType(mime);
        int colorFormat = -1;
        for (int c : cap.colorFormats) {
            if (c == MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar ||
                    c == MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar) {
                Log.v(LOG_TAG, "selecting color format: " + c);
                colorFormat = c;
                break;
            }
        }
        codec.release();
        return colorFormat;
    }

    @Override
    void resetContext(boolean isAsync, boolean signalEOSWithLastFrame) {
        super.resetContext(isAsync, signalEOSWithLastFrame);
        mNumBytesSubmitted = 0;
        mInputOffsetPts = 0;
    }

    @Override
    void flushCodec() {
        super.flushCodec();
        if (mIsAudio) {
            mInputOffsetPts =
                    (mNumBytesSubmitted + 1024) * 1000000L / (2 * mChannels * mSampleRate);
        } else {
            mInputOffsetPts = (mInputCount + 5) * 1000000L / mFrameRate;
        }
        mPrevOutputPts = mInputOffsetPts - 1;
        mNumBytesSubmitted = 0;
    }

    void setUpSource(String srcFile) throws IOException {
        String inpPath = mInpPrefix + srcFile;
        try (FileInputStream fInp = new FileInputStream(inpPath)) {
            int size = (int) new File(inpPath).length();
            mInputData = new byte[size];
            fInp.read(mInputData, 0, size);
        }
    }

    void fillImage(Image image) {
        Assert.assertTrue(image.getFormat() == ImageFormat.YUV_420_888);
        int imageWidth = image.getWidth();
        int imageHeight = image.getHeight();
        Image.Plane[] planes = image.getPlanes();
        int offset = mNumBytesSubmitted;
        for (int i = 0; i < planes.length; ++i) {
            ByteBuffer buf = planes[i].getBuffer();
            int width = imageWidth;
            int height = imageHeight;
            int tileWidth = INP_FRM_WIDTH;
            int tileHeight = INP_FRM_HEIGHT;
            int rowStride = planes[i].getRowStride();
            int pixelStride = planes[i].getPixelStride();
            if (i != 0) {
                width = imageWidth / 2;
                height = imageHeight / 2;
                tileWidth = INP_FRM_WIDTH / 2;
                tileHeight = INP_FRM_HEIGHT / 2;
            }
            if (pixelStride == 1) {
                if (width == rowStride && width == tileWidth && height == tileHeight) {
                    buf.put(mInputData, offset, width * height);
                } else {
                    for (int z = 0; z < height; z += tileHeight) {
                        int rowsToCopy = Math.min(height - z, tileHeight);
                        for (int y = 0; y < rowsToCopy; y++) {
                            for (int x = 0; x < width; x += tileWidth) {
                                int colsToCopy = Math.min(width - x, tileWidth);
                                buf.position((z + y) * rowStride + x);
                                buf.put(mInputData, offset + y * tileWidth, colsToCopy);
                            }
                        }
                    }
                }
            } else {
                // do it pixel-by-pixel
                for (int z = 0; z < height; z += tileHeight) {
                    int rowsToCopy = Math.min(height - z, tileHeight);
                    for (int y = 0; y < rowsToCopy; y++) {
                        int lineOffset = (z + y) * rowStride;
                        for (int x = 0; x < width; x += tileWidth) {
                            int colsToCopy = Math.min(width - x, tileWidth);
                            for (int w = 0; w < colsToCopy; w++) {
                                buf.position(lineOffset + (x + w) * pixelStride);
                                buf.put(mInputData[offset + y * tileWidth + w]);
                            }
                        }
                    }
                }
            }
            offset += tileWidth * tileHeight;
        }
    }

    void fillByteBuffer(ByteBuffer inputBuffer) {
        int offset = 0, frmOffset = mNumBytesSubmitted;
        for (int plane = 0; plane < 3; plane++) {
            int width = mWidth;
            int height = mHeight;
            int tileWidth = INP_FRM_WIDTH;
            int tileHeight = INP_FRM_HEIGHT;
            if (plane != 0) {
                width = mWidth / 2;
                height = mHeight / 2;
                tileWidth = INP_FRM_WIDTH / 2;
                tileHeight = INP_FRM_HEIGHT / 2;
            }
            for (int k = 0; k < height; k += tileHeight) {
                int rowsToCopy = Math.min(height - k, tileHeight);
                for (int j = 0; j < rowsToCopy; j++) {
                    for (int i = 0; i < width; i += tileWidth) {
                        int colsToCopy = Math.min(width - i, tileWidth);
                        inputBuffer.position(offset + (k + j) * width + i);
                        inputBuffer.put(mInputData, frmOffset + j * tileWidth, colsToCopy);
                    }
                }
            }
            offset += width * height;
            frmOffset += tileWidth * tileHeight;
        }
    }

    void enqueueInput(int bufferIndex) {
        ByteBuffer inputBuffer = mCodec.getInputBuffer(bufferIndex);
        if (mNumBytesSubmitted >= mInputData.length) {
            enqueueEOS(bufferIndex);
        } else {
            int size;
            int flags = 0;
            long pts = mInputOffsetPts;
            if (mIsAudio) {
                pts += mNumBytesSubmitted * 1000000L / (2 * mChannels * mSampleRate);
                size = Math.min(inputBuffer.capacity(), mInputData.length - mNumBytesSubmitted);
                inputBuffer.put(mInputData, mNumBytesSubmitted, size);
                if (mNumBytesSubmitted + size >= mInputData.length && mSignalEOSWithLastFrame) {
                    flags |= MediaCodec.BUFFER_FLAG_END_OF_STREAM;
                    mSawInputEOS = true;
                }
                mNumBytesSubmitted += size;
            } else {
                pts += mInputCount * 1000000L / mFrameRate;
                size = mWidth * mHeight * 3 / 2;
                int frmSize = INP_FRM_WIDTH * INP_FRM_HEIGHT * 3 / 2;
                if (mNumBytesSubmitted + frmSize > mInputData.length) {
                    fail("received partial frame to encode");
                } else {
                    Image img = mCodec.getInputImage(bufferIndex);
                    if (img != null) {
                        fillImage(img);
                    } else {
                        if (mWidth == INP_FRM_WIDTH && mHeight == INP_FRM_HEIGHT) {
                            inputBuffer.put(mInputData, mNumBytesSubmitted, size);
                        } else {
                            fillByteBuffer(inputBuffer);
                        }
                    }
                }
                if (mNumBytesSubmitted + frmSize >= mInputData.length && mSignalEOSWithLastFrame) {
                    flags |= MediaCodec.BUFFER_FLAG_END_OF_STREAM;
                    mSawInputEOS = true;
                }
                mNumBytesSubmitted += frmSize;
            }
            if (ENABLE_LOGS) {
                Log.v(LOG_TAG, "input: id: " + bufferIndex + " size: " + size + " pts: " + pts +
                        " flags: " + flags);
            }
            mCodec.queueInputBuffer(bufferIndex, 0, size, pts, flags);
            mOutputBuff.saveInPTS(pts);
            mInputCount++;
        }
    }

    void dequeueOutput(int bufferIndex, MediaCodec.BufferInfo info) {
        if (ENABLE_LOGS) {
            Log.v(LOG_TAG, "output: id: " + bufferIndex + " flags: " + info.flags + " size: " +
                    info.size + " timestamp: " + info.presentationTimeUs);
        }
        if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
            mSawOutputEOS = true;
        }
        if (info.size > 0) {
            if (mSaveToMem) {
                MediaCodec.BufferInfo copy = new MediaCodec.BufferInfo();
                copy.set(mOutputBuff.getOutStreamSize(), info.size, info.presentationTimeUs,
                        info.flags);
                mInfoList.add(copy);

                ByteBuffer buf = mCodec.getOutputBuffer(bufferIndex);
                mOutputBuff.saveToMemory(buf, info);
            }
            if ((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) == 0) {
                mOutputBuff.saveOutPTS(info.presentationTimeUs);
                mOutputCount++;
            }
        }
        mCodec.releaseOutputBuffer(bufferIndex, false);
    }

    @Override
    PersistableBundle validateMetrics(String codec, MediaFormat format) {
        PersistableBundle metrics = super.validateMetrics(codec, format);
        assertTrue(metrics.getString(MediaCodec.MetricsConstants.MIME_TYPE).equals(mMime));
        assertTrue(metrics.getInt(MediaCodec.MetricsConstants.ENCODER) == 1);
        return metrics;
    }

    void setUpParams(int limit) {
        int count = 0;
        for (int bitrate : mBitrates) {
            if (mIsAudio) {
                for (int rate : mEncParamList1) {
                    for (int channels : mEncParamList2) {
                        MediaFormat format = new MediaFormat();
                        format.setString(MediaFormat.KEY_MIME, mMime);
                        if (mMime.equals(MediaFormat.MIMETYPE_AUDIO_FLAC)) {
                            format.setInteger(MediaFormat.KEY_FLAC_COMPRESSION_LEVEL, bitrate);
                        } else {
                            format.setInteger(MediaFormat.KEY_BIT_RATE, bitrate);
                        }
                        format.setInteger(MediaFormat.KEY_SAMPLE_RATE, rate);
                        format.setInteger(MediaFormat.KEY_CHANNEL_COUNT, channels);
                        mFormats.add(format);
                        count++;
                        if (count >= limit) return;
                    }
                }
            } else {
                assertTrue("Wrong number of height, width parameters",
                        mEncParamList1.length == mEncParamList2.length);
                for (int i = 0; i < mEncParamList1.length; i++) {
                    MediaFormat format = new MediaFormat();
                    format.setString(MediaFormat.KEY_MIME, mMime);
                    format.setInteger(MediaFormat.KEY_BIT_RATE, bitrate);
                    format.setInteger(MediaFormat.KEY_WIDTH, mEncParamList1[i]);
                    format.setInteger(MediaFormat.KEY_HEIGHT, mEncParamList2[i]);
                    format.setInteger(MediaFormat.KEY_FRAME_RATE, mFrameRate);
                    format.setInteger(MediaFormat.KEY_MAX_B_FRAMES, mMaxBFrames);
                    format.setFloat(MediaFormat.KEY_I_FRAME_INTERVAL, 1.0f);
                    format.setInteger(MediaFormat.KEY_COLOR_FORMAT,
                            MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible);
                    mFormats.add(format);
                    count++;
                    if (count >= limit) return;
                }
            }
        }
    }

    void encodeToMemory(String file, String encoder, int frameLimit, MediaFormat format,
            boolean saveToMem) throws IOException, InterruptedException {
        mSaveToMem = saveToMem;
        mOutputBuff = new OutputManager();
        mInfoList.clear();
        mCodec = MediaCodec.createByCodecName(encoder);
        setUpSource(file);
        configureCodec(format, false, true, true);
        if (mIsAudio) {
            mSampleRate = format.getInteger(MediaFormat.KEY_SAMPLE_RATE);
            mChannels = format.getInteger(MediaFormat.KEY_CHANNEL_COUNT);
        } else {
            mWidth = format.getInteger(MediaFormat.KEY_WIDTH);
            mHeight = format.getInteger(MediaFormat.KEY_HEIGHT);
        }
        mCodec.start();
        doWork(frameLimit);
        queueEOS();
        waitForAllOutputs();
        mCodec.stop();
        mCodec.release();
        mSaveToMem = false;
    }

    ByteBuffer decodeElementaryStream(String decoder, MediaFormat format,
            ByteBuffer elementaryStream, ArrayList<MediaCodec.BufferInfo> infos)
            throws IOException, InterruptedException {
        String mime = format.getString(MediaFormat.KEY_MIME);
        CodecDecoderTestBase cdtb = new CodecDecoderTestBase(decoder, mime, null);
        cdtb.mOutputBuff = new OutputManager();
        cdtb.mSaveToMem = true;
        cdtb.mCodec = MediaCodec.createByCodecName(decoder);
        cdtb.mCodec.configure(format, null, null, 0);
        cdtb.mCodec.start();
        cdtb.doWork(elementaryStream, infos);
        cdtb.queueEOS();
        cdtb.waitForAllOutputs();
        cdtb.mCodec.stop();
        cdtb.mCodec.release();
        return cdtb.mOutputBuff.getBuffer();
    }
}
