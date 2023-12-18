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
 * limitations under the License
 */

package android.server.wm;

import static android.app.ActivityTaskManager.INVALID_STACK_ID;
import static android.provider.Settings.Global.ANIMATOR_DURATION_SCALE;
import static android.server.wm.CliIntentExtra.extraInt;
import static android.server.wm.ComponentNameUtils.getWindowName;
import static android.server.wm.app.Components.BACKGROUND_IMAGE_ACTIVITY;
import static android.server.wm.app.Components.BAD_BLUR_ACTIVITY;
import static android.server.wm.app.Components.BLUR_ACTIVITY;
import static android.server.wm.app.Components.BLUR_ATTRIBUTES_ACTIVITY;
import static android.server.wm.app.Components.BlurActivity.EXTRA_BACKGROUND_BLUR_RADIUS_PX;
import static android.server.wm.app.Components.BlurActivity.EXTRA_BLUR_BEHIND_RADIUS_PX;
import static android.server.wm.app.Components.BlurActivity.EXTRA_NO_BLUR_BACKGROUND_COLOR;
import static android.view.Display.DEFAULT_DISPLAY;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;

import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertFalse;
import static org.junit.Assume.assumeTrue;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.spy;

import android.content.ComponentName;
import android.content.ContentResolver;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Bundle;
import android.platform.test.annotations.Presubmit;
import android.provider.Settings;
import android.view.View;
import android.view.WindowManager;
import android.widget.LinearLayout;

import androidx.test.filters.FlakyTest;

import com.android.compatibility.common.util.ColorUtils;
import com.android.compatibility.common.util.SystemUtil;

import java.util.function.Consumer;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mockito;

@Presubmit
@FlakyTest(detail = "Promote once confirmed non-flaky")
public class BlurTests extends WindowManagerTestBase {
    private static final int BACKGROUND_BLUR_PX = 80;
    private static final int BLUR_BEHIND_PX = 40;
    private static final int NO_BLUR_BACKGROUND_COLOR = Color.BLACK;
    private static final int BLUR_BEHIND_DYNAMIC_UPDATE_WAIT_TIME = 300;
    private static final int BACKGROUND_BLUR_DYNAMIC_UPDATE_WAIT_TIME = 100;
    private static final int DISABLE_BLUR_BROADCAST_WAIT_TIME = 100;
    private float mSavedAnimatorDurationScale;
    private boolean mSavedWindowBlurDisabledSetting;

    @Before
    public void setUp() {
        assumeTrue(supportsBlur());

        mSavedWindowBlurDisabledSetting = Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.DISABLE_WINDOW_BLURS, 0) == 1;
        setForceBlurDisabled(false);
        SystemUtil.runWithShellPermissionIdentity(() -> {
            final ContentResolver resolver = getInstrumentation().getContext().getContentResolver();
            mSavedAnimatorDurationScale =
                    Settings.Global.getFloat(resolver, ANIMATOR_DURATION_SCALE, 1f);
            Settings.Global.putFloat(resolver, ANIMATOR_DURATION_SCALE, 0);
        });
        startTestActivity(BACKGROUND_IMAGE_ACTIVITY);
        verifyOnlyBackgroundImageVisible();
        assertTrue(mContext.getSystemService(WindowManager.class).isCrossWindowBlurEnabled());
    }

    @After
    public void tearDown() {
        if (!supportsBlur()) return;

        SystemUtil.runWithShellPermissionIdentity(() -> {
            Settings.Global.putFloat(getInstrumentation().getContext().getContentResolver(),
                    ANIMATOR_DURATION_SCALE, mSavedAnimatorDurationScale);
        });
        setForceBlurDisabled(mSavedWindowBlurDisabledSetting);
    }

    @Test
    public void testBackgroundBlurSimple() {
        startTestActivity(BLUR_ACTIVITY,
                          extraInt(EXTRA_BACKGROUND_BLUR_RADIUS_PX, BACKGROUND_BLUR_PX));

        final Rect windowFrame = getWindowFrame(BLUR_ACTIVITY);
        assertBackgroundBlur(takeScreenshot(), windowFrame);
    }

    @Test
    public void testBlurBehindSimple() {
        startTestActivity(BLUR_ACTIVITY,
                          extraInt(EXTRA_BLUR_BEHIND_RADIUS_PX, BLUR_BEHIND_PX),
                          extraInt(EXTRA_NO_BLUR_BACKGROUND_COLOR, NO_BLUR_BACKGROUND_COLOR));

        final Bitmap screenshot = takeScreenshot();
        final Rect windowFrame = getWindowFrame(BLUR_ACTIVITY);
        assertBlurBehind(screenshot, windowFrame);
        assertNoBackgroundBlur(screenshot, windowFrame);
    }

    @Test
    public void testNoBackgroundBlurWhenBlurDisabled() {
        setForceBlurDisabled(true);
        startTestActivity(BLUR_ACTIVITY,
                          extraInt(EXTRA_BACKGROUND_BLUR_RADIUS_PX, BACKGROUND_BLUR_PX),
                          extraInt(EXTRA_NO_BLUR_BACKGROUND_COLOR, Color.TRANSPARENT));
        verifyOnlyBackgroundImageVisible();
    }

    @Test
    public void testNoBackgroundBlurForNonTranslucentWindow() {
        startTestActivity(BAD_BLUR_ACTIVITY,
                          extraInt(EXTRA_BACKGROUND_BLUR_RADIUS_PX, BACKGROUND_BLUR_PX),
                          extraInt(EXTRA_NO_BLUR_BACKGROUND_COLOR, Color.TRANSPARENT));
        verifyOnlyBackgroundImageVisible();
    }

    @Test
    public void testNoBlurBehindWhenBlurDisabled() {
        setForceBlurDisabled(true);
        startTestActivity(BLUR_ACTIVITY,
                          extraInt(EXTRA_BLUR_BEHIND_RADIUS_PX, BLUR_BEHIND_PX),
                          extraInt(EXTRA_NO_BLUR_BACKGROUND_COLOR, Color.TRANSPARENT));
        verifyOnlyBackgroundImageVisible();
    }

    @Test
    public void testNoBlurBehindWhenFlagNotSet() {
        startTestActivity(BAD_BLUR_ACTIVITY,
                          extraInt(EXTRA_BLUR_BEHIND_RADIUS_PX, BLUR_BEHIND_PX),
                          extraInt(EXTRA_NO_BLUR_BACKGROUND_COLOR, Color.TRANSPARENT));
        verifyOnlyBackgroundImageVisible();
    }

    @Test
    public void testBackgroundBlurActivatesFallbackDynamically() throws Exception {
        startTestActivity(BLUR_ACTIVITY,
                          extraInt(EXTRA_BACKGROUND_BLUR_RADIUS_PX, BACKGROUND_BLUR_PX),
                          extraInt(EXTRA_NO_BLUR_BACKGROUND_COLOR, NO_BLUR_BACKGROUND_COLOR));
        final Rect windowFrame = getWindowFrame(BLUR_ACTIVITY);

        Bitmap screenshot = takeScreenshot();
        assertBackgroundBlur(takeScreenshot(), windowFrame);
        assertNoBlurBehind(screenshot, windowFrame);

        setForceBlurDisabled(true);
        Thread.sleep(BACKGROUND_BLUR_DYNAMIC_UPDATE_WAIT_TIME);

        screenshot = takeScreenshot();
        assertNoBackgroundBlur(screenshot, windowFrame);
        assertNoBlurBehind(screenshot, windowFrame);

        setForceBlurDisabled(false);
        Thread.sleep(BACKGROUND_BLUR_DYNAMIC_UPDATE_WAIT_TIME);

        screenshot = takeScreenshot();
        assertBackgroundBlur(takeScreenshot(), windowFrame);
        assertNoBlurBehind(screenshot, windowFrame);
    }

    @Test
    public void testBlurBehindDisabledDynamically() throws Exception {
        startTestActivity(BLUR_ACTIVITY,
                          extraInt(EXTRA_BLUR_BEHIND_RADIUS_PX, BLUR_BEHIND_PX),
                          extraInt(EXTRA_NO_BLUR_BACKGROUND_COLOR, NO_BLUR_BACKGROUND_COLOR));
        final Rect windowFrame = getWindowFrame(BLUR_ACTIVITY);

        Bitmap screenshot = takeScreenshot();
        assertBlurBehind(screenshot, windowFrame);
        assertNoBackgroundBlur(screenshot, windowFrame);

        setForceBlurDisabled(true);
        Thread.sleep(BLUR_BEHIND_DYNAMIC_UPDATE_WAIT_TIME);

        screenshot = takeScreenshot();
        assertNoBackgroundBlur(screenshot, windowFrame);
        assertNoBlurBehind(screenshot, windowFrame);

        setForceBlurDisabled(false);
        Thread.sleep(BLUR_BEHIND_DYNAMIC_UPDATE_WAIT_TIME);

        screenshot = takeScreenshot();
        assertBlurBehind(screenshot,  windowFrame);
        assertNoBackgroundBlur(screenshot, windowFrame);
    }

    @Test
    public void testBlurBehindAndBackgroundBlur() throws Exception {
        startTestActivity(BLUR_ACTIVITY,
                          extraInt(EXTRA_BLUR_BEHIND_RADIUS_PX, BLUR_BEHIND_PX),
                          extraInt(EXTRA_NO_BLUR_BACKGROUND_COLOR, NO_BLUR_BACKGROUND_COLOR),
                          extraInt(EXTRA_BACKGROUND_BLUR_RADIUS_PX, BACKGROUND_BLUR_PX));
        final Rect windowFrame = getWindowFrame(BLUR_ACTIVITY);

        Bitmap screenshot = takeScreenshot();
        assertBlurBehind(screenshot, windowFrame);
        assertBackgroundBlurOverBlurBehind(screenshot, windowFrame);

        setForceBlurDisabled(true);
        Thread.sleep(BLUR_BEHIND_DYNAMIC_UPDATE_WAIT_TIME);

        screenshot = takeScreenshot();
        assertNoBackgroundBlur(screenshot, windowFrame);
        assertNoBlurBehind(screenshot, windowFrame);

        setForceBlurDisabled(false);
        Thread.sleep(BLUR_BEHIND_DYNAMIC_UPDATE_WAIT_TIME);

        screenshot = takeScreenshot();
        assertBlurBehind(screenshot, windowFrame);
        assertBackgroundBlurOverBlurBehind(screenshot, windowFrame);
    }

    @Test
    public void testBlurBehindAndBackgroundBlurSetWithAttributes() {
        startTestActivity(BLUR_ATTRIBUTES_ACTIVITY);
        final Rect windowFrame = getWindowFrame(BLUR_ATTRIBUTES_ACTIVITY);
        final Bitmap screenshot = takeScreenshot();

        assertBlurBehind(screenshot, windowFrame);
        assertBackgroundBlurOverBlurBehind(screenshot, windowFrame);
    }

    @Test
    public void testBlurDestroyedAfterActivityFinished() {
        startTestActivity(BLUR_ACTIVITY,
                          extraInt(EXTRA_BLUR_BEHIND_RADIUS_PX, BLUR_BEHIND_PX),
                          extraInt(EXTRA_NO_BLUR_BACKGROUND_COLOR, NO_BLUR_BACKGROUND_COLOR),
                          extraInt(EXTRA_BACKGROUND_BLUR_RADIUS_PX, BACKGROUND_BLUR_PX));
        final Rect windowFrame = getWindowFrame(BLUR_ACTIVITY);
        Bitmap screenshot = takeScreenshot();

        assertBlurBehind(screenshot, windowFrame);
        assertBackgroundBlurOverBlurBehind(screenshot, windowFrame);

        mBroadcastActionTrigger.finishBroadcastReceiverActivity();
        mWmState.waitAndAssertActivityRemoved(BLUR_ACTIVITY);

        verifyOnlyBackgroundImageVisible();
    }

    @Test
    public void testIsCrossWindowBlurEnabledUpdatedCorrectly() throws Exception {
        setForceBlurDisabled(true);
        Thread.sleep(DISABLE_BLUR_BROADCAST_WAIT_TIME);
        assertFalse(mContext.getSystemService(WindowManager.class).isCrossWindowBlurEnabled());

        setForceBlurDisabled(false);
        Thread.sleep(DISABLE_BLUR_BROADCAST_WAIT_TIME);
        assertTrue(mContext.getSystemService(WindowManager.class).isCrossWindowBlurEnabled());
    }

    @Test
    public void testBlurListener() throws Exception {
        ListenerTestActivity activity = startActivity(ListenerTestActivity.class);
        Mockito.verify(activity.mBlurEnabledListener).accept(true);

        setForceBlurDisabled(true);
        Thread.sleep(DISABLE_BLUR_BROADCAST_WAIT_TIME);
        assertFalse(mContext.getSystemService(WindowManager.class).isCrossWindowBlurEnabled());
        Mockito.verify(activity.mBlurEnabledListener).accept(false);

        setForceBlurDisabled(false);
        Thread.sleep(DISABLE_BLUR_BROADCAST_WAIT_TIME);
        assertTrue(mContext.getSystemService(WindowManager.class).isCrossWindowBlurEnabled());
        Mockito.verify(activity.mBlurEnabledListener, times(2)).accept(true);
    }

    public static class BlurListener implements Consumer<Boolean> {
        @Override
        public void accept(Boolean enabled) {}
    }

    public static class ListenerTestActivity extends FocusableActivity {
        Consumer<Boolean> mBlurEnabledListener = spy(new BlurListener());

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            View v = new LinearLayout(this);
            v.addOnAttachStateChangeListener(new View.OnAttachStateChangeListener() {
                @Override
                public void onViewAttachedToWindow(View view) {
                    getWindowManager().addCrossWindowBlurEnabledListener(mBlurEnabledListener);
                }

                @Override
                public void onViewDetachedFromWindow(View view) {
                    getWindowManager().removeCrossWindowBlurEnabledListener(mBlurEnabledListener);
                }
            });
            setContentView(v);
        }
    }

    private void startTestActivity(ComponentName activityName, final CliIntentExtra... extras) {
        launchActivity(activityName, extras);
        assertNotEquals(mWmState.getRootTaskIdByActivity(activityName), INVALID_STACK_ID);
        waitAndAssertResumedActivity(activityName, activityName + " must be resumed");
        mWmState.waitForAppTransitionIdleOnDisplay(DEFAULT_DISPLAY);
    }


    private Rect getWindowFrame(ComponentName activityName) {
        String windowName = getWindowName(activityName);
        mWmState.computeState(activityName);
        return mWmState.getMatchingVisibleWindowState(windowName).get(0).getFrame();
    }

    private void verifyOnlyBackgroundImageVisible() {
        final Bitmap screenshot = takeScreenshot();
        final int height = screenshot.getHeight();
        final int width = screenshot.getWidth();

        final int blueWidth = width / 2;

        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                if (x < blueWidth) {
                    ColorUtils.verifyColor("failed for pixel (x, y) = (" + x + ", " + y + ")",
                            Color.BLUE, screenshot.getPixel(x, y), 0);
                } else {
                    ColorUtils.verifyColor("failed for pixel (x, y) = (" + x + ", " + y + ")",
                            Color.RED, screenshot.getPixel(x, y), 0);
                }
            }
        }
    }

    private static void assertBlurBehind(Bitmap screenshot, Rect windowFrame) {
        assertBlur(screenshot, BLUR_BEHIND_PX, 0, windowFrame.top);
        assertBlur(screenshot, BLUR_BEHIND_PX, windowFrame.bottom, screenshot.getHeight());
    }

    private static void assertBackgroundBlur(Bitmap screenshot, Rect windowFrame) {
        assertBlur(screenshot, BACKGROUND_BLUR_PX, windowFrame.top, windowFrame.bottom);
    }

    private static void assertBackgroundBlurOverBlurBehind(Bitmap screenshot, Rect windowFrame) {
        // We are assuming that the background blur will become bigger by roughly half of the blur
        // behind radius
        assertBlur(screenshot, BACKGROUND_BLUR_PX + ((int) (BLUR_BEHIND_PX*0.5f)),
                windowFrame.top, windowFrame.bottom);
    }

    private static void assertNoBlurBehind(Bitmap screenshot, Rect windowFrame) {
        for (int x = 0; x < screenshot.getWidth(); x++) {
            for (int y = 0; y < screenshot.getHeight(); y++) {
                if (x < windowFrame.left) {
                    ColorUtils.verifyColor("failed for pixel (x, y) = (" + x + ", " + y + ")",
                            Color.BLUE, screenshot.getPixel(x, y), 0);
                } else if (x < screenshot.getWidth() / 2) {
                    if (y < windowFrame.top || y > windowFrame.bottom) {
                        ColorUtils.verifyColor("failed for pixel (x, y) = (" + x + ", " + y + ")",
                                Color.BLUE, screenshot.getPixel(x, y), 0);
                    }
                } else if (x <= windowFrame.right) {
                    if (y < windowFrame.top || y > windowFrame.bottom) {
                        ColorUtils.verifyColor("failed for pixel (x, y) = (" + x + ", " + y + ")",
                                Color.RED, screenshot.getPixel(x, y), 0);
                    }
                } else if (x > windowFrame.right) {
                    ColorUtils.verifyColor("failed for pixel (x, y) = (" + x + ", " + y + ")",
                            Color.RED, screenshot.getPixel(x, y), 0);
                }

            }
        }
    }

    private static void assertNoBackgroundBlur(Bitmap screenshot, Rect windowFrame) {
        for (int y = windowFrame.top; y < windowFrame.bottom; y++) {
            for (int x = windowFrame.left; x < windowFrame.right; x++) {
                ColorUtils.verifyColor("failed for pixel (x, y) = (" + x + ", " + y + ")",
                        NO_BLUR_BACKGROUND_COLOR, screenshot.getPixel(x, y), 0);
            }
        }
    }

    private static void assertBlur(Bitmap screenshot, int blurRadius, int startHeight,
                                   int endHeight) {
        final int width = screenshot.getWidth();

        // Adjust the test to check a smaller part of the blurred area in order to accept various
        // blur algorithm approximations used in RenderEngine
        final int stepSize = blurRadius / 4;
        final int blurAreaStartX = width / 2 - blurRadius + stepSize;
        final int blurAreaEndX = width / 2 + blurRadius;

        Color previousColor;
        Color currentColor;
        final int unaffectedBluePixelX = width / 2 - blurRadius - 1;
        final int unaffectedRedPixelX = width / 2 + blurRadius + 1;
        for (int y = startHeight; y < endHeight; y++) {
            ColorUtils.verifyColor(
                    "failed for pixel (x, y) = (" + unaffectedBluePixelX + ", " + y + ")",
                    Color.BLUE, screenshot.getPixel(unaffectedBluePixelX, y), 0);
            previousColor = Color.valueOf(Color.BLUE);
            for (int x = blurAreaStartX; x <= blurAreaEndX; x += stepSize) {
                currentColor = screenshot.getColor(x, y);
                assertTrue("assertBlur failed for blue for pixel (x, y) = (" + x + ", " + y + ");"
                        + " previousColor blue: " + previousColor.blue()
                        + ", currentColor blue: " + currentColor.blue()
                        , previousColor.blue() > currentColor.blue());
                assertTrue("assertBlur failed for red for pixel (x, y) = (" + x + ", " + y + ");"
                       + " previousColor red: " + previousColor.red()
                       + ", currentColor red: " + currentColor.red(),
                       previousColor.red() < currentColor.red());

                previousColor = currentColor;
            }
            ColorUtils.verifyColor(
                    "failed for pixel (x, y) = (" + unaffectedRedPixelX + ", " + y + ")",
                    Color.RED, screenshot.getPixel(unaffectedRedPixelX, y), 0);
        }
    }

    private void setForceBlurDisabled(boolean disable) {
        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.DISABLE_WINDOW_BLURS, disable ? 1 : 0);
    }
}
