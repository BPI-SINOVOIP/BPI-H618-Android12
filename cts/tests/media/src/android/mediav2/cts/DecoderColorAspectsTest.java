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

import android.media.MediaFormat;

import androidx.test.filters.SmallTest;
import androidx.test.rule.ActivityTestRule;

import org.junit.Assume;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

@RunWith(Parameterized.class)
public class DecoderColorAspectsTest extends CodecDecoderTestBase {
    private static final String LOG_TAG = DecoderColorAspectsTest.class.getSimpleName();
    private final int mColorRange;
    private final int mColorStandard;
    private final int mColorTransferCurve;
    private final boolean mCanIgnoreColorBox;
    private ArrayList<String> mCheckESList;

    public DecoderColorAspectsTest(String decoderName, String mime, String testFile, int range,
            int standard, int transferCurve, boolean canIgnoreColorBox) {
        super(decoderName, mime, testFile);
        mColorRange = range;
        mColorStandard = standard;
        mColorTransferCurve = transferCurve;
        mCheckESList = new ArrayList<>();
        mCheckESList.add(MediaFormat.MIMETYPE_VIDEO_AVC);
        mCheckESList.add(MediaFormat.MIMETYPE_VIDEO_HEVC);
        /* TODO (b/165492703) Mpeg2 and (b/165787556) AV1 has problems in signalling color
            aspects information via elementary stream. */
        // mCheckESList.add(MediaFormat.MIMETYPE_VIDEO_MPEG2);
        // mCheckESList.add(MediaFormat.MIMETYPE_VIDEO_AV1);
        mCanIgnoreColorBox = canIgnoreColorBox;
    }

    @Parameterized.Parameters(name = "{index}({0}_{1}_{3}_{4}_{5})")
    public static Collection<Object[]> input() {
        final boolean isEncoder = false;
        final boolean needAudio = true;
        final boolean needVideo = true;

        // testClip, colorRange, colorStandard, colorTransfer
        final List<Object[]> exhaustiveArgsList = Arrays.asList(new Object[][]{
                // h264 clips
                {MediaFormat.MIMETYPE_VIDEO_AVC, "bbb_qcif_color_bt709_lr_sdr_avc.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT709,
                        MediaFormat.COLOR_TRANSFER_SDR_VIDEO, true},
                {MediaFormat.MIMETYPE_VIDEO_AVC, "bbb_qcif_color_bt601_625_fr_gamma22_avc.mp4",
                        MediaFormat.COLOR_RANGE_FULL, MediaFormat.COLOR_STANDARD_BT601_PAL,
                        /* MediaFormat.COLOR_TRANSFER_GAMMA2_2 */ 4, true},
                {MediaFormat.MIMETYPE_VIDEO_AVC, "bbb_qcif_color_bt601_525_lr_gamma28_avc.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT601_NTSC,
                        /* MediaFormat.COLOR_TRANSFER_GAMMA2_8 */ 5, true},
                {MediaFormat.MIMETYPE_VIDEO_AVC, "bbb_qcif_color_bt709_lr_srgb_avc.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT709,
                        /* MediaFormat.COLOR_TRANSFER_SRGB */ 2, true},
                {MediaFormat.MIMETYPE_VIDEO_AVC, "bbb_qcif_color_unspcfd_avc.mp4",
                        UNSPECIFIED, UNSPECIFIED, UNSPECIFIED, true},
                {MediaFormat.MIMETYPE_VIDEO_AVC, "bbb_qcif_color_bt470m_linear_fr_avc.mp4",
                        MediaFormat.COLOR_RANGE_FULL, /* MediaFormat.COLOR_STANDARD_BT470M */ 8,
                        MediaFormat.COLOR_TRANSFER_LINEAR, true},
                {MediaFormat.MIMETYPE_VIDEO_AVC,
                        "bikes_qcif_color_bt2020_smpte2084_bt2020Ncl_lr_avc.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT2020,
                        MediaFormat.COLOR_TRANSFER_ST2084, true},
                {MediaFormat.MIMETYPE_VIDEO_AVC,
                        "bikes_qcif_color_bt2020_smpte2086Hlg_bt2020Ncl_fr_avc.mp4",
                        MediaFormat.COLOR_RANGE_FULL, MediaFormat.COLOR_STANDARD_BT2020,
                        MediaFormat.COLOR_TRANSFER_HLG, true},

                // h265 clips
                {MediaFormat.MIMETYPE_VIDEO_HEVC, "bbb_qcif_color_bt709_lr_sdr_hevc.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT709,
                        MediaFormat.COLOR_TRANSFER_SDR_VIDEO, true},
                {MediaFormat.MIMETYPE_VIDEO_HEVC, "bbb_qcif_color_bt601_625_fr_gamma22_hevc.mp4",
                        MediaFormat.COLOR_RANGE_FULL, MediaFormat.COLOR_STANDARD_BT601_PAL,
                        /* MediaFormat.COLOR_TRANSFER_GAMMA2_2 */ 4, true},
                {MediaFormat.MIMETYPE_VIDEO_HEVC, "bbb_qcif_color_bt601_525_lr_gamma28_hevc.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT601_NTSC,
                        /* MediaFormat.COLOR_TRANSFER_GAMMA2_8 */ 5, true},
                {MediaFormat.MIMETYPE_VIDEO_HEVC, "bbb_qcif_color_bt709_lr_srgb_hevc.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT709,
                        /* MediaFormat.COLOR_TRANSFER_SRGB */ 2, true},
                {MediaFormat.MIMETYPE_VIDEO_HEVC, "bbb_qcif_color_unspcfd_hevc.mp4",
                        UNSPECIFIED, UNSPECIFIED, UNSPECIFIED, true},
                {MediaFormat.MIMETYPE_VIDEO_HEVC, "bbb_qcif_color_bt470m_linear_fr_hevc.mp4",
                        MediaFormat.COLOR_RANGE_FULL, /* MediaFormat.COLOR_STANDARD_BT470M */ 8,
                        MediaFormat.COLOR_TRANSFER_LINEAR, true},
                {MediaFormat.MIMETYPE_VIDEO_HEVC,
                        "bikes_qcif_color_bt2020_smpte2084_bt2020Ncl_lr_hevc.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT2020,
                        MediaFormat.COLOR_TRANSFER_ST2084, true},
                {MediaFormat.MIMETYPE_VIDEO_HEVC,
                        "bikes_qcif_color_bt2020_smpte2086Hlg_bt2020Ncl_fr_hevc.mp4",
                        MediaFormat.COLOR_RANGE_FULL, MediaFormat.COLOR_STANDARD_BT2020,
                        MediaFormat.COLOR_TRANSFER_HLG, true},

                // Mpeg2 clips
                {MediaFormat.MIMETYPE_VIDEO_MPEG2, "bbb_qcif_color_bt709_lr_sdr_mpeg2.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT709,
                        MediaFormat.COLOR_TRANSFER_SDR_VIDEO, true},
                {MediaFormat.MIMETYPE_VIDEO_MPEG2, "bbb_qcif_color_bt601_625_lr_gamma22_mpeg2.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT601_PAL,
                        /* MediaFormat.COLOR_TRANSFER_GAMMA2_2 */ 4, true},
                {MediaFormat.MIMETYPE_VIDEO_MPEG2, "bbb_qcif_color_bt601_525_lr_gamma28_mpeg2.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT601_NTSC,
                        /* MediaFormat.COLOR_TRANSFER_GAMMA2_8 */ 5, true},
                {MediaFormat.MIMETYPE_VIDEO_MPEG2, "bbb_qcif_color_bt709_lr_srgb_mpeg2.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT709,
                        /* MediaFormat.COLOR_TRANSFER_SRGB */ 2, true},
                {MediaFormat.MIMETYPE_VIDEO_MPEG2, "bbb_qcif_color_unspcfd_lr_mpeg2.mp4",
                        UNSPECIFIED, UNSPECIFIED, UNSPECIFIED, true},
                {MediaFormat.MIMETYPE_VIDEO_MPEG2, "bbb_qcif_color_bt470m_linear_lr_mpeg2.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, /* MediaFormat.COLOR_STANDARD_BT470M */ 8,
                        MediaFormat.COLOR_TRANSFER_LINEAR, true},

                // Mpeg4 clips
                {MediaFormat.MIMETYPE_VIDEO_MPEG4, "bbb_qcif_color_bt709_lr_sdr_mpeg4.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT709,
                        MediaFormat.COLOR_TRANSFER_SDR_VIDEO, false},
                {MediaFormat.MIMETYPE_VIDEO_MPEG4, "bbb_qcif_color_bt601_625_fr_gamma22_mpeg4.mp4",
                        MediaFormat.COLOR_RANGE_FULL, MediaFormat.COLOR_STANDARD_BT601_PAL,
                        /* MediaFormat.COLOR_TRANSFER_GAMMA2_2 */ 4, false},
                {MediaFormat.MIMETYPE_VIDEO_MPEG4, "bbb_qcif_color_bt601_525_lr_gamma28_mpeg4.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT601_NTSC,
                        /* MediaFormat.COLOR_TRANSFER_GAMMA2_8 */ 5, false},
                {MediaFormat.MIMETYPE_VIDEO_MPEG4, "bbb_qcif_color_bt709_lr_srgb_mpeg4.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT709,
                        /* MediaFormat.COLOR_TRANSFER_SRGB */ 2, false},
                {MediaFormat.MIMETYPE_VIDEO_MPEG4, "bbb_qcif_color_unspcfd_mpeg4.mp4",
                        UNSPECIFIED, UNSPECIFIED, UNSPECIFIED, false},
                {MediaFormat.MIMETYPE_VIDEO_MPEG4, "bbb_qcif_color_bt470m_linear_fr_mpeg4.mp4",
                        MediaFormat.COLOR_RANGE_FULL, /* MediaFormat.COLOR_STANDARD_BT470M */ 8,
                        MediaFormat.COLOR_TRANSFER_LINEAR, false},

                // Vp8 clips
                {MediaFormat.MIMETYPE_VIDEO_VP8, "bbb_qcif_color_bt709_lr_sdr_vp8.webm",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT709,
                        MediaFormat.COLOR_TRANSFER_SDR_VIDEO, false},
                {MediaFormat.MIMETYPE_VIDEO_VP8, "bbb_qcif_color_bt601_625_fr_gamma22_vp8.mkv",
                        MediaFormat.COLOR_RANGE_FULL, MediaFormat.COLOR_STANDARD_BT601_PAL,
                        /* MediaFormat.COLOR_TRANSFER_GAMMA2_2 */ 4, false},
                {MediaFormat.MIMETYPE_VIDEO_VP8, "bbb_qcif_color_bt601_525_lr_gamma28_vp8.mkv",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT601_NTSC,
                        /* MediaFormat.COLOR_TRANSFER_GAMMA2_8 */ 5, false},
                {MediaFormat.MIMETYPE_VIDEO_VP8, "bbb_qcif_color_bt709_lr_srgb_vp8.mkv",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT709,
                        /* MediaFormat.COLOR_TRANSFER_SRGB */ 2, false},
                {MediaFormat.MIMETYPE_VIDEO_VP8, "bbb_qcif_color_unspcfd_vp8.mkv",
                        UNSPECIFIED, UNSPECIFIED, UNSPECIFIED, false},
                {MediaFormat.MIMETYPE_VIDEO_VP8, "bbb_qcif_color_bt470m_linear_fr_vp8.mkv",
                        MediaFormat.COLOR_RANGE_FULL, /* MediaFormat.COLOR_STANDARD_BT470M */ 8,
                        MediaFormat.COLOR_TRANSFER_LINEAR, false},

                // Vp9 clips
                {MediaFormat.MIMETYPE_VIDEO_VP9, "bbb_qcif_color_bt709_lr_sdr_vp9.webm",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT709,
                        MediaFormat.COLOR_TRANSFER_SDR_VIDEO, false},
                {MediaFormat.MIMETYPE_VIDEO_VP9, "bbb_qcif_color_bt601_625_fr_gamma22_vp9.mkv",
                        MediaFormat.COLOR_RANGE_FULL, MediaFormat.COLOR_STANDARD_BT601_PAL,
                        /* MediaFormat.COLOR_TRANSFER_GAMMA2_2 */ 4, false},
                {MediaFormat.MIMETYPE_VIDEO_VP9, "bbb_qcif_color_smpte170_lr_gamma28_vp9.mkv",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT601_NTSC,
                        /* MediaFormat.COLOR_TRANSFER_GAMMA2_8 */ 5, false},
                {MediaFormat.MIMETYPE_VIDEO_VP9, "bbb_qcif_color_bt709_lr_srgb_vp9.mkv",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT709,
                        /* MediaFormat.COLOR_TRANSFER_SRGB */ 2, false},
                {MediaFormat.MIMETYPE_VIDEO_VP9, "bbb_qcif_color_unspcfd_vp9.mkv",
                        UNSPECIFIED, UNSPECIFIED, UNSPECIFIED, false},
                {MediaFormat.MIMETYPE_VIDEO_VP9, "bbb_qcif_color_bt470m_linear_fr_vp9.mkv",
                        MediaFormat.COLOR_RANGE_FULL, /* MediaFormat.COLOR_STANDARD_BT470M */ 8,
                        MediaFormat.COLOR_TRANSFER_LINEAR, false},
                {MediaFormat.MIMETYPE_VIDEO_VP9,
                        "bikes_qcif_color_bt2020_smpte2084_bt2020Ncl_lr_vp9.mkv",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT2020,
                        MediaFormat.COLOR_TRANSFER_ST2084, false},
                {MediaFormat.MIMETYPE_VIDEO_VP9,
                        "bikes_qcif_color_bt2020_smpte2086Hlg_bt2020Ncl_fr_vp9.mkv",
                        MediaFormat.COLOR_RANGE_FULL, MediaFormat.COLOR_STANDARD_BT2020,
                        MediaFormat.COLOR_TRANSFER_HLG, false},

                // AV1 clips
                {MediaFormat.MIMETYPE_VIDEO_AV1, "bbb_qcif_color_bt709_lr_sdr_av1.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT709,
                        MediaFormat.COLOR_TRANSFER_SDR_VIDEO, true},
                {MediaFormat.MIMETYPE_VIDEO_AV1, "bbb_qcif_color_bt601_625_fr_gamma22_av1.mp4",
                        MediaFormat.COLOR_RANGE_FULL, MediaFormat.COLOR_STANDARD_BT601_PAL,
                        /* MediaFormat.COLOR_TRANSFER_GAMMA2_2 */ 4, true},
                {MediaFormat.MIMETYPE_VIDEO_AV1, "bbb_qcif_color_bt601_525_lr_gamma28_av1.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT601_NTSC,
                        /* MediaFormat.COLOR_TRANSFER_GAMMA2_8 */ 5, true},
                {MediaFormat.MIMETYPE_VIDEO_AV1, "bbb_qcif_color_bt709_lr_srgb_av1.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT709,
                        /* MediaFormat.COLOR_TRANSFER_SRGB */ 2, true},
                {MediaFormat.MIMETYPE_VIDEO_AV1, "bbb_qcif_color_unspcfd_av1.mp4",
                        UNSPECIFIED, UNSPECIFIED, UNSPECIFIED, true},
                {MediaFormat.MIMETYPE_VIDEO_AV1, "bbb_qcif_color_bt470m_linear_fr_av1.mp4",
                        MediaFormat.COLOR_RANGE_FULL, /* MediaFormat.COLOR_STANDARD_BT470M */ 8,
                        MediaFormat.COLOR_TRANSFER_LINEAR, true},
                {MediaFormat.MIMETYPE_VIDEO_AV1,
                        "bikes_qcif_color_bt2020_smpte2084_bt2020Ncl_lr_av1.mp4",
                        MediaFormat.COLOR_RANGE_LIMITED, MediaFormat.COLOR_STANDARD_BT2020,
                        MediaFormat.COLOR_TRANSFER_ST2084, true},
                {MediaFormat.MIMETYPE_VIDEO_AV1,
                        "bikes_qcif_color_bt2020_smpte2086Hlg_bt2020Ncl_fr_av1.mp4",
                        MediaFormat.COLOR_RANGE_FULL, MediaFormat.COLOR_STANDARD_BT2020,
                        MediaFormat.COLOR_TRANSFER_HLG, true},
        });
        return prepareParamList(exhaustiveArgsList, isEncoder, needAudio, needVideo, false);
    }

    @Rule
    public ActivityTestRule<CodecTestActivity> mActivityRule =
            new ActivityTestRule<>(CodecTestActivity.class);

    @SmallTest
    @Test(timeout = PER_TEST_TIMEOUT_SMALL_TEST_MS)
    public void testColorAspects() throws IOException, InterruptedException {
        MediaFormat format = setUpSource(mInpPrefix, mTestFile);
        mExtractor.release();
        ArrayList<MediaFormat> formats = new ArrayList<>();
        formats.add(format);
        Assume.assumeTrue(areFormatsSupported(mCodecName, mMime, formats));
        CodecTestActivity activity = mActivityRule.getActivity();
        setUpSurface(activity);
        activity.setScreenParams(getWidth(format), getHeight(format), true);
        {
            validateColorAspects(mCodecName, mInpPrefix, mTestFile, mColorRange, mColorStandard,
                    mColorTransferCurve, false);
            // If color metadata can also be signalled via elementary stream, then verify if the
            // elementary stream contains color aspects as expected
            if (mCanIgnoreColorBox && mCheckESList.contains(mMime)) {
                validateColorAspects(mCodecName, mInpPrefix, mTestFile, mColorRange,
                        mColorStandard, mColorTransferCurve, true);
            }
        }
        tearDownSurface();
    }
}
