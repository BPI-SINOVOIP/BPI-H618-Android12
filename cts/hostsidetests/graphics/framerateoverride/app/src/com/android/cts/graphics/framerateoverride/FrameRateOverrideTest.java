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

package com.android.cts.graphics.framerateoverride;

import android.Manifest;
import android.app.compat.CompatChanges;
import android.hardware.display.DisplayManager;
import android.os.Handler;
import android.os.Looper;
import android.support.test.uiautomator.UiDevice;
import android.sysprop.SurfaceFlingerProperties;
import android.util.Log;
import android.view.Display;
import android.view.Window;
import android.view.WindowManager;

import androidx.test.InstrumentationRegistry;
import androidx.test.rule.ActivityTestRule;
import androidx.test.runner.AndroidJUnit4;

import com.android.cts.graphics.framerateoverride.FrameRateOverrideTestActivity.FrameRateObserver;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for frame rate override and the behaviour of {@link Display#getRefreshRate()} and
 * {@link Display.Mode#getRefreshRate()} Api.
 */
@RunWith(AndroidJUnit4.class)
public final class FrameRateOverrideTest {
    private static final String TAG = "FrameRateOverrideTest";
    // See b/170503758 for more details
    private static final long DISPLAY_MODE_RETURNS_PHYSICAL_REFRESH_RATE_CHANGEID = 170503758;

    // The tolerance within which we consider refresh rates are equal
    private static final float REFRESH_RATE_TOLERANCE = 0.01f;

    private int mInitialMatchContentFrameRate;
    private DisplayManager mDisplayManager;


    @Rule
    public ActivityTestRule<FrameRateOverrideTestActivity> mActivityRule =
            new ActivityTestRule<>(FrameRateOverrideTestActivity.class);

    @Before
    public void setUp() throws Exception {
        final UiDevice uiDevice =
                UiDevice.getInstance(
                        androidx.test.platform.app.InstrumentationRegistry.getInstrumentation());
        uiDevice.wakeUp();
        uiDevice.executeShellCommand("wm dismiss-keyguard");

        InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .adoptShellPermissionIdentity(Manifest.permission.LOG_COMPAT_CHANGE,
                        Manifest.permission.READ_COMPAT_CHANGE_CONFIG,
                        Manifest.permission.MODIFY_REFRESH_RATE_SWITCHING_TYPE,
                        Manifest.permission.OVERRIDE_DISPLAY_MODE_REQUESTS);

        mDisplayManager = mActivityRule.getActivity().getSystemService(DisplayManager.class);
        mInitialMatchContentFrameRate = toSwitchingType(
                mDisplayManager.getMatchContentFrameRateUserPreference());
        mDisplayManager.setRefreshRateSwitchingType(DisplayManager.SWITCHING_TYPE_NONE);
        mDisplayManager.setShouldAlwaysRespectAppRequestedMode(true);
        boolean changeIsEnabled =
                CompatChanges.isChangeEnabled(DISPLAY_MODE_RETURNS_PHYSICAL_REFRESH_RATE_CHANGEID);
        Log.e(TAG, "DISPLAY_MODE_RETURNS_PHYSICAL_REFRESH_RATE_CHANGEID is "
                + (changeIsEnabled ? "enabled" : "disabled"));
    }

    @After
    public void tearDown() {
        mDisplayManager.setRefreshRateSwitchingType(mInitialMatchContentFrameRate);
        mDisplayManager.setShouldAlwaysRespectAppRequestedMode(false);
        InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .dropShellPermissionIdentity();
    }

    private int toSwitchingType(int matchContentFrameRateUserPreference) {
        switch (matchContentFrameRateUserPreference) {
            case DisplayManager.MATCH_CONTENT_FRAMERATE_NEVER:
                return DisplayManager.SWITCHING_TYPE_NONE;
            case DisplayManager.MATCH_CONTENT_FRAMERATE_SEAMLESSS_ONLY:
                return DisplayManager.SWITCHING_TYPE_WITHIN_GROUPS;
            case DisplayManager.MATCH_CONTENT_FRAMERATE_ALWAYS:
                return DisplayManager.SWITCHING_TYPE_ACROSS_AND_WITHIN_GROUPS;
            default:
                return -1;
        }
    }

    private void setMode(Display.Mode mode) {
        Handler handler = new Handler(Looper.getMainLooper());
        handler.post(() -> {
            Window window = mActivityRule.getActivity().getWindow();
            WindowManager.LayoutParams params = window.getAttributes();
            params.preferredDisplayModeId = mode.getModeId();
            window.setAttributes(params);
        });

    }

    private static boolean areEqual(float a, float b) {
        return Math.abs(a - b) <= REFRESH_RATE_TOLERANCE;
    }

    // Find refresh rates where the device also natively supports half that rate with the same
    // resolution (for example, a 120Hz mode when the device also supports a 60Hz mode).
    private List<Display.Mode> getModesToTest() {
        List<Display.Mode> modesToTest = new ArrayList<>();
        if (!SurfaceFlingerProperties.enable_frame_rate_override().orElse(false)) {
            return modesToTest;
        }
        Display.Mode[] modes = mActivityRule.getActivity().getDisplay().getSupportedModes();
        for (Display.Mode mode : modes) {
            for (Display.Mode otherMode : modes) {
                if (mode.getModeId() == otherMode.getModeId()) {
                    continue;
                }

                if (mode.getPhysicalHeight() != otherMode.getPhysicalHeight()
                        || mode.getPhysicalWidth() != otherMode.getPhysicalWidth()) {
                    continue;
                }

                if (areEqual(mode.getRefreshRate(), 2 * otherMode.getRefreshRate())) {
                    modesToTest.add(mode);
                }
            }
        }

        return modesToTest;
    }

    private void testFrameRateOverride(FrameRateObserver frameRateObserver)
            throws InterruptedException {
        FrameRateOverrideTestActivity activity = mActivityRule.getActivity();
        for (Display.Mode mode : getModesToTest()) {
            setMode(mode);
            activity.testFrameRateOverride(frameRateObserver, mode.getRefreshRate());
        }
    }

    /**
     * Test run by
     * FrameRateOverrideHostTest.testBackpressureDisplayModeReturnsPhysicalRefreshRateEnabled and
     * FrameRateOverrideHostTest.testBackpressureDisplayModeReturnsPhysicalRefreshRateDisabled
     */
    @Test
    public void testBackpressure()
            throws InterruptedException {
        FrameRateOverrideTestActivity activity = mActivityRule.getActivity();
        testFrameRateOverride(activity.new BackpressureFrameRateObserver());
    }

    /**
     * Test run by
     * FrameRateOverrideHostTest.testChoreographerDisplayModeReturnsPhysicalRefreshRateEnabled and
     * FrameRateOverrideHostTest.testChoreographerDisplayModeReturnsPhysicalRefreshRateDisabled
     */
    @Test
    public void testChoreographer()
            throws InterruptedException {
        FrameRateOverrideTestActivity activity = mActivityRule.getActivity();
        testFrameRateOverride(activity.new ChoreographerFrameRateObserver());
    }

    /**
     * Test run by
     * FrameRateOverrideHostTest
     * .testDisplayGetRefreshRateDisplayModeReturnsPhysicalRefreshRateEnabled
     * and
     * FrameRateOverrideHostTest
     * .testDisplayGetRefreshRateDisplayModeReturnsPhysicalRefreshRateDisabled
     */
    @Test
    public void testDisplayGetRefreshRate()
            throws InterruptedException {
        FrameRateOverrideTestActivity activity = mActivityRule.getActivity();
        testFrameRateOverride(activity.new DisplayGetRefreshRateFrameRateObserver());
    }

    /**
     * Test run by
     * FrameRateOverrideHostTest
     * .testDisplayModeGetRefreshRateDisplayModeReturnsPhysicalRefreshRateEnabled
     */
    @Test
    public void testDisplayModeGetRefreshRateDisplayModeReturnsPhysicalRefreshRateEnabled()
            throws InterruptedException {
        FrameRateOverrideTestActivity activity = mActivityRule.getActivity();
        testFrameRateOverride(
                activity.new DisplayModeGetRefreshRateFrameRateObserver(
                        /*displayModeReturnsPhysicalRefreshRateEnabled*/ true));
    }

    /**
     * Test run by
     * FrameRateOverrideHostTest
     * .testDisplayModeGetRefreshRateDisplayModeReturnsPhysicalRefreshRateDisabled
     */
    @Test
    public void testDisplayModeGetRefreshRateDisplayModeReturnsPhysicalRefreshRateDisabled()
            throws InterruptedException {
        FrameRateOverrideTestActivity activity = mActivityRule.getActivity();
        testFrameRateOverride(
                activity.new DisplayModeGetRefreshRateFrameRateObserver(
                        /*displayModeReturnsPhysicalRefreshRateEnabled*/ false));
    }
}
