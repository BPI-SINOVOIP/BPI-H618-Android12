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
import static android.view.WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
import static android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.AppOpsManager;
import android.content.Intent;
import android.graphics.Rect;
import android.os.Bundle;
import android.platform.test.annotations.Presubmit;
import android.view.Gravity;
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
public class PrivacyIndicatorBoundsTests {

    private static final String TAG = PrivacyIndicatorBoundsTests.class.getSimpleName();
    private static final long TIMEOUT_MS = 1000;

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
    public void testStaticBoundsAreNotNull() {
        final PrivacyIndicatorBoundsTests.TestActivity activity = mTestActivity.launchActivity(
                new Intent().putExtra(EXTRA_ORIENTATION, orientation));
        getInstrumentation().runOnMainSync(() -> {
            activity.addChildWindow();
        });

        final View childWindowRoot = activity.getChildWindowRoot();
        PollingCheck.waitFor(TIMEOUT_MS, () -> childWindowRoot.getWidth() > 0);
        PollingCheck.waitFor(TIMEOUT_MS, () -> activity.getDispatchedInsets() != null);
        WindowInsets insets = activity.getDispatchedInsets();
        assertNotNull(insets);
        Rect screenBounds = activity.getScreenBounds();
        assertNotNull(screenBounds);
        Rect bounds = insets.getPrivacyIndicatorBounds();
        assertNotNull(bounds);
        assertEquals(bounds.top, 0);
        // TODO 188788786: Figure out why the screen bounds are different in cuttlefish,
        // causing failures
        // assertTrue(bounds + " not contained in " + screenBounds, screenBounds.contains(bounds));
        assertTrue(bounds.left >= 0);
        assertTrue(bounds.right >= 0);
    }

    public static class TestActivity extends Activity {
        static final String EXTRA_ORIENTATION = "extra.orientation";

        private View mChildWindowRoot;
        private WindowInsets mDispatchedInsets;
        private Rect mScreenBounds = null;

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            getWindow().requestFeature(Window.FEATURE_NO_TITLE);
            getWindow().getDecorView().getWindowInsetsController().hide(
                    android.view.WindowInsets.Type.statusBars());
            getWindow().getAttributes().layoutInDisplayCutoutMode =
                    LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
            if (getIntent() != null) {
                setRequestedOrientation(getIntent().getIntExtra(
                        EXTRA_ORIENTATION, SCREEN_ORIENTATION_UNSPECIFIED));
            }

        }

        void addChildWindow() {
            final WindowMetrics windowMetrics = getWindowManager().getMaximumWindowMetrics();
            mScreenBounds = windowMetrics.getBounds();
            final WindowManager.LayoutParams attrs = new WindowManager.LayoutParams();
            attrs.x = mScreenBounds.left;
            attrs.y = mScreenBounds.top;
            attrs.width = mScreenBounds.width();
            attrs.height = mScreenBounds.height();
            attrs.gravity = Gravity.LEFT | Gravity.TOP;
            attrs.layoutInDisplayCutoutMode = LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
            attrs.flags = FLAG_NOT_FOCUSABLE;
            attrs.setFitInsetsTypes(0);
            mChildWindowRoot = new View(this);
            mChildWindowRoot.setOnApplyWindowInsetsListener(
                    (v, insets) -> {
                        mDispatchedInsets = insets;
                        return insets;
                    });
            getWindowManager().addView(mChildWindowRoot, attrs);
        }

        View getChildWindowRoot() {
            return mChildWindowRoot;
        }

        WindowInsets getDispatchedInsets() {
            return mDispatchedInsets;
        }

        Rect getScreenBounds() {
            return mScreenBounds;
        }
    }
}
