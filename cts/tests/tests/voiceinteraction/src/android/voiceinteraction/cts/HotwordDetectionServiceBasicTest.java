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

package android.voiceinteraction.cts;

import static android.content.pm.PackageManager.FEATURE_MICROPHONE;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertEquals;

import android.app.Instrumentation;
import android.app.compat.CompatChanges;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.ParcelFileDescriptor;
import android.os.Parcelable;
import android.os.Process;
import android.platform.test.annotations.AppModeFull;
import android.provider.DeviceConfig;
import android.service.voice.HotwordDetectedResult;
import android.service.voice.HotwordDetectionService;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.Until;
import android.util.Log;
import android.voiceinteraction.common.Utils;
import android.voiceinteraction.service.EventPayloadParcelable;
import android.voiceinteraction.service.MainHotwordDetectionService;

import androidx.annotation.NonNull;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.RequiresDevice;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.compatibility.common.util.BlockingBroadcastReceiver;
import com.android.compatibility.common.util.RequiredFeatureRule;
import com.android.compatibility.common.util.SystemUtil;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests for using the VoiceInteractionService that included a basic HotwordDetectionService.
 */
@RunWith(AndroidJUnit4.class)
@AppModeFull(reason = "No real use case for instant mode hotword detection service")
public final class HotwordDetectionServiceBasicTest
        extends AbstractVoiceInteractionBasicTestCase {
    static final String TAG = "HotwordDetectionServiceBasicTest";

    @Rule
    public RequiredFeatureRule REQUIRES_MIC_RULE = new RequiredFeatureRule(FEATURE_MICROPHONE);

    private static final String INDICATORS_FLAG = "camera_mic_icons_enabled";
    private static final String PRIVACY_CHIP_PKG = "com.android.systemui";
    private static final String PRIVACY_CHIP_ID = "privacy_chip";
    private static final Long PERMISSION_INDICATORS_NOT_PRESENT = 162547999L;
    private static final Long CLEAR_CHIP_MS = 5000L;

    private static Instrumentation sInstrumentation = InstrumentationRegistry.getInstrumentation();
    private static UiDevice sUiDevice = UiDevice.getInstance(sInstrumentation);
    private static PackageManager sPkgMgr = sInstrumentation.getContext().getPackageManager();
    private static boolean wasIndicatorEnabled = false;

    @BeforeClass
    public static void enableIndicators() {
        wasIndicatorEnabled = setIndicatorEnabledStateIfNeeded(true);
    }

    @AfterClass
    public static void resetIndicators() {
        if (!wasIndicatorEnabled) {
            setIndicatorEnabledStateIfNeeded(false);
        }
    }

    // Checks if the privacy indicators are enabled on this device. Sets the state to the parameter,
    // And returns the original enable state (to allow this state to be reset after the test)
    private static boolean setIndicatorEnabledStateIfNeeded(boolean shouldEnable) {
        return SystemUtil.runWithShellPermissionIdentity(() -> {
            boolean currentlyEnabled = DeviceConfig.getBoolean(DeviceConfig.NAMESPACE_PRIVACY,
                    INDICATORS_FLAG, shouldEnable);
            if (currentlyEnabled != shouldEnable) {
                DeviceConfig.setProperty(DeviceConfig.NAMESPACE_PRIVACY, INDICATORS_FLAG,
                        Boolean.toString(shouldEnable), false);
            }
            return currentlyEnabled;
        });
    }

    @Test
    public void testHotwordDetectionService_getMaxCustomInitializationStatus()
            throws Throwable {
        assertThat(HotwordDetectionService.getMaxCustomInitializationStatus()).isEqualTo(2);
    }

    @Test
    public void testHotwordDetectionService_validHotwordDetectionComponentName_triggerSuccess()
            throws Throwable {
        testHotwordDetection(Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_TEST,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_SUCCESS);
    }

    @Test
    public void testVoiceInteractionService_withoutManageHotwordDetectionPermission_triggerFailure()
            throws Throwable {
        testHotwordDetection(Utils.VIS_WITHOUT_MANAGE_HOTWORD_DETECTION_PERMISSION_TEST,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_SECURITY_EXCEPTION);
    }

    @Test
    public void testVoiceInteractionService_holdBindHotwordDetectionPermission_triggerFailure()
            throws Throwable {
        testHotwordDetection(Utils.VIS_HOLD_BIND_HOTWORD_DETECTION_PERMISSION_TEST,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_SECURITY_EXCEPTION);
    }

    @Test
    @RequiresDevice
    public void testHotwordDetectionService_onDetectFromDsp_success()
            throws Throwable {
        Thread.sleep(CLEAR_CHIP_MS);
        // Create AlwaysOnHotwordDetector and wait the HotwordDetectionService ready
        testHotwordDetection(Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_TEST,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_SUCCESS);

        verifyDetectedResult(
                performAndGetDetectionResult(Utils.HOTWORD_DETECTION_SERVICE_DSP_ONDETECT_TEST),
                MainHotwordDetectionService.DETECTED_RESULT);
        verifyMicrophoneChip(true);
    }

    @Test
    @RequiresDevice
    public void testHotwordDetectionService_onDetectFromDsp_rejection()
            throws Throwable {
        Thread.sleep(CLEAR_CHIP_MS);
        // Create AlwaysOnHotwordDetector and wait the HotwordDetectionService ready
        testHotwordDetection(Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_TEST,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_SUCCESS);

        assertThat(performAndGetDetectionResult(Utils.HOTWORD_DETECTION_SERVICE_DSP_ONREJECT_TEST))
                .isEqualTo(MainHotwordDetectionService.REJECTED_RESULT);
        verifyMicrophoneChip(false);
    }

    @Test
    public void testHotwordDetectionService_onDetectFromExternalSource_success()
            throws Throwable {
        Thread.sleep(CLEAR_CHIP_MS);
        // Create AlwaysOnHotwordDetector and wait the HotwordDetectionService ready
        testHotwordDetection(Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_TEST,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_SUCCESS);

        verifyDetectedResult(
                performAndGetDetectionResult(
                        Utils.HOTWORD_DETECTION_SERVICE_EXTERNAL_SOURCE_ONDETECT_TEST),
                MainHotwordDetectionService.DETECTED_RESULT);
        verifyMicrophoneChip(true);
    }

    @Test
    @RequiresDevice
    public void testHotwordDetectionService_onDetectFromMic_success()
            throws Throwable {
        Thread.sleep(CLEAR_CHIP_MS);
        // Create SoftwareHotwordDetector and wait the HotwordDetectionService ready
        testHotwordDetection(Utils.HOTWORD_DETECTION_SERVICE_FROM_SOFTWARE_TRIGGER_TEST,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_SUCCESS);

        verifyDetectedResult(
                performAndGetDetectionResult(Utils.HOTWORD_DETECTION_SERVICE_MIC_ONDETECT_TEST),
                MainHotwordDetectionService.DETECTED_RESULT);
        verifyMicrophoneChip(true);
    }

    @Test
    @RequiresDevice
    public void testHotwordDetectionService_onStopDetection()
            throws Throwable {
        // Create SoftwareHotwordDetector and wait the HotwordDetectionService ready
        testHotwordDetection(Utils.HOTWORD_DETECTION_SERVICE_FROM_SOFTWARE_TRIGGER_TEST,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_SUCCESS);

        // The HotwordDetectionService can't report any result after recognition is stopped. So
        // restart it after stopping; then the service can report a special result.
        perform(Utils.HOTWORD_DETECTION_SERVICE_MIC_ONDETECT_TEST);
        perform(Utils.HOTWORD_DETECTION_SERVICE_CALL_STOP_RECOGNITION);
        EventPayloadParcelable result =
                (EventPayloadParcelable) performAndGetDetectionResult(
                        Utils.HOTWORD_DETECTION_SERVICE_MIC_ONDETECT_TEST);

        verifyDetectedResult(
                result, MainHotwordDetectionService.DETECTED_RESULT_AFTER_STOP_DETECTION);
    }

    @Test
    @RequiresDevice
    public void testHotwordDetectionService_concurrentCapture() throws Throwable {
        // Create SoftwareHotwordDetector and wait the HotwordDetectionService ready
        testHotwordDetection(Utils.HOTWORD_DETECTION_SERVICE_FROM_SOFTWARE_TRIGGER_TEST,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_SUCCESS);

        SystemUtil.runWithShellPermissionIdentity(() -> {
            AudioRecord record =
                    new AudioRecord.Builder()
                            .setAudioAttributes(
                                    new AudioAttributes.Builder()
                                            .setInternalCapturePreset(MediaRecorder.AudioSource.MIC)
                                            .build())
                            .setAudioFormat(
                                    new AudioFormat.Builder()
                                            .setChannelMask(AudioFormat.CHANNEL_IN_MONO)
                                            .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                                            .build())
                            .setBufferSizeInBytes(10240) // something large enough to not fail
                            .build();
            assertThat(record.getState()).isEqualTo(AudioRecord.STATE_INITIALIZED);

            try {
                record.startRecording();
                verifyDetectedResult(
                        performAndGetDetectionResult(
                                Utils.HOTWORD_DETECTION_SERVICE_MIC_ONDETECT_TEST),
                        MainHotwordDetectionService.DETECTED_RESULT);
                // TODO: Test that it still works after restarting the process or killing audio
                //  server.
            } finally {
                record.release();
            }
        });
    }

    @Test
    public void testHotwordDetectionService_processDied_triggerOnError()
            throws Throwable {
        // Create AlwaysOnHotwordDetector and wait the HotwordDetectionService ready
        testHotwordDetection(Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_TEST,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_SUCCESS);

        // Use AlwaysOnHotwordDetector to test process died of HotwordDetectionService
        testHotwordDetection(Utils.HOTWORD_DETECTION_SERVICE_PROCESS_DIED_TEST,
                Utils.HOTWORD_DETECTION_SERVICE_TRIGGER_RESULT_INTENT,
                Utils.HOTWORD_DETECTION_SERVICE_GET_ERROR);
    }

    private void testHotwordDetection(int testType, String expectedIntent, int expectedResult) {
        final BlockingBroadcastReceiver receiver = new BlockingBroadcastReceiver(mContext,
                expectedIntent);
        receiver.register();
        perform(testType);
        final Intent intent = receiver.awaitForBroadcast(TIMEOUT_MS);
        receiver.unregisterQuietly();

        assertThat(intent).isNotNull();
        assertThat(intent.getIntExtra(Utils.KEY_TEST_RESULT, -1)).isEqualTo(expectedResult);
    }

    @NonNull
    private Parcelable performAndGetDetectionResult(int testType) {
        final BlockingBroadcastReceiver receiver = new BlockingBroadcastReceiver(mContext,
                Utils.HOTWORD_DETECTION_SERVICE_ONDETECT_RESULT_INTENT);
        receiver.register();
        perform(testType);
        final Intent intent = receiver.awaitForBroadcast(TIMEOUT_MS);
        receiver.unregisterQuietly();

        assertThat(intent).isNotNull();
        final Parcelable result = intent.getParcelableExtra(Utils.KEY_TEST_RESULT);
        assertThat(result).isNotNull();
        return result;
    }

    private void perform(int testType) {
        mActivityTestRule.getScenario().onActivity(
                activity -> activity.triggerHotwordDetectionServiceTest(
                        Utils.HOTWORD_DETECTION_SERVICE_BASIC, testType));
    }

    // TODO: Implement HotwordDetectedResult#equals to override the Bundle equality check; then
    // simply check that the HotwordDetectedResults are equal.
    private void verifyDetectedResult(Parcelable result, HotwordDetectedResult expected) {
        assertThat(result).isInstanceOf(EventPayloadParcelable.class);
        HotwordDetectedResult hotwordDetectedResult =
                ((EventPayloadParcelable) result).mHotwordDetectedResult;
        ParcelFileDescriptor audioStream = ((EventPayloadParcelable) result).mAudioStream;
        assertThat(hotwordDetectedResult).isNotNull();
        assertThat(hotwordDetectedResult.getAudioChannel()).isEqualTo(
                expected.getAudioChannel());
        assertThat(hotwordDetectedResult.getConfidenceLevel()).isEqualTo(
                expected.getConfidenceLevel());
        assertThat(hotwordDetectedResult.isHotwordDetectionPersonalized()).isEqualTo(
                expected.isHotwordDetectionPersonalized());
        assertThat(hotwordDetectedResult.getHotwordDurationMillis()).isEqualTo(
                expected.getHotwordDurationMillis());
        assertThat(hotwordDetectedResult.getHotwordOffsetMillis()).isEqualTo(
                expected.getHotwordOffsetMillis());
        assertThat(hotwordDetectedResult.getHotwordPhraseId()).isEqualTo(
                expected.getHotwordPhraseId());
        assertThat(hotwordDetectedResult.getPersonalizedScore()).isEqualTo(
                expected.getPersonalizedScore());
        assertThat(hotwordDetectedResult.getScore()).isEqualTo(expected.getScore());
        assertThat(audioStream).isNull();
    }

    private void verifyMicrophoneChip(boolean shouldBePresent) throws Exception {
        if (sPkgMgr.hasSystemFeature(PackageManager.FEATURE_LEANBACK)) {
            // TODO ntmyren: test TV indicator
        } else if (sPkgMgr.hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE)) {
            // TODO ntmyren: test Auto indicator
        } else {
            verifyMicrophoneChipHandheld(shouldBePresent);
        }
    }

    private void verifyMicrophoneChipHandheld(boolean shouldBePresent) throws Exception {
        // If the change Id is not present, then isChangeEnabled will return true. To bypass this,
        // the change is set to "false" if present.
        if (SystemUtil.callWithShellPermissionIdentity(() -> CompatChanges.isChangeEnabled(
                PERMISSION_INDICATORS_NOT_PRESENT, Process.SYSTEM_UID))) {
            return;
        }
        // Ensure the privacy chip is present (or not)
        final boolean chipFound = sUiDevice.wait(Until.hasObject(
                By.res(PRIVACY_CHIP_PKG, PRIVACY_CHIP_ID)), CLEAR_CHIP_MS) == true;
        assertEquals("chip display state", shouldBePresent, chipFound);
    }

    @Override
    public String getVoiceInteractionService() {
        return "android.voiceinteraction.cts/"
                + "android.voiceinteraction.service.BasicVoiceInteractionService";
    }
}
