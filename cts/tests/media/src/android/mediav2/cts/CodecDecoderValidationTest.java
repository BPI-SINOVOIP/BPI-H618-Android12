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
 * The following test validates decoder for the given input clip. For audio components, we check
 * if the output buffers timestamp is strictly increasing. If possible the decoded output rms is
 * compared against a reference value and the error is expected to be within a tolerance of 5%. For
 * video components, we check if the output buffers timestamp is identical to the sorted input pts
 * list. Also for video standard post mpeg4, the decoded output checksum is compared against
 * reference checksum.
 */
@RunWith(Parameterized.class)
public class CodecDecoderValidationTest extends CodecDecoderTestBase {
    private static final String LOG_TAG = CodecDecoderValidationTest.class.getSimpleName();

    private final String[] mSrcFiles;
    private final String mRefFile;
    private final float mRmsError;
    private final long mRefCRC;
    private final int mSupport;

    public CodecDecoderValidationTest(String decoder, String mime, String[] srcFiles,
            String refFile, float rmsError, long refCRC, int support) {
        super(decoder, mime, null);
        mSrcFiles = srcFiles;
        mRefFile = refFile;
        mRmsError = rmsError;
        mRefCRC = refCRC;
        mSupport = support;
    }

    @Parameterized.Parameters(name = "{index}({0}_{1})")
    public static Collection<Object[]> input() {
        final boolean isEncoder = false;
        final boolean needAudio = true;
        final boolean needVideo = true;
        // mime, array list of test files (underlying elementary stream is same, except they
        // are placed in different containers), ref file, rms error, checksum
        final List<Object[]> exhaustiveArgsList = Arrays.asList(new Object[][]{
                // vp9 test vectors with no-show frames signalled in alternate ways
                {MediaFormat.MIMETYPE_VIDEO_VP9, new String[]{
                        "bbb_340x280_768kbps_30fps_split_non_display_frame_vp9.webm",
                        "bbb_340x280_768kbps_30fps_vp9.webm"}, null, -1.0f, 4122701060L, CODEC_ALL},
                {MediaFormat.MIMETYPE_VIDEO_VP9, new String[]{
                        "bbb_520x390_1mbps_30fps_split_non_display_frame_vp9.webm",
                        "bbb_520x390_1mbps_30fps_vp9.webm"}, null, -1.0f, 1201859039L, CODEC_ALL},

                // mpeg2 test vectors with interlaced fields signalled in alternate ways
                {MediaFormat.MIMETYPE_VIDEO_MPEG2, new String[]{
                        "bbb_512x288_30fps_1mbps_mpeg2_interlaced_nob_1field.ts",
                        "bbb_512x288_30fps_1mbps_mpeg2_interlaced_nob_2fields.mp4"}, null, -1.0f,
                        -1L, CODEC_ALL},

                // clips with crop parameters
//                /* TODO(b/163299340) */
//                {MediaFormat.MIMETYPE_VIDEO_HEVC, new String[]{"bbb_560x280_1mbps_30fps_hevc.mkv"},
//                        null, -1.0f, 26298353L, CODEC_ALL},
//                /* TODO(b/163299340) */
//                {MediaFormat.MIMETYPE_VIDEO_AVC, new String[]{"bbb_504x224_768kbps_30fps_avc.mp4"},
//                        null, -1.0f, 4060874918L, CODEC_ALL},

                // misc mp3 test vectors
                {MediaFormat.MIMETYPE_AUDIO_MPEG, new String[]{"bbb_1ch_16kHz_lame_vbr.mp3"},
                        "bbb_1ch_16kHz_s16le.raw", 119.256f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_MPEG, new String[]{"bbb_2ch_44kHz_lame_vbr.mp3"},
                        "bbb_2ch_44kHz_s16le.raw", 53.066f, -1L, CODEC_ALL},

                // mp3 test vectors with CRC
                {MediaFormat.MIMETYPE_AUDIO_MPEG, new String[]{"bbb_2ch_44kHz_lame_crc.mp3"},
                        "bbb_2ch_44kHz_s16le.raw", 104.09f, -1L, CODEC_ALL},

                // vp9 test vectors with AQ mode enabled
                {MediaFormat.MIMETYPE_VIDEO_VP9, new String[]{"bbb_1280x720_800kbps_30fps_vp9" +
                        ".webm"}, null, -1.0f, 1319105122L, CODEC_ALL},
                {MediaFormat.MIMETYPE_VIDEO_VP9, new String[]{"bbb_1280x720_1200kbps_30fps_vp9" +
                        ".webm"}, null, -1.0f, 4128150660L, CODEC_ALL},
                {MediaFormat.MIMETYPE_VIDEO_VP9, new String[]{"bbb_1280x720_1600kbps_30fps_vp9" +
                        ".webm"}, null, -1.0f, 156928091L, CODEC_ALL},
                {MediaFormat.MIMETYPE_VIDEO_VP9, new String[]{"bbb_1280x720_2000kbps_30fps_vp9" +
                        ".webm"}, null, -1.0f, 3902485256L, CODEC_ALL},

                // video test vectors of non standard sizes
                {MediaFormat.MIMETYPE_VIDEO_MPEG2, new String[]{
                        "bbb_642x642_2mbps_30fps_mpeg2.mp4"}, null, -1.0f, -1L, CODEC_ANY},
                {MediaFormat.MIMETYPE_VIDEO_AVC, new String[]{
                        "bbb_642x642_1mbps_30fps_avc.mp4"}, null, -1.0f, 3947092788L, CODEC_ANY},
                {MediaFormat.MIMETYPE_VIDEO_VP8, new String[]{
                        "bbb_642x642_1mbps_30fps_vp8.webm"}, null, -1.0f, 516982978L, CODEC_ANY},
                {MediaFormat.MIMETYPE_VIDEO_HEVC, new String[]{
                        "bbb_642x642_768kbps_30fps_hevc.mp4"}, null, -1.0f, 3018465268L, CODEC_ANY},
                {MediaFormat.MIMETYPE_VIDEO_VP9, new String[]{
                        "bbb_642x642_768kbps_30fps_vp9.webm"}, null, -1.0f, 4032809269L, CODEC_ANY},
                {MediaFormat.MIMETYPE_VIDEO_AV1, new String[]{
                        "bbb_642x642_768kbps_30fps_av1.mp4"}, null, -1.0f, 3684481474L, CODEC_ANY},
                {MediaFormat.MIMETYPE_VIDEO_MPEG4, new String[]{
                        "bbb_130x130_192kbps_15fps_mpeg4.mp4"}, null, -1.0f, -1L, CODEC_ANY},

                //  audio test vectors covering cdd sec 5.1.3
                // amr nb
                {MediaFormat.MIMETYPE_AUDIO_AMR_NB, new String[]{
                        "audio/bbb_mono_8kHz_12.2kbps_amrnb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_NB, new String[]{
                        "audio/bbb_mono_8kHz_10.2kbps_amrnb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_NB, new String[]{
                        "audio/bbb_mono_8kHz_7.95kbps_amrnb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_NB, new String[]{
                        "audio/bbb_mono_8kHz_7.40kbps_amrnb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_NB, new String[]{
                        "audio/bbb_mono_8kHz_6.70kbps_amrnb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_NB, new String[]{
                        "audio/bbb_mono_8kHz_5.90kbps_amrnb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_NB, new String[]{
                        "audio/bbb_mono_8kHz_5.15kbps_amrnb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_NB, new String[]{
                        "audio/bbb_mono_8kHz_4.75kbps_amrnb.3gp"}, null, -1.0f, -1L, CODEC_ALL},

                // amr wb
                {MediaFormat.MIMETYPE_AUDIO_AMR_WB, new String[]{
                        "audio/bbb_mono_16kHz_6.6kbps_amrwb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_WB, new String[]{
                        "audio/bbb_mono_16kHz_8.85kbps_amrwb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_WB, new String[]{
                        "audio/bbb_mono_16kHz_12.65kbps_amrwb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_WB, new String[]{
                        "audio/bbb_mono_16kHz_14.25kbps_amrwb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_WB, new String[]{
                        "audio/bbb_mono_16kHz_15.85kbps_amrwb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_WB, new String[]{
                        "audio/bbb_mono_16kHz_18.25kbps_amrwb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_WB, new String[]{
                        "audio/bbb_mono_16kHz_19.85kbps_amrwb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_WB, new String[]{
                        "audio/bbb_mono_16kHz_23.05kbps_amrwb.3gp"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AMR_WB, new String[]{
                        "audio/bbb_mono_16kHz_23.85kbps_amrwb.3gp"}, null, -1.0f, -1L, CODEC_ALL},

                // flac
                // TODO(add content for 96kHz and 192kHz)
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_1ch_8kHz_lvl4_flac.mka"},
                        "audio/bbb_1ch_8kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_1ch_12kHz_lvl4_flac.mka"},
                        "audio/bbb_1ch_12kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_1ch_16kHz_lvl4_flac.mka"},
                        "audio/bbb_1ch_16kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_1ch_22kHz_lvl4_flac.mka"},
                        "audio/bbb_1ch_22kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_1ch_24kHz_lvl4_flac.mka"},
                        "audio/bbb_1ch_24kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_1ch_32kHz_lvl4_flac.mka"},
                        "audio/bbb_1ch_32kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_1ch_44kHz_lvl4_flac.mka"},
                        "audio/bbb_1ch_44kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_1ch_48kHz_lvl4_flac.mka"},
                        "audio/bbb_1ch_48kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_2ch_8kHz_lvl4_flac.mka"},
                        "audio/bbb_2ch_8kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_2ch_12kHz_lvl4_flac.mka"},
                        "audio/bbb_2ch_12kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_2ch_16kHz_lvl4_flac.mka"},
                        "audio/bbb_2ch_16kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_2ch_22kHz_lvl4_flac.mka"},
                        "audio/bbb_2ch_22kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_2ch_24kHz_lvl4_flac.mka"},
                        "audio/bbb_2ch_24kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_2ch_32kHz_lvl4_flac.mka"},
                        "audio/bbb_2ch_32kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_2ch_44kHz_lvl4_flac.mka"},
                        "audio/bbb_2ch_44kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_FLAC, new String[]{"audio/bbb_2ch_48kHz_lvl4_flac.mka"},
                        "audio/bbb_2ch_48kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},

                // opus
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_1ch_8kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_1ch_12kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_1ch_16kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_1ch_24kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_1ch_32kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_1ch_48kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_2ch_8kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_2ch_12kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_2ch_16kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_2ch_24kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_2ch_32kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_2ch_48kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_5ch_8kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_5ch_12kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_5ch_16kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_5ch_24kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_5ch_32kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_5ch_48kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_6ch_8kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_6ch_12kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_6ch_16kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_6ch_24kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_6ch_32kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_OPUS, new String[]{
                        "audio/bbb_6ch_48kHz_opus.ogg"}, null, -1.0f, -1L, CODEC_ALL},

                // raw
                // TODO: add content for larger sampling rates and float pcm
                {MediaFormat.MIMETYPE_AUDIO_RAW, new String[]{"audio/bbb_1ch_8kHz.wav"},
                        "audio/bbb_1ch_8kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_RAW, new String[]{"audio/bbb_1ch_16kHz.wav"},
                        "audio/bbb_1ch_16kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_RAW, new String[]{"audio/bbb_1ch_22kHz.wav"},
                        "audio/bbb_1ch_22kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_RAW, new String[]{"audio/bbb_1ch_24kHz.wav"},
                        "audio/bbb_1ch_24kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_RAW, new String[]{"audio/bbb_1ch_32kHz.wav"},
                        "audio/bbb_1ch_32kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_RAW, new String[]{"audio/bbb_1ch_44kHz.wav"},
                        "audio/bbb_1ch_44kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_RAW, new String[]{"audio/bbb_1ch_48kHz.wav"},
                        "audio/bbb_1ch_48kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_RAW, new String[]{"audio/bbb_2ch_8kHz.wav"},
                        "audio/bbb_2ch_8kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_RAW, new String[]{"audio/bbb_2ch_16kHz.wav"},
                        "audio/bbb_2ch_16kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_RAW, new String[]{"audio/bbb_2ch_22kHz.wav"},
                        "audio/bbb_2ch_22kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_RAW, new String[]{"audio/bbb_2ch_24kHz.wav"},
                        "audio/bbb_2ch_24kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_RAW, new String[]{"audio/bbb_2ch_32kHz.wav"},
                        "audio/bbb_2ch_32kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_RAW, new String[]{"audio/bbb_2ch_44kHz.wav"},
                        "audio/bbb_2ch_44kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_RAW, new String[]{"audio/bbb_2ch_48kHz.wav"},
                        "audio/bbb_2ch_48kHz_s16le_3s.raw", 0.0f, -1L, CODEC_ALL},

                // aac-lc
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_8kHz_aac_lc.m4a"},
                        "audio/bbb_1ch_8kHz_s16le_3s.raw", 26.907248f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_12kHz_aac_lc.m4a"},
                        "audio/bbb_1ch_12kHz_s16le_3s.raw", 23.366642f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_16kHz_aac_lc.m4a"},
                        "audio/bbb_1ch_16kHz_s16le_3s.raw", 21.354156f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_22kHz_aac_lc.m4a"},
                        "audio/bbb_1ch_22kHz_s16le_3s.raw", 25.980762f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_24kHz_aac_lc.m4a"},
                        "audio/bbb_1ch_24kHz_s16le_3s.raw", 26.362852f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_32kHz_aac_lc.m4a"},
                        "audio/bbb_1ch_32kHz_s16le_3s.raw", 28.635643f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_44kHz_aac_lc.m4a"},
                        "audio/bbb_1ch_44kHz_s16le_3s.raw", 29.291637f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_48kHz_aac_lc.m4a"},
                        "audio/bbb_1ch_48kHz_s16le_3s.raw", 29.325756f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_8kHz_aac_lc.m4a"},
                        "audio/bbb_2ch_8kHz_s16le_3s.raw", 26.362852f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_12kHz_aac_lc.m4a"},
                        "audio/bbb_2ch_12kHz_s16le_3s.raw", 21.931713f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_16kHz_aac_lc.m4a"},
                        "audio/bbb_2ch_16kHz_s16le_3s.raw", 22.068077f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_22kHz_aac_lc.m4a"},
                        "audio/bbb_2ch_22kHz_s16le_3s.raw", 25.317978f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_24kHz_aac_lc.m4a"},
                        "audio/bbb_2ch_24kHz_s16le_3s.raw", 25.651510f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_32kHz_aac_lc.m4a"},
                        "audio/bbb_2ch_32kHz_s16le_3s.raw", 27.294687f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_44kHz_aac_lc.m4a"},
                        "audio/bbb_2ch_44kHz_s16le_3s.raw", 27.313000f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_48kHz_aac_lc.m4a"},
                        "audio/bbb_2ch_48kHz_s16le_3s.raw", 27.676704f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_5ch_8kHz_aac_lc.m4a"},
                        "audio/bbb_5ch_8kHz_s16le_3s.raw", 43.116123f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_5ch_12kHz_aac_lc.m4a"},
                        "audio/bbb_5ch_12kHz_s16le_3s.raw", 35.972210f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_5ch_16kHz_aac_lc.m4a"},
                        "audio/bbb_5ch_16kHz_s16le_3s.raw", 32.710854f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_5ch_22kHz_aac_lc.m4a"},
                        "audio/bbb_5ch_22kHz_s16le_3s.raw", 39.281040f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_5ch_24kHz_aac_lc.m4a"},
                        "audio/bbb_5ch_24kHz_s16le_3s.raw", 40.951191f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_5ch_32kHz_aac_lc.m4a"},
                        "audio/bbb_5ch_32kHz_s16le_3s.raw", 49.436829f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_5ch_44kHz_aac_lc.m4a"},
                        "audio/bbb_5ch_44kHz_s16le_3s.raw", 43.886215f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_5ch_48kHz_aac_lc.m4a"},
                        "audio/bbb_5ch_48kHz_s16le_3s.raw", 44.271889f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_6ch_8kHz_aac_lc.m4a"},
                        "audio/bbb_6ch_8kHz_s16le_3s.raw", 39.661064f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_6ch_12kHz_aac_lc.m4a"},
                        "audio/bbb_6ch_12kHz_s16le_3s.raw", 34.971416f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_6ch_16kHz_aac_lc.m4a"},
                        "audio/bbb_6ch_16kHz_s16le_3s.raw", 29.068884f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_6ch_22kHz_aac_lc.m4a"},
                        "audio/bbb_6ch_22kHz_s16le_3s.raw", 29.427877f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_6ch_24kHz_aac_lc.m4a"},
                        "audio/bbb_6ch_24kHz_s16le_3s.raw", 30.331501f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_6ch_32kHz_aac_lc.m4a"},
                        "audio/bbb_6ch_32kHz_s16le_3s.raw", 33.926392f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_6ch_44kHz_aac_lc.m4a"},
                        "audio/bbb_6ch_44kHz_s16le_3s.raw", 31.733263f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_6ch_48kHz_aac_lc.m4a"},
                        "audio/bbb_6ch_48kHz_s16le_3s.raw", 31.032242f, -1L, CODEC_ALL},

                // aac-he
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_2ch_16kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_2ch_22kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_2ch_24kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_2ch_32kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_2ch_44kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_2ch_48kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_5ch_16kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_5ch_22kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_5ch_24kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_5ch_32kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_5ch_44kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_5ch_48kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_6ch_16kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_6ch_22kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_6ch_24kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_6ch_32kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_6ch_44kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_6ch_48kHz_aac_he.m4a"}, null, -1.0f, -1L, CODEC_ALL},

                // aac-eld
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_16kHz_aac_eld.m4a"},
                        "audio/bbb_1ch_16kHz_s16le_3s.raw", -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_22kHz_aac_eld.m4a"},
                        "audio/bbb_1ch_22kHz_s16le_3s.raw", 24.959969f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_24kHz_aac_eld.m4a"},
                        "audio/bbb_1ch_24kHz_s16le_3s.raw", 26.495283f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_32kHz_aac_eld.m4a"},
                        "audio/bbb_1ch_32kHz_s16le_3s.raw", 31.464266f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_44kHz_aac_eld.m4a"},
                        "audio/bbb_1ch_44kHz_s16le_3s.raw", 33.852623f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_48kHz_aac_eld.m4a"},
                        "audio/bbb_1ch_48kHz_s16le_3s.raw", 33.136082f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_16kHz_aac_eld.m4a"},
                        "audio/bbb_2ch_16kHz_s16le_3s.raw", -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_22kHz_aac_eld.m4a"},
                        "audio/bbb_2ch_22kHz_s16le_3s.raw", 24.959969f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_24kHz_aac_eld.m4a"},
                        "audio/bbb_2ch_24kHz_s16le_3s.raw", 26.962938f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_32kHz_aac_eld.m4a"},
                        "audio/bbb_2ch_32kHz_s16le_3s.raw", 27.784887f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_44kHz_aac_eld.m4a"},
                        "audio/bbb_2ch_44kHz_s16le_3s.raw", 29.223278f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_48kHz_aac_eld.m4a"},
                        "audio/bbb_2ch_48kHz_s16le_3s.raw", 29.171904f, -1L, CODEC_ALL},

                // aac-hev2
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_2ch_16kHz_aac_hev2.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_2ch_22kHz_aac_hev2.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_2ch_24kHz_aac_hev2.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_2ch_32kHz_aac_hev2.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_2ch_44kHz_aac_hev2.m4a"}, null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{
                        "audio/bbb_2ch_48kHz_aac_hev2.m4a"}, null, -1.0f, -1L, CODEC_ALL},

                // aac-usac
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_8kHz_usac.m4a"},
                        null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_16kHz_usac.m4a"},
                        null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_22kHz_usac.m4a"},
                        null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_24kHz_usac.m4a"},
                        null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_32kHz_usac.m4a"},
                        null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_44kHz_usac.m4a"},
                        null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_1ch_48kHz_usac.m4a"},
                        null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_8kHz_usac.m4a"},
                        null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_16kHz_usac.m4a"},
                        null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_22kHz_usac.m4a"},
                        null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_24kHz_usac.m4a"},
                        null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_32kHz_usac.m4a"},
                        null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_44kHz_usac.m4a"},
                        null, -1.0f, -1L, CODEC_ALL},
                {MediaFormat.MIMETYPE_AUDIO_AAC, new String[]{"audio/bbb_2ch_48kHz_usac.m4a"},
                        null, -1.0f, -1L, CODEC_ALL},
        });
        return prepareParamList(exhaustiveArgsList, isEncoder, needAudio, needVideo, false);
    }

    /**
     * Test decodes and compares decoded output of two files.
     */
    @LargeTest
    @Test(timeout = PER_TEST_TIMEOUT_LARGE_TEST_MS)
    public void testDecodeAndValidate() throws IOException, InterruptedException {
        ArrayList<MediaFormat> formats = new ArrayList<>();
        for (String file : mSrcFiles) {
            formats.add(setUpSource(file));
            mExtractor.release();
        }
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
        final int mode = MediaExtractor.SEEK_TO_CLOSEST_SYNC;
        {
            OutputManager ref = null;
            for (String file : mSrcFiles) {
                decodeToMemory(file, mCodecName, 0, mode, Integer.MAX_VALUE);
                String log = String.format("codec: %s, test file: %s:: ", mCodecName, file);
                assertTrue(log + " unexpected error", !mAsyncHandle.hasSeenError());
                assertTrue(log + "no input sent", 0 != mInputCount);
                assertTrue(log + "output received", 0 != mOutputCount);
                if (ref == null) ref = mOutputBuff;
                if (mIsAudio) {
                    assertTrue("reference output pts is not strictly increasing",
                            mOutputBuff.isPtsStrictlyIncreasing(mPrevOutputPts));
                } else if (!mIsInterlaced) {
                    assertTrue("input pts list and output pts list are not identical",
                            mOutputBuff.isOutPtsListIdenticalToInpPtsList(false));
                }
                if (mIsInterlaced) {
                    assertTrue(log + "decoder outputs are not identical",
                            ref.equalsInterlaced(mOutputBuff));
                } else {
                    assertTrue(log + "decoder outputs are not identical", ref.equals(mOutputBuff));
                }
            }
            CodecDecoderTest.verify(mOutputBuff, mRefFile, mRmsError, mRefCRC);
        }
    }
}
