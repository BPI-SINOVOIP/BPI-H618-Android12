/*
 * Copyright (C) 2010 The Android Open Source Project
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

package android.media.cts;

import android.content.pm.PackageManager;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.Size;
import android.hardware.cts.helpers.CameraUtils;
import android.media.CamcorderProfile;
import android.media.EncoderProfiles;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaRecorder;
import android.test.AndroidTestCase;
import android.util.Log;

import java.util.Arrays;
import java.util.List;

@NonMediaMainlineTest
public class CamcorderProfileTest extends AndroidTestCase {

    private static final String TAG = "CamcorderProfileTest";
    private static final int MIN_HIGH_SPEED_FPS = 100;
    private static final Integer[] ALL_SUPPORTED_QUALITIES = {
        CamcorderProfile.QUALITY_LOW,
        CamcorderProfile.QUALITY_HIGH,
        CamcorderProfile.QUALITY_QCIF,
        CamcorderProfile.QUALITY_CIF,
        CamcorderProfile.QUALITY_480P,
        CamcorderProfile.QUALITY_720P,
        CamcorderProfile.QUALITY_1080P,
        CamcorderProfile.QUALITY_QVGA,
        CamcorderProfile.QUALITY_2160P,
        CamcorderProfile.QUALITY_VGA,
        CamcorderProfile.QUALITY_4KDCI,
        CamcorderProfile.QUALITY_QHD,
        CamcorderProfile.QUALITY_2K,
        CamcorderProfile.QUALITY_8KUHD,

        CamcorderProfile.QUALITY_TIME_LAPSE_LOW,
        CamcorderProfile.QUALITY_TIME_LAPSE_HIGH,
        CamcorderProfile.QUALITY_TIME_LAPSE_QCIF,
        CamcorderProfile.QUALITY_TIME_LAPSE_CIF,
        CamcorderProfile.QUALITY_TIME_LAPSE_480P,
        CamcorderProfile.QUALITY_TIME_LAPSE_720P,
        CamcorderProfile.QUALITY_TIME_LAPSE_1080P,
        CamcorderProfile.QUALITY_TIME_LAPSE_QVGA,
        CamcorderProfile.QUALITY_TIME_LAPSE_2160P,
        CamcorderProfile.QUALITY_TIME_LAPSE_VGA,
        CamcorderProfile.QUALITY_TIME_LAPSE_4KDCI,
        CamcorderProfile.QUALITY_TIME_LAPSE_QHD,
        CamcorderProfile.QUALITY_TIME_LAPSE_2K,
        CamcorderProfile.QUALITY_TIME_LAPSE_8KUHD,

        CamcorderProfile.QUALITY_HIGH_SPEED_LOW,
        CamcorderProfile.QUALITY_HIGH_SPEED_HIGH,
        CamcorderProfile.QUALITY_HIGH_SPEED_480P,
        CamcorderProfile.QUALITY_HIGH_SPEED_720P,
        CamcorderProfile.QUALITY_HIGH_SPEED_1080P,
        CamcorderProfile.QUALITY_HIGH_SPEED_2160P,
        CamcorderProfile.QUALITY_HIGH_SPEED_CIF,
        CamcorderProfile.QUALITY_HIGH_SPEED_VGA,
        CamcorderProfile.QUALITY_HIGH_SPEED_4KDCI,
    };
    private static final int LAST_QUALITY = CamcorderProfile.QUALITY_8KUHD;
    private static final int LAST_TIMELAPSE_QUALITY = CamcorderProfile.QUALITY_TIME_LAPSE_8KUHD;
    private static final int LAST_HIGH_SPEED_QUALITY = CamcorderProfile.QUALITY_HIGH_SPEED_4KDCI;
    private static final Integer[] UNKNOWN_QUALITIES = {
        LAST_QUALITY + 1, // Unknown normal profile quality
        LAST_TIMELAPSE_QUALITY + 1, // Unknown timelapse profile quality
        LAST_HIGH_SPEED_QUALITY + 1 // Unknown high speed timelapse profile quality
    };

    // Uses get without id if cameraId == -1 and get with id otherwise.
    private CamcorderProfile getWithOptionalId(int quality, int cameraId) {
        if (cameraId == -1) {
            return CamcorderProfile.get(quality);
        } else {
            return CamcorderProfile.get(cameraId, quality);
        }
    }

    private void checkProfile(CamcorderProfile profile, List<Size> videoSizes) {
        Log.v(TAG, String.format("profile: duration=%d, quality=%d, " +
            "fileFormat=%d, videoCodec=%d, videoBitRate=%d, videoFrameRate=%d, " +
            "videoFrameWidth=%d, videoFrameHeight=%d, audioCodec=%d, " +
            "audioBitRate=%d, audioSampleRate=%d, audioChannels=%d",
            profile.duration,
            profile.quality,
            profile.fileFormat,
            profile.videoCodec,
            profile.videoBitRate,
            profile.videoFrameRate,
            profile.videoFrameWidth,
            profile.videoFrameHeight,
            profile.audioCodec,
            profile.audioBitRate,
            profile.audioSampleRate,
            profile.audioChannels));
        assertTrue(profile.duration > 0);
        assertTrue(Arrays.asList(ALL_SUPPORTED_QUALITIES).contains(profile.quality));
        assertTrue(profile.videoBitRate > 0);
        assertTrue(profile.videoFrameRate > 0);
        assertTrue(profile.videoFrameWidth > 0);
        assertTrue(profile.videoFrameHeight > 0);
        assertTrue(profile.audioBitRate > 0);
        assertTrue(profile.audioSampleRate > 0);
        assertTrue(profile.audioChannels > 0);
        assertTrue(isSizeSupported(profile.videoFrameWidth,
                                   profile.videoFrameHeight,
                                   videoSizes));
    }

    private void checkAllProfiles(EncoderProfiles allProfiles, CamcorderProfile profile,
                                  List<Size> videoSizes) {
        Log.v(TAG, String.format("profile: duration=%d, quality=%d, " +
            "fileFormat=%d, videoCodec=%d, videoBitRate=%d, videoFrameRate=%d, " +
            "videoFrameWidth=%d, videoFrameHeight=%d, audioCodec=%d, " +
            "audioBitRate=%d, audioSampleRate=%d, audioChannels=%d",
            profile.duration,
            profile.quality,
            profile.fileFormat,
            profile.videoCodec,
            profile.videoBitRate,
            profile.videoFrameRate,
            profile.videoFrameWidth,
            profile.videoFrameHeight,
            profile.audioCodec,
            profile.audioBitRate,
            profile.audioSampleRate,
            profile.audioChannels));
        // generic fields must match the corresponding CamcorderProfile
        assertEquals(profile.duration, allProfiles.getDefaultDurationSeconds());
        assertEquals(profile.fileFormat, allProfiles.getRecommendedFileFormat());
        boolean first = true;
        for (EncoderProfiles.VideoProfile videoProfile : allProfiles.getVideoProfiles()) {
            if (first) {
                // the first profile must be the default profile which must match
                // the corresponding CamcorderProfile
                assertEquals(profile.videoCodec, videoProfile.getCodec());
                assertEquals(profile.videoBitRate, videoProfile.getBitrate());
                assertEquals(profile.videoFrameRate, videoProfile.getFrameRate());
                first = false;
            }
            // all profiles must be the same size
            assertEquals(profile.videoFrameWidth, videoProfile.getWidth());
            assertEquals(profile.videoFrameHeight, videoProfile.getHeight());
            assertTrue(videoProfile.getMediaType() != null);
            switch (videoProfile.getCodec()) {
              // don't validate profile for regular codecs as vendors may use vendor specific profile
            case MediaRecorder.VideoEncoder.H263:
                assertEquals(MediaFormat.MIMETYPE_VIDEO_H263, videoProfile.getMediaType());
                break;
            case MediaRecorder.VideoEncoder.H264:
                assertEquals(MediaFormat.MIMETYPE_VIDEO_AVC, videoProfile.getMediaType());
                break;
            case MediaRecorder.VideoEncoder.MPEG_4_SP:
                assertEquals(MediaFormat.MIMETYPE_VIDEO_MPEG4, videoProfile.getMediaType());
                break;
            case MediaRecorder.VideoEncoder.VP8:
                assertEquals(MediaFormat.MIMETYPE_VIDEO_VP8, videoProfile.getMediaType());
                break;
            case MediaRecorder.VideoEncoder.HEVC:
                  assertEquals(MediaFormat.MIMETYPE_VIDEO_HEVC, videoProfile.getMediaType());
                  break;
            }
            // Cannot validate profile as vendors may use vendor specific profile. Just read it.
            int codecProfile = videoProfile.getProfile();
        }
        first = true;
        for (EncoderProfiles.AudioProfile audioProfile : allProfiles.getAudioProfiles()) {
            if (first) {
                // the first profile must be the default profile which must match
                // the corresponding CamcorderProfile
                assertEquals(profile.audioCodec, audioProfile.getCodec());
                assertEquals(profile.audioBitRate, audioProfile.getBitrate());
                assertEquals(profile.audioSampleRate, audioProfile.getSampleRate());
                assertEquals(profile.audioChannels, audioProfile.getChannels());
                first = false;
            }
            assertTrue(audioProfile.getMediaType() != null);
            switch (audioProfile.getCodec()) {
            // don't validate profile for regular codecs as vendors may use vendor specific profile
            case MediaRecorder.AudioEncoder.AMR_NB:
                assertEquals(MediaFormat.MIMETYPE_AUDIO_AMR_NB, audioProfile.getMediaType());
                break;
            case MediaRecorder.AudioEncoder.AMR_WB:
                assertEquals(MediaFormat.MIMETYPE_AUDIO_AMR_WB, audioProfile.getMediaType());
                break;
            case MediaRecorder.AudioEncoder.AAC:
                assertEquals(MediaFormat.MIMETYPE_AUDIO_AAC, audioProfile.getMediaType());
                break;
            case MediaRecorder.AudioEncoder.HE_AAC:
                assertEquals(MediaFormat.MIMETYPE_AUDIO_AAC, audioProfile.getMediaType());
                assertEquals(MediaCodecInfo.CodecProfileLevel.AACObjectHE,
                             audioProfile.getProfile());
                break;
            case MediaRecorder.AudioEncoder.AAC_ELD:
                assertEquals(MediaFormat.MIMETYPE_AUDIO_AAC, audioProfile.getMediaType());
                assertEquals(MediaCodecInfo.CodecProfileLevel.AACObjectELD,
                             audioProfile.getProfile());
                break;
            case MediaRecorder.AudioEncoder.VORBIS:
                assertEquals(MediaFormat.MIMETYPE_AUDIO_VORBIS, audioProfile.getMediaType());
                break;
            case MediaRecorder.AudioEncoder.OPUS:
                assertEquals(MediaFormat.MIMETYPE_AUDIO_OPUS, audioProfile.getMediaType());
                break;
            default:
                // there may be some extended profiles we don't know about and that's OK
                break;
            }
        }
    }

    private void assertProfileEquals(CamcorderProfile expectedProfile,
            CamcorderProfile actualProfile) {
        assertEquals(expectedProfile.duration, actualProfile.duration);
        assertEquals(expectedProfile.fileFormat, actualProfile.fileFormat);
        assertEquals(expectedProfile.videoCodec, actualProfile.videoCodec);
        assertEquals(expectedProfile.videoBitRate, actualProfile.videoBitRate);
        assertEquals(expectedProfile.videoFrameRate, actualProfile.videoFrameRate);
        assertEquals(expectedProfile.videoFrameWidth, actualProfile.videoFrameWidth);
        assertEquals(expectedProfile.videoFrameHeight, actualProfile.videoFrameHeight);
        assertEquals(expectedProfile.audioCodec, actualProfile.audioCodec);
        assertEquals(expectedProfile.audioBitRate, actualProfile.audioBitRate);
        assertEquals(expectedProfile.audioSampleRate, actualProfile.audioSampleRate);
        assertEquals(expectedProfile.audioChannels, actualProfile.audioChannels);
    }

    private void checkSpecificProfileDimensions(CamcorderProfile profile, int quality) {
        Log.v(TAG, String.format("specific profile: quality=%d, width = %d, height = %d",
                    profile.quality, profile.videoFrameWidth, profile.videoFrameHeight));

        switch (quality) {
            case CamcorderProfile.QUALITY_QCIF:
            case CamcorderProfile.QUALITY_TIME_LAPSE_QCIF:
                assertEquals(176, profile.videoFrameWidth);
                assertEquals(144, profile.videoFrameHeight);
                break;

            case CamcorderProfile.QUALITY_CIF:
            case CamcorderProfile.QUALITY_TIME_LAPSE_CIF:
                assertEquals(352, profile.videoFrameWidth);
                assertEquals(288, profile.videoFrameHeight);
                break;

            case CamcorderProfile.QUALITY_480P:
            case CamcorderProfile.QUALITY_TIME_LAPSE_480P:
                assertTrue(720 == profile.videoFrameWidth ||  // SMPTE 293M/ITU-R Rec. 601
                           640 == profile.videoFrameWidth ||  // ATSC/NTSC (square sampling)
                           704 == profile.videoFrameWidth);   // ATSC/NTSC (non-square sampling)
                assertEquals(480, profile.videoFrameHeight);
                break;

            case CamcorderProfile.QUALITY_720P:
            case CamcorderProfile.QUALITY_TIME_LAPSE_720P:
                assertEquals(1280, profile.videoFrameWidth);
                assertEquals(720, profile.videoFrameHeight);
                break;

            case CamcorderProfile.QUALITY_1080P:
            case CamcorderProfile.QUALITY_TIME_LAPSE_1080P:
                // 1080p could be either 1920x1088 or 1920x1080.
                assertEquals(1920, profile.videoFrameWidth);
                assertTrue(1088 == profile.videoFrameHeight ||
                           1080 == profile.videoFrameHeight);
                break;
            case CamcorderProfile.QUALITY_2K:
            case CamcorderProfile.QUALITY_TIME_LAPSE_2K:
                // 2K could be either 2048x1088 or 2048x1080
                assertEquals(2048, profile.videoFrameWidth);
                assertTrue(1088 == profile.videoFrameHeight ||
                           1080 == profile.videoFrameHeight);
                break;
            case CamcorderProfile.QUALITY_QHD:
            case CamcorderProfile.QUALITY_TIME_LAPSE_QHD:
                assertEquals(2560, profile.videoFrameWidth);
                assertEquals(1440, profile.videoFrameHeight);
                break;
            case CamcorderProfile.QUALITY_2160P:
            case CamcorderProfile.QUALITY_TIME_LAPSE_2160P:
                assertEquals(3840, profile.videoFrameWidth);
                assertEquals(2160, profile.videoFrameHeight);
                break;
            case CamcorderProfile.QUALITY_HIGH_SPEED_480P:
                assertTrue(720 == profile.videoFrameWidth ||  // SMPTE 293M/ITU-R Rec. 601
                640 == profile.videoFrameWidth ||  // ATSC/NTSC (square sampling)
                704 == profile.videoFrameWidth);   // ATSC/NTSC (non-square sampling)
                assertEquals(480, profile.videoFrameHeight);
                assertTrue(profile.videoFrameRate >= MIN_HIGH_SPEED_FPS);
                break;
            case CamcorderProfile.QUALITY_HIGH_SPEED_720P:
                assertEquals(1280, profile.videoFrameWidth);
                assertEquals(720, profile.videoFrameHeight);
                assertTrue(profile.videoFrameRate >= MIN_HIGH_SPEED_FPS);
                break;
            case CamcorderProfile.QUALITY_HIGH_SPEED_1080P:
                // 1080p could be either 1920x1088 or 1920x1080.
                assertEquals(1920, profile.videoFrameWidth);
                assertTrue(1088 == profile.videoFrameHeight ||
                           1080 == profile.videoFrameHeight);
                assertTrue(profile.videoFrameRate >= MIN_HIGH_SPEED_FPS);
                break;
        }
    }

    // Checks if the existing specific profiles have the correct dimensions.
    // Also checks that the mimimum quality specific profile matches the low profile and
    // similarly that the maximum quality specific profile matches the high profile.
    private void checkSpecificProfiles(
            int cameraId,
            CamcorderProfile low,
            CamcorderProfile high,
            int[] specificQualities,
            List<Size> videoSizes) {

        // For high speed levels, low and high quality are optional,skip the test if
        // they are missing.
        if (low == null && high == null) {
            // No profile should be available if low and high are unavailable.
            for (int quality : specificQualities) {
                assertFalse(CamcorderProfile.hasProfile(cameraId, quality));
            }
            return;
        }

        CamcorderProfile minProfile = null;
        CamcorderProfile maxProfile = null;

        for (int i = 0; i < specificQualities.length; i++) {
            int quality = specificQualities[i];
            if ((cameraId != -1 && CamcorderProfile.hasProfile(cameraId, quality)) ||
                (cameraId == -1 && CamcorderProfile.hasProfile(quality))) {
                CamcorderProfile profile = getWithOptionalId(quality, cameraId);
                checkSpecificProfileDimensions(profile, quality);

                assertTrue(isSizeSupported(profile.videoFrameWidth,
                                           profile.videoFrameHeight,
                                           videoSizes));

                if (minProfile == null) {
                    minProfile = profile;
                }
                maxProfile = profile;
            }
        }

        assertNotNull(minProfile);
        assertNotNull(maxProfile);

        Log.v(TAG, String.format("min profile: quality=%d, width = %d, height = %d",
                    minProfile.quality, minProfile.videoFrameWidth, minProfile.videoFrameHeight));
        Log.v(TAG, String.format("max profile: quality=%d, width = %d, height = %d",
                    maxProfile.quality, maxProfile.videoFrameWidth, maxProfile.videoFrameHeight));

        assertProfileEquals(low, minProfile);
        assertProfileEquals(high, maxProfile);

    }

    private void checkGet(int cameraId) {
        Log.v(TAG, (cameraId == -1)
                   ? "Checking get without id"
                   : "Checking get with id = " + cameraId);

        final List<Size> videoSizes = getSupportedVideoSizes(cameraId);

        /**
         * Check all possible supported profiles: get profile should work, and the profile
         * should be sane. Note that, timelapse and high speed video sizes may not be listed
         * as supported video sizes from camera, skip the size check.
         */
        for (Integer quality : ALL_SUPPORTED_QUALITIES) {
            if (CamcorderProfile.hasProfile(cameraId, quality) || isProfileMandatory(quality)) {
                List<Size> videoSizesToCheck = null;
                if (quality >= CamcorderProfile.QUALITY_LOW &&
                                quality <= CamcorderProfile.QUALITY_2K) {
                    videoSizesToCheck = videoSizes;
                }
                CamcorderProfile profile = getWithOptionalId(quality, cameraId);
                checkProfile(profile, videoSizesToCheck);
                if (cameraId >= 0) {
                    EncoderProfiles allProfiles =
                        CamcorderProfile.getAll(String.valueOf(cameraId), quality);
                    checkAllProfiles(allProfiles, profile, videoSizesToCheck);
                }
            }
        }

        /**
         * Check unknown profiles: hasProfile() should return false.
         */
        for (Integer quality : UNKNOWN_QUALITIES) {
            assertFalse("Unknown profile quality " + quality + " shouldn't be supported by camera "
                    + cameraId, CamcorderProfile.hasProfile(cameraId, quality));
        }

        // High speed low and high profile are optional,
        // but they should be both present or missing.
        CamcorderProfile lowHighSpeedProfile = null;
        CamcorderProfile highHighSpeedProfile = null;
        if (CamcorderProfile.hasProfile(cameraId, CamcorderProfile.QUALITY_HIGH_SPEED_LOW)) {
            lowHighSpeedProfile =
                    getWithOptionalId(CamcorderProfile.QUALITY_HIGH_SPEED_LOW, cameraId);
        }
        if (CamcorderProfile.hasProfile(cameraId, CamcorderProfile.QUALITY_HIGH_SPEED_HIGH)) {
            highHighSpeedProfile =
                    getWithOptionalId(CamcorderProfile.QUALITY_HIGH_SPEED_HIGH, cameraId);
        }
        if (lowHighSpeedProfile != null) {
            assertNotNull("high speed high quality profile should be supported if low" +
                    " is supported ",
                    highHighSpeedProfile);
            checkProfile(lowHighSpeedProfile, null);
            checkProfile(highHighSpeedProfile, null);
        } else {
            assertNull("high speed high quality profile shouldn't be supported if " +
                    "low is unsupported ", highHighSpeedProfile);
        }

        int[] specificProfileQualities = {CamcorderProfile.QUALITY_QCIF,
                                          CamcorderProfile.QUALITY_QVGA,
                                          CamcorderProfile.QUALITY_CIF,
                                          CamcorderProfile.QUALITY_480P,
                                          CamcorderProfile.QUALITY_720P,
                                          CamcorderProfile.QUALITY_1080P,
                                          CamcorderProfile.QUALITY_2K,
                                          CamcorderProfile.QUALITY_QHD,
                                          CamcorderProfile.QUALITY_2160P};

        int[] specificTimeLapseProfileQualities = {CamcorderProfile.QUALITY_TIME_LAPSE_QCIF,
                                                   CamcorderProfile.QUALITY_TIME_LAPSE_QVGA,
                                                   CamcorderProfile.QUALITY_TIME_LAPSE_CIF,
                                                   CamcorderProfile.QUALITY_TIME_LAPSE_480P,
                                                   CamcorderProfile.QUALITY_TIME_LAPSE_720P,
                                                   CamcorderProfile.QUALITY_TIME_LAPSE_1080P,
                                                   CamcorderProfile.QUALITY_TIME_LAPSE_2K,
                                                   CamcorderProfile.QUALITY_TIME_LAPSE_QHD,
                                                   CamcorderProfile.QUALITY_TIME_LAPSE_2160P};

        int[] specificHighSpeedProfileQualities = {CamcorderProfile.QUALITY_HIGH_SPEED_480P,
                                                   CamcorderProfile.QUALITY_HIGH_SPEED_720P,
                                                   CamcorderProfile.QUALITY_HIGH_SPEED_1080P,
                                                   CamcorderProfile.QUALITY_HIGH_SPEED_2160P};

        CamcorderProfile lowProfile =
                getWithOptionalId(CamcorderProfile.QUALITY_LOW, cameraId);
        CamcorderProfile highProfile =
                getWithOptionalId(CamcorderProfile.QUALITY_HIGH, cameraId);
        CamcorderProfile lowTimeLapseProfile =
                getWithOptionalId(CamcorderProfile.QUALITY_TIME_LAPSE_LOW, cameraId);
        CamcorderProfile highTimeLapseProfile =
                getWithOptionalId(CamcorderProfile.QUALITY_TIME_LAPSE_HIGH, cameraId);
        checkSpecificProfiles(cameraId, lowProfile, highProfile,
                specificProfileQualities, videoSizes);
        checkSpecificProfiles(cameraId, lowTimeLapseProfile, highTimeLapseProfile,
                specificTimeLapseProfileQualities, null);
        checkSpecificProfiles(cameraId, lowHighSpeedProfile, highHighSpeedProfile,
                specificHighSpeedProfileQualities, null);
    }

    public void testGet() {
        /*
         * Device may not have rear camera for checkGet(-1).
         * Checking PackageManager.FEATURE_CAMERA is included or not to decide the flow.
         * Continue if the feature is included.
         * Otherwise, exit test.
         */
        PackageManager pm = mContext.getPackageManager();
        if (!pm.hasSystemFeature(PackageManager.FEATURE_CAMERA)) {
            return;
        }
        checkGet(-1);
    }

    public void testGetWithId() {
        int nCamera = Camera.getNumberOfCameras();
        for (int cameraId = 0; cameraId < nCamera; cameraId++) {
            boolean isExternal = false;
            try {
                isExternal = CameraUtils.isExternal(mContext, cameraId);
            } catch (Exception e) {
                Log.e(TAG, "Unable to query external camera: " + e);
            }

            if (!isExternal) {
                checkGet(cameraId);
            }
        }
    }

    private List<Size> getSupportedVideoSizes(int cameraId) {
        Camera camera = (cameraId == -1)? Camera.open(): Camera.open(cameraId);
        Parameters parameters = camera.getParameters();
        List<Size> videoSizes = parameters.getSupportedVideoSizes();
        if (videoSizes == null) {
            videoSizes = parameters.getSupportedPreviewSizes();
            assertNotNull(videoSizes);
        }
        camera.release();
        return videoSizes;
    }

    private boolean isSizeSupported(int width, int height, List<Size> sizes) {
        if (sizes == null) return true;

        for (Size size: sizes) {
            if (size.width == width && size.height == height) {
                return true;
            }
        }
        Log.e(TAG, "Size (" + width + "x" + height + ") is not supported");
        return false;
    }

    private boolean isProfileMandatory(int quality) {
        return (quality == CamcorderProfile.QUALITY_LOW) ||
                (quality == CamcorderProfile.QUALITY_HIGH) ||
                (quality == CamcorderProfile.QUALITY_TIME_LAPSE_LOW) ||
                (quality == CamcorderProfile.QUALITY_TIME_LAPSE_HIGH);
    }
}
