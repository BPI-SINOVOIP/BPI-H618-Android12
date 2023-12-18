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

package android.mediapc.cts;

import android.app.Instrumentation;
import android.content.Context;
import android.content.pm.PackageManager;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.media.MediaRecorder;
import android.util.Log;
import android.util.Pair;
import android.view.Surface;

import androidx.test.filters.LargeTest;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.rule.ActivityTestRule;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import static android.mediapc.cts.CodecTestBase.selectCodecs;
import static android.mediapc.cts.CodecTestBase.selectHardwareCodecs;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

/**
 * The following test class validates the codec initialization latency (time for codec create +
 * configure) for the audio encoders and hardware video encoders available in the device, under the
 * load condition (Transcode + MediaRecorder session Audio(Microphone) and 1080p Video(Camera)).
 */
@RunWith(Parameterized.class)
public class EncoderInitializationLatencyTest {
    private static final String LOG_TAG = EncoderInitializationLatencyTest.class.getSimpleName();
    private static final boolean[] boolStates = {false, true};
    private static final int MAX_AUDIOENC_INITIALIZATION_LATENCY_MS;
    private static final int MAX_VIDEOENC_INITIALIZATION_LATENCY_MS;
    private static final String AVC = MediaFormat.MIMETYPE_VIDEO_AVC;
    private static final String HEVC = MediaFormat.MIMETYPE_VIDEO_HEVC;
    private static final String AVC_TRANSCODE_FILE = "bbb_1280x720_3mbps_30fps_avc.mp4";
    private static String AVC_DECODER_NAME;
    private static String AVC_ENCODER_NAME;
    static {
        AVC_DECODER_NAME = selectHardwareCodecs(AVC, null, null, false).get(0);
        AVC_ENCODER_NAME = selectHardwareCodecs(AVC, null, null, true).get(0);

        if (Utils.isRPerfClass()) {
            MAX_AUDIOENC_INITIALIZATION_LATENCY_MS = 50;
            MAX_VIDEOENC_INITIALIZATION_LATENCY_MS = 65;
        } else {
            // Performance class Build.VERSION_CODES.S and beyond
            MAX_AUDIOENC_INITIALIZATION_LATENCY_MS = 40;
            MAX_VIDEOENC_INITIALIZATION_LATENCY_MS = 50;
        }
    }

    private final String mMime;
    private final String mEncoderName;

    private LoadStatus mTranscodeLoadStatus = null;
    private Thread mTranscodeLoadThread = null;
    private MediaRecorder mMediaRecorderLoad = null;
    private File mTempRecordedFile = null;
    private Surface mSurface = null;
    private Exception mException = null;

    @Before
    public void setUp() throws Exception {
        assumeTrue("Test requires performance class.", Utils.isPerfClass());
        Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        Context context = instrumentation.getTargetContext();
        PackageManager packageManager = context.getPackageManager();
        assertNotNull(packageManager.getSystemAvailableFeatures());
        assumeTrue("The device doesn't have a camera",
                packageManager.hasSystemFeature(PackageManager.FEATURE_CAMERA_ANY));
        assumeTrue("The device doesn't have a microphone",
                packageManager.hasSystemFeature(PackageManager.FEATURE_MICROPHONE));
        createSurface();
        startLoad();
    }

    @After
    public void tearDown() throws Exception {
        stopLoad();
        releaseSurface();
    }

    public EncoderInitializationLatencyTest(String mimeType, String encoderName) {
        mMime = mimeType;
        mEncoderName = encoderName;
    }

    @Rule
    public ActivityTestRule<TestActivity> mActivityRule =
            new ActivityTestRule<>(TestActivity.class);

    // Returns the list of all available hardware video encoders in the device.
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

    // Returns the list of all available audio encoders in the device.
    static ArrayList<String> getMimesOfAvailableAudioEncoders() {
        MediaCodecList codecList = new MediaCodecList(MediaCodecList.REGULAR_CODECS);
        MediaCodecInfo[] codecInfos = codecList.getCodecInfos();
        ArrayList<String> listOfMimes = new ArrayList<>();
        for (MediaCodecInfo codecInfo : codecInfos) {
            if (!codecInfo.isEncoder()) continue;
            String[] types = codecInfo.getSupportedTypes();
            for (String type : types) {
                if (type.startsWith("audio/") && !listOfMimes.contains(type)) {
                    listOfMimes.add(type);
                }
            }
        }
        return listOfMimes;
    }

    // Returns the list of parameters with mimetype and their encoder(for audio - all encoders,
    // video - hardware encoders).
    // Parameters {0}_{1} -- Mime_EncoderName
    @Parameterized.Parameters(name = "{index}({0}_{1})")
    public static Collection<Object[]> inputParams() {
        // Prepares the params list with the required Hardware video encoders and all available
        // audio encoders present in the device.
        final List<Object[]> argsList = new ArrayList<>();
        ArrayList<String> mimesList = getMimesOfAvailableHardwareVideoEncoders();
        mimesList.addAll(getMimesOfAvailableAudioEncoders());
        for (String mime : mimesList) {
            ArrayList<String> listOfEncoders;
            if (mime.startsWith("audio/")) {
                listOfEncoders = selectCodecs(mime, null, null, true);
            } else {
                listOfEncoders = selectHardwareCodecs(mime, null, null, true);
            }
            for (String encoder : listOfEncoders) {
                argsList.add(new Object[] {mime, encoder});
            }
        }
        return argsList;
    }

    private MediaRecorder createMediaRecorderLoad(Surface surface) throws Exception {
        MediaRecorder mediaRecorder = new MediaRecorder();
        mediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        mediaRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
        mediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.DEFAULT);
        mediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AAC);
        mediaRecorder.setVideoEncoder(mMime.equalsIgnoreCase(HEVC) ?
                MediaRecorder.VideoEncoder.HEVC : MediaRecorder.VideoEncoder.H264);
        mediaRecorder.setOutputFile(mTempRecordedFile);
        mediaRecorder.setVideoSize(1920, 1080);
        mediaRecorder.setOrientationHint(0);
        mediaRecorder.setPreviewDisplay(surface);
        mediaRecorder.prepare();
        return mediaRecorder;
    }

    private void startLoad() throws Exception {
        // TODO: b/183671436
        // Create Transcode load (AVC Decoder(720p) + AVC Encoder(720p))
        mTranscodeLoadStatus = new LoadStatus();
        mTranscodeLoadThread = new Thread(() -> {
            try {
                TranscodeLoad transcodeLoad = new TranscodeLoad(AVC, AVC_TRANSCODE_FILE,
                        AVC_DECODER_NAME, AVC_ENCODER_NAME, mTranscodeLoadStatus);
                transcodeLoad.doTranscode();
            } catch (Exception e) {
                mException = e;
            }
        });
        // Create MediaRecorder Session - Audio (Microphone) + 1080p Video (Camera)
        // Create a temp file to dump the MediaRecorder output. Later it will be deleted.
        mTempRecordedFile = new File(WorkDir.getMediaDirString() + "tempOut.mp4");
        mTempRecordedFile.createNewFile();
        mMediaRecorderLoad = createMediaRecorderLoad(mSurface);
        // Start the Loads
        mTranscodeLoadThread.start();
        mMediaRecorderLoad.start();
    }

    private void stopLoad() throws Exception {
        if (mTranscodeLoadStatus != null) {
            mTranscodeLoadStatus.setLoadFinished();
            mTranscodeLoadStatus = null;
        }
        if (mTranscodeLoadThread != null) {
            mTranscodeLoadThread.join();
            mTranscodeLoadThread = null;
        }
        if (mMediaRecorderLoad != null) {
            // Note that a RuntimeException is intentionally thrown to the application, if no valid
            // audio/video data has been received when stop() is called. This happens if stop() is
            // called immediately after start(). So sleep for 1000ms inorder to make sure some
            // data has been received between start() and stop().
            Thread.sleep(1000);
            mMediaRecorderLoad.stop();
            mMediaRecorderLoad.release();
            mMediaRecorderLoad = null;
            if(mTempRecordedFile != null && mTempRecordedFile.exists()) {
                mTempRecordedFile.delete();
                mTempRecordedFile = null;
            }
        }
        if (mException != null) throw mException;
    }

    private void createSurface() throws InterruptedException {
        mActivityRule.getActivity().waitTillSurfaceIsCreated();
        mSurface = mActivityRule.getActivity().getSurface();
        assertTrue("Surface created is null.", mSurface != null);
        assertTrue("Surface created is invalid.", mSurface.isValid());
        mActivityRule.getActivity().setScreenParams(1920, 1080, true);
    }

    private void releaseSurface() {
        if (mSurface != null) {
            mSurface.release();
            mSurface = null;
        }
    }

    /**
     * This test validates that the initialization latency(time for codec create + configure)
     * for the audio encoders <= 30ms and for video encoders <= 40ms measuring 10 times in
     * succession(5 times alternating sync and async modes). This also logs the stats min, max, avg
     * of the encoder initialization latencies.
     */
    @LargeTest
    @Test(timeout = CodecTestBase.PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testInitializationLatency() throws Exception {
        final int NUM_MEASUREMENTS = 5;
        // Test gathers initialization latency for a number of iterations and
        // percentile is a variable used to control how many of these iterations
        // need to meet the pass criteria. For NUM_MEASUREMENTS at 5, sync and Async
        // modes which is a total of 10 iterations, this translates to index 7.
        final int percentile = 70;
        long expectedMaxCodecInitializationLatencyMs = mMime.startsWith("audio/") ?
                MAX_AUDIOENC_INITIALIZATION_LATENCY_MS : MAX_VIDEOENC_INITIALIZATION_LATENCY_MS;
        long sumOfEncoderInitializationLatencyMs = 0;
        int count = 0;
        long[] encoderInitializationLatencyMs = new long[NUM_MEASUREMENTS * boolStates.length];
        for (int i = 0; i < NUM_MEASUREMENTS; i++) {
            for (boolean isAsync : boolStates) {
                EncoderInitializationLatency encoderInitializationLatency =
                        new EncoderInitializationLatency(mMime, mEncoderName, isAsync);
                long latency = encoderInitializationLatency.calculateEncoderInitializationLatency();
                encoderInitializationLatencyMs[count] = latency;
                sumOfEncoderInitializationLatencyMs += latency;
                count++;
            }
        }
        Arrays.sort(encoderInitializationLatencyMs);

        String statsLog = String.format("CodecInitialization latency for mime: %s, " +
                "Encoder: %s, in Ms :: ", mMime, mEncoderName);
        Log.i(LOG_TAG, "Min " + statsLog + encoderInitializationLatencyMs[0]);
        Log.i(LOG_TAG, "Max " + statsLog + encoderInitializationLatencyMs[count - 1]);
        Log.i(LOG_TAG, "Avg " + statsLog + (sumOfEncoderInitializationLatencyMs / count));

        String errorLog = String.format(
                "CodecInitialization latency for mime: %s, Encoder: %s is not as expected. "
                        + "act/exp in Ms :: %d/%d",
                mMime, mEncoderName, encoderInitializationLatencyMs[percentile * count / 100],
                expectedMaxCodecInitializationLatencyMs);
        assertTrue(errorLog,
                encoderInitializationLatencyMs[percentile * count / 100]
                        <= expectedMaxCodecInitializationLatencyMs);

    }
}

/**
 * The following class calculates the encoder initialization latency (time for codec create +
 * configure). And also logs the time taken by the encoder for:
 * (create + configure + start),
 * (create + configure + start + first frame to enqueue),
 * (create + configure + start + first frame to dequeue).
 */
class EncoderInitializationLatency extends CodecEncoderTestBase {
    private static final String LOG_TAG = EncoderInitializationLatency.class.getSimpleName();

    private final String mEncoderName;
    private final boolean mIsAsync;

    EncoderInitializationLatency(String mime, String encoderName, boolean isAsync) {
        super(mime);
        mEncoderName = encoderName;
        mIsAsync = isAsync;
        mSampleRate = 8000;
        mFrameRate = 60;
    }

    private MediaFormat setUpFormat() throws IOException {
        MediaFormat format = new MediaFormat();
        format.setString(MediaFormat.KEY_MIME, mMime);
        if (mIsAudio) {
            if (mMime.equals(MediaFormat.MIMETYPE_AUDIO_FLAC)) {
                format.setInteger(MediaFormat.KEY_FLAC_COMPRESSION_LEVEL, 10000);
            } else {
                format.setInteger(MediaFormat.KEY_BIT_RATE, 128000);
            }
            format.setInteger(MediaFormat.KEY_SAMPLE_RATE, mSampleRate);
            format.setInteger(MediaFormat.KEY_CHANNEL_COUNT, 1);
        } else {
            MediaCodec codec = MediaCodec.createByCodecName(mEncoderName);
            MediaCodecInfo.CodecCapabilities codecCapabilities =
                    codec.getCodecInfo().getCapabilitiesForType(mMime);
            if (codecCapabilities.getVideoCapabilities().isSizeSupported(1920, 1080)) {
                format.setInteger(MediaFormat.KEY_WIDTH, 1920);
                format.setInteger(MediaFormat.KEY_HEIGHT, 1080);
                format.setInteger(MediaFormat.KEY_BIT_RATE, 8000000);
            } else if (codecCapabilities.getVideoCapabilities().isSizeSupported(1280, 720)) {
                format.setInteger(MediaFormat.KEY_WIDTH, 1280);
                format.setInteger(MediaFormat.KEY_HEIGHT, 720);
                format.setInteger(MediaFormat.KEY_BIT_RATE, 5000000);
            } else if (codecCapabilities.getVideoCapabilities().isSizeSupported(640, 480)) {
                format.setInteger(MediaFormat.KEY_WIDTH, 640);
                format.setInteger(MediaFormat.KEY_HEIGHT, 480);
                format.setInteger(MediaFormat.KEY_BIT_RATE, 2000000);
            } else if (codecCapabilities.getVideoCapabilities().isSizeSupported(352, 288)) {
                format.setInteger(MediaFormat.KEY_WIDTH, 352);
                format.setInteger(MediaFormat.KEY_HEIGHT, 288);
                format.setInteger(MediaFormat.KEY_BIT_RATE, 512000);
            } else {
                format.setInteger(MediaFormat.KEY_WIDTH, 176);
                format.setInteger(MediaFormat.KEY_HEIGHT, 144);
                format.setInteger(MediaFormat.KEY_BIT_RATE, 128000);
            }
            codec.release();
            format.setInteger(MediaFormat.KEY_FRAME_RATE, mFrameRate);
            format.setFloat(MediaFormat.KEY_I_FRAME_INTERVAL, 1.0f);
            format.setInteger(MediaFormat.KEY_COLOR_FORMAT,
                    MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible);
        }
        return format;
    }

    public long calculateEncoderInitializationLatency() throws Exception {
        MediaFormat format = setUpFormat();
        if (mIsAudio) {
            mSampleRate = format.getInteger(MediaFormat.KEY_SAMPLE_RATE);
            mChannels = format.getInteger(MediaFormat.KEY_CHANNEL_COUNT);
        } else {
            mWidth = format.getInteger(MediaFormat.KEY_WIDTH);
            mHeight = format.getInteger(MediaFormat.KEY_HEIGHT);
        }
        setUpSource(mInputFile);
        MediaCodec.BufferInfo outInfo = new MediaCodec.BufferInfo();
        long enqueueTimeStamp = 0;
        long dequeueTimeStamp = 0;
        long baseTimeStamp = System.nanoTime();
        mCodec = MediaCodec.createByCodecName(mEncoderName);
        resetContext(mIsAsync, false);
        mAsyncHandle.setCallBack(mCodec, mIsAsync);
        mCodec.configure(format, null, MediaCodec.CONFIGURE_FLAG_ENCODE, null);
        long configureTimeStamp = System.nanoTime();
        mCodec.start();
        long startTimeStamp = System.nanoTime();
        if (mIsAsync) {
            // We will keep on feeding the input to encoder until we see the first dequeued frame.
            while (!mAsyncHandle.hasSeenError() && !mSawInputEOS) {
                Pair<Integer, MediaCodec.BufferInfo> element = mAsyncHandle.getWork();
                if (element != null) {
                    int bufferID = element.first;
                    MediaCodec.BufferInfo info = element.second;
                    if (info != null) {
                        dequeueTimeStamp = System.nanoTime();
                        dequeueOutput(bufferID, info);
                        break;
                    } else {
                        if (enqueueTimeStamp == 0) {
                            enqueueTimeStamp = System.nanoTime();
                        }
                        enqueueInput(bufferID);
                    }
                }
            }
        } else {
            while (!mSawOutputEOS) {
                if (!mSawInputEOS) {
                    int inputBufferId = mCodec.dequeueInputBuffer(Q_DEQ_TIMEOUT_US);
                    if (inputBufferId > 0) {
                        if (enqueueTimeStamp == 0) {
                            enqueueTimeStamp = System.nanoTime();
                        }
                        enqueueInput(inputBufferId);
                    }
                }
                int outputBufferId = mCodec.dequeueOutputBuffer(outInfo, Q_DEQ_TIMEOUT_US);
                if (outputBufferId >= 0) {
                    dequeueTimeStamp = System.nanoTime();
                    dequeueOutput(outputBufferId, outInfo);
                    break;
                }
            }
        }
        queueEOS();
        waitForAllOutputs();
        mCodec.stop();
        mCodec.release();
        Log.d(LOG_TAG, "Encode mMime: " + mMime + " Encoder: " + mEncoderName +
                " Time for (create + configure) in ns: " + (configureTimeStamp - baseTimeStamp));
        Log.d(LOG_TAG, "Encode mMime: " + mMime + " Encoder: " + mEncoderName +
                " Time for (create + configure + start) in ns: " +
                (startTimeStamp - baseTimeStamp));
        Log.d(LOG_TAG, "Encode mMime: " + mMime + " Encoder: " + mEncoderName +
                " Time for (create + configure + start + first frame to enqueue) in ns: " +
                (enqueueTimeStamp - baseTimeStamp));
        Log.d(LOG_TAG, "Encode mMime: " + mMime + " Encoder: " + mEncoderName +
                " Time for (create + configure + start + first frame to dequeue) in ns: " +
                (dequeueTimeStamp - baseTimeStamp));
        long timeToConfigureMs = (configureTimeStamp - baseTimeStamp) / 1000000;
        return timeToConfigureMs;
    }
}
