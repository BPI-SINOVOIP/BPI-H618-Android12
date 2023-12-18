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
 * limitations under the License
 */

package android.voicerecognition.cts;

import static com.android.compatibility.common.util.ShellUtils.runShellCommand;
import static com.android.compatibility.common.util.SystemUtil.runWithShellPermissionIdentity;

import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assume.assumeTrue;

import android.Manifest;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.SystemClock;
import android.provider.DeviceConfig;
import android.provider.Settings;
import android.server.wm.WindowManagerStateHelper;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.util.Log;

import androidx.test.InstrumentationRegistry;
import androidx.test.rule.ActivityTestRule;
import androidx.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.SettingsStateChangerRule;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;
import java.util.stream.Collectors;

@RunWith(AndroidJUnit4.class)
public final class RecognitionServiceMicIndicatorTest {

    private final String TAG = "RecognitionServiceMicIndicatorTest";
    // same as Settings.Secure.VOICE_RECOGNITION_SERVICE
    private final String VOICE_RECOGNITION_SERVICE = "voice_recognition_service";
    private final String INDICATORS_FLAG = "camera_mic_icons_enabled";
    // Same as PrivacyItemController DEFAULT_MIC_CAMERA
    private final boolean DEFAULT_MIC_CAMERA = true;
    // Th notification privacy indicator
    private final String PRIVACY_CHIP_PACKAGE_NAME = "com.android.systemui";
    private final String PRIVACY_CHIP_ID = "privacy_chip";
    private final String PRIVACY_DIALOG_PACKAGE_NAME = "com.android.systemui";
    private final String PRIVACY_DIALOG_CONTENT_ID = "text";
    private final String CAR_PRIVACY_DIALOG_CONTENT_ID = "car_ui_list_item_title";
    private final String TV_MIC_INDICATOR_WINDOW_TITLE = "MicrophoneCaptureIndicator";
    // The cts app label
    private final String APP_LABEL = "CtsVoiceRecognitionTestCases";
    // A simple test voice recognition service implementation
    private final String CTS_VOICE_RECOGNITION_SERVICE =
            "android.recognitionservice.service/android.recognitionservice.service"
                    + ".CtsVoiceRecognitionService";
    private final long INDICATOR_DISMISS_TIMEOUT = 5000L;
    private final long UI_WAIT_TIMEOUT = 1000L;

    protected final Context mContext = InstrumentationRegistry.getTargetContext();
    private final String mOriginalVoiceRecognizer = Settings.Secure.getString(
            mContext.getContentResolver(), VOICE_RECOGNITION_SERVICE);
    private UiDevice mUiDevice;
    private SpeechRecognitionActivity mActivity;
    private String mCameraLabel;
    private String mOriginalIndicatorsState;

    @Rule
    public ActivityTestRule<SpeechRecognitionActivity> mActivityTestRule =
            new ActivityTestRule<>(SpeechRecognitionActivity.class);

    @Rule
    public final SettingsStateChangerRule mVoiceRecognitionServiceSetterRule =
            new SettingsStateChangerRule(mContext, VOICE_RECOGNITION_SERVICE,
                    mOriginalVoiceRecognizer);

    @Before
    public void setup() {
        prepareDevice();
        mUiDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mActivity = mActivityTestRule.getActivity();
        mActivity.init(false, null);

        final PackageManager pm = mContext.getPackageManager();
        try {
            mCameraLabel = pm.getPermissionGroupInfo(Manifest.permission_group.CAMERA, 0).loadLabel(
                    pm).toString();
        } catch (PackageManager.NameNotFoundException e) {
        }
        runWithShellPermissionIdentity(() -> {
            mOriginalIndicatorsState =
                    DeviceConfig.getProperty(DeviceConfig.NAMESPACE_PRIVACY, INDICATORS_FLAG);
            Log.v(TAG, "setup(): mOriginalIndicatorsState=" + mOriginalIndicatorsState);
        });
        setIndicatorsEnabledState(Boolean.toString(true));
    }

    @After
    public void teardown() {
        // press back to close the dialog
        mUiDevice.pressHome();
        // Restore original value.
        setIndicatorsEnabledState(mOriginalIndicatorsState);
    }

    private void prepareDevice() {
        // Unlock screen.
        runShellCommand("input keyevent KEYCODE_WAKEUP");
        // Dismiss keyguard, in case it's set as "Swipe to unlock".
        runShellCommand("wm dismiss-keyguard");
    }

    private void setCurrentRecognizer(String recognizer) {
        runWithShellPermissionIdentity(
                () -> Settings.Secure.putString(mContext.getContentResolver(),
                        VOICE_RECOGNITION_SERVICE, recognizer));
        mUiDevice.waitForIdle();
    }

    private void setIndicatorsEnabledState(String enabled) {
        runWithShellPermissionIdentity(
                () -> DeviceConfig.setProperty(DeviceConfig.NAMESPACE_PRIVACY, INDICATORS_FLAG,
                        enabled, false));
        mUiDevice.waitForIdle();
    }

    private boolean hasPreInstalledRecognizer(String packageName) {
        Log.v(TAG, "hasPreInstalledRecognizer package=" + packageName);
        try {
            final PackageManager pm = mContext.getPackageManager();
            final ApplicationInfo info = pm.getApplicationInfo(packageName, 0);
            return ((info.flags & ApplicationInfo.FLAG_SYSTEM) != 0);
        } catch (PackageManager.NameNotFoundException e) {
            return false;
        }
    }

    private static String getComponentPackageNameFromString(String from) {
        ComponentName componentName = from != null ? ComponentName.unflattenFromString(from) : null;
        return componentName != null ? componentName.getPackageName() : "";
    }

    @Test
    public void testNonTrustedRecognitionServiceCanBlameCallingApp() throws Throwable {
        // We treat trusted if the current voice recognizer is also a preinstalled app. This is a
        // untrusted case.
        setCurrentRecognizer(CTS_VOICE_RECOGNITION_SERVICE);

        // verify that the untrusted app cannot blame the calling app mic access
        testVoiceRecognitionServiceBlameCallingApp(/* trustVoiceService */ false);
    }

    @Test
    public void testTrustedRecognitionServiceCanBlameCallingApp() throws Throwable {
        // We treat trusted if the current voice recognizer is also a preinstalled app. This is a
        // trusted case.
        boolean hasPreInstalledRecognizer = hasPreInstalledRecognizer(
                getComponentPackageNameFromString(mOriginalVoiceRecognizer));
        assumeTrue("No preinstalled recognizer.", hasPreInstalledRecognizer);

        // verify that the trusted app can blame the calling app mic access
        testVoiceRecognitionServiceBlameCallingApp(/* trustVoiceService */ true);
    }

    private void testVoiceRecognitionServiceBlameCallingApp(boolean trustVoiceService)
            throws Throwable {
        // Start SpeechRecognition
        mActivity.startListening();

        if (isTv()) {
            assertTvIndicatorsShown(trustVoiceService);
        } else {
            assertPrivacyChipAndIndicatorsPresent(trustVoiceService);
        }
    }

    private void assertTvIndicatorsShown(boolean trustVoiceService) {
        Log.v(TAG, "assertTvIndicatorsShown");
        final WindowManagerStateHelper wmState = new WindowManagerStateHelper();
        wmState.waitFor(
                state -> {
                    if (trustVoiceService) {
                        return state.containsWindow(TV_MIC_INDICATOR_WINDOW_TITLE)
                                && state.isWindowVisible(TV_MIC_INDICATOR_WINDOW_TITLE);
                    } else {
                        return !state.containsWindow(TV_MIC_INDICATOR_WINDOW_TITLE);
                    }
                },
                "Waiting for the mic indicator window to come up");
    }

    private void assertPrivacyChipAndIndicatorsPresent(boolean trustVoiceService) {
        // Open notification and verify the privacy indicator is shown
        mUiDevice.openQuickSettings();
        SystemClock.sleep(UI_WAIT_TIMEOUT);

        final UiObject2 privacyChip =
                mUiDevice.findObject(By.res(PRIVACY_CHIP_PACKAGE_NAME, PRIVACY_CHIP_ID));
        assertWithMessage("Can not find mic indicator").that(privacyChip).isNotNull();

        // Click the privacy indicator and verify the calling app name display status in the dialog.
        privacyChip.click();
        SystemClock.sleep(UI_WAIT_TIMEOUT);

        // Make sure dialog is shown
        String contentId = isCar() ? CAR_PRIVACY_DIALOG_CONTENT_ID : PRIVACY_DIALOG_CONTENT_ID;
        List<UiObject2> recognitionCallingAppLabels = mUiDevice.findObjects(
                By.res(PRIVACY_DIALOG_PACKAGE_NAME, contentId));
        assertWithMessage("No permission dialog shown after clicking  privacy chip.").that(
                recognitionCallingAppLabels).isNotEmpty();

        // get dialog content
        final String dialogDescription =
                recognitionCallingAppLabels
                        .stream()
                        .map(UiObject2::getText)
                        .collect(Collectors.joining("\n"));
        Log.i(TAG, "Retrieved dialog description " + dialogDescription);

        if (isCar()) {
            // Make sure Voice recognizer's app label is present in dialog.
            assertWithMessage(
                    "Voice recognition service can blame the calling app name " + APP_LABEL
                            + ", but does not find it.")
                    .that(dialogDescription)
                    .contains(APP_LABEL);
        } else if (trustVoiceService) {
            // Check trust recognizer can blame calling apmic permission
            assertWithMessage(
                    "Trusted voice recognition service can blame the calling app name " + APP_LABEL
                            + ", but does not find it.")
                    .that(dialogDescription)
                    .contains(APP_LABEL);

            // Check trust recognizer cannot blame non-mic permission
            assertWithMessage("Trusted voice recognition service cannot blame non-mic permission")
                    .that(dialogDescription)
                    .doesNotContain(mCameraLabel);
        } else {
            assertWithMessage(
                    "Untrusted voice recognition service cannot blame the calling app name "
                            + APP_LABEL)
                    .that(dialogDescription)
                    .doesNotContain(APP_LABEL);
        }

        // Wait for the privacy indicator to disappear to avoid the test becoming flaky.
        SystemClock.sleep(INDICATOR_DISMISS_TIMEOUT);
    }

    private boolean isTv() {
        PackageManager pm = mContext.getPackageManager();
        return pm.hasSystemFeature(PackageManager.FEATURE_LEANBACK);
    }

    private boolean isCar() {
        PackageManager pm = mContext.getPackageManager();
        return pm.hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE);
    }
}
