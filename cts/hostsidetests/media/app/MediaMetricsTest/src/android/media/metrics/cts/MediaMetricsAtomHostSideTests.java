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

package android.media.metrics.cts;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;
import android.media.metrics.LogSessionId;
import android.media.metrics.MediaMetricsManager;
import android.media.metrics.NetworkEvent;
import android.media.metrics.PlaybackErrorEvent;
import android.media.metrics.PlaybackMetrics;
import android.media.metrics.PlaybackSession;
import android.media.metrics.PlaybackStateEvent;
import android.media.metrics.RecordingSession;
import android.media.metrics.TrackChangeEvent;
import android.os.Bundle;
import android.provider.DeviceConfig;

import androidx.test.InstrumentationRegistry;

import com.android.compatibility.common.util.SystemUtil;

import java.lang.InterruptedException;

import org.junit.Test;

public class MediaMetricsAtomHostSideTests {
    private static final String MEDIA_METRICS_MODE = "media_metrics_mode";
    private static final String PLAYER_METRICS_APP_ALLOWLIST = "player_metrics_app_allowlist";
    private static final String PLAYER_METRICS_APP_BLOCKLIST = "player_metrics_app_blocklist";
    private static final String PLAYER_METRICS_PER_APP_ATTRIBUTION_ALLOWLIST =
            "player_metrics_per_app_attribution_allowlist";
    private static final String PLAYER_METRICS_PER_APP_ATTRIBUTION_BLOCKLIST =
            "player_metrics_per_app_attribution_blocklist";
    private static final String MEDIA_METRICS_MODE_ON = "1";
    private static final String MEDIA_METRICS_MODE_BLOCKLIST = "2";
    private static final String MEDIA_METRICS_MODE_ALLOWLIST = "3";
    private static final String TEST_PKG = "android.media.metrics.cts";
    private static final int DEVICE_PROPERTY_PROPAGATION_DELAY_MICROSECONDS = 1000;

    static {
        System.loadLibrary("CtsMediaMetricsHostTestAppJni");
    }

    private static void resetProperties() throws InterruptedException {
        // ensure the event/metric is sent.
        Thread.sleep(1000);
        // see DeviceConfig#resetToDefaults(int, String).
        SystemUtil.runWithShellPermissionIdentity(() -> {
            DeviceConfig.setProperties(
                    new DeviceConfig.Properties.Builder(DeviceConfig.NAMESPACE_MEDIA).build());
        });
        // ensure the property is set.
        Thread.sleep(DEVICE_PROPERTY_PROPAGATION_DELAY_MICROSECONDS);
    }

    private static void turnOnForTesting() throws InterruptedException {
        SystemUtil.runWithShellPermissionIdentity(() -> {
            DeviceConfig.setProperty(
                    DeviceConfig.NAMESPACE_MEDIA,
                    MEDIA_METRICS_MODE,
                    MEDIA_METRICS_MODE_ON,
                    /*makeDefault=*/ false);
        });
        // ensure the property is set.
        Thread.sleep(DEVICE_PROPERTY_PROPAGATION_DELAY_MICROSECONDS);
    }


    @Test
    public void testPlaybackStateEvent_default() throws Exception {
        turnOnForTesting();
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        PlaybackStateEvent e =
                new PlaybackStateEvent.Builder().build();
        s.reportPlaybackStateEvent(e);
        resetProperties();
    }


    @Test
    public void testPlaybackStateEvent() throws Exception {
        turnOnForTesting();
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        PlaybackStateEvent e =
                new PlaybackStateEvent.Builder()
                        .setTimeSinceCreatedMillis(1763L)
                        .setState(PlaybackStateEvent.STATE_JOINING_FOREGROUND)
                        .setMetricsBundle(new Bundle())
                        .build();
        s.reportPlaybackStateEvent(e);
        resetProperties();
    }

    @Test
    public void testPlaybackErrorEvent_default() throws Exception {
        turnOnForTesting();
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        PlaybackErrorEvent e =
                new PlaybackErrorEvent.Builder()
                        .setException(new Exception(""))
                        .build();
        s.reportPlaybackErrorEvent(e);
        resetProperties();
    }

    @Test
    public void testPlaybackErrorEvent() throws Exception {
        turnOnForTesting();
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        PlaybackErrorEvent e =
                new PlaybackErrorEvent.Builder()
                        .setTimeSinceCreatedMillis(17630000L)
                        .setErrorCode(PlaybackErrorEvent.ERROR_RUNTIME)
                        .setSubErrorCode(378)
                        .setException(new Exception("test exception"))
                        .setMetricsBundle(new Bundle())
                        .build();
        s.reportPlaybackErrorEvent(e);
        resetProperties();
    }

    @Test
    public void testTrackChangeEvent_default() throws Exception {
        turnOnForTesting();
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        TrackChangeEvent e =
                new TrackChangeEvent.Builder(TrackChangeEvent.TRACK_TYPE_AUDIO).build();
        s.reportTrackChangeEvent(e);
        resetProperties();
    }

    @Test
    public void testTrackChangeEvent_text() throws Exception {
        turnOnForTesting();
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        TrackChangeEvent e =
                new TrackChangeEvent.Builder(TrackChangeEvent.TRACK_TYPE_TEXT)
                        .setTimeSinceCreatedMillis(37278L)
                        .setTrackState(TrackChangeEvent.TRACK_STATE_ON)
                        .setTrackChangeReason(TrackChangeEvent.TRACK_CHANGE_REASON_MANUAL)
                        .setContainerMimeType("text/foo")
                        .setSampleMimeType("text/plain")
                        .setCodecName("codec_1")
                        .setBitrate(1024)
                        .setLanguage("EN")
                        .setLanguageRegion("US")
                        .build();
        s.reportTrackChangeEvent(e);
        resetProperties();
    }

    @Test
    public void testTrackChangeEvent_audio() throws Exception {
        turnOnForTesting();
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        TrackChangeEvent e =
                new TrackChangeEvent.Builder(TrackChangeEvent.TRACK_TYPE_AUDIO)
                        .setTimeSinceCreatedMillis(37278L)
                        .setTrackState(TrackChangeEvent.TRACK_STATE_OFF)
                        .setTrackChangeReason(TrackChangeEvent.TRACK_CHANGE_REASON_INITIAL)
                        .setContainerMimeType("audio/foo")
                        .setSampleMimeType("audio/avc")
                        .setCodecName("codec_2")
                        .setBitrate(1025)
                        .setLanguage("EN")
                        .setLanguageRegion("US")
                        .setAudioSampleRate(89)
                        .setChannelCount(3)
                        .build();
        s.reportTrackChangeEvent(e);
        resetProperties();
    }

    @Test
    public void testTrackChangeEvent_video() throws Exception {
        turnOnForTesting();
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        TrackChangeEvent e =
                new TrackChangeEvent.Builder(TrackChangeEvent.TRACK_TYPE_VIDEO)
                        .setTimeSinceCreatedMillis(37278L)
                        .setTrackState(TrackChangeEvent.TRACK_STATE_OFF)
                        .setTrackChangeReason(TrackChangeEvent.TRACK_CHANGE_REASON_INITIAL)
                        .setContainerMimeType("video/foo")
                        .setSampleMimeType("video/mpeg")
                        .setCodecName("codec_3")
                        .setBitrate(1025)
                        .setLanguage("EN")
                        .setLanguageRegion("US")
                        .setHeight(1080)
                        .setWidth(1440)
                        .setVideoFrameRate(60)
                        .setMetricsBundle(new Bundle())
                        .build();
        s.reportTrackChangeEvent(e);
        resetProperties();
    }

    @Test
    public void testNetworkEvent_default() throws Exception {
        turnOnForTesting();
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        NetworkEvent e =
                new NetworkEvent.Builder().build();
        s.reportNetworkEvent(e);
        resetProperties();
    }

    @Test
    public void testNetworkEvent() throws Exception {
        turnOnForTesting();
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        NetworkEvent e =
                new NetworkEvent.Builder()
                        .setTimeSinceCreatedMillis(3032L)
                        .setNetworkType(NetworkEvent.NETWORK_TYPE_WIFI)
                        .setMetricsBundle(new Bundle())
                        .build();
        s.reportNetworkEvent(e);
        resetProperties();
    }

    @Test
    public void testPlaybackMetrics_default() throws Exception {
        turnOnForTesting();
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        PlaybackMetrics e =
                new PlaybackMetrics.Builder().build();
        s.reportPlaybackMetrics(e);
        resetProperties();
    }

    @Test
    public void testPlaybackMetrics() throws Exception {
        turnOnForTesting();
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        PlaybackMetrics e =
                new PlaybackMetrics.Builder()
                        .setMediaDurationMillis(233L)
                        .setStreamSource(PlaybackMetrics.STREAM_SOURCE_NETWORK)
                        .setStreamType(PlaybackMetrics.STREAM_TYPE_OTHER)
                        .setPlaybackType(PlaybackMetrics.PLAYBACK_TYPE_LIVE)
                        .setDrmType(PlaybackMetrics.DRM_TYPE_WIDEVINE_L1)
                        .setContentType(PlaybackMetrics.CONTENT_TYPE_MAIN)
                        .setPlayerName("ExoPlayer")
                        .setPlayerVersion("1.01x")
                        .setVideoFramesPlayed(1024)
                        .setVideoFramesDropped(32)
                        .setAudioUnderrunCount(22)
                        .setNetworkBytesRead(102400)
                        .setLocalBytesRead(2000)
                        .setNetworkTransferDurationMillis(6000)
                        .setDrmSessionId(new byte[] {2, 3, 3, 10})
                        .setMetricsBundle(new Bundle())
                        .addExperimentId(123)
                        .build();
        s.reportPlaybackMetrics(e);
        resetProperties();
    }

    @Test
    public void testSessionId() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);

        try(PlaybackSession s = manager.createPlaybackSession()) {
            LogSessionId idObj = s.getSessionId();
            assertThat(idObj).isNotEqualTo(null);
            assertThat(idObj.getStringId().length()).isGreaterThan(0);
        }
    }

    @Test
    public void testRecordingSession() throws Exception {
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);

        try(RecordingSession s = manager.createRecordingSession()) {
            assertThat(s).isNotEqualTo(null);
            LogSessionId idObj = s.getSessionId();
            assertThat(idObj).isNotEqualTo(null);
            assertThat(idObj.getStringId().length()).isGreaterThan(0);
        }
    }

    @Test
    public void testAppBlocklist() throws Exception {
        SystemUtil.runWithShellPermissionIdentity(() -> {
            DeviceConfig.setProperties(
                    new DeviceConfig.Properties.Builder(DeviceConfig.NAMESPACE_MEDIA)
                            .setString(MEDIA_METRICS_MODE, MEDIA_METRICS_MODE_BLOCKLIST)
                            .setString(PLAYER_METRICS_PER_APP_ATTRIBUTION_BLOCKLIST, "")
                            .setString(PLAYER_METRICS_APP_BLOCKLIST, TEST_PKG)
                            .build());
        });
        Thread.sleep(DEVICE_PROPERTY_PROPAGATION_DELAY_MICROSECONDS);
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        PlaybackStateEvent e =
                new PlaybackStateEvent.Builder()
                        .setTimeSinceCreatedMillis(1763L)
                        .setState(PlaybackStateEvent.STATE_JOINING_FOREGROUND)
                        .setMetricsBundle(new Bundle())
                        .build();
        s.reportPlaybackStateEvent(e);
        resetProperties();
    }

    @Test
    public void testAttributionBlocklist() throws Exception {
        SystemUtil.runWithShellPermissionIdentity(() -> {
            DeviceConfig.setProperties(
                    new DeviceConfig.Properties.Builder(DeviceConfig.NAMESPACE_MEDIA)
                            .setString(MEDIA_METRICS_MODE, MEDIA_METRICS_MODE_BLOCKLIST)
                            .setString(PLAYER_METRICS_PER_APP_ATTRIBUTION_BLOCKLIST, TEST_PKG)
                            .setString(PLAYER_METRICS_APP_BLOCKLIST, "")
                            .build());
        });
        Thread.sleep(DEVICE_PROPERTY_PROPAGATION_DELAY_MICROSECONDS);
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        PlaybackMetrics e =
                new PlaybackMetrics.Builder()
                        .setMediaDurationMillis(233L)
                        .setStreamSource(PlaybackMetrics.STREAM_SOURCE_NETWORK)
                        .setStreamType(PlaybackMetrics.STREAM_TYPE_OTHER)
                        .setPlaybackType(PlaybackMetrics.PLAYBACK_TYPE_LIVE)
                        .setDrmType(PlaybackMetrics.DRM_TYPE_WIDEVINE_L1)
                        .setContentType(PlaybackMetrics.CONTENT_TYPE_MAIN)
                        .setPlayerName("ExoPlayer")
                        .setPlayerVersion("1.01x")
                        .setVideoFramesPlayed(1024)
                        .setVideoFramesDropped(32)
                        .setAudioUnderrunCount(22)
                        .setNetworkBytesRead(102400)
                        .setLocalBytesRead(2000)
                        .setNetworkTransferDurationMillis(6000)
                        .setDrmSessionId(new byte[] {2, 3, 3, 10})
                        .setMetricsBundle(new Bundle())
                        .addExperimentId(123)
                        .build();
        s.reportPlaybackMetrics(e);
        resetProperties();
    }

    @Test
    public void testAppAllowlist() throws Exception {
        SystemUtil.runWithShellPermissionIdentity(() -> {
            DeviceConfig.setProperties(
                    new DeviceConfig.Properties.Builder(DeviceConfig.NAMESPACE_MEDIA)
                            .setString(MEDIA_METRICS_MODE, MEDIA_METRICS_MODE_ALLOWLIST)
                            .setString(PLAYER_METRICS_PER_APP_ATTRIBUTION_ALLOWLIST, "")
                            .setString(PLAYER_METRICS_APP_ALLOWLIST, TEST_PKG)
                            .build());
        });
        Thread.sleep(DEVICE_PROPERTY_PROPAGATION_DELAY_MICROSECONDS);
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        PlaybackStateEvent e =
                new PlaybackStateEvent.Builder()
                        .setTimeSinceCreatedMillis(1763L)
                        .setState(PlaybackStateEvent.STATE_JOINING_FOREGROUND)
                        .setMetricsBundle(new Bundle())
                        .build();
        s.reportPlaybackStateEvent(e);
        resetProperties();
    }

    @Test
    public void testAttributionAllowlist() throws Exception {
        SystemUtil.runWithShellPermissionIdentity(() -> {
            DeviceConfig.setProperties(
                    new DeviceConfig.Properties.Builder(DeviceConfig.NAMESPACE_MEDIA)
                            .setString(MEDIA_METRICS_MODE, MEDIA_METRICS_MODE_ALLOWLIST)
                            .setString(PLAYER_METRICS_PER_APP_ATTRIBUTION_ALLOWLIST, TEST_PKG)
                            .setString(PLAYER_METRICS_APP_ALLOWLIST, "")
                            .build());
        });
        Thread.sleep(DEVICE_PROPERTY_PROPAGATION_DELAY_MICROSECONDS);
        Context context = InstrumentationRegistry.getContext();
        MediaMetricsManager manager = context.getSystemService(MediaMetricsManager.class);
        PlaybackSession s = manager.createPlaybackSession();
        PlaybackMetrics e =
                new PlaybackMetrics.Builder()
                        .setMediaDurationMillis(233L)
                        .setStreamSource(PlaybackMetrics.STREAM_SOURCE_NETWORK)
                        .setStreamType(PlaybackMetrics.STREAM_TYPE_OTHER)
                        .setPlaybackType(PlaybackMetrics.PLAYBACK_TYPE_LIVE)
                        .setDrmType(PlaybackMetrics.DRM_TYPE_WIDEVINE_L1)
                        .setContentType(PlaybackMetrics.CONTENT_TYPE_MAIN)
                        .setPlayerName("ExoPlayer")
                        .setPlayerVersion("1.01x")
                        .setVideoFramesPlayed(1024)
                        .setVideoFramesDropped(32)
                        .setAudioUnderrunCount(22)
                        .setNetworkBytesRead(102400)
                        .setLocalBytesRead(2000)
                        .setNetworkTransferDurationMillis(6000)
                        .setDrmSessionId(new byte[] {2, 3, 3, 10})
                        .setMetricsBundle(new Bundle())
                        .addExperimentId(123)
                        .build();
        s.reportPlaybackMetrics(e);
        resetProperties();
    }

    /**
     * Open aaudio mmap output stream and then close
     */
    @Test
    public native void testAAudioMmapOutputStream();

    /**
     * Open aaudio mmap input stream and then close
     */
    @Test
    public native void testAAudioMmapInputStream();

    /**
     * Open aaudio legacy output stream and then close
     */
    @Test
    public native void testAAudioLegacyOutputStream();

    /**
     * Open aaudio legacy input stream and then close
     */
    @Test
    public native void testAAudioLegacyInputStream();
}
