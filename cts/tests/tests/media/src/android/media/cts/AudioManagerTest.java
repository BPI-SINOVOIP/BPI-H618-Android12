/*
 * Copyright (C) 2009 The Android Open Source Project
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

import static org.junit.Assert.assertNotEquals;

import static android.media.AudioManager.ADJUST_LOWER;
import static android.media.AudioManager.ADJUST_RAISE;
import static android.media.AudioManager.ADJUST_SAME;
import static android.media.AudioManager.MODE_IN_CALL;
import static android.media.AudioManager.MODE_IN_COMMUNICATION;
import static android.media.AudioManager.MODE_NORMAL;
import static android.media.AudioManager.MODE_RINGTONE;
import static android.media.AudioManager.RINGER_MODE_NORMAL;
import static android.media.AudioManager.RINGER_MODE_SILENT;
import static android.media.AudioManager.RINGER_MODE_VIBRATE;
import static android.media.AudioManager.STREAM_ACCESSIBILITY;
import static android.media.AudioManager.STREAM_ALARM;
import static android.media.AudioManager.STREAM_DTMF;
import static android.media.AudioManager.STREAM_MUSIC;
import static android.media.AudioManager.STREAM_NOTIFICATION;
import static android.media.AudioManager.STREAM_RING;
import static android.media.AudioManager.STREAM_SYSTEM;
import static android.media.AudioManager.STREAM_VOICE_CALL;
import static android.media.AudioManager.USE_DEFAULT_STREAM_TYPE;
import static android.media.AudioManager.VIBRATE_SETTING_OFF;
import static android.media.AudioManager.VIBRATE_SETTING_ON;
import static android.media.AudioManager.VIBRATE_SETTING_ONLY_SILENT;
import static android.media.AudioManager.VIBRATE_TYPE_NOTIFICATION;
import static android.media.AudioManager.VIBRATE_TYPE_RINGER;
import static android.provider.Settings.System.SOUND_EFFECTS_ENABLED;

import android.Manifest;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.media.AudioAttributes;
import android.media.AudioDeviceAttributes;
import android.media.AudioDeviceInfo;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioProfile;
import android.media.AudioDescriptor;
import android.media.MediaPlayer;
import android.media.MediaRecorder;
import android.media.MicrophoneInfo;
import android.media.audiopolicy.AudioProductStrategy;
import android.os.Build;
import android.os.SystemClock;
import android.os.Vibrator;
import android.platform.test.annotations.AppModeFull;
import android.provider.Settings;
import android.provider.Settings.System;
import android.test.InstrumentationTestCase;
import android.text.TextUtils;
import android.util.Log;
import android.view.SoundEffectConstants;

import androidx.test.InstrumentationRegistry;

import com.android.compatibility.common.util.ApiLevelUtil;
import com.android.compatibility.common.util.CddTest;
import com.android.compatibility.common.util.MediaUtils;
import com.android.compatibility.common.util.SettingsStateKeeperRule;
import com.android.internal.annotations.GuardedBy;

import org.junit.ClassRule;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.function.Predicate;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

@NonMediaMainlineTest
public class AudioManagerTest extends InstrumentationTestCase {
    private final static String TAG = "AudioManagerTest";

    private final static long ASYNC_TIMING_TOLERANCE_MS = 50;
    private final static long POLL_TIME_VOLUME_ADJUST = 200;
    private final static long POLL_TIME_UPDATE_INTERRUPTION_FILTER = 5000;
    private final static int MP3_TO_PLAY = R.raw.testmp3; // ~ 5 second mp3
    private final static long POLL_TIME_PLAY_MUSIC = 2000;
    private final static long TIME_TO_PLAY = 2000;
    private final static String APPOPS_OP_STR = "android:write_settings";
    private final static Set<Integer> ALL_KNOWN_ENCAPSULATION_TYPES = new HashSet<>() {{
            add(AudioProfile.AUDIO_ENCAPSULATION_TYPE_IEC61937);
    }};
    private final static Set<Integer> ALL_ENCAPSULATION_TYPES = new HashSet<>() {{
            add(AudioProfile.AUDIO_ENCAPSULATION_TYPE_NONE);
            add(AudioProfile.AUDIO_ENCAPSULATION_TYPE_IEC61937);
    }};
    private final static HashSet<Integer> ALL_AUDIO_STANDARDS = new HashSet<>() {{
            add(AudioDescriptor.STANDARD_NONE);
            add(AudioDescriptor.STANDARD_EDID);
    }};
    private AudioManager mAudioManager;
    private NotificationManager mNm;
    private boolean mHasVibrator;
    private boolean mUseFixedVolume;
    private boolean mIsTelevision;
    private boolean mIsSingleVolume;
    private boolean mSkipRingerTests;
    // From N onwards, ringer mode adjustments that toggle DND are not allowed unless
    // package has DND access. Many tests in this package toggle DND access in order
    // to get device out of the DND state for the test to proceed correctly.
    // But DND access is disabled completely on low ram devices,
    // so completely skip those tests here.
    // These tests are migrated to CTS verifier tests to ensure test coverage.
    private Context mContext;
    private int mOriginalRingerMode;
    private Map<Integer, Integer> mOriginalStreamVolumes = new HashMap<>();
    private NotificationManager.Policy mOriginalNotificationPolicy;
    private int mOriginalZen;
    private boolean mDoNotCheckUnmute;
    private boolean mAppsBypassingDnd;

    @ClassRule
    public static final SettingsStateKeeperRule mSurroundSoundFormatsSettingsKeeper =
            new SettingsStateKeeperRule(InstrumentationRegistry.getTargetContext(),
                    Settings.Global.ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS);

    @ClassRule
    public static final SettingsStateKeeperRule mSurroundSoundModeSettingsKeeper =
            new SettingsStateKeeperRule(InstrumentationRegistry.getTargetContext(),
                    Settings.Global.ENCODED_SURROUND_OUTPUT);

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = getInstrumentation().getContext();
        Utils.enableAppOps(mContext.getPackageName(), APPOPS_OP_STR, getInstrumentation());
        mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        Vibrator vibrator = (Vibrator) mContext.getSystemService(Context.VIBRATOR_SERVICE);
        mNm = (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        mAppsBypassingDnd = NotificationManager.getService().areChannelsBypassingDnd();
        mHasVibrator = (vibrator != null) && vibrator.hasVibrator();
        mUseFixedVolume = mContext.getResources().getBoolean(
                Resources.getSystem().getIdentifier("config_useFixedVolume", "bool", "android"));
        PackageManager packageManager = mContext.getPackageManager();
        mIsTelevision = packageManager != null
                && (packageManager.hasSystemFeature(PackageManager.FEATURE_LEANBACK)
                        || packageManager.hasSystemFeature(PackageManager.FEATURE_TELEVISION));
        mIsSingleVolume = mContext.getResources().getBoolean(
                Resources.getSystem().getIdentifier("config_single_volume", "bool", "android"));
        mSkipRingerTests = mUseFixedVolume || mIsTelevision || mIsSingleVolume;

        // Store the original volumes that that they can be recovered in tearDown().
        final int[] streamTypes = {
            STREAM_VOICE_CALL,
            STREAM_SYSTEM,
            STREAM_RING,
            STREAM_MUSIC,
            STREAM_ALARM,
            STREAM_NOTIFICATION,
            STREAM_DTMF,
            STREAM_ACCESSIBILITY,
        };
        mOriginalRingerMode = mAudioManager.getRingerMode();
        for (int streamType : streamTypes) {
            mOriginalStreamVolumes.put(streamType, mAudioManager.getStreamVolume(streamType));
        }

        try {
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            mOriginalNotificationPolicy = mNm.getNotificationPolicy();
            mOriginalZen = mNm.getCurrentInterruptionFilter();
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), false);
        }

        // Check original microphone mute/unmute status
        mDoNotCheckUnmute = false;
        if (mAudioManager.isMicrophoneMute()) {
            mAudioManager.setMicrophoneMute(false);
            if (mAudioManager.isMicrophoneMute()) {
                Log.w(TAG, "Mic seems muted by hardware! Please unmute and rerrun the test.");
                mDoNotCheckUnmute = true;
            }
        }
    }

    @Override
    protected void tearDown() throws Exception {
        try {
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            mNm.setNotificationPolicy(mOriginalNotificationPolicy);
            setInterruptionFilter(mOriginalZen);

            // Recover the volume and the ringer mode that the test may have overwritten.
            for (Map.Entry<Integer, Integer> e : mOriginalStreamVolumes.entrySet()) {
                mAudioManager.setStreamVolume(e.getKey(), e.getValue(),
                                              AudioManager.FLAG_ALLOW_RINGER_MODES);
            }
            mAudioManager.setRingerMode(mOriginalRingerMode);
        } finally {
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), false);
        }
    }

    @AppModeFull(reason = "Instant apps cannot hold android.permission.MODIFY_AUDIO_SETTINGS")
    public void testMicrophoneMute() throws Exception {
        mAudioManager.setMicrophoneMute(true);
        assertTrue(mAudioManager.isMicrophoneMute());
        mAudioManager.setMicrophoneMute(false);
        assertFalse(mAudioManager.isMicrophoneMute() && !mDoNotCheckUnmute);
    }

    @AppModeFull(reason = "Instant apps cannot hold android.permission.MODIFY_AUDIO_SETTINGS")
    public void testMicrophoneMuteIntent() throws Exception {
        if (!mDoNotCheckUnmute) {
            final MyBlockingIntentReceiver receiver = new MyBlockingIntentReceiver(
                    AudioManager.ACTION_MICROPHONE_MUTE_CHANGED);
            final boolean initialMicMute = mAudioManager.isMicrophoneMute();
            try {
                mContext.registerReceiver(receiver,
                        new IntentFilter(AudioManager.ACTION_MICROPHONE_MUTE_CHANGED));
                // change the mic mute state
                mAudioManager.setMicrophoneMute(!initialMicMute);
                // verify a change was reported
                final boolean intentFired = receiver.waitForExpectedAction(500/*ms*/);
                assertTrue("ACTION_MICROPHONE_MUTE_CHANGED wasn't fired", intentFired);
                // verify the mic mute state is expected
                final boolean newMicMute = mAudioManager.isMicrophoneMute();
                assertTrue("new mic mute state not as expected (" + !initialMicMute + ")",
                        (newMicMute == !initialMicMute));
            } finally {
                mContext.unregisterReceiver(receiver);
                mAudioManager.setMicrophoneMute(initialMicMute);
            }
        }
    }

    @AppModeFull(reason = "Instant apps cannot hold android.permission.MODIFY_AUDIO_SETTINGS")
    public void testSpeakerphoneIntent() throws Exception {
        final MyBlockingIntentReceiver receiver = new MyBlockingIntentReceiver(
                AudioManager.ACTION_SPEAKERPHONE_STATE_CHANGED);
        final boolean initialSpeakerphoneState = mAudioManager.isSpeakerphoneOn();
        try {
            mContext.registerReceiver(receiver,
                    new IntentFilter(AudioManager.ACTION_SPEAKERPHONE_STATE_CHANGED));
            // change the speakerphone state
            mAudioManager.setSpeakerphoneOn(!initialSpeakerphoneState);
            // verify a change was reported
            final boolean intentFired = receiver.waitForExpectedAction(500/*ms*/);
            assertTrue("ACTION_SPEAKERPHONE_STATE_CHANGED wasn't fired", intentFired);
            // verify the speakerphon state is expected
            final boolean newSpeakerphoneState = mAudioManager.isSpeakerphoneOn();
            assertTrue("new mic mute state not as expected ("
                    + !initialSpeakerphoneState + ")",
                    newSpeakerphoneState == !initialSpeakerphoneState);
        } finally {
            mContext.unregisterReceiver(receiver);
            mAudioManager.setSpeakerphoneOn(initialSpeakerphoneState);
        }
    }

    private static final class MyBlockingIntentReceiver extends BroadcastReceiver {
        private final SafeWaitObject mLock = new SafeWaitObject();
        // the action for the intent to check
        private final String mAction;
        @GuardedBy("mLock")
        private boolean mIntentReceived = false;

        MyBlockingIntentReceiver(String action) {
            mAction = action;
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            if (!TextUtils.equals(intent.getAction(), mAction)) {
                // move along, this is not the action we're looking for
                return;
            }
            synchronized (mLock) {
                mIntentReceived = true;
                mLock.safeNotify();
            }
        }

        public boolean waitForExpectedAction(long timeOutMs) {
            synchronized (mLock) {
                try {
                    mLock.safeWait(timeOutMs);
                } catch (InterruptedException e) { }
                return mIntentReceived;
            }
        }
    }

    public void testSoundEffects() throws Exception {
        Settings.System.putInt(mContext.getContentResolver(), SOUND_EFFECTS_ENABLED, 1);

        // should hear sound after loadSoundEffects() called.
        mAudioManager.loadSoundEffects();
        Thread.sleep(TIME_TO_PLAY);
        float volume = 0.5f;  // volume should be between 0.f to 1.f (or -1).
        mAudioManager.playSoundEffect(SoundEffectConstants.CLICK);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_UP);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_DOWN);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_LEFT);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_RIGHT);

        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_UP, volume);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_DOWN, volume);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_LEFT, volume);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_RIGHT, volume);

        // won't hear sound after unloadSoundEffects() called();
        mAudioManager.unloadSoundEffects();
        mAudioManager.playSoundEffect(AudioManager.FX_KEY_CLICK);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_UP);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_DOWN);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_LEFT);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_RIGHT);

        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_UP, volume);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_DOWN, volume);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_LEFT, volume);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_RIGHT, volume);
    }

    public void testCheckingZenModeBlockDoesNotRequireNotificationPolicyAccess() throws Exception {
        try {
            // set zen mode to priority only, so playSoundEffect will check notification policy
            Utils.toggleNotificationPolicyAccess(mContext.getPackageName(), getInstrumentation(),
                    true);
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);
            Settings.System.putInt(mContext.getContentResolver(), SOUND_EFFECTS_ENABLED, 1);

            // take away write-notification policy access from the package
            Utils.toggleNotificationPolicyAccess(mContext.getPackageName(), getInstrumentation(),
                    false);

            // playSoundEffect should NOT throw a security exception; all apps have read-access
            mAudioManager.playSoundEffect(SoundEffectConstants.CLICK);
        } finally {
            Utils.toggleNotificationPolicyAccess(mContext.getPackageName(), getInstrumentation(),
                    true);
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
            Utils.toggleNotificationPolicyAccess(mContext.getPackageName(), getInstrumentation(),
                    false);
        }
    }

    public void testMusicActive() throws Exception {
        if (mAudioManager.isMusicActive()) {
            return;
        }
        MediaPlayer mp = MediaPlayer.create(mContext, MP3_TO_PLAY);
        assertNotNull(mp);
        mp.setAudioStreamType(STREAM_MUSIC);
        mp.start();
        assertMusicActive(true);
        mp.stop();
        mp.release();
        assertMusicActive(false);
    }

    @AppModeFull(reason = "Instant apps cannot hold android.permission.MODIFY_AUDIO_SETTINGS")
    public void testAccessMode() throws Exception {
        mAudioManager.setMode(MODE_RINGTONE);
        assertEquals(MODE_RINGTONE, mAudioManager.getMode());
        mAudioManager.setMode(MODE_IN_COMMUNICATION);
        assertEquals(MODE_IN_COMMUNICATION, mAudioManager.getMode());
        mAudioManager.setMode(MODE_NORMAL);
        assertEquals(MODE_NORMAL, mAudioManager.getMode());
    }

    public void testSetSurroundFormatEnabled() throws Exception {
        getInstrumentation().getUiAutomation().adoptShellPermissionIdentity(
                Manifest.permission.WRITE_SETTINGS);

        int audioFormat = AudioFormat.ENCODING_DTS;

        mAudioManager.setSurroundFormatEnabled(audioFormat, true /*enabled*/);
        assertTrue(mAudioManager.isSurroundFormatEnabled(audioFormat));

        mAudioManager.setSurroundFormatEnabled(audioFormat, false /*enabled*/);
        assertFalse(mAudioManager.isSurroundFormatEnabled(audioFormat));

        getInstrumentation().getUiAutomation().dropShellPermissionIdentity();
    }

    public void testSetEncodedSurroundMode() throws Exception {
        getInstrumentation().getUiAutomation().adoptShellPermissionIdentity(
                Manifest.permission.WRITE_SETTINGS);

        int expectedSurroundFormatsMode = Settings.Global.ENCODED_SURROUND_OUTPUT_MANUAL;
        mAudioManager.setEncodedSurroundMode(expectedSurroundFormatsMode);
        assertEquals(expectedSurroundFormatsMode, mAudioManager.getEncodedSurroundMode());

        expectedSurroundFormatsMode = Settings.Global.ENCODED_SURROUND_OUTPUT_NEVER;
        mAudioManager.setEncodedSurroundMode(expectedSurroundFormatsMode);
        assertEquals(expectedSurroundFormatsMode, mAudioManager.getEncodedSurroundMode());

        getInstrumentation().getUiAutomation().dropShellPermissionIdentity();
    }

    @SuppressWarnings("deprecation")
    @AppModeFull(reason = "Instant apps cannot hold android.permission.MODIFY_AUDIO_SETTINGS")
    public void testRouting() throws Exception {
        // setBluetoothA2dpOn is a no-op, and getRouting should always return -1
        boolean oldA2DP = mAudioManager.isBluetoothA2dpOn();
        mAudioManager.setBluetoothA2dpOn(true);
        assertEquals(oldA2DP , mAudioManager.isBluetoothA2dpOn());
        mAudioManager.setBluetoothA2dpOn(false);
        assertEquals(oldA2DP , mAudioManager.isBluetoothA2dpOn());

        assertEquals(-1, mAudioManager.getRouting(MODE_RINGTONE));
        assertEquals(-1, mAudioManager.getRouting(MODE_NORMAL));
        assertEquals(-1, mAudioManager.getRouting(MODE_IN_CALL));
        assertEquals(-1, mAudioManager.getRouting(MODE_IN_COMMUNICATION));

        mAudioManager.setBluetoothScoOn(true);
        assertTrueCheckTimeout(mAudioManager, p -> p.isBluetoothScoOn(),
                DEFAULT_ASYNC_CALL_TIMEOUT_MS, "isBluetoothScoOn returned false");

        mAudioManager.setBluetoothScoOn(false);
        assertTrueCheckTimeout(mAudioManager, p -> !p.isBluetoothScoOn(),
                DEFAULT_ASYNC_CALL_TIMEOUT_MS, "isBluetoothScoOn returned true");

        mAudioManager.setSpeakerphoneOn(true);
        assertTrueCheckTimeout(mAudioManager, p -> p.isSpeakerphoneOn(),
                DEFAULT_ASYNC_CALL_TIMEOUT_MS, "isSpeakerPhoneOn() returned false");

        mAudioManager.setSpeakerphoneOn(false);
        assertTrueCheckTimeout(mAudioManager, p -> !p.isSpeakerphoneOn(),
                DEFAULT_ASYNC_CALL_TIMEOUT_MS, "isSpeakerPhoneOn() returned true");
    }

    public void testVibrateNotification() throws Exception {
        if (mUseFixedVolume || !mHasVibrator) {
            return;
        }
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        // VIBRATE_SETTING_ON
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_ON);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ON : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION));
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertEquals(mHasVibrator, mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
        assertEquals(mHasVibrator ? RINGER_MODE_VIBRATE : RINGER_MODE_SILENT,
                mAudioManager.getRingerMode());
        assertEquals(mHasVibrator, mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        // VIBRATE_SETTING_OFF
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_OFF);
        assertEquals(VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION));
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
        assertEquals(mHasVibrator ? RINGER_MODE_VIBRATE : RINGER_MODE_SILENT,
                mAudioManager.getRingerMode());
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        // VIBRATE_SETTING_ONLY_SILENT
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_ONLY_SILENT);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ONLY_SILENT : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION));
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
        assertEquals(mHasVibrator ? RINGER_MODE_VIBRATE : RINGER_MODE_SILENT,
                mAudioManager.getRingerMode());
        assertEquals(mHasVibrator, mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        // VIBRATE_TYPE_NOTIFICATION
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_ON);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ON : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION));
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_OFF);
        assertEquals(VIBRATE_SETTING_OFF, mAudioManager
                .getVibrateSetting(VIBRATE_TYPE_NOTIFICATION));
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_ONLY_SILENT);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ONLY_SILENT : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION));
    }

    public void testVibrateRinger() throws Exception {
        if (mUseFixedVolume || !mHasVibrator) {
            return;
        }
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        // VIBRATE_TYPE_RINGER
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_ON);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ON : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER));
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertEquals(mHasVibrator, mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
        assertEquals(mHasVibrator ? RINGER_MODE_VIBRATE : RINGER_MODE_SILENT,
                mAudioManager.getRingerMode());
        assertEquals(mHasVibrator, mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        // VIBRATE_SETTING_OFF
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_OFF);
        assertEquals(VIBRATE_SETTING_OFF, mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER));
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
        assertEquals(mHasVibrator ? RINGER_MODE_VIBRATE : RINGER_MODE_SILENT,
                mAudioManager.getRingerMode());
        // Note: as of Froyo, if VIBRATE_TYPE_RINGER is set to OFF, it will
        // not vibrate, even in RINGER_MODE_VIBRATE. This allows users to
        // disable the vibration for incoming calls only.
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        // VIBRATE_SETTING_ONLY_SILENT
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_ONLY_SILENT);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ONLY_SILENT : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER));
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
        assertEquals(mHasVibrator ? RINGER_MODE_VIBRATE : RINGER_MODE_SILENT,
                mAudioManager.getRingerMode());
        assertEquals(mHasVibrator, mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        // VIBRATE_TYPE_NOTIFICATION
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_ON);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ON : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER));
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_OFF);
        assertEquals(VIBRATE_SETTING_OFF, mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER));
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_ONLY_SILENT);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ONLY_SILENT : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER));
    }

    public void testAccessRingMode() throws Exception {
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertEquals(RINGER_MODE_NORMAL, mAudioManager.getRingerMode());

        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        // AudioService#setRingerMode() has:
        // if (isTelevision) return;
        if (mSkipRingerTests) {
            assertEquals(RINGER_MODE_NORMAL, mAudioManager.getRingerMode());
        } else {
            assertEquals(RINGER_MODE_SILENT, mAudioManager.getRingerMode());
        }

        mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
        if (mSkipRingerTests) {
            assertEquals(RINGER_MODE_NORMAL, mAudioManager.getRingerMode());
        } else {
            assertEquals(mHasVibrator ? RINGER_MODE_VIBRATE : RINGER_MODE_SILENT,
                    mAudioManager.getRingerMode());
        }
    }

    public void testSetRingerModePolicyAccess() throws Exception {
        if (mSkipRingerTests) {
            return;
        }
        // Apps without policy access cannot change silent -> normal or silent -> vibrate.
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        assertEquals(RINGER_MODE_SILENT, mAudioManager.getRingerMode());
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), false);

        try {
            mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
            fail("Apps without notification policy access cannot change ringer mode");
        } catch (SecurityException e) {
        }

        try {
            mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
            fail("Apps without notification policy access cannot change ringer mode");
        } catch (SecurityException e) {
        }

        // Apps without policy access cannot change normal -> silent.
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertEquals(RINGER_MODE_NORMAL, mAudioManager.getRingerMode());
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), false);

        try {
            mAudioManager.setRingerMode(RINGER_MODE_SILENT);
            fail("Apps without notification policy access cannot change ringer mode");
        } catch (SecurityException e) {
        }
        assertEquals(RINGER_MODE_NORMAL, mAudioManager.getRingerMode());

        if (mHasVibrator) {
            // Apps without policy access cannot change vibrate -> silent.
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
            assertEquals(RINGER_MODE_VIBRATE, mAudioManager.getRingerMode());
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), false);

            try {
                mAudioManager.setRingerMode(RINGER_MODE_SILENT);
                fail("Apps without notification policy access cannot change ringer mode");
            } catch (SecurityException e) {
            }

            // Apps without policy access can change vibrate -> normal and vice versa.
            assertEquals(RINGER_MODE_VIBRATE, mAudioManager.getRingerMode());
            mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
            assertEquals(RINGER_MODE_NORMAL, mAudioManager.getRingerMode());
            mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
            assertEquals(RINGER_MODE_VIBRATE, mAudioManager.getRingerMode());
        }
    }

    public void testVolume() throws Exception {
        if (MediaUtils.check(mIsTelevision, "No volume test due to fixed/full vol devices"))
            return;
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        int volume, volumeDelta;
        int[] streams = {STREAM_ALARM,
                STREAM_MUSIC,
                STREAM_VOICE_CALL,
                STREAM_RING};

        mAudioManager.adjustVolume(ADJUST_RAISE, 0);
        // adjusting volume is asynchronous, wait before other volume checks
        Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
        mAudioManager.adjustSuggestedStreamVolume(
                ADJUST_LOWER, USE_DEFAULT_STREAM_TYPE, 0);
        Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
        int maxMusicVolume = mAudioManager.getStreamMaxVolume(STREAM_MUSIC);

        for (int stream : streams) {
            if (mIsSingleVolume && stream != STREAM_MUSIC) {
                continue;
            }

            // set ringer mode to back normal to not interfere with volume tests
            mAudioManager.setRingerMode(RINGER_MODE_NORMAL);

            int maxVolume = mAudioManager.getStreamMaxVolume(stream);
            int minVolume = mAudioManager.getStreamMinVolume(stream);

            // validate min
            assertTrue(String.format("minVolume(%d) must be >= 0", minVolume), minVolume >= 0);
            assertTrue(String.format("minVolume(%d) must be < maxVolume(%d)", minVolume,
                    maxVolume),
                    minVolume < maxVolume);

            final int minNonZeroVolume = Math.max(minVolume, 1);
            mAudioManager.setStreamVolume(stream, minNonZeroVolume, 0);
            if (mUseFixedVolume) {
                assertEquals(maxVolume, mAudioManager.getStreamVolume(stream));
                continue;
            }
            assertEquals(String.format("stream=%d", stream),
                    minNonZeroVolume, mAudioManager.getStreamVolume(stream));

            if (stream == STREAM_MUSIC && mAudioManager.isWiredHeadsetOn()) {
                // due to new regulations, music sent over a wired headset may be volume limited
                // until the user explicitly increases the limit, so we can't rely on being able
                // to set the volume to getStreamMaxVolume(). Instead, determine the current limit
                // by increasing the volume until it won't go any higher, then use that volume as
                // the maximum for the purposes of this test
                int curvol = 0;
                int prevvol = 0;
                do {
                    prevvol = curvol;
                    mAudioManager.adjustStreamVolume(stream, ADJUST_RAISE, 0);
                    curvol = mAudioManager.getStreamVolume(stream);
                } while (curvol != prevvol);
                maxVolume = maxMusicVolume = curvol;
            }
            mAudioManager.setStreamVolume(stream, maxVolume, 0);
            mAudioManager.adjustStreamVolume(stream, ADJUST_RAISE, 0);
            assertEquals(maxVolume, mAudioManager.getStreamVolume(stream));

            volumeDelta = getVolumeDelta(mAudioManager.getStreamVolume(stream));
            mAudioManager.adjustSuggestedStreamVolume(ADJUST_LOWER, stream, 0);
            assertStreamVolumeEquals(stream, maxVolume - volumeDelta,
                    "Vol ADJUST_LOWER suggested stream:" + stream + " maxVol:" + maxVolume);

            // volume lower
            mAudioManager.setStreamVolume(stream, maxVolume, 0);
            volume = mAudioManager.getStreamVolume(stream);
            while (volume > minVolume) {
                volumeDelta = getVolumeDelta(mAudioManager.getStreamVolume(stream));
                mAudioManager.adjustStreamVolume(stream, ADJUST_LOWER, 0);
                assertStreamVolumeEquals(stream,  Math.max(0, volume - volumeDelta),
                        "Vol ADJUST_LOWER on stream:" + stream + " vol:" + volume
                                + " minVol:" + minVolume + " volDelta:" + volumeDelta);
                volume = mAudioManager.getStreamVolume(stream);
            }

            mAudioManager.adjustStreamVolume(stream, ADJUST_SAME, 0);

            // volume raise
            mAudioManager.setStreamVolume(stream, minNonZeroVolume, 0);
            volume = mAudioManager.getStreamVolume(stream);
            while (volume < maxVolume) {
                volumeDelta = getVolumeDelta(mAudioManager.getStreamVolume(stream));
                mAudioManager.adjustStreamVolume(stream, ADJUST_RAISE, 0);
                assertStreamVolumeEquals(stream,   Math.min(volume + volumeDelta, maxVolume),
                        "Vol ADJUST_RAISE on stream:" + stream + " vol:" + volume
                                + " maxVol:" + maxVolume + " volDelta:" + volumeDelta);
                volume = mAudioManager.getStreamVolume(stream);
            }

            // volume same
            mAudioManager.setStreamVolume(stream, maxVolume, 0);
            for (int k = 0; k < maxVolume; k++) {
                mAudioManager.adjustStreamVolume(stream, ADJUST_SAME, 0);
                assertEquals("Vol ADJUST_RAISE onADJUST_SAME stream:" + stream,
                        maxVolume, mAudioManager.getStreamVolume(stream));
            }

            mAudioManager.setStreamVolume(stream, maxVolume, 0);
        }

        if (mUseFixedVolume) {
            return;
        }

        // adjust volume
        mAudioManager.adjustVolume(ADJUST_RAISE, 0);
        Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);

        boolean isMusicPlayingBeforeTest = false;
        if (mAudioManager.isMusicActive()) {
            isMusicPlayingBeforeTest = true;
        }

        MediaPlayer mp = MediaPlayer.create(mContext, MP3_TO_PLAY);
        assertNotNull(mp);
        mp.setAudioStreamType(STREAM_MUSIC);
        mp.setLooping(true);
        mp.start();
        assertMusicActive(true);

        // adjust volume as ADJUST_SAME
        for (int k = 0; k < maxMusicVolume; k++) {
            mAudioManager.adjustVolume(ADJUST_SAME, 0);
            assertStreamVolumeEquals(STREAM_MUSIC, maxMusicVolume);
        }

        // adjust volume as ADJUST_RAISE
        mAudioManager.setStreamVolume(STREAM_MUSIC, 0, 0);
        volumeDelta = getVolumeDelta(mAudioManager.getStreamVolume(STREAM_MUSIC));
        mAudioManager.adjustVolume(ADJUST_RAISE, 0);
        assertStreamVolumeEquals(STREAM_MUSIC, Math.min(volumeDelta, maxMusicVolume));

        // adjust volume as ADJUST_LOWER
        mAudioManager.setStreamVolume(STREAM_MUSIC, maxMusicVolume, 0);
        maxMusicVolume = mAudioManager.getStreamVolume(STREAM_MUSIC);
        volumeDelta = getVolumeDelta(mAudioManager.getStreamVolume(STREAM_MUSIC));
        mAudioManager.adjustVolume(ADJUST_LOWER, 0);
        assertStreamVolumeEquals(STREAM_MUSIC, Math.max(0, maxMusicVolume - volumeDelta));

        mp.stop();
        mp.release();
        if (!isMusicPlayingBeforeTest) {
            assertMusicActive(false);
        }
    }

    public void testAccessibilityVolume() throws Exception {
        if (mUseFixedVolume) {
            Log.i("AudioManagerTest", "testAccessibilityVolume() skipped: fixed volume");
            return;
        }
        final int maxA11yVol = mAudioManager.getStreamMaxVolume(STREAM_ACCESSIBILITY);
        assertTrue("Max a11yVol not strictly positive", maxA11yVol > 0);
        int originalVol = mAudioManager.getStreamVolume(STREAM_ACCESSIBILITY);

        // changing STREAM_ACCESSIBILITY is subject to permission, shouldn't be able to change it
        // test setStreamVolume
        final int testSetVol;
        if (originalVol != maxA11yVol) {
            testSetVol = maxA11yVol;
        } else {
            testSetVol = maxA11yVol - 1;
        }
        mAudioManager.setStreamVolume(STREAM_ACCESSIBILITY, testSetVol, 0);
        assertStreamVolumeEquals(STREAM_ACCESSIBILITY, originalVol,
                "Should not be able to change A11y vol");

        // test adjustStreamVolume
        //        LOWER
        if (originalVol > 0) {
            mAudioManager.adjustStreamVolume(STREAM_ACCESSIBILITY, ADJUST_LOWER, 0);
            assertStreamVolumeEquals(STREAM_ACCESSIBILITY, originalVol,
                    "Should not be able to change A11y vol");
        }
        //        RAISE
        if (originalVol < maxA11yVol) {
            mAudioManager.adjustStreamVolume(STREAM_ACCESSIBILITY, ADJUST_RAISE, 0);
            assertStreamVolumeEquals(STREAM_ACCESSIBILITY, originalVol,
                    "Should not be able to change A11y vol");
        }
    }

    public void testSetVoiceCallVolumeToZeroPermission() {
        // Verify that only apps with MODIFY_PHONE_STATE can set VOICE_CALL_STREAM to 0
        mAudioManager.setStreamVolume(STREAM_VOICE_CALL, 0, 0);
        assertTrue("MODIFY_PHONE_STATE is required in order to set voice call volume to 0",
                    mAudioManager.getStreamVolume(STREAM_VOICE_CALL) != 0);
    }

    public void testMuteFixedVolume() throws Exception {
        int[] streams = {
                STREAM_VOICE_CALL,
                STREAM_MUSIC,
                STREAM_RING,
                STREAM_ALARM,
                STREAM_NOTIFICATION,
                STREAM_SYSTEM};
        if (mUseFixedVolume) {
            for (int stream : streams) {
                mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_MUTE, 0);
                assertFalse("Muting should not affect a fixed volume device.",
                        mAudioManager.isStreamMute(stream));

                mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_TOGGLE_MUTE, 0);
                assertFalse("Toggling mute should not affect a fixed volume device.",
                        mAudioManager.isStreamMute(stream));

                mAudioManager.setStreamMute(stream, true);
                assertFalse("Muting should not affect a fixed volume device.",
                        mAudioManager.isStreamMute(stream));
            }
        }
    }

    public void testMuteDndAffectedStreams() throws Exception {
        if (mSkipRingerTests) {
            return;
        }
        int[] streams = { STREAM_RING };
        // Mute streams
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), false);
        // Verify streams cannot be unmuted without policy access.
        for (int stream : streams) {
            try {
                mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_UNMUTE, 0);
                assertEquals("Apps without Notification policy access can't change ringer mode",
                        RINGER_MODE_SILENT, mAudioManager.getRingerMode());
            } catch (SecurityException e) {
            }

            try {
                mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_TOGGLE_MUTE,
                        0);
                assertEquals("Apps without Notification policy access can't change ringer mode",
                        RINGER_MODE_SILENT, mAudioManager.getRingerMode());
            } catch (SecurityException e) {
            }

            try {
                mAudioManager.setStreamMute(stream, false);
                assertEquals("Apps without Notification policy access can't change ringer mode",
                        RINGER_MODE_SILENT, mAudioManager.getRingerMode());
            } catch (SecurityException e) {
            }
        }

        // This ensures we're out of vibrate or silent modes.
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        for (int stream : streams) {
            // ensure each stream is on and turned up.
            mAudioManager.setStreamVolume(stream,
                    mAudioManager.getStreamMaxVolume(stream),
                    0);

            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), false);
            try {
                mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_MUTE, 0);
                assertEquals("Apps without Notification policy access can't change ringer mode",
                        RINGER_MODE_NORMAL, mAudioManager.getRingerMode());
            } catch (SecurityException e) {
            }
            try {
                mAudioManager.adjustStreamVolume(
                        stream, AudioManager.ADJUST_TOGGLE_MUTE, 0);
                assertEquals("Apps without Notification policy access can't change ringer mode",
                        RINGER_MODE_NORMAL, mAudioManager.getRingerMode());
            } catch (SecurityException e) {
            }

            try {
                mAudioManager.setStreamMute(stream, true);
                assertEquals("Apps without Notification policy access can't change ringer mode",
                        RINGER_MODE_NORMAL, mAudioManager.getRingerMode());
            } catch (SecurityException e) {
            }
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            testStreamMuting(stream);
        }
    }

    public void testMuteDndUnaffectedStreams() throws Exception {
        if (mSkipRingerTests) {
            return;
        }
        int[] streams = {
                STREAM_VOICE_CALL,
                STREAM_MUSIC,
                STREAM_ALARM
        };

        int muteAffectedStreams = System.getInt(mContext.getContentResolver(),
                System.MUTE_STREAMS_AFFECTED,
                // same defaults as in AudioService. Should be kept in sync.
                 (1 << STREAM_MUSIC) |
                         (1 << STREAM_RING) |
                         (1 << STREAM_NOTIFICATION) |
                         (1 << STREAM_SYSTEM) |
                         (1 << STREAM_VOICE_CALL));

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        // This ensures we're out of vibrate or silent modes.
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), false);
        for (int stream : streams) {
            // ensure each stream is on and turned up.
            mAudioManager.setStreamVolume(stream,
                    mAudioManager.getStreamMaxVolume(stream),
                    0);
            if (((1 << stream) & muteAffectedStreams) == 0) {
                mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_MUTE, 0);
                assertFalse("Stream " + stream + " should not be affected by mute.",
                        mAudioManager.isStreamMute(stream));
                mAudioManager.setStreamMute(stream, true);
                assertFalse("Stream " + stream + " should not be affected by mute.",
                        mAudioManager.isStreamMute(stream));
                mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_TOGGLE_MUTE,
                        0);
                assertFalse("Stream " + stream + " should not be affected by mute.",
                        mAudioManager.isStreamMute(stream));
                continue;
            }
            testStreamMuting(stream);
        }
    }

    private void testStreamMuting(int stream) {
        // Voice call requires MODIFY_PHONE_STATE, so we should not be able to mute
        if (stream == STREAM_VOICE_CALL) {
            mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_MUTE, 0);
            assertFalse("Muting voice call stream (" + stream + ") should require "
                            + "MODIFY_PHONE_STATE.", mAudioManager.isStreamMute(stream));
        } else {
            mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_MUTE, 0);
            assertTrue("Muting stream " + stream + " failed.",
                    mAudioManager.isStreamMute(stream));

            mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_UNMUTE, 0);
            assertFalse("Unmuting stream " + stream + " failed.",
                    mAudioManager.isStreamMute(stream));

            mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_TOGGLE_MUTE, 0);
            assertTrue("Toggling mute on stream " + stream + " failed.",
                    mAudioManager.isStreamMute(stream));

            mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_TOGGLE_MUTE, 0);
            assertFalse("Toggling mute on stream " + stream + " failed.",
                    mAudioManager.isStreamMute(stream));

            mAudioManager.setStreamMute(stream, true);
            assertTrue("Muting stream " + stream + " using setStreamMute failed",
                    mAudioManager.isStreamMute(stream));

            // mute it three more times to verify the ref counting is gone.
            mAudioManager.setStreamMute(stream, true);
            mAudioManager.setStreamMute(stream, true);
            mAudioManager.setStreamMute(stream, true);

            mAudioManager.setStreamMute(stream, false);
            assertFalse("Unmuting stream " + stream + " using setStreamMute failed.",
                    mAudioManager.isStreamMute(stream));
        }
    }

    public void testSetInvalidRingerMode() {
        int ringerMode = mAudioManager.getRingerMode();
        mAudioManager.setRingerMode(-1337);
        assertEquals(ringerMode, mAudioManager.getRingerMode());

        mAudioManager.setRingerMode(-3007);
        assertEquals(ringerMode, mAudioManager.getRingerMode());
    }

    public void testAdjustVolumeInTotalSilenceMode() throws Exception {
        if (mSkipRingerTests) {
            return;
        }
        try {
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            mAudioManager.setStreamVolume(STREAM_MUSIC, 1, 0);
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_NONE);
            Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
            int musicVolume = mAudioManager.getStreamVolume(STREAM_MUSIC);
            mAudioManager.adjustStreamVolume(
                    STREAM_MUSIC, ADJUST_RAISE, 0);
            assertStreamVolumeEquals(STREAM_MUSIC, musicVolume);

        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testAdjustVolumeInAlarmsOnlyMode() throws Exception {
        if (mSkipRingerTests) {
            return;
        }
        try {
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            mAudioManager.setStreamVolume(STREAM_MUSIC, 1, 0);

            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALARMS);
            Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
            int musicVolume = mAudioManager.getStreamVolume(STREAM_MUSIC);
            mAudioManager.adjustStreamVolume(
                    STREAM_MUSIC, ADJUST_RAISE, 0);
            int volumeDelta =
                    getVolumeDelta(mAudioManager.getStreamVolume(STREAM_MUSIC));
            assertStreamVolumeEquals(STREAM_MUSIC, musicVolume + volumeDelta);

        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testSetStreamVolumeInTotalSilenceMode() throws Exception {
        if (mSkipRingerTests) {
            return;
        }
        try {
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            mAudioManager.setStreamVolume(STREAM_RING, 1, 0);
            mAudioManager.setStreamVolume(STREAM_MUSIC, 1, 0);

            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_NONE);
            // delay for streams interruption filter to get into correct state
            Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);

            // cannot adjust music, can adjust ringer since it could exit DND
            int musicVolume = mAudioManager.getStreamVolume(STREAM_MUSIC);
            mAudioManager.setStreamVolume(STREAM_MUSIC, 7, 0);
            assertStreamVolumeEquals(STREAM_MUSIC, musicVolume);
            mAudioManager.setStreamVolume(STREAM_RING, 7, 0);
            assertStreamVolumeEquals(STREAM_RING, 7);
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testSetStreamVolumeInAlarmsOnlyMode() throws Exception {
        if (mSkipRingerTests) {
            return;
        }
        try {
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            mAudioManager.setStreamVolume(STREAM_RING, 1, 0);
            mAudioManager.setStreamVolume(STREAM_MUSIC, 1, 0);
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALARMS);
            // delay for streams to get into correct volume states
            Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);

            // can still adjust music and ring volume
            mAudioManager.setStreamVolume(STREAM_MUSIC, 3, 0);
            assertStreamVolumeEquals(STREAM_MUSIC, 3);
            mAudioManager.setStreamVolume(STREAM_RING, 7, 0);
            assertStreamVolumeEquals(STREAM_RING, 7);
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testSetStreamVolumeInPriorityOnlyMode() throws Exception {
        if (mSkipRingerTests) {
            return;
        }
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);

        try {
            // turn off zen, set stream volumes to check for later
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);

            final int testRingerVol = getTestRingerVol();
            mAudioManager.setStreamVolume(STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(STREAM_ALARM, 1, 0);
            int musicVolume = mAudioManager.getStreamVolume(STREAM_MUSIC);
            int alarmVolume = mAudioManager.getStreamVolume(STREAM_ALARM);

            // disallow all sounds in priority only, turn on priority only DND, try to change volume
            mNm.setNotificationPolicy(new NotificationManager.Policy(0, 0 , 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);
            // delay for streams to get into correct volume states
            Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
            mAudioManager.setStreamVolume(STREAM_MUSIC, 3, 0);
            mAudioManager.setStreamVolume(STREAM_ALARM, 5, 0);
            mAudioManager.setStreamVolume(STREAM_RING, testRingerVol, 0);

            // Turn off zen and make sure stream levels are still the same prior to zen
            // aside from ringer since ringer can exit dnd
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
            Thread.sleep(ASYNC_TIMING_TOLERANCE_MS); // delay for streams to get into correct states
            assertEquals(musicVolume, mAudioManager.getStreamVolume(STREAM_MUSIC));
            assertEquals(alarmVolume, mAudioManager.getStreamVolume(STREAM_ALARM));
            assertEquals(testRingerVol, mAudioManager.getStreamVolume(STREAM_RING));
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testAdjustVolumeInPriorityOnly() throws Exception {
        if (mSkipRingerTests) {
            return;
        }

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        try {
            // turn off zen, set stream volumes to check for later
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
            mAudioManager.setStreamVolume(STREAM_RING, 1, 0);
            mAudioManager.setStreamVolume(STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(STREAM_ALARM, 1, 0);
            int ringVolume = mAudioManager.getStreamVolume(STREAM_RING);
            int musicVolume = mAudioManager.getStreamVolume(STREAM_MUSIC);
            int alarmVolume = mAudioManager.getStreamVolume(STREAM_ALARM);

            // disallow all sounds in priority only, turn on priority only DND, try to change volume
            mNm.setNotificationPolicy(new NotificationManager.Policy(0, 0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);
            // delay for streams to get into correct mute states
            Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
            mAudioManager.adjustStreamVolume(
                    STREAM_RING, ADJUST_RAISE, 0);
            mAudioManager.adjustStreamVolume(
                    STREAM_MUSIC, ADJUST_RAISE, 0);
            mAudioManager.adjustStreamVolume(
                    STREAM_ALARM, ADJUST_RAISE, 0);

            // Turn off zen and make sure stream levels are still the same prior to zen
            // aside from ringer since ringer can exit dnd
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
            Thread.sleep(ASYNC_TIMING_TOLERANCE_MS); // delay for streams to get into correct states
            assertEquals(musicVolume, mAudioManager.getStreamVolume(STREAM_MUSIC));
            assertEquals(alarmVolume, mAudioManager.getStreamVolume(STREAM_ALARM));

            int volumeDelta =
                    getVolumeDelta(mAudioManager.getStreamVolume(STREAM_RING));
            assertEquals(ringVolume + volumeDelta,
                    mAudioManager.getStreamVolume(STREAM_RING));
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testPriorityOnlyMuteAll() throws Exception {
        if (mSkipRingerTests) {
            return;
        }

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        try {
            // ensure volume is not muted/0 to start test
            mAudioManager.setStreamVolume(STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(STREAM_ALARM, 1, 0);
            mAudioManager.setStreamVolume(STREAM_SYSTEM, 1, 0);
            mAudioManager.setStreamVolume(STREAM_RING, 1, 0);

            // disallow all sounds in priority only, turn on priority only DND
            mNm.setNotificationPolicy(new NotificationManager.Policy(0, 0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);

            assertStreamMuted(STREAM_MUSIC, true,
                    "Music (media) stream should be muted");
            assertStreamMuted(STREAM_SYSTEM, true,
                    "System stream should be muted");
            assertStreamMuted(STREAM_ALARM, true,
                    "Alarm stream should be muted");

            // if channels cannot bypass DND, the Ringer stream should be muted, else it
            // shouldn't be muted
            assertStreamMuted(STREAM_RING, !mAppsBypassingDnd,
                    "Ringer stream should be muted if channels cannot bypassDnd");
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testPriorityOnlyMediaAllowed() throws Exception {
        if (mSkipRingerTests) {
            return;
        }
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        try {
            // ensure volume is not muted/0 to start test
            mAudioManager.setStreamVolume(STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(STREAM_ALARM, 1, 0);
            mAudioManager.setStreamVolume(STREAM_SYSTEM, 1, 0);
            mAudioManager.setStreamVolume(STREAM_RING, 1, 0);

            // allow only media in priority only
            mNm.setNotificationPolicy(new NotificationManager.Policy(
                    NotificationManager.Policy.PRIORITY_CATEGORY_MEDIA, 0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);

            assertStreamMuted(STREAM_MUSIC, false,
                    "Music (media) stream should not be muted");
            assertStreamMuted(STREAM_SYSTEM, true,
                    "System stream should be muted");
            assertStreamMuted(STREAM_ALARM, true,
                    "Alarm stream should be muted");

            // if channels cannot bypass DND, the Ringer stream should be muted, else it
            // shouldn't be muted
            assertStreamMuted(STREAM_RING, !mAppsBypassingDnd,
                    "Ringer stream should be muted if channels cannot bypassDnd");
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testPriorityOnlySystemAllowed() throws Exception {
        if (mSkipRingerTests) {
            return;
        }

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        try {
            // ensure volume is not muted/0 to start test
            mAudioManager.setStreamVolume(STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(STREAM_ALARM, 1, 0);
            mAudioManager.setStreamVolume(STREAM_SYSTEM, 1, 0);
            mAudioManager.setStreamVolume(STREAM_RING, 1, 0);

            // allow only system in priority only
            mNm.setNotificationPolicy(new NotificationManager.Policy(
                    NotificationManager.Policy.PRIORITY_CATEGORY_SYSTEM, 0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);

            assertStreamMuted(STREAM_MUSIC, true,
                    "Music (media) stream should be muted");
            assertStreamMuted(STREAM_SYSTEM, false,
                    "System stream should not be muted");
            assertStreamMuted(STREAM_ALARM, true,
                    "Alarm stream should be muted");
            assertStreamMuted(STREAM_RING, false,
                    "Ringer stream should not be muted");
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testPriorityOnlySystemDisallowedWithRingerMuted() throws Exception {
        if (mSkipRingerTests) {
            return;
        }

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        try {
            // ensure volume is not muted/0 to start test, but then mute ringer
            mAudioManager.setStreamVolume(STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(STREAM_ALARM, 1, 0);
            mAudioManager.setStreamVolume(STREAM_SYSTEM, 1, 0);
            mAudioManager.setStreamVolume(STREAM_RING, 0, 0);
            mAudioManager.setRingerMode(RINGER_MODE_SILENT);

            // allow only system in priority only
            mNm.setNotificationPolicy(new NotificationManager.Policy(
                    NotificationManager.Policy.PRIORITY_CATEGORY_SYSTEM, 0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);

            assertStreamMuted(STREAM_MUSIC, true,
                    "Music (media) stream should be muted");
            assertStreamMuted(STREAM_SYSTEM, true,
                    "System stream should be muted");
            assertStreamMuted(STREAM_ALARM, true,
                    "Alarm stream should be muted");
            assertStreamMuted(STREAM_RING, true,
                    "Ringer stream should be muted");
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testPriorityOnlyAlarmsAllowed() throws Exception {
        if (mSkipRingerTests) {
            return;
        }

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        try {
            // ensure volume is not muted/0 to start test
            mAudioManager.setStreamVolume(STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(STREAM_ALARM, 1, 0);
            mAudioManager.setStreamVolume(STREAM_SYSTEM, 1, 0);
            mAudioManager.setStreamVolume(STREAM_RING, 1, 0);

            // allow only alarms in priority only
            mNm.setNotificationPolicy(new NotificationManager.Policy(
                    NotificationManager.Policy.PRIORITY_CATEGORY_ALARMS, 0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);

            assertStreamMuted(STREAM_MUSIC, true,
                    "Music (media) stream should be muted");
            assertStreamMuted(STREAM_SYSTEM, true,
                    "System stream should be muted");
            assertStreamMuted(STREAM_ALARM, false,
                    "Alarm stream should not be muted");

            // if channels cannot bypass DND, the Ringer stream should be muted, else it
            // shouldn't be muted
            assertStreamMuted(STREAM_RING, !mAppsBypassingDnd,
                    "Ringer stream should be muted if channels cannot bypassDnd");
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testPriorityOnlyRingerAllowed() throws Exception {
        if (mSkipRingerTests) {
            return;
        }

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        try {
            // ensure volume is not muted/0 to start test
            mAudioManager.setStreamVolume(STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(STREAM_ALARM, 1, 0);
            mAudioManager.setStreamVolume(STREAM_SYSTEM, 1, 0);
            mAudioManager.setStreamVolume(STREAM_RING, 1, 0);

            // allow only reminders in priority only
            mNm.setNotificationPolicy(new NotificationManager.Policy(
                    NotificationManager.Policy.PRIORITY_CATEGORY_REMINDERS, 0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);

            assertStreamMuted(STREAM_MUSIC, true,
                    "Music (media) stream should be muted");
            assertStreamMuted(STREAM_SYSTEM, true,
                    "System stream should be muted");
            assertStreamMuted(STREAM_ALARM, true,
                    "Alarm stream should be muted");
            assertStreamMuted(STREAM_RING, false,
                    "Ringer stream should not be muted");
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testPriorityOnlyChannelsCanBypassDnd() throws Exception {
        if (mSkipRingerTests) {
            return;
        }

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);

        final String NOTIFICATION_CHANNEL_ID = "test_id_" + SystemClock.uptimeMillis();
        NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL_ID, "TEST",
                NotificationManager.IMPORTANCE_DEFAULT);
        try {
            // ensure volume is not muted/0 to start test
            mAudioManager.setStreamVolume(STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(STREAM_ALARM, 1, 0);
            mAudioManager.setStreamVolume(STREAM_SYSTEM, 1, 0);
            mAudioManager.setStreamVolume(STREAM_RING, 1, 0);

            // create a channel that can bypass dnd
            channel.setBypassDnd(true);
            mNm.createNotificationChannel(channel);

            // allow nothing
            mNm.setNotificationPolicy(new NotificationManager.Policy(0,0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);

            assertStreamMuted(STREAM_MUSIC, true,
                    "Music (media) stream should be muted");
            assertStreamMuted(STREAM_SYSTEM, true,
                    "System stream should be muted");
            assertStreamMuted(STREAM_ALARM, true,
                    "Alarm stream should be muted");
            assertStreamMuted(STREAM_RING, false,
                    "Ringer stream should not be muted."
                            + " areChannelsBypassing="
                            + NotificationManager.getService().areChannelsBypassingDnd());

            // delete the channel that can bypass dnd
            mNm.deleteNotificationChannel(NOTIFICATION_CHANNEL_ID);

            assertStreamMuted(STREAM_MUSIC, true,
                    "Music (media) stream should be muted");
            assertStreamMuted(STREAM_SYSTEM, true,
                    "System stream should be muted");
            assertStreamMuted(STREAM_ALARM, true,
                    "Alarm stream should be muted");
            // if channels cannot bypass DND, the Ringer stream should be muted, else it
            // shouldn't be muted
            assertStreamMuted(STREAM_RING, !mAppsBypassingDnd,
                    "Ringer stream should be muted if apps are bypassing dnd"
                            + " areChannelsBypassing="
                            + NotificationManager.getService().areChannelsBypassingDnd());
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
            mNm.deleteNotificationChannel(NOTIFICATION_CHANNEL_ID);
            Utils.toggleNotificationPolicyAccess(mContext.getPackageName(), getInstrumentation(),
                    false);
        }
    }

    public void testAdjustVolumeWithIllegalDirection() throws Exception {
        // Call the method with illegal direction. System should not reboot.
        mAudioManager.adjustVolume(37, 0);
    }

    private final int[] PUBLIC_STREAM_TYPES = { STREAM_VOICE_CALL,
            STREAM_SYSTEM, STREAM_RING, STREAM_MUSIC,
            STREAM_ALARM, STREAM_NOTIFICATION,
            STREAM_DTMF,  STREAM_ACCESSIBILITY };

    public void testGetStreamVolumeDbWithIllegalArguments() throws Exception {
        Exception ex = null;
        // invalid stream type
        try {
            float gain = mAudioManager.getStreamVolumeDb(-100 /*streamType*/, 0,
                    AudioDeviceInfo.TYPE_BUILTIN_SPEAKER);
        } catch (Exception e) {
            ex = e; // expected
        }
        assertNotNull("No exception was thrown for an invalid stream type", ex);
        assertEquals("Wrong exception thrown for invalid stream type",
                ex.getClass(), IllegalArgumentException.class);

        // invalid volume index
        ex = null;
        try {
            float gain = mAudioManager.getStreamVolumeDb(STREAM_MUSIC, -101 /*volume*/,
                    AudioDeviceInfo.TYPE_BUILTIN_SPEAKER);
        } catch (Exception e) {
            ex = e; // expected
        }
        assertNotNull("No exception was thrown for an invalid volume index", ex);
        assertEquals("Wrong exception thrown for invalid volume index",
                ex.getClass(), IllegalArgumentException.class);

        // invalid out of range volume index
        ex = null;
        try {
            final int maxVol = mAudioManager.getStreamMaxVolume(STREAM_MUSIC);
            float gain = mAudioManager.getStreamVolumeDb(STREAM_MUSIC, maxVol + 1,
                    AudioDeviceInfo.TYPE_BUILTIN_SPEAKER);
        } catch (Exception e) {
            ex = e; // expected
        }
        assertNotNull("No exception was thrown for an invalid out of range volume index", ex);
        assertEquals("Wrong exception thrown for invalid out of range volume index",
                ex.getClass(), IllegalArgumentException.class);

        // invalid device type
        ex = null;
        try {
            float gain = mAudioManager.getStreamVolumeDb(STREAM_MUSIC, 0,
                    -102 /*deviceType*/);
        } catch (Exception e) {
            ex = e; // expected
        }
        assertNotNull("No exception was thrown for an invalid device type", ex);
        assertEquals("Wrong exception thrown for invalid device type",
                ex.getClass(), IllegalArgumentException.class);

        // invalid input device type
        ex = null;
        try {
            float gain = mAudioManager.getStreamVolumeDb(STREAM_MUSIC, 0,
                    AudioDeviceInfo.TYPE_BUILTIN_MIC);
        } catch (Exception e) {
            ex = e; // expected
        }
        assertNotNull("No exception was thrown for an invalid input device type", ex);
        assertEquals("Wrong exception thrown for invalid input device type",
                ex.getClass(), IllegalArgumentException.class);
    }

    public void testGetStreamVolumeDb() throws Exception {
        for (int streamType : PUBLIC_STREAM_TYPES) {
            // verify mininum index is strictly inferior to maximum index
            final int minIndex = mAudioManager.getStreamMinVolume(streamType);
            final int maxIndex = mAudioManager.getStreamMaxVolume(streamType);
            assertTrue("Min vol index (" + minIndex + ") for stream " + streamType + " not inferior"
                    + " to max vol index (" + maxIndex + ")", minIndex <= maxIndex);
            float prevGain = Float.NEGATIVE_INFINITY;
            // verify gain increases with the volume indices
            for (int idx = minIndex ; idx <= maxIndex ; idx++) {
                float gain = mAudioManager.getStreamVolumeDb(streamType, idx,
                        AudioDeviceInfo.TYPE_BUILTIN_SPEAKER);
                assertTrue("Non-monotonically increasing gain at index " + idx + " for stream"
                        + streamType, prevGain <= gain);
                prevGain = gain;
            }
        }
    }

    public void testAdjustSuggestedStreamVolumeWithIllegalArguments() throws Exception {
        // Call the method with illegal direction. System should not reboot.
        mAudioManager.adjustSuggestedStreamVolume(37, STREAM_MUSIC, 0);

        // Call the method with illegal stream. System should not reboot.
        mAudioManager.adjustSuggestedStreamVolume(ADJUST_RAISE, 66747, 0);
    }

    @CddTest(requirement="5.4.1/C-1-4")
    public void testGetMicrophones() throws Exception {
        if (!mContext.getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_MICROPHONE)) {
            return;
        }
        List<MicrophoneInfo> microphones = mAudioManager.getMicrophones();
        assertTrue(microphones.size() > 0);
        for (int i = 0; i < microphones.size(); i++) {
            MicrophoneInfo microphone = microphones.get(i);
            Log.i(TAG, "deviceId:" + microphone.getDescription());
            Log.i(TAG, "portId:" + microphone.getId());
            Log.i(TAG, "type:" + microphone.getType());
            Log.i(TAG, "address:" + microphone.getAddress());
            Log.i(TAG, "deviceLocation:" + microphone.getLocation());
            Log.i(TAG, "deviceGroup:" + microphone.getGroup()
                    + " index:" + microphone.getIndexInTheGroup());
            MicrophoneInfo.Coordinate3F position = microphone.getPosition();
            Log.i(TAG, "position:" + position.x + " " + position.y + " " + position.z);
            MicrophoneInfo.Coordinate3F orientation = microphone.getOrientation();
            Log.i(TAG, "orientation:" + orientation.x + " "
                    + orientation.y + " " + orientation.z);
            Log.i(TAG, "frequencyResponse:" + microphone.getFrequencyResponse());
            Log.i(TAG, "channelMapping:" + microphone.getChannelMapping());
            Log.i(TAG, "sensitivity:" + microphone.getSensitivity());
            Log.i(TAG, "max spl:" + microphone.getMaxSpl());
            Log.i(TAG, "min spl:" + microphone.getMinSpl());
            Log.i(TAG, "directionality:" + microphone.getDirectionality());
            Log.i(TAG, "--------------");
        }
    }

    public void testIsHapticPlaybackSupported() {
        // Calling the API to make sure it doesn't crash.
        Log.i(TAG, "isHapticPlaybackSupported: " + AudioManager.isHapticPlaybackSupported());
    }

    public void testGetAudioHwSyncForSession() {
        // AudioManager.getAudioHwSyncForSession is not supported before S
        if (ApiLevelUtil.isAtMost(Build.VERSION_CODES.R)) {
            Log.i(TAG, "testGetAudioHwSyncForSession skipped, release: " + Build.VERSION.SDK_INT);
            return;
        }
        try {
            int sessionId = mAudioManager.generateAudioSessionId();
            assertNotEquals("testGetAudioHwSyncForSession cannot get audio session ID",
                    AudioManager.ERROR, sessionId);
            int hwSyncId = mAudioManager.getAudioHwSyncForSession(sessionId);
            Log.i(TAG, "getAudioHwSyncForSession: " + hwSyncId);
        } catch (UnsupportedOperationException e) {
            Log.i(TAG, "getAudioHwSyncForSession not supported");
        } catch (Exception e) {
            fail("Unexpected exception thrown by getAudioHwSyncForSession: " + e);
        }
    }

    private void setInterruptionFilter(int filter) {
        mNm.setInterruptionFilter(filter);
        final long startPoll = SystemClock.uptimeMillis();
        int currentFilter = -1;
        while (SystemClock.uptimeMillis() - startPoll < POLL_TIME_UPDATE_INTERRUPTION_FILTER) {
            currentFilter = mNm.getCurrentInterruptionFilter();
            if (currentFilter == filter) {
                return;
            }
        }
        Log.e(TAG, "interruption filter unsuccessfully set. wanted=" + filter
                + " actual=" + currentFilter);
    }

    private int getVolumeDelta(int volume) {
        return 1;
    }

    private int getTestRingerVol() {
        final int currentRingVol = mAudioManager.getStreamVolume(STREAM_RING);
        final int maxRingVol = mAudioManager.getStreamMaxVolume(STREAM_RING);
        if (currentRingVol != maxRingVol) {
            return maxRingVol;
        } else {
            return maxRingVol - 1;
        }
    }

    public void testAllowedCapturePolicy() throws Exception {
        final int policy = mAudioManager.getAllowedCapturePolicy();
        assertEquals("Wrong default capture policy", AudioAttributes.ALLOW_CAPTURE_BY_ALL, policy);

        for (int setPolicy : new int[] { AudioAttributes.ALLOW_CAPTURE_BY_NONE,
                                      AudioAttributes.ALLOW_CAPTURE_BY_SYSTEM,
                                      AudioAttributes.ALLOW_CAPTURE_BY_ALL}) {
            mAudioManager.setAllowedCapturePolicy(setPolicy);
            final int getPolicy = mAudioManager.getAllowedCapturePolicy();
            assertEquals("Allowed capture policy doesn't match", setPolicy, getPolicy);
        }
    }

    public void testIsHdmiSystemAudidoSupported() {
        // just make sure the call works
        boolean isSupported = mAudioManager.isHdmiSystemAudioSupported();
        Log.d(TAG, "isHdmiSystemAudioSupported() = " + isSupported);
    }

    public void testIsBluetoothScoAvailableOffCall() {
        // just make sure the call works
        boolean isSupported = mAudioManager.isBluetoothScoAvailableOffCall();
        Log.d(TAG, "isBluetoothScoAvailableOffCall() = " + isSupported);
    }

    public void testStartStopBluetoothSco() {
        mAudioManager.startBluetoothSco();
        mAudioManager.stopBluetoothSco();
    }

    public void testStartStopBluetoothScoVirtualCall() {
        mAudioManager.startBluetoothScoVirtualCall();
        mAudioManager.stopBluetoothSco();
    }

    public void testGetAdditionalOutputDeviceDelay() {
        AudioDeviceInfo[] devices = mAudioManager.getDevices(AudioManager.GET_DEVICES_ALL);
        for (AudioDeviceInfo device : devices) {
            long delay = mAudioManager.getAdditionalOutputDeviceDelay(device);
            assertTrue("getAdditionalOutputDeviceDelay() = " + delay +" (should be >= 0)",
                    delay >= 0);
            delay = mAudioManager.getMaxAdditionalOutputDeviceDelay(device);
            assertTrue("getMaxAdditionalOutputDeviceDelay() = " + delay +" (should be >= 0)",
                    delay >= 0);
        }
    }

    static class MyPrevDevForStrategyListener implements
            AudioManager.OnPreferredDevicesForStrategyChangedListener {
        @Override
        public void onPreferredDevicesForStrategyChanged(AudioProductStrategy strategy,
                List<AudioDeviceAttributes> devices) {
            fail("onPreferredDevicesForStrategyChanged must not be called");
        }
    }

    public void testPreferredDevicesForStrategy() {
        // setPreferredDeviceForStrategy
        AudioDeviceInfo[] devices = mAudioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS);
        if (devices.length <= 0) {
            Log.i(TAG, "Skip testPreferredDevicesForStrategy as there is no output device");
            return;
        }
        final AudioDeviceAttributes ada = new AudioDeviceAttributes(devices[0]);

        final AudioAttributes mediaAttr = new AudioAttributes.Builder().setUsage(
                AudioAttributes.USAGE_MEDIA).build();
        final List<AudioProductStrategy> strategies =
                AudioProductStrategy.getAudioProductStrategies();
        AudioProductStrategy strategyForMedia = null;
        for (AudioProductStrategy strategy : strategies) {
            if (strategy.supportsAudioAttributes(mediaAttr)) {
                strategyForMedia = strategy;
                break;
            }
        }
        if (strategyForMedia == null) {
            Log.i(TAG, "Skip testPreferredDevicesForStrategy as there is no strategy for media");
            return;
        }

        try {
            mAudioManager.setPreferredDeviceForStrategy(strategyForMedia, ada);
            fail("setPreferredDeviceForStrategy must fail due to no permission");
        } catch (SecurityException e) {
        }
        try {
            mAudioManager.getPreferredDeviceForStrategy(strategyForMedia);
            fail("getPreferredDeviceForStrategy must fail due to no permission");
        } catch (SecurityException e) {
        }
        final List<AudioDeviceAttributes> adas = new ArrayList<>();
        adas.add(ada);
        try {
            mAudioManager.setPreferredDevicesForStrategy(strategyForMedia, adas);
            fail("setPreferredDevicesForStrategy must fail due to no permission");
        } catch (SecurityException e) {
        }
        try {
            mAudioManager.getPreferredDevicesForStrategy(strategyForMedia);
            fail("getPreferredDevicesForStrategy must fail due to no permission");
        } catch (SecurityException e) {
        }
        MyPrevDevForStrategyListener listener = new MyPrevDevForStrategyListener();
        try {
            mAudioManager.addOnPreferredDevicesForStrategyChangedListener(
                    Executors.newSingleThreadExecutor(), listener);
            fail("addOnPreferredDevicesForStrategyChangedListener must fail due to no permission");
        } catch (SecurityException e) {
        }
        // There is not listener added at server side. Nothing to remove.
        mAudioManager.removeOnPreferredDevicesForStrategyChangedListener(listener);
    }

    static class MyPrevDevicesForCapturePresetChangedListener implements
            AudioManager.OnPreferredDevicesForCapturePresetChangedListener {
        @Override
        public void onPreferredDevicesForCapturePresetChanged(
                int capturePreset, List<AudioDeviceAttributes> devices) {
            fail("onPreferredDevicesForCapturePresetChanged must not be called");
        }
    }

    public void testPreferredDeviceForCapturePreset() {
        AudioDeviceInfo[] devices = mAudioManager.getDevices(AudioManager.GET_DEVICES_INPUTS);
        if (devices.length <= 0) {
            Log.i(TAG, "Skip testPreferredDevicesForStrategy as there is no input device");
            return;
        }
        final AudioDeviceAttributes ada = new AudioDeviceAttributes(devices[0]);

        try {
            mAudioManager.setPreferredDeviceForCapturePreset(MediaRecorder.AudioSource.MIC, ada);
            fail("setPreferredDeviceForCapturePreset must fail due to no permission");
        } catch (SecurityException e) {
        }
        try {
            mAudioManager.getPreferredDevicesForCapturePreset(MediaRecorder.AudioSource.MIC);
            fail("getPreferredDevicesForCapturePreset must fail due to no permission");
        } catch (SecurityException e) {
        }
        try {
            mAudioManager.clearPreferredDevicesForCapturePreset(MediaRecorder.AudioSource.MIC);
            fail("clearPreferredDevicesForCapturePreset must fail due to no permission");
        } catch (SecurityException e) {
        }
        MyPrevDevicesForCapturePresetChangedListener listener =
                new MyPrevDevicesForCapturePresetChangedListener();
        try {
            mAudioManager.addOnPreferredDevicesForCapturePresetChangedListener(
                Executors.newSingleThreadExecutor(), listener);
            fail("addOnPreferredDevicesForCapturePresetChangedListener must fail"
                    + "due to no permission");
        } catch (SecurityException e) {
        }
        // There is not listener added at server side. Nothing to remove.
        mAudioManager.removeOnPreferredDevicesForCapturePresetChangedListener(listener);
    }

    public void testGetDevices() {
        AudioDeviceInfo[] devices = mAudioManager.getDevices(AudioManager.GET_DEVICES_ALL);
        for (AudioDeviceInfo device : devices) {
            HashSet<Integer> formats = IntStream.of(device.getEncodings()).boxed()
                    .collect(Collectors.toCollection(HashSet::new));
            HashSet<Integer> channelMasks = IntStream.of(device.getChannelMasks()).boxed()
                    .collect(Collectors.toCollection(HashSet::new));
            HashSet<Integer> channelIndexMasks = IntStream.of(device.getChannelIndexMasks()).boxed()
                    .collect(Collectors.toCollection(HashSet::new));
            HashSet<Integer> sampleRates = IntStream.of(device.getSampleRates()).boxed()
                    .collect(Collectors.toCollection(HashSet::new));
            HashSet<Integer> formatsFromProfile = new HashSet<>();
            HashSet<Integer> channelMasksFromProfile = new HashSet<>();
            HashSet<Integer> channelIndexMasksFromProfile = new HashSet<>();
            HashSet<Integer> sampleRatesFromProfile = new HashSet<>();
            for (AudioProfile profile : device.getAudioProfiles()) {
                formatsFromProfile.add(profile.getFormat());
                channelMasksFromProfile.addAll(Arrays.stream(profile.getChannelMasks()).boxed()
                        .collect(Collectors.toList()));
                channelIndexMasksFromProfile.addAll(Arrays.stream(profile.getChannelIndexMasks())
                        .boxed().collect(Collectors.toList()));
                sampleRatesFromProfile.addAll(Arrays.stream(profile.getSampleRates()).boxed()
                        .collect(Collectors.toList()));
                assertTrue(ALL_ENCAPSULATION_TYPES.contains(profile.getEncapsulationType()));
            }
            for (AudioDescriptor descriptor : device.getAudioDescriptors()) {
                assertNotEquals(AudioDescriptor.STANDARD_NONE, descriptor.getStandard());
                assertNotNull(descriptor.getDescriptor());
                assertTrue(
                        ALL_KNOWN_ENCAPSULATION_TYPES.contains(descriptor.getEncapsulationType()));
            }
            assertEquals(formats, formatsFromProfile);
            assertEquals(channelMasks, channelMasksFromProfile);
            assertEquals(channelIndexMasks, channelIndexMasksFromProfile);
            assertEquals(sampleRates, sampleRatesFromProfile);
        }
    }

    private void assertStreamVolumeEquals(int stream, int expectedVolume) throws Exception {
        assertStreamVolumeEquals(stream, expectedVolume,
                "Unexpected stream volume for stream=" + stream);
    }

    // volume adjustments are asynchronous, we poll the volume in case the volume state hasn't
    // been adjusted yet
    private void assertStreamVolumeEquals(int stream, int expectedVolume, String msg)
            throws Exception {
        final long startPoll = SystemClock.uptimeMillis();
        int actualVolume = mAudioManager.getStreamVolume(stream);
        while (SystemClock.uptimeMillis() - startPoll < POLL_TIME_VOLUME_ADJUST
                && expectedVolume != actualVolume) {
            actualVolume = mAudioManager.getStreamVolume(stream);
        }
        assertEquals(msg, expectedVolume, actualVolume);
    }

    // volume adjustments are asynchronous, we poll the volume in case the mute state hasn't
    // changed yet
    private void assertStreamMuted(int stream, boolean expectedMuteState, String msg)
            throws Exception{
        final long startPoll = SystemClock.uptimeMillis();
        boolean actualMuteState = mAudioManager.isStreamMute(stream);
        while (SystemClock.uptimeMillis() - startPoll < POLL_TIME_VOLUME_ADJUST
                && expectedMuteState != actualMuteState) {
            actualMuteState = mAudioManager.isStreamMute(stream);
        }
        assertEquals(msg, expectedMuteState, actualMuteState);
    }

    private void assertMusicActive(boolean expectedIsMusicActive) throws Exception {
        final long startPoll = SystemClock.uptimeMillis();
        boolean actualIsMusicActive = mAudioManager.isMusicActive();
        while (SystemClock.uptimeMillis() - startPoll < POLL_TIME_PLAY_MUSIC
                && expectedIsMusicActive != actualIsMusicActive) {
            actualIsMusicActive = mAudioManager.isMusicActive();
        }
        assertEquals(actualIsMusicActive, actualIsMusicActive);
    }

    private static final long REPEATED_CHECK_POLL_PERIOD_MS = 100; // 100ms
    private static final long DEFAULT_ASYNC_CALL_TIMEOUT_MS = 5 * REPEATED_CHECK_POLL_PERIOD_MS;

    /**
     * Makes multiple attempts over a given timeout period to test the predicate on an AudioManager
     * instance. Test success is evaluated against a true predicate result.
     * @param am the AudioManager instance to use for the test
     * @param predicate the test to run either until it returns true, or until the timeout expires
     * @param timeoutMs the maximum time allowed for the test to pass
     * @param errorString the string to be displayed in case of failure
     * @throws Exception
     */
    private void assertTrueCheckTimeout(AudioManager am, Predicate<AudioManager> predicate,
            long timeoutMs, String errorString) throws Exception {
        long checkStart = SystemClock.uptimeMillis();
        boolean result = false;
        while (SystemClock.uptimeMillis() - checkStart < timeoutMs) {
            result = predicate.test(am);
            if (result) {
                break;
            }
            Thread.sleep(REPEATED_CHECK_POLL_PERIOD_MS);
        }
        assertTrue(errorString, result);
    }

    // getParameters() & setParameters() are deprecated, so don't test

    // setAdditionalOutputDeviceDelay(), getAudioVolumeGroups(), getVolumeIndexForAttributes()
    // getMinVolumeIndexForAttributes(), getMaxVolumeIndexForAttributes() &
    // setVolumeIndexForAttributes() require privledged permission MODIFY_AUDIO_ROUTING
    // and thus cannot be tested here.
}
