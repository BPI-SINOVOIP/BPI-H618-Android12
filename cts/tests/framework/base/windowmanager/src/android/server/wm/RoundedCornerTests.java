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

import static android.content.pm.ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
import static android.content.pm.ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
import static android.content.pm.ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
import static android.content.pm.ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT;
import static android.content.pm.ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
import static android.server.wm.RoundedCornerTests.TestActivity.EXTRA_ORIENTATION;
import static android.view.RoundedCorner.POSITION_BOTTOM_LEFT;
import static android.view.RoundedCorner.POSITION_BOTTOM_RIGHT;
import static android.view.RoundedCorner.POSITION_TOP_LEFT;
import static android.view.RoundedCorner.POSITION_TOP_RIGHT;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
import static android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION_PANEL;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Rect;
import android.os.Bundle;
import android.platform.test.annotations.Presubmit;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.RoundedCorner;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.WindowMetrics;

import androidx.test.rule.ActivityTestRule;

import com.android.compatibility.common.util.PollingCheck;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

@Presubmit
@RunWith(Parameterized.class)
public class RoundedCornerTests {
    private static final String TAG = "RoundedCornerTests";
    private final static int POSITION_LENGTH = 4;
    private final static long TIMEOUT = 1000; // milliseconds

    @Parameterized.Parameters(name= "{1}({0})")
    public static Object[][] data() {
        return new Object[][]{
                {SCREEN_ORIENTATION_PORTRAIT, "SCREEN_ORIENTATION_PORTRAIT"},
                {SCREEN_ORIENTATION_LANDSCAPE, "SCREEN_ORIENTATION_LANDSCAPE"},
                {SCREEN_ORIENTATION_REVERSE_LANDSCAPE, "SCREEN_ORIENTATION_REVERSE_LANDSCAPE"},
                {SCREEN_ORIENTATION_REVERSE_PORTRAIT, "SCREEN_ORIENTATION_REVERSE_PORTRAIT"},
        };
    }

    @Parameterized.Parameter(0)
    public int orientation;

    @Parameterized.Parameter(1)
    public String orientationName;

    @Rule
    public final ActivityTestRule<TestActivity> mTestActivity =
            new ActivityTestRule<>(TestActivity.class, false /* initialTouchMode */,
                    false /* launchActivity */);

    @Test
    public void testRoundedCorner_fullscreen() {
        final TestActivity activity = mTestActivity.launchActivity(
                new Intent().putExtra(EXTRA_ORIENTATION, orientation));
        runOnMainSync(() -> {
            activity.addChildWindow(
                    activity.calculateWindowBounds(false /* excludeRoundedCorners */));
        });
        // Make sure the child window has been laid out.
        final View childWindowRoot = activity.getChildWindowRoot();
        PollingCheck.waitFor(TIMEOUT, () -> childWindowRoot.getWidth() > 0);
        PollingCheck.waitFor(TIMEOUT, () -> activity.getDispatchedInsets() != null);
        final WindowInsets insets = activity.getDispatchedInsets();

        final Display display = activity.getDisplay();
        for (int i = 0; i < POSITION_LENGTH; i++) {
            assertEquals(insets.getRoundedCorner(i), display.getRoundedCorner(i));
        }
    }

    @Test
    public void testRoundedCorner_excludeRoundedCorners() {
        final TestActivity activity = mTestActivity.launchActivity(
                new Intent().putExtra(EXTRA_ORIENTATION, orientation));
        if (!activity.hasRoundedCorners()) {
            Log.d(TAG, "There is no rounded corner on the display. Skipped!!");
            return;
        }
        runOnMainSync(() -> {
            activity.addChildWindow(
                    activity.calculateWindowBounds(true /* excludeRoundedCorners */));
        });

        // Make sure the child window has been laid out.
        final View childWindowRoot = activity.getChildWindowRoot();
        PollingCheck.waitFor(TIMEOUT, () -> childWindowRoot.getWidth() > 0);
        PollingCheck.waitFor(TIMEOUT, () -> activity.getDispatchedInsets() != null);
        final WindowInsets insets = activity.getDispatchedInsets();

        for (int i = 0; i < POSITION_LENGTH; i++) {
            assertNull("The rounded corners should be null.", insets.getRoundedCorner(i));
        }
    }

    private void runOnMainSync(Runnable runnable) {
        getInstrumentation().runOnMainSync(runnable);
    }

    public static class TestActivity extends Activity {
        static final String EXTRA_ORIENTATION = "extra.orientation";

        private View mChildWindowRoot;
        private WindowInsets mDispatchedInsets;

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            getWindow().requestFeature(Window.FEATURE_NO_TITLE);
            getWindow().getAttributes().layoutInDisplayCutoutMode =
                    LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
            if (getIntent() != null) {
                setRequestedOrientation(getIntent().getIntExtra(
                        EXTRA_ORIENTATION, SCREEN_ORIENTATION_UNSPECIFIED));
            }
        }

        void addChildWindow(Rect bounds) {
            final WindowManager.LayoutParams attrs = new WindowManager.LayoutParams();
            attrs.x = bounds.left;
            attrs.y = bounds.top;
            attrs.width = bounds.width();
            attrs.height = bounds.height();
            attrs.gravity = Gravity.LEFT | Gravity.TOP;
            attrs.layoutInDisplayCutoutMode = LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
            attrs.flags = FLAG_NOT_FOCUSABLE;
            attrs.setFitInsetsTypes(0);
            mChildWindowRoot = new View(this);
            mChildWindowRoot.setOnApplyWindowInsetsListener(
                    (v, insets) -> mDispatchedInsets = insets);
            getWindowManager().addView(mChildWindowRoot, attrs);
        }

        View getChildWindowRoot() {
            return mChildWindowRoot;
        }

        WindowInsets getDispatchedInsets() {
            return mDispatchedInsets;
        }

        boolean hasRoundedCorners() {
            final Display display = getDisplay();
            return display.getRoundedCorner(POSITION_TOP_LEFT) != null
                    || display.getRoundedCorner(POSITION_TOP_RIGHT) != null
                    || display.getRoundedCorner(POSITION_BOTTOM_RIGHT) != null
                    || display.getRoundedCorner(POSITION_BOTTOM_LEFT) != null;
        }

        Rect calculateWindowBounds(boolean excludeRoundedCorners) {
            final Display display = getDisplay();
            final WindowMetrics windowMetrics = getWindowManager().getMaximumWindowMetrics();
            if (!excludeRoundedCorners) {
                return windowMetrics.getBounds();
            }
            final Rect bounds = new Rect();
            final int width = windowMetrics.getBounds().width();
            final int height = windowMetrics.getBounds().height();
            final RoundedCorner topLeft = display.getRoundedCorner(POSITION_TOP_LEFT);
            final RoundedCorner topRight = display.getRoundedCorner(POSITION_TOP_RIGHT);
            final RoundedCorner bottomRight = display.getRoundedCorner(POSITION_BOTTOM_RIGHT);
            final RoundedCorner bottomLeft = display.getRoundedCorner(POSITION_BOTTOM_LEFT);

            bounds.left = Math.max(topLeft != null ? topLeft.getCenter().x : 0,
                    bottomLeft != null ? bottomLeft.getCenter().x : 0);
            bounds.top = Math.max(topLeft != null ? topLeft.getCenter().y : 0,
                    bottomLeft != null ? bottomLeft.getCenter().y : 0);
            bounds.right = Math.min(topRight != null ? topRight.getCenter().x : width,
                    bottomRight != null ? bottomRight.getCenter().x : width);
            bounds.bottom = Math.min(bottomRight != null ? bottomRight.getCenter().y : height,
                    bottomLeft != null ? bottomLeft.getCenter().y : height);

            Log.d(TAG, "Window bounds with rounded corners excluded = " + bounds);
            return bounds;
        }
    }
}
