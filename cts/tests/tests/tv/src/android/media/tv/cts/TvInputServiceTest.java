/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.media.tv.cts;

import static androidx.test.ext.truth.view.MotionEventSubject.assertThat;
import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.app.Instrumentation;
import android.content.Context;
import android.content.pm.PackageManager;
import android.media.PlaybackParams;
import android.media.tv.TvContentRating;
import android.media.tv.TvContract;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.media.tv.TvRecordingClient;
import android.media.tv.TvTrackInfo;
import android.media.tv.TvView;
import android.media.tv.cts.TvInputServiceTest.CountingTvInputService.CountingRecordingSession;
import android.media.tv.cts.TvInputServiceTest.CountingTvInputService.CountingSession;
import android.net.Uri;
import android.os.Bundle;
import android.os.IBinder;
import android.os.SystemClock;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.View;
import android.widget.LinearLayout;

import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.compatibility.common.util.PollingCheck;
import com.android.compatibility.common.util.RequiredFeatureRule;

import com.google.common.truth.Truth;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.function.Consumer;
import java.util.function.Function;

/**
 * Test {@link android.media.tv.TvInputService}.
 */
@RunWith(AndroidJUnit4.class)
public class TvInputServiceTest {

    private static final String TAG = "TvInputServiceTest";

    @Rule
    public RequiredFeatureRule featureRule = new RequiredFeatureRule(
            PackageManager.FEATURE_LIVE_TV);

    @Rule
    public ActivityScenarioRule<TvViewStubActivity> activityRule =
            new ActivityScenarioRule(TvViewStubActivity.class);


    private static final Uri CHANNEL_0 = TvContract.buildChannelUri(0);
    /** The maximum time to wait for an operation. */
    private static final long TIME_OUT = 5000L;
    private static final TvTrackInfo TEST_TV_TRACK =
            new TvTrackInfo.Builder(TvTrackInfo.TYPE_VIDEO, "testTrackId")
                    .setVideoWidth(1920)
                    .setVideoHeight(1080)
                    .setLanguage("und")
                    .build();

    private TvRecordingClient mTvRecordingClient;
    private Instrumentation mInstrumentation;
    private TvInputManager mManager;
    private TvInputInfo mStubInfo;
    private TvInputInfo mFaultyStubInfo;
    private final StubCallback mCallback = new StubCallback();
    private final StubTimeShiftPositionCallback mTimeShiftPositionCallback =
            new StubTimeShiftPositionCallback();
    private final StubRecordingCallback mRecordingCallback = new StubRecordingCallback();

    private static class StubCallback extends TvView.TvInputCallback {
        private int mChannelRetunedCount;
        private int mVideoAvailableCount;
        private int mVideoUnavailableCount;
        private int mTrackSelectedCount;
        private int mTrackChangedCount;
        private int mVideoSizeChanged;
        private int mContentAllowedCount;
        private int mContentBlockedCount;
        private int mTimeShiftStatusChangedCount;

        private Uri mChannelRetunedUri;
        private Integer mVideoUnavailableReason;
        private Integer mTrackSelectedType;
        private String mTrackSelectedTrackId;
        private List<TvTrackInfo> mTracksChangedTrackList;
        private TvContentRating mContentBlockedRating;
        private Integer mTimeShiftStatusChangedStatus;

        @Override
        public void onChannelRetuned(String inputId, Uri channelUri) {
            mChannelRetunedCount++;
            mChannelRetunedUri = channelUri;
        }

        @Override
        public void onVideoAvailable(String inputId) {
            mVideoAvailableCount++;
        }

        @Override
        public void onVideoUnavailable(String inputId, int reason) {
            mVideoUnavailableCount++;
            mVideoUnavailableReason = reason;
        }

        @Override
        public void onTrackSelected(String inputId, int type, String trackId) {
            mTrackSelectedCount++;
            mTrackSelectedType = type;
            mTrackSelectedTrackId = trackId;
        }

        @Override
        public void onTracksChanged(String inputId, List<TvTrackInfo> trackList) {
            mTrackChangedCount++;
            mTracksChangedTrackList = trackList;
        }

        @Override
        public void onVideoSizeChanged(String inputId, int width, int height) {
            mVideoSizeChanged++;
        }

        @Override
        public void onContentAllowed(String inputId) {
            mContentAllowedCount++;
        }

        @Override
        public void onContentBlocked(String inputId, TvContentRating rating) {
            mContentBlockedCount++;
            mContentBlockedRating = rating;
        }

        @Override
        public void onTimeShiftStatusChanged(String inputId, int status) {
            mTimeShiftStatusChangedCount++;
            mTimeShiftStatusChangedStatus = status;
        }

        public void resetCounts() {
            mChannelRetunedCount = 0;
            mVideoAvailableCount = 0;
            mVideoUnavailableCount = 0;
            mTrackSelectedCount = 0;
            mTrackChangedCount = 0;
            mContentAllowedCount = 0;
            mContentBlockedCount = 0;
            mTimeShiftStatusChangedCount = 0;
        }

        public void resetPassedValues() {
            mChannelRetunedUri = null;
            mVideoUnavailableReason = null;
            mTrackSelectedType = null;
            mTrackSelectedTrackId = null;
            mTracksChangedTrackList = null;
            mContentBlockedRating = null;
            mTimeShiftStatusChangedStatus = null;
        }
    }

    private static class StubTimeShiftPositionCallback extends TvView.TimeShiftPositionCallback {
        private int mTimeShiftStartPositionChanged;
        private int mTimeShiftCurrentPositionChanged;

        @Override
        public void onTimeShiftStartPositionChanged(String inputId, long timeMs) {
            mTimeShiftStartPositionChanged++;
        }

        @Override
        public void onTimeShiftCurrentPositionChanged(String inputId, long timeMs) {
            mTimeShiftCurrentPositionChanged++;
        }

        public void resetCounts() {
            mTimeShiftStartPositionChanged = 0;
            mTimeShiftCurrentPositionChanged = 0;
        }
    }

    private static Bundle createTestBundle() {
        Bundle b = new Bundle();
        b.putString("stringKey", new String("Test String"));
        return b;
    }

    @Before
    public void setUp() {
        mInstrumentation = InstrumentationRegistry
                .getInstrumentation();
        mTvRecordingClient = new TvRecordingClient(mInstrumentation.getTargetContext(),
                "TvInputServiceTest",
                mRecordingCallback, null);
        mManager = (TvInputManager) mInstrumentation.getTargetContext().getSystemService(
                Context.TV_INPUT_SERVICE);
        for (TvInputInfo info : mManager.getTvInputList()) {
            if (info.getServiceInfo().name.equals(CountingTvInputService.class.getName())) {
                mStubInfo = info;
            }
            if (info.getServiceInfo().name.equals(FaultyTvInputService.class.getName())) {
                mFaultyStubInfo = info;
            }
            if (mStubInfo != null && mFaultyStubInfo != null) {
                break;
            }
        }
        assertThat(mStubInfo).isNotNull();

        CountingTvInputService.sSession = null;
        resetCounts();
        resetPassedValues();
    }

    @After
    public void tearDown() {
        activityRule.getScenario().onActivity(activity -> {
            activity.getTvView().reset();
        });
    }

    @Test
    public void verifyCommandTuneForRecording() {
        final CountingRecordingSession session = tuneForRecording(CHANNEL_0);

        assertThat(session.mSessionId).isNotEmpty();
        assertThat(session.mTuneCount).isEqualTo(1);
        assertThat(session.mTunedChannelUri).isEqualTo(CHANNEL_0);
    }

    @Test
    public void verifyCommandTuneForRecordingWithBundle() {
        final Bundle bundle = createTestBundle();

        final CountingRecordingSession session = tuneForRecording(CHANNEL_0, bundle);

        assertThat(session.mSessionId).isNotEmpty();
        assertThat(session.mTuneCount).isEqualTo(1);
        assertThat(session.mTuneWithBundleCount).isEqualTo(1);
        assertThat(session.mTunedChannelUri).isEqualTo(CHANNEL_0);
        assertBundlesAreEqual(session.mTuneWithBundleData, bundle);
    }

    @Test
    public void verifyCommandRelease() {
        final CountingRecordingSession session = tuneForRecording(CHANNEL_0);

        mTvRecordingClient.release();

        PollingCheck.waitFor(TIME_OUT, () -> session.mReleaseCount > 0);
        assertThat(session.mReleaseCount).isEqualTo(1);
    }

    @Test
    public void verifyCommandStartRecording() {
        final CountingRecordingSession session = tuneForRecording(CHANNEL_0);
        notifyTuned(CHANNEL_0);

        mTvRecordingClient.startRecording(CHANNEL_0);

        PollingCheck.waitFor(TIME_OUT, () -> session.mStartRecordingCount > 0);
        assertThat(session.mStartRecordingCount).isEqualTo(1);
        assertThat(session.mProgramHint).isEqualTo(CHANNEL_0);
    }

    @Test
    public void verifyCommandStartRecordingWithBundle() {
        Bundle bundle = createTestBundle();
        final CountingRecordingSession session = tuneForRecording(CHANNEL_0, bundle);
        notifyTuned(CHANNEL_0);

        mTvRecordingClient.startRecording(CHANNEL_0, bundle);
        PollingCheck.waitFor(TIME_OUT, () -> session.mStartRecordingWithBundleCount > 0);

        assertThat(session.mStartRecordingCount).isEqualTo(1);
        assertThat(session.mStartRecordingWithBundleCount).isEqualTo(1);
        assertThat(session.mProgramHint).isEqualTo(CHANNEL_0);
        assertBundlesAreEqual(session.mStartRecordingWithBundleData, bundle);
    }

    @Test
    public void verifyCommandPauseResumeRecordingWithBundle() {
        final CountingRecordingSession session = tuneForRecording(CHANNEL_0);
        notifyTuned(CHANNEL_0);
        mTvRecordingClient.startRecording(CHANNEL_0);

        final Bundle bundle = createTestBundle();
        mTvRecordingClient.pauseRecording(bundle);
        PollingCheck.waitFor(TIME_OUT, () -> session.mPauseRecordingWithBundleCount > 0);

        assertThat(session.mPauseRecordingWithBundleCount).isEqualTo(1);

        mTvRecordingClient.resumeRecording(bundle);
        PollingCheck.waitFor(TIME_OUT, () -> session.mResumeRecordingWithBundleCount > 0);

        assertThat(session.mResumeRecordingWithBundleCount).isEqualTo(1);
        assertBundlesAreEqual(session.mResumeRecordingWithBundleData, bundle);

    }

    @Test
    public void verifyCommandPauseResumeRecording() {
        final CountingRecordingSession session = tuneForRecording(CHANNEL_0);
        notifyTuned(CHANNEL_0);
        mTvRecordingClient.startRecording(CHANNEL_0);

        mTvRecordingClient.pauseRecording();
        PollingCheck.waitFor(TIME_OUT, () -> session.mPauseRecordingWithBundleCount > 0);

        assertThat(session.mPauseRecordingWithBundleCount).isEqualTo(1);

        mTvRecordingClient.resumeRecording();
        PollingCheck.waitFor(TIME_OUT, () -> session.mResumeRecordingWithBundleCount > 0);

        assertThat(session.mPauseRecordingWithBundleCount).isEqualTo(1);
        assertBundlesAreEqual(session.mResumeRecordingWithBundleData, Bundle.EMPTY);
    }

    @Test
    public void verifyCommandStopRecording() {
        final CountingRecordingSession session = tuneForRecording(CHANNEL_0);
        notifyTuned(CHANNEL_0);
        mTvRecordingClient.startRecording(CHANNEL_0);

        mTvRecordingClient.stopRecording();
        PollingCheck.waitFor(TIME_OUT, () -> session.mStopRecordingCount > 0);

        assertThat(session.mStopRecordingCount).isEqualTo(1);
    }

    @Test
    public void verifyCommandSendAppPrivateCommandForRecording() {
        Bundle bundle = createTestBundle();
        final CountingRecordingSession session = tuneForRecording(CHANNEL_0);
        final String action = "android.media.tv.cts.TvInputServiceTest.privateCommand";

        mTvRecordingClient.sendAppPrivateCommand(action, bundle);
        PollingCheck.waitFor(TIME_OUT, () -> session.mAppPrivateCommandCount > 0);

        assertThat(session.mAppPrivateCommandCount).isEqualTo(1);
        assertBundlesAreEqual(session.mAppPrivateCommandData, bundle);
        assertThat(session.mAppPrivateCommandAction).isEqualTo(action);
    }

    @Test
    public void verifyCallbackTuned() {
        tuneForRecording(CHANNEL_0);

        notifyTuned(CHANNEL_0);

        assertThat(mRecordingCallback.mTunedCount).isEqualTo(1);
        assertThat(mRecordingCallback.mTunedChannelUri).isEqualTo(CHANNEL_0);
    }


    @Test
    public void verifyCallbackError() {
        final CountingRecordingSession session = tuneForRecording(CHANNEL_0);
        notifyTuned(CHANNEL_0);
        mTvRecordingClient.startRecording(CHANNEL_0);
        final int error = TvInputManager.RECORDING_ERROR_UNKNOWN;

        session.notifyError(error);
        PollingCheck.waitFor(TIME_OUT, () -> mRecordingCallback.mErrorCount > 0);

        assertThat(mRecordingCallback.mErrorCount).isEqualTo(1);
        assertThat(mRecordingCallback.mError).isEqualTo(error);
    }

    @Test
    public void verifyCallbackRecordingStopped() {
        final CountingRecordingSession session = tuneForRecording(CHANNEL_0);
        notifyTuned(CHANNEL_0);
        mTvRecordingClient.startRecording(CHANNEL_0);

        session.notifyRecordingStopped(CHANNEL_0);
        PollingCheck.waitFor(TIME_OUT, () -> mRecordingCallback.mRecordingStoppedCount > 0);

        assertThat(mRecordingCallback.mRecordingStoppedCount).isEqualTo(1);
        assertThat(mRecordingCallback.mRecordedProgramUri).isEqualTo(CHANNEL_0);
    }

    @Test
    public void verifyCallbackConnectionFailed() {
        resetCounts();

        mTvRecordingClient.tune("invalid_input_id", CHANNEL_0);
        PollingCheck.waitFor(TIME_OUT, () -> mRecordingCallback.mConnectionFailedCount > 0);

        assertThat(mRecordingCallback.mConnectionFailedCount).isEqualTo(1);
    }

    @Test
    public void verifyCallbackDisconnected() {
        resetCounts();

        mTvRecordingClient.tune(mFaultyStubInfo.getId(), CHANNEL_0);

        PollingCheck.waitFor(TIME_OUT, () -> mRecordingCallback.mDisconnectedCount > 0);
    }

    @Test
    public void verifyCommandTune() {
        resetCounts();
        resetPassedValues();

        final CountingSession session = tune(CHANNEL_0);

        assertWithMessage("session").that(session).isNotNull();
        assertWithMessage("tvInputSessionId").that(session.mSessionId).isNotEmpty();
        assertWithMessage("mTuneCount").that(session.mTuneCount).isGreaterThan(0);
        assertWithMessage("mCreateOverlayView").that(session.mCreateOverlayView).isGreaterThan(0);
        assertWithMessage("mTunedChannelUri").that(session.mTunedChannelUri).isEqualTo(CHANNEL_0);
    }

    @Test
    public void verifyCommandTuneWithBundle() {
        Bundle bundle = createTestBundle();
        resetCounts();
        resetPassedValues();

        onTvView(tvView -> tvView.tune(mStubInfo.getId(), CHANNEL_0, bundle));
        final CountingSession session = waitForSessionCheck(s -> s.mTuneWithBundleCount > 0);

        assertThat(session.mTuneCount).isEqualTo(1);
        assertThat(session.mTuneWithBundleCount).isEqualTo(1);
        assertThat(session.mTunedChannelUri).isEqualTo(CHANNEL_0);
        assertBundlesAreEqual(session.mTuneWithBundleData, bundle);
    }

    @Test
    public void verifyCommandSetStreamVolume() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final float volume = 0.8f;

        onTvView(tvView -> tvView.setStreamVolume(volume));
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT, () -> session.mSetStreamVolumeCount > 0);

        assertThat(session.mSetStreamVolumeCount).isEqualTo(1);
        assertThat(session.mStreamVolume).isEqualTo(volume);
    }

    @Test
    public void verifyCommandSetCaptionEnabled() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final boolean enable = true;
        onTvView(tvView -> tvView.setCaptionEnabled(enable));
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT, () -> session.mSetCaptionEnabledCount > 0);
        assertThat(session.mSetCaptionEnabledCount).isEqualTo(1);
        assertThat(session.mCaptionEnabled).isEqualTo(enable);
    }

    @Test
    public void verifyCommandSelectTrack() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        verifyCallbackTracksChanged();
        final int dummyTrackType = TEST_TV_TRACK.getType();
        final String dummyTrackId = TEST_TV_TRACK.getId();

        onTvView(tvView -> tvView.selectTrack(dummyTrackType, dummyTrackId));
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT, () -> session.mSelectTrackCount > 0);

        assertThat(session.mSelectTrackCount).isEqualTo(1);
        assertThat(session.mSelectTrackType).isEqualTo(dummyTrackType);
        assertThat(session.mSelectTrackId).isEqualTo(dummyTrackId);
    }

    @Test
    public void verifyCommandDispatchKeyDown() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final int keyCode = KeyEvent.KEYCODE_Q;
        final KeyEvent event = new KeyEvent(KeyEvent.ACTION_DOWN, keyCode);

        onTvView(tvView -> tvView.dispatchKeyEvent(event));
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT, () -> session.mKeyDownCount > 0);

        assertThat(session.mKeyDownCount).isEqualTo(1);
        assertThat(session.mKeyDownCode).isEqualTo(keyCode);
        assertKeyEventEquals(session.mKeyDownEvent, event);
    }

    @Test
    public void verifyCommandDispatchKeyMultiple() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final int keyCode = KeyEvent.KEYCODE_Q;
        final KeyEvent event = new KeyEvent(KeyEvent.ACTION_MULTIPLE, keyCode);

        onTvView(tvView -> tvView.dispatchKeyEvent(event));
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT, () -> session.mKeyMultipleCount > 0);

        assertThat(session.mKeyMultipleCount).isEqualTo(1);
        assertKeyEventEquals(session.mKeyMultipleEvent, event);
        assertThat(session.mKeyMultipleNumber).isEqualTo(event.getRepeatCount());
    }

    @Test
    public void verifyCommandDispatchKeyUp() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final int keyCode = KeyEvent.KEYCODE_Q;
        final KeyEvent event = new KeyEvent(KeyEvent.ACTION_UP, keyCode);

        onTvView(tvView -> tvView.dispatchKeyEvent(event));
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT, () -> session.mKeyUpCount > 0);

        assertThat(session.mKeyUpCount).isEqualTo(1);
        assertThat(session.mKeyUpCode).isEqualTo(keyCode);
        assertKeyEventEquals(session.mKeyUpEvent, event);

    }

    @Test
    public void verifyCommandDispatchTouchEvent() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final long now = SystemClock.uptimeMillis();
        final MotionEvent event = MotionEvent.obtain(now, now, MotionEvent.ACTION_DOWN, 1.0f, 1.0f,
                1.0f, 1.0f, 0, 1.0f, 1.0f, 0, 0);
        event.setSource(InputDevice.SOURCE_TOUCHSCREEN);

        onTvView(tvView -> tvView.dispatchTouchEvent(event));
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT, () -> session.mTouchEventCount > 0);

        assertThat(session.mTouchEventCount).isEqualTo(1);
        assertMotionEventEquals(session.mTouchEvent, event);
    }

    @Test
    public void verifyCommandDispatchTrackballEvent() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final long now = SystemClock.uptimeMillis();
        final MotionEvent event = MotionEvent.obtain(now, now, MotionEvent.ACTION_DOWN, 1.0f, 1.0f,
                1.0f, 1.0f, 0, 1.0f, 1.0f, 0, 0);
        event.setSource(InputDevice.SOURCE_TRACKBALL);
        onTvView(tvView -> tvView.dispatchTouchEvent(event));
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT, () -> session.mTrackballEventCount > 0);

        assertThat(session.mTrackballEventCount).isEqualTo(1);
        assertMotionEventEquals(session.mTrackballEvent, event);
    }

    @Test
    public void verifyCommandDispatchGenericMotionEvent() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final long now = SystemClock.uptimeMillis();
        final MotionEvent event = MotionEvent.obtain(now, now, MotionEvent.ACTION_DOWN, 1.0f, 1.0f,
                1.0f, 1.0f, 0, 1.0f, 1.0f, 0, 0);
        onTvView(tvView -> tvView.dispatchGenericMotionEvent(event));
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT, () -> session.mGenricMotionEventCount > 0);

        assertThat(session.mGenricMotionEventCount).isEqualTo(1);
        assertMotionEventEquals(session.mGenricMotionEvent, event);
    }

    @Test
    public void verifyCommandTimeShiftPause() {
        final CountingSession session = tune(CHANNEL_0);
        onTvView(tvView -> tvView.timeShiftPause());
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT, () -> session.mTimeShiftPauseCount > 0);

        assertThat(session.mTimeShiftPauseCount).isEqualTo(1);
    }

    @Test
    public void verifyCommandTimeShiftResume() {
        final CountingSession session = tune(CHANNEL_0);

        onTvView(tvView -> {
            tvView.timeShiftResume();
        });
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT, () -> session.mTimeShiftResumeCount > 0);

        assertThat(session.mTimeShiftResumeCount).isEqualTo(1);
    }

    @Test
    public void verifyCommandTimeShiftSeekTo() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final long timeMs = 0;

        onTvView(tvView -> tvView.timeShiftSeekTo(timeMs));
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT, () -> session.mTimeShiftSeekToCount > 0);

        assertThat(session.mTimeShiftSeekToCount).isEqualTo(1);
        assertThat(session.mTimeShiftSeekTo).isEqualTo(timeMs);
    }

    @Test
    public void verifyCommandTimeShiftSetPlaybackParams() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final PlaybackParams param = new PlaybackParams().setSpeed(2.0f)
                .setAudioFallbackMode(PlaybackParams.AUDIO_FALLBACK_MODE_DEFAULT);
        onTvView(tvView -> tvView.timeShiftSetPlaybackParams(param));
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT,
                () -> session != null && session.mTimeShiftSetPlaybackParamsCount > 0);

        assertThat(session.mTimeShiftSetPlaybackParamsCount).isEqualTo(1);
        assertPlaybackParamsEquals(session.mTimeShiftSetPlaybackParams, param);
    }

    @Test
    public void verifyCommandTimeShiftPlay() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final Uri fakeRecordedProgramUri = TvContract.buildRecordedProgramUri(0);

        onTvView(tvView -> tvView.timeShiftPlay(mStubInfo.getId(), fakeRecordedProgramUri));
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT, () -> session.mTimeShiftPlayCount > 0);

        assertThat(session.mTimeShiftPlayCount).isEqualTo(1);
        assertThat(session.mRecordedProgramUri).isEqualTo(fakeRecordedProgramUri);
    }

    @Test
    public void verifyCommandSetTimeShiftPositionCallback() {
        tune(CHANNEL_0);

        onTvView(tvView -> tvView.setTimeShiftPositionCallback(mTimeShiftPositionCallback));
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT,
                () -> mTimeShiftPositionCallback.mTimeShiftCurrentPositionChanged > 0
                        && mTimeShiftPositionCallback.mTimeShiftStartPositionChanged > 0);

        assertThat(mTimeShiftPositionCallback.mTimeShiftCurrentPositionChanged).isEqualTo(1);
        assertThat(mTimeShiftPositionCallback.mTimeShiftStartPositionChanged).isEqualTo(1);
    }

    @Test
    public void verifyCommandOverlayViewSizeChanged() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final int width = 10;
        final int height = 20;

        // There is a first OverlayViewSizeChange called on initial tune.
        assertThat(session.mOverlayViewSizeChangedCount).isEqualTo(1);

        onTvView(tvView -> tvView.setLayoutParams(new LinearLayout.LayoutParams(width, height)));

        PollingCheck.waitFor(TIME_OUT, () -> session.mOverlayViewSizeChangedCount > 1);

        assertThat(session.mOverlayViewSizeChangedCount).isEqualTo(2);
        assertThat(session.mOverlayViewSizeChangedWidth).isEqualTo(width);
        assertThat(session.mOverlayViewSizeChangedHeight).isEqualTo(height);
    }

    @Test
    public void verifyCommandSendAppPrivateCommand() {
        Bundle bundle = createTestBundle();
        tune(CHANNEL_0);
        final String action = "android.media.tv.cts.TvInputServiceTest.privateCommand";

        onTvView(tvView -> tvView.sendAppPrivateCommand(action, bundle));
        mInstrumentation.waitForIdleSync();
        final CountingSession session = waitForSessionCheck(s -> s.mAppPrivateCommandCount > 0);

        assertThat(session.mAppPrivateCommandCount).isEqualTo(1);
        assertBundlesAreEqual(session.mAppPrivateCommandData, bundle);
        assertThat(session.mAppPrivateCommandAction).isEqualTo(action);
    }

    @Test
    public void verifyCallbackChannelRetuned() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();

        session.notifyChannelRetuned(CHANNEL_0);
        PollingCheck.waitFor(TIME_OUT, () -> mCallback.mChannelRetunedCount > 0);

        assertThat(mCallback.mChannelRetunedCount).isEqualTo(1);
        assertThat(mCallback.mChannelRetunedUri).isEqualTo(CHANNEL_0);

    }

    @Test
    public void verifyCallbackVideoAvailable() {
        final CountingSession session = tune(CHANNEL_0);
        resetCounts();

        session.notifyVideoAvailable();
        PollingCheck.waitFor(TIME_OUT, () -> mCallback.mVideoAvailableCount > 0);

        assertThat(mCallback.mVideoAvailableCount).isEqualTo(1);
    }

    @Test
    public void verifyCallbackVideoUnavailable() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final int reason = TvInputManager.VIDEO_UNAVAILABLE_REASON_TUNING;

        session.notifyVideoUnavailable(reason);
        PollingCheck.waitFor(TIME_OUT, () -> mCallback.mVideoUnavailableCount > 0);

        assertThat(mCallback.mVideoUnavailableCount).isEqualTo(1);
        assertThat(mCallback.mVideoUnavailableReason).isEqualTo(reason);
    }

    @Test
    public void verifyCallbackTracksChanged() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        ArrayList<TvTrackInfo> tracks = new ArrayList<>();
        tracks.add(TEST_TV_TRACK);

        session.notifyTracksChanged(tracks);
        PollingCheck.waitFor(TIME_OUT, () -> mCallback.mTrackChangedCount > 0
                && Objects.equals(mCallback.mTracksChangedTrackList, tracks));

        assertThat(mCallback.mTrackChangedCount).isEqualTo(1);
        assertThat(mCallback.mTracksChangedTrackList).isEqualTo(tracks);
    }

    @Test
    @Ignore("b/174076887")
    public void verifyCallbackVideoSizeChanged() {
        final CountingSession session = tune(CHANNEL_0);
        resetCounts();
        ArrayList<TvTrackInfo> tracks = new ArrayList<>();
        tracks.add(TEST_TV_TRACK);

        session.notifyTracksChanged(tracks);
        mInstrumentation.waitForIdleSync();
        PollingCheck.waitFor(TIME_OUT, () -> mCallback.mVideoSizeChanged > 0);

        assertThat(mCallback.mVideoSizeChanged).isEqualTo(1);
    }

    @Test
    public void verifyCallbackTrackSelected() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();

        session.notifyTrackSelected(TEST_TV_TRACK.getType(), TEST_TV_TRACK.getId());
        PollingCheck.waitFor(TIME_OUT, () -> mCallback.mTrackSelectedCount > 0);

        assertThat(mCallback.mTrackSelectedCount).isEqualTo(1);
        assertThat(mCallback.mTrackSelectedType).isEqualTo(TEST_TV_TRACK.getType());
        assertThat(mCallback.mTrackSelectedTrackId).isEqualTo(TEST_TV_TRACK.getId());
    }

    @Test
    public void verifyCallbackContentAllowed() {
        final CountingSession session = tune(CHANNEL_0);
        resetCounts();

        session.notifyContentAllowed();
        PollingCheck.waitFor(TIME_OUT, () -> mCallback.mContentAllowedCount > 0);

        assertThat(mCallback.mContentAllowedCount).isEqualTo(1);
    }

    @Test
    public void verifyCallbackContentBlocked() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final TvContentRating rating = TvContentRating.createRating("android.media.tv", "US_TVPG",
                "US_TVPG_TV_MA", "US_TVPG_S", "US_TVPG_V");

        session.notifyContentBlocked(rating);
        PollingCheck.waitFor(TIME_OUT, () -> mCallback.mContentBlockedCount > 0);

        assertThat(mCallback.mContentBlockedCount).isEqualTo(1);
        assertThat(mCallback.mContentBlockedRating).isEqualTo(rating);

    }

    @Test
    public void verifyCallbackTimeShiftStatusChanged() {
        final CountingSession session = tune(CHANNEL_0);
        resetPassedValues();
        final int status = TvInputManager.TIME_SHIFT_STATUS_AVAILABLE;

        session.notifyTimeShiftStatusChanged(status);
        PollingCheck.waitFor(TIME_OUT, () -> mCallback.mTimeShiftStatusChangedCount > 0);

        assertThat(mCallback.mTimeShiftStatusChangedCount).isEqualTo(1);
        assertThat(mCallback.mTimeShiftStatusChangedStatus).isEqualTo(status);
    }

    @Test
    public void verifyCallbackLayoutSurface() {
        final CountingSession session = tune(CHANNEL_0);
        final int left = 10;
        final int top = 20;
        final int right = 30;
        final int bottom = 40;

        session.layoutSurface(left, top, right, bottom);
        PollingCheck.waitFor(TIME_OUT, () -> {
            final AtomicBoolean retValue = new AtomicBoolean();
            onTvView(tvView -> {
                int childCount = tvView.getChildCount();
                for (int i = 0; i < childCount; ++i) {
                    View v = tvView.getChildAt(i);
                    if (v instanceof SurfaceView) {
                        retValue.set(v.getLeft() == left && v.getTop() == top
                                && v.getRight() == right
                                && v.getBottom() == bottom
                        );
                        break;
                    }
                }
            });
            mInstrumentation.waitForIdleSync();
            return retValue.get();
        });
    }

    public static void assertKeyEventEquals(KeyEvent actual, KeyEvent expected) {
        if ((expected == null) != (actual == null)) {
            // Fail miss matched nulls early using the StandardSubject
            Truth.assertThat(actual).isEqualTo(expected);
        } else if (expected != null && actual != null) {
            assertThat(actual.getDownTime()).isEqualTo(expected.getDownTime());
            assertThat(actual.getEventTime()).isEqualTo(expected.getEventTime());
            assertThat(actual.getAction()).isEqualTo(expected.getAction());
            assertThat(actual.getKeyCode()).isEqualTo(expected.getKeyCode());
            assertThat(actual.getRepeatCount()).isEqualTo(expected.getRepeatCount());
            assertThat(actual.getMetaState()).isEqualTo(expected.getMetaState());
            assertThat(actual.getDeviceId()).isEqualTo(expected.getDeviceId());
            assertThat(actual.getScanCode()).isEqualTo(expected.getScanCode());
            assertThat(actual.getFlags()).isEqualTo(expected.getFlags());
            assertThat(actual.getSource()).isEqualTo(expected.getSource());
            assertThat(actual.getCharacters()).isEqualTo(expected.getCharacters());
        }// else both null so do nothing
    }

    public static void assertMotionEventEquals(MotionEvent actual, MotionEvent expected) {
        if ((expected == null) != (actual == null)) {
            // Fail miss matched nulls early using the StandardSubject
            Truth.assertThat(actual).isEqualTo(expected);
        } else if (expected != null && actual != null) {
            assertThat(actual).hasDownTime(expected.getDownTime());
            assertThat(actual).hasEventTime(expected.getEventTime());
            assertThat(actual).hasAction(expected.getAction());
            assertThat(actual).x().isEqualTo(expected.getX());
            assertThat(actual).y().isEqualTo(expected.getY());
            assertThat(actual).pressure().isEqualTo(expected.getPressure());
            assertThat(actual).size().isEqualTo(expected.getSize());
            assertThat(actual).hasMetaState(expected.getMetaState());
            assertThat(actual).xPrecision().isEqualTo(expected.getXPrecision());
            assertThat(actual).yPrecision().isEqualTo(expected.getYPrecision());
            assertThat(actual).hasDeviceId(expected.getDeviceId());
            assertThat(actual).hasEdgeFlags(expected.getEdgeFlags());
            assertThat(actual.getSource()).isEqualTo(expected.getSource());

        } // else both null so do nothing
    }

    public static void assertPlaybackParamsEquals(PlaybackParams actual, PlaybackParams expected) {
        if ((expected == null) != (actual == null)) {
            // Fail miss matched nulls early using the StandardSubject
            Truth.assertThat(actual).isEqualTo(expected);
        } else if (expected != null && actual != null) {
            assertThat(actual.getAudioFallbackMode()).isEqualTo(expected.getAudioFallbackMode());
            assertThat(actual.getSpeed()).isEqualTo(expected.getSpeed());
        } // else both null so do nothing
    }

    private static void assertBundlesAreEqual(Bundle actual, Bundle expected) {
        if ((expected == null) != (actual == null)) {
            // Fail miss matched nulls early using the StandardSubject
            Truth.assertThat(actual).isEqualTo(expected);
        } else if (expected != null && actual != null) {
            assertThat(actual.keySet()).isEqualTo(expected.keySet());
            for (String key : expected.keySet()) {
                assertThat(actual.get(key)).isEqualTo(expected.get(key));
            }
        }
    }

    private void notifyTuned(Uri uri) {
        final CountingRecordingSession session = CountingTvInputService.sRecordingSession;
        session.notifyTuned(uri);
        PollingCheck.waitFor(TIME_OUT, () -> mRecordingCallback.mTunedCount > 0);
    }

    private void onTvView(Consumer<TvView> tvViewConsumer) {
        activityRule.getScenario().onActivity(viewAction(tvViewConsumer));

    }

    private void resetCounts() {
        if (CountingTvInputService.sSession != null) {
            CountingTvInputService.sSession.resetCounts();
        }
        if (CountingTvInputService.sRecordingSession != null) {
            CountingTvInputService.sRecordingSession.resetCounts();
        }
        mCallback.resetCounts();
        mTimeShiftPositionCallback.resetCounts();
        mRecordingCallback.resetCounts();
    }

    private void resetPassedValues() {
        if (CountingTvInputService.sSession != null) {
            CountingTvInputService.sSession.resetPassedValues();
        }
        if (CountingTvInputService.sRecordingSession != null) {
            CountingTvInputService.sRecordingSession.resetPassedValues();
        }
        mCallback.resetPassedValues();
        mRecordingCallback.resetPassedValues();
    }

    @NonNull
    private static PollingCheck.PollingCheckCondition recordingSessionCheck(
            ToBooleanFunction<CountingRecordingSession> toBooleanFunction) {
        return () -> {
            final CountingRecordingSession session = CountingTvInputService.sRecordingSession;
            return session != null && toBooleanFunction.apply(session);
        };
    }

    @NonNull
    private static PollingCheck.PollingCheckCondition sessionCheck(
            ToBooleanFunction<CountingSession> toBooleanFunction) {
        return () -> {
            final CountingSession session = CountingTvInputService.sSession;
            return session != null && toBooleanFunction.apply(session);
        };
    }

    @NonNull
    private CountingSession tune(Uri uri) {
        onTvView(tvView -> {
            tvView.setCallback(mCallback);
            tvView.tune(mStubInfo.getId(), CHANNEL_0);
        });
        return waitForSessionCheck(session -> session.mTuneCount > 0);
    }

    @NonNull
    private CountingRecordingSession tuneForRecording(Uri uri) {
        mTvRecordingClient.tune(mStubInfo.getId(), uri);
        return waitForRecordingSessionCheck(s -> s.mTuneCount > 0);
    }

    @NonNull
    private CountingRecordingSession tuneForRecording(Uri uri, Bundle bundle) {
        mTvRecordingClient.tune(mStubInfo.getId(), uri, bundle);
        return waitForRecordingSessionCheck(s -> s.mTuneCount > 0 && s.mTuneWithBundleCount > 0);
    }

    @NonNull
    private static ActivityScenario.ActivityAction<TvViewStubActivity> viewAction(
            Consumer<TvView> consumer) {
        return activity -> consumer.accept(activity.getTvView());
    }

    @NonNull
    private static CountingSession waitForSessionCheck(
            ToBooleanFunction<CountingSession> countingSessionToBooleanFunction) {
        PollingCheck.waitFor(TIME_OUT, sessionCheck(countingSessionToBooleanFunction));
        return CountingTvInputService.sSession;
    }

    @NonNull
    private static CountingRecordingSession waitForRecordingSessionCheck(
            ToBooleanFunction<CountingRecordingSession> toBool) {
        PollingCheck.waitFor(TIME_OUT, recordingSessionCheck(toBool));
        return CountingTvInputService.sRecordingSession;
    }

    public static class CountingTvInputService extends StubTvInputService {

        static CountingSession sSession;
        static CountingRecordingSession sRecordingSession;

        @Override
        public Session onCreateSession(String inputId) {
            return onCreateSession(inputId, null);
        }

        @Override
        public Session onCreateSession(String inputId, String tvInputSessionId) {
            if(sSession != null){
                Log.w(TAG,"onCreateSession called with sSession set to "+ sSession);
            }
            sSession = new CountingSession(this, tvInputSessionId);
            sSession.setOverlayViewEnabled(true);
            return sSession;
        }

        @Override
        public RecordingSession onCreateRecordingSession(String inputId) {
            return onCreateRecordingSession(inputId, null);
        }

        @Override
        public RecordingSession onCreateRecordingSession(String inputId, String tvInputSessionId) {
            if (sRecordingSession != null) {
                Log.w(TAG, "onCreateRecordingSession called with sRecordingSession set to "
                        + sRecordingSession);
            }
            sRecordingSession = new CountingRecordingSession(this, tvInputSessionId);
            return sRecordingSession;
        }

        @Override
        public IBinder createExtension() {
            super.createExtension();
            return null;
        }

        public static class CountingSession extends Session {
            public final String mSessionId;

            public volatile int mTuneCount;
            public volatile int mTuneWithBundleCount;
            public volatile int mSetStreamVolumeCount;
            public volatile int mSetCaptionEnabledCount;
            public volatile int mSelectTrackCount;
            public volatile int mCreateOverlayView;
            public volatile int mKeyDownCount;
            public volatile int mKeyLongPressCount;
            public volatile int mKeyMultipleCount;
            public volatile int mKeyUpCount;
            public volatile int mTouchEventCount;
            public volatile int mTrackballEventCount;
            public volatile int mGenricMotionEventCount;
            public volatile int mOverlayViewSizeChangedCount;
            public volatile int mTimeShiftPauseCount;
            public volatile int mTimeShiftResumeCount;
            public volatile int mTimeShiftSeekToCount;
            public volatile int mTimeShiftSetPlaybackParamsCount;
            public volatile int mTimeShiftPlayCount;
            public volatile long mTimeShiftGetCurrentPositionCount;
            public volatile long mTimeShiftGetStartPositionCount;
            public volatile int mAppPrivateCommandCount;

            public volatile String mAppPrivateCommandAction;
            public volatile Bundle mAppPrivateCommandData;
            public volatile Uri mTunedChannelUri;
            public volatile Bundle mTuneWithBundleData;
            public volatile Float mStreamVolume;
            public volatile Boolean mCaptionEnabled;
            public volatile Integer mSelectTrackType;
            public volatile String mSelectTrackId;
            public volatile Integer mKeyDownCode;
            public volatile KeyEvent mKeyDownEvent;
            public volatile Integer mKeyLongPressCode;
            public volatile KeyEvent mKeyLongPressEvent;
            public volatile Integer mKeyMultipleCode;
            public volatile Integer mKeyMultipleNumber;
            public volatile KeyEvent mKeyMultipleEvent;
            public volatile Integer mKeyUpCode;
            public volatile KeyEvent mKeyUpEvent;
            public volatile MotionEvent mTouchEvent;
            public volatile MotionEvent mTrackballEvent;
            public volatile MotionEvent mGenricMotionEvent;
            public volatile Long mTimeShiftSeekTo;
            public volatile PlaybackParams mTimeShiftSetPlaybackParams;
            public volatile Uri mRecordedProgramUri;
            public volatile Integer mOverlayViewSizeChangedWidth;
            public volatile Integer mOverlayViewSizeChangedHeight;


            CountingSession(Context context, @Nullable String sessionId) {

                super(context);
                mSessionId = sessionId;

            }

            public void resetCounts() {
                mTuneCount = 0;
                mTuneWithBundleCount = 0;
                mSetStreamVolumeCount = 0;
                mSetCaptionEnabledCount = 0;
                mSelectTrackCount = 0;
                mCreateOverlayView = 0;
                mKeyDownCount = 0;
                mKeyLongPressCount = 0;
                mKeyMultipleCount = 0;
                mKeyUpCount = 0;
                mTouchEventCount = 0;
                mTrackballEventCount = 0;
                mGenricMotionEventCount = 0;
                mOverlayViewSizeChangedCount = 0;
                mTimeShiftPauseCount = 0;
                mTimeShiftResumeCount = 0;
                mTimeShiftSeekToCount = 0;
                mTimeShiftSetPlaybackParamsCount = 0;
                mTimeShiftPlayCount = 0;
                mTimeShiftGetCurrentPositionCount = 0;
                mTimeShiftGetStartPositionCount = 0;
                mAppPrivateCommandCount = 0;
            }

            public void resetPassedValues() {
                mAppPrivateCommandAction = null;
                mAppPrivateCommandData = null;
                mTunedChannelUri = null;
                mTuneWithBundleData = null;
                mStreamVolume = null;
                mCaptionEnabled = null;
                mSelectTrackType = null;
                mSelectTrackId = null;
                mKeyDownCode = null;
                mKeyDownEvent = null;
                mKeyLongPressCode = null;
                mKeyLongPressEvent = null;
                mKeyMultipleCode = null;
                mKeyMultipleNumber = null;
                mKeyMultipleEvent = null;
                mKeyUpCode = null;
                mKeyUpEvent = null;
                mTouchEvent = null;
                mTrackballEvent = null;
                mGenricMotionEvent = null;
                mTimeShiftSeekTo = null;
                mTimeShiftSetPlaybackParams = null;
                mRecordedProgramUri = null;
                mOverlayViewSizeChangedWidth = null;
                mOverlayViewSizeChangedHeight = null;
            }

            @Override
            public void onAppPrivateCommand(String action, Bundle data) {
                mAppPrivateCommandCount++;
                mAppPrivateCommandAction = action;
                mAppPrivateCommandData = data;
            }

            @Override
            public void onRelease() {
            }

            @Override
            public boolean onSetSurface(Surface surface) {
                return false;
            }

            @Override
            public boolean onTune(Uri channelUri) {
                mTuneCount++;
                mTunedChannelUri = channelUri;
                return false;
            }

            @Override
            public boolean onTune(Uri channelUri, Bundle data) {
                mTuneWithBundleCount++;
                mTuneWithBundleData = data;
                // Also calls {@link #onTune(Uri)} since it will never be called if the
                // implementation overrides {@link #onTune(Uri, Bundle)}.
                onTune(channelUri);
                return false;
            }

            @Override
            public void onSetStreamVolume(float volume) {
                mSetStreamVolumeCount++;
                mStreamVolume = volume;
            }

            @Override
            public void onSetCaptionEnabled(boolean enabled) {
                mSetCaptionEnabledCount++;
                mCaptionEnabled = enabled;
            }

            @Override
            public boolean onSelectTrack(int type, String id) {
                mSelectTrackCount++;
                mSelectTrackType = type;
                mSelectTrackId = id;
                return false;
            }

            @Override
            public View onCreateOverlayView() {
                mCreateOverlayView++;
                return null;
            }

            @Override
            public boolean onKeyDown(int keyCode, KeyEvent event) {
                mKeyDownCount++;
                mKeyDownCode = keyCode;
                mKeyDownEvent = event;
                return false;
            }

            @Override
            public boolean onKeyLongPress(int keyCode, KeyEvent event) {
                mKeyLongPressCount++;
                mKeyLongPressCode = keyCode;
                mKeyLongPressEvent = event;
                return false;
            }

            @Override
            public boolean onKeyMultiple(int keyCode, int count, KeyEvent event) {
                mKeyMultipleCount++;
                mKeyMultipleCode = keyCode;
                mKeyMultipleNumber = count;
                mKeyMultipleEvent = event;
                return false;
            }

            @Override
            public boolean onKeyUp(int keyCode, KeyEvent event) {
                mKeyUpCount++;
                mKeyUpCode = keyCode;
                mKeyUpEvent = event;
                return false;
            }

            @Override
            public boolean onTouchEvent(MotionEvent event) {
                mTouchEventCount++;
                mTouchEvent = event;
                return false;
            }

            @Override
            public boolean onTrackballEvent(MotionEvent event) {
                mTrackballEventCount++;
                mTrackballEvent = event;
                return false;
            }

            @Override
            public boolean onGenericMotionEvent(MotionEvent event) {
                mGenricMotionEventCount++;
                mGenricMotionEvent = event;
                return false;
            }

            @Override
            public void onTimeShiftPause() {
                mTimeShiftPauseCount++;
            }

            @Override
            public void onTimeShiftResume() {
                mTimeShiftResumeCount++;
            }

            @Override
            public void onTimeShiftSeekTo(long timeMs) {
                mTimeShiftSeekToCount++;
                mTimeShiftSeekTo = timeMs;
            }

            @Override
            public void onTimeShiftSetPlaybackParams(PlaybackParams param) {
                mTimeShiftSetPlaybackParamsCount++;
                mTimeShiftSetPlaybackParams = param;
            }

            @Override
            public void onTimeShiftPlay(Uri recordedProgramUri) {
                mTimeShiftPlayCount++;
                mRecordedProgramUri = recordedProgramUri;
            }

            @Override
            public long onTimeShiftGetCurrentPosition() {
                return ++mTimeShiftGetCurrentPositionCount;
            }

            @Override
            public long onTimeShiftGetStartPosition() {
                return ++mTimeShiftGetStartPositionCount;
            }

            @Override
            public void onOverlayViewSizeChanged(int width, int height) {
                mOverlayViewSizeChangedCount++;
                mOverlayViewSizeChangedWidth = width;
                mOverlayViewSizeChangedHeight = height;
            }
        }

        public static class CountingRecordingSession extends RecordingSession {
            public final String mSessionId;

            public volatile int mTuneCount;
            public volatile int mTuneWithBundleCount;
            public volatile int mReleaseCount;
            public volatile int mStartRecordingCount;
            public volatile int mStartRecordingWithBundleCount;
            public volatile int mPauseRecordingWithBundleCount;
            public volatile int mResumeRecordingWithBundleCount;
            public volatile int mStopRecordingCount;
            public volatile int mAppPrivateCommandCount;

            public volatile Uri mTunedChannelUri;
            public volatile Bundle mTuneWithBundleData;
            public volatile Uri mProgramHint;
            public volatile Bundle mStartRecordingWithBundleData;
            public volatile Bundle mPauseRecordingWithBundleData;
            public volatile Bundle mResumeRecordingWithBundleData;
            public volatile String mAppPrivateCommandAction;
            public volatile Bundle mAppPrivateCommandData;

            CountingRecordingSession(Context context, @Nullable String sessionId) {
                super(context);
                mSessionId = sessionId;
            }

            public void resetCounts() {
                mTuneCount = 0;
                mTuneWithBundleCount = 0;
                mReleaseCount = 0;
                mStartRecordingCount = 0;
                mStartRecordingWithBundleCount = 0;
                mPauseRecordingWithBundleCount = 0;
                mResumeRecordingWithBundleCount = 0;
                mStopRecordingCount = 0;
                mAppPrivateCommandCount = 0;
            }

            public void resetPassedValues() {
                mTunedChannelUri = null;
                mTuneWithBundleData = null;
                mProgramHint = null;
                mStartRecordingWithBundleData = null;
                mPauseRecordingWithBundleData = null;
                mResumeRecordingWithBundleData = null;
                mAppPrivateCommandAction = null;
                mAppPrivateCommandData = null;
            }

            @Override
            public void onTune(Uri channelUri) {
                mTuneCount++;
                mTunedChannelUri = channelUri;
            }

            @Override
            public void onTune(Uri channelUri, Bundle data) {
                mTuneWithBundleCount++;
                mTuneWithBundleData = data;
                // Also calls {@link #onTune(Uri)} since it will never be called if the
                // implementation overrides {@link #onTune(Uri, Bundle)}.
                onTune(channelUri);
            }

            @Override
            public void onRelease() {
                mReleaseCount++;
            }

            @Override
            public void onStartRecording(Uri programHint) {
                mStartRecordingCount++;
                mProgramHint = programHint;
            }

            @Override
            public void onStartRecording(Uri programHint, Bundle data) {
                mStartRecordingWithBundleCount++;
                mProgramHint = programHint;
                mStartRecordingWithBundleData = data;
                // Also calls {@link #onStartRecording(Uri)} since it will never be called if the
                // implementation overrides {@link #onStartRecording(Uri, Bundle)}.
                onStartRecording(programHint);
            }

            @Override
            public void onPauseRecording(Bundle data) {
                mPauseRecordingWithBundleCount++;
                mPauseRecordingWithBundleData = data;
            }

            @Override
            public void onResumeRecording(Bundle data) {
                mResumeRecordingWithBundleCount++;
                mResumeRecordingWithBundleData = data;

            }

            @Override
            public void onStopRecording() {
                mStopRecordingCount++;
            }

            @Override
            public void onAppPrivateCommand(String action, Bundle data) {
                mAppPrivateCommandCount++;
                mAppPrivateCommandAction = action;
                mAppPrivateCommandData = data;
            }
        }
    }

    private static class StubRecordingCallback extends TvRecordingClient.RecordingCallback {
        private int mTunedCount;
        private int mRecordingStoppedCount;
        private int mErrorCount;
        private int mConnectionFailedCount;
        private int mDisconnectedCount;

        private Uri mTunedChannelUri;
        private Uri mRecordedProgramUri;
        private Integer mError;

        @Override
        public void onTuned(Uri channelUri) {
            mTunedCount++;
            mTunedChannelUri = channelUri;
        }

        @Override
        public void onRecordingStopped(Uri recordedProgramUri) {
            mRecordingStoppedCount++;
            mRecordedProgramUri = recordedProgramUri;
        }

        @Override
        public void onError(int error) {
            mErrorCount++;
            mError = error;
        }

        @Override
        public void onConnectionFailed(String inputId) {
            mConnectionFailedCount++;
        }

        @Override
        public void onDisconnected(String inputId) {
            mDisconnectedCount++;
        }

        public void resetCounts() {
            mTunedCount = 0;
            mRecordingStoppedCount = 0;
            mErrorCount = 0;
            mConnectionFailedCount = 0;
            mDisconnectedCount = 0;
        }

        public void resetPassedValues() {
            mTunedChannelUri = null;
            mRecordedProgramUri = null;
            mError = null;
        }
    }


    // Copied from {@link com.android.internal.util.ToBooleanFunction}
    /**
     * Represents a function that produces an boolean-valued result.  This is the
     * {@code boolean}-producing primitive specialization for {@link Function}.
     *
     * <p>This is a <a href="package-summary.html">functional interface</a>
     * whose functional method is {@link #apply(Object)}.
     *
     * @param <T> the type of the input to the function
     *
     * @see Function
     */
    @FunctionalInterface
    private  interface ToBooleanFunction<T> {

        /**
         * Applies this function to the given argument.
         *
         * @param value the function argument
         * @return the function result
         */
        boolean apply(T value);
    }

}
