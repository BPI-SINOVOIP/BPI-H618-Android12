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

import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import androidx.test.rule.ActivityTestRule;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static android.mediapc.cts.CodecTestBase.selectCodecs;
import static android.mediapc.cts.CodecTestBase.selectHardwareCodecs;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

public class FrameDropTestBase {
    private static final String LOG_TAG = FrameDropTestBase.class.getSimpleName();
    static final boolean[] boolStates = {false, true};
    static final String AVC = MediaFormat.MIMETYPE_VIDEO_AVC;
    static final String HEVC = MediaFormat.MIMETYPE_VIDEO_HEVC;
    static final String VP8 = MediaFormat.MIMETYPE_VIDEO_VP8;
    static final String VP9 = MediaFormat.MIMETYPE_VIDEO_VP9;
    static final String AV1 = MediaFormat.MIMETYPE_VIDEO_AV1;
    static final String AAC = MediaFormat.MIMETYPE_AUDIO_AAC;
    static final String AAC_LOAD_FILE_NAME = "bbb_1c_128kbps_aac_audio.mp4";
    static final String AVC_LOAD_FILE_NAME = "bbb_1280x720_3mbps_30fps_avc.mp4";
    static final long DECODE_31S = 31000; // In ms
    static final int MAX_ADAPTIVE_PLAYBACK_FRAME_DROP = 0;
    static final int FRAME_RATE = Utils.isSPerfClass() ? 60 : 30;
    static final int MAX_FRAME_DROP_FOR_30S;

    final String mMime;
    final String mDecoderName;
    final boolean mIsAsync;
    Surface mSurface;

    private LoadStatus mLoadStatus = null;
    private Thread mTranscodeLoadThread = null;
    private Thread mAudioPlaybackLoadThread = null;
    private Exception mTranscodeLoadException = null;
    private Exception mAudioPlaybackLoadException = null;

    static String AVC_DECODER_NAME;
    static String AVC_ENCODER_NAME;
    static String AAC_DECODER_NAME;
    static Map<String, String> m540pTestFiles = new HashMap<>();
    static Map<String, String> m1080pTestFiles = new HashMap<>();
    static {
        AVC_DECODER_NAME = selectHardwareCodecs(AVC, null, null, false).get(0);
        AVC_ENCODER_NAME = selectHardwareCodecs(AVC, null, null, true).get(0);
        AAC_DECODER_NAME = selectCodecs(AAC, null, null, false).get(0);
    }
    static {
        if (Utils.isSPerfClass()) {
            // Two frame drops per 10 seconds at 60 fps is 6 drops per 30 seconds
            MAX_FRAME_DROP_FOR_30S = 6;
            m540pTestFiles.put(AVC, "bbb_960x540_3mbps_60fps_avc.mp4");
            m540pTestFiles.put(HEVC, "bbb_960x540_3mbps_60fps_hevc.mp4");
            m540pTestFiles.put(VP8, "bbb_960x540_3mbps_60fps_vp8.webm");
            m540pTestFiles.put(VP9, "bbb_960x540_3mbps_60fps_vp9.webm");
            m540pTestFiles.put(AV1, "bbb_960x540_3mbps_60fps_av1.mp4");

            m1080pTestFiles.put(AVC, "bbb_1920x1080_8mbps_60fps_avc.mp4");
            m1080pTestFiles.put(HEVC, "bbb_1920x1080_6mbps_60fps_hevc.mp4");
            m1080pTestFiles.put(VP8, "bbb_1920x1080_8mbps_60fps_vp8.webm");
            m1080pTestFiles.put(VP9, "bbb_1920x1080_6mbps_60fps_vp9.webm");
            m1080pTestFiles.put(AV1, "bbb_1920x1080_6mbps_60fps_av1.mp4");
        } else if (Utils.isRPerfClass()) {
            // One frame drops per 10 seconds at 30 fps is 3 drops per 30 seconds
            MAX_FRAME_DROP_FOR_30S = 3;
            m540pTestFiles.put(AVC, "bbb_960x540_2mbps_30fps_avc.mp4");
            m540pTestFiles.put(HEVC, "bbb_960x540_2mbps_30fps_hevc.mp4");
            m540pTestFiles.put(VP8, "bbb_960x540_2mbps_30fps_vp8.webm");
            m540pTestFiles.put(VP9, "bbb_960x540_2mbps_30fps_vp9.webm");
            m540pTestFiles.put(AV1, "bbb_960x540_2mbps_30fps_av1.mp4");

            m1080pTestFiles.put(AVC, "bbb_1920x1080_6mbps_30fps_avc.mp4");
            m1080pTestFiles.put(HEVC, "bbb_1920x1080_4mbps_30fps_hevc.mp4");
            m1080pTestFiles.put(VP8, "bbb_1920x1080_6mbps_30fps_vp8.webm");
            m1080pTestFiles.put(VP9, "bbb_1920x1080_4mbps_30fps_vp9.webm");
            m1080pTestFiles.put(AV1, "bbb_1920x1080_4mbps_30fps_av1.mp4");
        } else {
            MAX_FRAME_DROP_FOR_30S = 0;
            Log.e(LOG_TAG, "Unknown performance class.");
        }
    }

    @Before
    public void setUp() throws Exception {
        assumeTrue("Test requires performance class.", Utils.isPerfClass());
        createSurface();
        startLoad();
    }

    @After
    public void tearDown() throws Exception {
        stopLoad();
        releaseSurface();
    }

    @Rule
    public ActivityTestRule<TestActivity> mActivityRule =
            new ActivityTestRule<>(TestActivity.class);

    public FrameDropTestBase(String mimeType, String decoderName, boolean isAsync) {
        mMime = mimeType;
        mDecoderName = decoderName;
        mIsAsync = isAsync;
    }

    // Returns the list of objects with mimeTypes and their hardware decoders supporting the
    // given features combining with sync and async modes.
    static List<Object[]> prepareArgumentsList(String[] features) {
        final List<Object[]> argsList = new ArrayList<>();
        final String[] mimesList = new String[] {AVC, HEVC, VP8, VP9, AV1};
        for (String mime : mimesList) {
            MediaFormat format = MediaFormat.createVideoFormat(mime, 1920, 1080);
            format.setInteger(MediaFormat.KEY_FRAME_RATE, FRAME_RATE);
            ArrayList<MediaFormat> formats = new ArrayList<>();
            formats.add(format);
            ArrayList<String> listOfDecoders =
                    selectHardwareCodecs(mime, formats, features, false);
            for (String decoder : listOfDecoders) {
                for (boolean isAsync : boolStates) {
                    argsList.add(new Object[]{mime, decoder, isAsync});
                }
            }
        }
        return argsList;
    }

    private void createSurface() throws InterruptedException {
        mActivityRule.getActivity().waitTillSurfaceIsCreated();
        mSurface = mActivityRule.getActivity().getSurface();
        assertTrue("Surface created is null.", mSurface != null);
        assertTrue("Surface created is invalid.", mSurface.isValid());
        // As we display 1920x1080 and 960x540 only which are of same aspect ratio, we will
        // be setting screen params to 1920x1080
        mActivityRule.getActivity().setScreenParams(1920, 1080, true);
    }

    private void releaseSurface() {
        if (mSurface != null) {
            mSurface.release();
            mSurface = null;
        }
    }

    private Thread createTranscodeLoad() {
        Thread transcodeLoadThread = new Thread(() -> {
            try {
                TranscodeLoad transcodeLoad = new TranscodeLoad(AVC, AVC_LOAD_FILE_NAME,
                        AVC_DECODER_NAME, AVC_ENCODER_NAME, mLoadStatus);
                transcodeLoad.doTranscode();
            } catch (Exception e) {
                mTranscodeLoadException = e;
            }
        });
        return transcodeLoadThread;
    }

    private Thread createAudioPlaybackLoad() {
        Thread audioPlaybackLoadThread = new Thread(() -> {
            try {
                AudioPlaybackLoad audioPlaybackLoad = new AudioPlaybackLoad(AAC, AAC_LOAD_FILE_NAME,
                        AAC_DECODER_NAME, mLoadStatus);
                audioPlaybackLoad.doDecodeAndPlayback();
            } catch (Exception e) {
                mAudioPlaybackLoadException = e;
            }
        });
        return audioPlaybackLoadThread;
    }

    private void startLoad() {
        // TODO: b/183671436
        // Start Transcode load (Decoder(720p) + Encoder(720p))
        mLoadStatus = new LoadStatus();
        mTranscodeLoadThread = createTranscodeLoad();
        mTranscodeLoadThread.start();
        // Start 128kbps AAC audio playback
        mAudioPlaybackLoadThread = createAudioPlaybackLoad();
        mAudioPlaybackLoadThread.start();
    }

    private void stopLoad() throws Exception {
        if (mLoadStatus != null) {
            mLoadStatus.setLoadFinished();
            mLoadStatus = null;
        }
        if (mTranscodeLoadThread != null) {
            mTranscodeLoadThread.join();
            mTranscodeLoadThread = null;
        }
        if (mAudioPlaybackLoadThread != null) {
            mAudioPlaybackLoadThread.join();
            mAudioPlaybackLoadThread = null;
        }
        if (mTranscodeLoadException != null) throw mTranscodeLoadException;
        if (mAudioPlaybackLoadException != null) throw mAudioPlaybackLoadException;
    }
}
