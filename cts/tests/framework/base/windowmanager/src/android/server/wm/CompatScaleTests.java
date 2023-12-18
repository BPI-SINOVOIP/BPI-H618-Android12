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

package android.server.wm;

import static android.server.wm.app.Components.UI_SCALING_TEST_ACTIVITY;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.ComponentName;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.Arrays;

/**
 * The test is focused on compatibility scaling, and tests the feature form two sides.
 * 1. It checks that the applications "sees" the metrics in PXs, but the DP metrics remain the same.
 * 2. It checks the WindowManagerServer state, and makes sure that the scaling is correctly
 * reflected in the WindowState.
 *
 * This is achieved by launching a {@link android.server.wm.app.UiScalingTestActivity} and having it
 * reporting the metrics it receives.
 * The Activity also draws 3 UI elements: a text, a red square with a 100dp side and a blue square
 * with a 100px side.
 * The text and the red square should have the same when rendered on the screen (by HWC) both when
 * the compat downscaling is enabled and disabled.
 * TODO(b/180098454): Add tests to make sure that the UI elements, which have their sizes declared
 * in DPs (the text and the red square) have the same sizes on the screen (after composition).
 *
 * <p>Build/Install/Run:
 *     atest CtsWindowManagerDeviceTestCases:CompatScaleTests
 */
@RunWith(Parameterized.class)
public class CompatScaleTests extends ActivityManagerTestBase {

    @Parameterized.Parameters(name = "{0}")
    public static Iterable<Object[]> data() {
        return Arrays.asList(new Object[][] {
                { "DOWNSCALE_30", 0.3f },
                { "DOWNSCALE_35", 0.35f },
                { "DOWNSCALE_40", 0.4f },
                { "DOWNSCALE_45", 0.45f },
                { "DOWNSCALE_50", 0.5f },
                { "DOWNSCALE_55", 0.55f },
                { "DOWNSCALE_60", 0.6f },
                { "DOWNSCALE_65", 0.65f },
                { "DOWNSCALE_70", 0.7f },
                { "DOWNSCALE_75", 0.75f },
                { "DOWNSCALE_80", 0.8f },
                { "DOWNSCALE_85", 0.85f },
                { "DOWNSCALE_90", 0.9f },
        });
    }

    private static final ComponentName ACTIVITY_UNDER_TEST = UI_SCALING_TEST_ACTIVITY;
    private static final String PACKAGE_UNDER_TEST = ACTIVITY_UNDER_TEST.getPackageName();
    private static final float EPSILON_GLOBAL_SCALE = 0.01f;

    private final String mCompatChangeName;
    private final float mCompatScale;
    private final float mInvCompatScale;
    private CommandSession.SizeInfo mAppSizesNormal;
    private CommandSession.SizeInfo mAppSizesDownscaled;
    private WindowManagerState.WindowState mWindowStateNormal;
    private WindowManagerState.WindowState mWindowStateDownscaled;

    public CompatScaleTests(String compatChangeName, float compatScale) {
        mCompatChangeName = compatChangeName;
        mCompatScale = compatScale;
        mInvCompatScale = 1 / mCompatScale;
    }

    // TODO(b/180343437): replace @Before with @BeforeParam
    @Before
    public void launchInNormalAndDownscaleMode_collectSizesAndWindowState() {
        // Launch activity with downscaling *disabled* and get the sizes it reports and its Window
        // state.
        launchActivity();
        mAppSizesNormal = getActivityReportedSizes();
        mWindowStateNormal = getPackageWindowState();

        // Now launch the same activity with downscaling *enabled* and get the sizes it reports and
        // its Window state.
        enableDownscaling(mCompatChangeName);
        launchActivity();
        mAppSizesDownscaled = getActivityReportedSizes();
        mWindowStateDownscaled = getPackageWindowState();
    }

    /**
     * Tests that the Density DPI that the application receives from the
     * {@link android.content.res.Configuration} is correctly scaled in the downscaled mode.
     * @see android.content.res.Configuration#densityDpi
     */
    @Test
    public void test_config_densityDpi_scalesCorrectly_inCompatDownscalingMode() {
        assertScaled("Density DPI should scale by " + mCompatScale,
                mAppSizesNormal.densityDpi, mCompatScale, mAppSizesDownscaled.densityDpi);
    }

    /**
     * Tests that the screen sizes in DPs that the application receives from the
     * {@link android.content.res.Configuration} are NOT scaled in the downscaled mode.
     * @see android.content.res.Configuration#screenWidthDp
     * @see android.content.res.Configuration#screenHeightDp
     * @see android.content.res.Configuration#smallestScreenWidthDp
     */
    @Test
    public void test_config_screenSize_inDPs_doesNotChange_inCompatDownscalingMode() {
        assertEquals("Width shouldn't change",
                mAppSizesNormal.widthDp, mAppSizesDownscaled.widthDp);
        assertEquals("Height shouldn't change",
                mAppSizesNormal.heightDp, mAppSizesDownscaled.heightDp);
        assertEquals("Smallest Width shouldn't change",
                mAppSizesNormal.smallestWidthDp, mAppSizesDownscaled.smallestWidthDp);
    }

    /**
     * Tests that the Window sizes in PXs that the application receives from the
     * {@link android.content.res.Configuration} are scaled correctly in the downscaled mode.
     * @see android.content.res.Configuration#windowConfiguration
     * @see android.app.WindowConfiguration#getBounds()
     * @see android.app.WindowConfiguration#getAppBounds()
     */
    @Test
    public void test_config_windowSizes_inPXs_scaleCorrectly_inCompatDownscalingMode() {
        assertScaled("Width should scale by " + mCompatScale,
                mAppSizesNormal.windowWidth, mCompatScale, mAppSizesDownscaled.windowWidth);
        assertScaled("Height should scale by " + mCompatScale,
                mAppSizesNormal.windowHeight, mCompatScale, mAppSizesDownscaled.windowHeight);
        assertScaled("App width should scale by " + mCompatScale,
                mAppSizesNormal.windowAppWidth, mCompatScale, mAppSizesDownscaled.windowAppWidth);
        assertScaled("App height should scale by " + mCompatScale,
                mAppSizesNormal.windowAppHeight, mCompatScale, mAppSizesDownscaled.windowAppHeight);
    }

    /**
     * Tests that the {@link android.util.DisplayMetrics} in PXs that the application can obtain via
     * {@link android.content.res.Resources#getDisplayMetrics()} are scaled correctly in the
     * downscaled mode.
     * @see android.util.DisplayMetrics#widthPixels
     * @see android.util.DisplayMetrics#heightPixels
     */
    @Test
    public void test_displayMetrics_inPXs_scaleCorrectly_inCompatDownscalingMode() {
        assertScaled("Width should scale by " + mCompatScale,
                mAppSizesNormal.metricsWidth, mCompatScale, mAppSizesDownscaled.metricsWidth);
        assertScaled("Height should scale by " + mCompatScale,
                mAppSizesNormal.metricsHeight, mCompatScale, mAppSizesDownscaled.metricsHeight);
    }

    /**
     * Tests that the dimensions of a {@link android.view.Display} in PXs that the application can
     * obtain via {@link android.view.View#getDisplay()} are scaled correctly in the downscaled
     * mode.
     * @see android.view.Display#getSize(android.graphics.Point)
     */
    @Test
    public void test_displaySize_inPXs_scaleCorrectly_inCompatDownscalingMode() {
        assertScaled("Width should scale by " + mCompatScale,
                mAppSizesNormal.displayWidth, mCompatScale, mAppSizesDownscaled.displayWidth);
        assertScaled("Height should scale by " + mCompatScale,
                mAppSizesNormal.displayHeight, mCompatScale, mAppSizesDownscaled.displayHeight);
    }

    /**
     * Test that compatibility downscaling is reflected correctly on the WM side.
     * @see android.server.wm.WindowManagerState.WindowState
     */
    @Test
    public void test_windowState_inCompatDownscalingMode() {
        // Check the "normal" window's state for disabled compat mode and appropriate global scale.
        assertFalse("The Window should not be in the size compat mode",
                mWindowStateNormal.hasCompatScale());
        assertEquals("The window should not be scaled",
                1f, mWindowStateNormal.getGlobalScale(), EPSILON_GLOBAL_SCALE);

        // Check the "downscaled" window's state for enabled compat mode and appropriate global
        // scale.
        assertTrue("The Window should be in the size compat mode",
                mWindowStateDownscaled.hasCompatScale());
        assertEquals("The window should have global scale of " + mInvCompatScale,
                mInvCompatScale, mWindowStateDownscaled.getGlobalScale(), EPSILON_GLOBAL_SCALE);

        // Make sure the frame sizes changed correctly.
        assertEquals("Window frame on should not change",
                mWindowStateNormal.getFrame(), mWindowStateDownscaled.getFrame());
        assertScaled("Requested width should scale by " + mCompatScale,
                mWindowStateNormal.getRequestedWidth(), mCompatScale,
                mWindowStateDownscaled.getRequestedWidth());
        assertScaled("Requested height should scale by " + mCompatScale,
                mWindowStateNormal.getRequestedHeight(), mCompatScale,
                mWindowStateDownscaled.getRequestedHeight());
    }

    @After
    public void tearDown() {
        disableDownscaling(mCompatChangeName);
    }

    private void launchActivity() {
        launchActivityInNewTask(ACTIVITY_UNDER_TEST);
        mWmState.computeState(new WaitForValidActivityState(ACTIVITY_UNDER_TEST));
    }

    private CommandSession.SizeInfo getActivityReportedSizes() {
        final CommandSession.SizeInfo details =
                getLastReportedSizesForActivity(ACTIVITY_UNDER_TEST);
        assertNotNull(details);
        return details;
    }

    private WindowManagerState.WindowState getPackageWindowState() {
        return getPackageWindowState(PACKAGE_UNDER_TEST);
    }

    private static void enableDownscaling(String compatChangeName) {
        executeShellCommand("am compat enable " + compatChangeName + " " + PACKAGE_UNDER_TEST);
        executeShellCommand("am compat enable DOWNSCALED " + PACKAGE_UNDER_TEST);
    }

    private static void disableDownscaling(String compatChangeName) {
        executeShellCommand("am compat disable DOWNSCALED " + PACKAGE_UNDER_TEST);
        executeShellCommand("am compat disable " + compatChangeName + " " + PACKAGE_UNDER_TEST);
    }

    private static void assertScaled(String message, int baseValue, float expectedScale,
            int actualValue) {
        // In order to account for possible rounding errors, let's calculate the actual scale and
        // compare it's against the expected scale (allowing a small delta).
        final float actualScale = ((float) actualValue) / baseValue;
        assertEquals(message, expectedScale, actualScale, EPSILON_GLOBAL_SCALE);
    }
}
