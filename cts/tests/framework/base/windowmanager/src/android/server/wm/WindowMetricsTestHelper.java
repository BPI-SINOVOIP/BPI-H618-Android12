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

package android.server.wm;

import static android.app.WindowConfiguration.WINDOWING_MODE_FREEFORM;
import static android.view.WindowInsets.Type.displayCutout;
import static android.view.WindowInsets.Type.navigationBars;
import static android.view.WindowInsets.Type.statusBars;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.content.Context;
import android.graphics.Insets;
import android.graphics.Point;
import android.graphics.Rect;
import android.view.Display;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.WindowMetrics;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/** Helper class to test {@link WindowMetrics} behaviors. */
public class WindowMetricsTestHelper {
    public static void assertMetricsMatchesLayout(WindowMetrics currentMetrics,
            WindowMetrics maxMetrics, Rect layoutBounds, WindowInsets layoutInsets) {
        assertMetricsMatchesLayout(currentMetrics, maxMetrics, layoutBounds, layoutInsets,
                false /* isFreeformActivity */);
    }

    public static void assertMetricsMatchesLayout(WindowMetrics currentMetrics,
            WindowMetrics maxMetrics, Rect layoutBounds, WindowInsets layoutInsets,
            boolean isFreeformActivity) {
        assertEquals(layoutBounds, currentMetrics.getBounds());
        // Freeform activities doesn't guarantee max window metrics bounds is larger than current
        // window metrics bounds. The bounds of a freeform activity is unlimited except that
        // it must be contained in display bounds.
        if (!isFreeformActivity) {
            assertTrue(maxMetrics.getBounds().width()
                    >= currentMetrics.getBounds().width());
            assertTrue(maxMetrics.getBounds().height()
                    >= currentMetrics.getBounds().height());
        }
        final int insetsType = statusBars() | navigationBars() | displayCutout();
        assertEquals(layoutInsets.getInsets(insetsType),
                currentMetrics.getWindowInsets().getInsets(insetsType));
        assertEquals(layoutInsets.getDisplayCutout(),
                currentMetrics.getWindowInsets().getDisplayCutout());
    }

    /**
     * Verifies two scenarios for a {@link Context}.
     * <ul>
     *     <li>{@link WindowManager#getCurrentWindowMetrics()} matches
     *     {@link Display#getSize(Point)}</li>
     *     <li>{@link WindowManager#getMaximumWindowMetrics()} and {@link Display#getSize(Point)}
     *     either matches DisplayArea bounds which the {@link Context} is attached to, or matches
     *     {@link WindowManager#getCurrentWindowMetrics()} if sandboxing is applied.</li>
     * </ul>
     * @param context the context under test
     * @param displayAreaBounds the bounds of the DisplayArea
     */
    public static void assertMetricsValidity(Context context, Rect displayAreaBounds) {
        final boolean isFreeForm = context instanceof Activity
                && context.getResources().getConfiguration().windowConfiguration
                .getWindowingMode() == WINDOWING_MODE_FREEFORM;
        final WindowManager windowManager = context.getSystemService(WindowManager.class);
        // Freeform activity doesn't inset the navigation bar and cutout area.
        final Rect currentBounds =
                isFreeForm ? windowManager.getCurrentWindowMetrics().getBounds() :
                        getBoundsExcludingNavigationBarAndCutout(
                                windowManager.getCurrentWindowMetrics());
        final Rect maxBounds = windowManager.getMaximumWindowMetrics().getBounds();
        final Display display = context.getDisplay();
        assertBoundsMatchDisplay(maxBounds, currentBounds, display);

        // Max window bounds should match either DisplayArea bounds, or current window bounds.
        if (maxWindowBoundsSandboxed(displayAreaBounds, maxBounds)) {
            // Max window bounds are sandboxed, so max window bounds and real display size
            // should match current window bounds.
            assertEquals("Max window size matches current window size, due to sandboxing",
                    currentBounds, maxBounds);
        } else {
            // Max window bounds are not sandboxed, so max window bounds and real display size
            // should match display area bounds.
            assertEquals("Display area bounds must match max window size",
                    displayAreaBounds, maxBounds);
        }
    }

    /**
     * Verifies for the provided bounds and display:
     * <ul>
     *     <li>{@link WindowManager#getCurrentWindowMetrics()} matches
     *     {@link Display#getSize(Point)}</li>
     *     <li>{@link WindowManager#getMaximumWindowMetrics()} matches
     *     {@link Display#getRealSize(Point)}
     * </ul>
     * @param maxBounds the bounds from {@link WindowManager#getMaximumWindowMetrics()}
     * @param currentBounds the bounds from {@link WindowManager#getCurrentWindowMetrics()}
     * @param display the display to compare bounds against
     */
    static void assertBoundsMatchDisplay(Rect maxBounds, Rect currentBounds, Display display) {
        // Check window bounds
        final Point displaySize = new Point();
        display.getSize(displaySize);
        assertEquals("Reported display width must match window width",
                displaySize.x, currentBounds.width());
        assertEquals("Reported display height must match window height",
                displaySize.y, currentBounds.height());

        // Max window bounds should match real display size.
        final Point realDisplaySize = new Point();
        display.getRealSize(realDisplaySize);
        assertEquals("Reported real display width must match max window width",
                realDisplaySize.x, maxBounds.width());
        assertEquals("Reported real display height must match max window height",
                realDisplaySize.y, maxBounds.height());
    }

    public static Rect getBoundsExcludingNavigationBarAndCutout(WindowMetrics windowMetrics) {
        WindowInsets windowInsets = windowMetrics.getWindowInsets();
        final Insets insetsWithCutout =
                windowInsets.getInsetsIgnoringVisibility(navigationBars() | displayCutout());

        final Rect bounds = windowMetrics.getBounds();
        return inset(bounds, insetsWithCutout);
    }

    /**
     * Returns {@code true} if the bounds from {@link WindowManager#getMaximumWindowMetrics()} are
     * sandboxed, so are smaller than the DisplayArea.
     */
    static boolean maxWindowBoundsSandboxed(Rect displayAreaBounds, Rect maxBounds) {
        return maxBounds.width() < displayAreaBounds.width()
                || maxBounds.height() < displayAreaBounds.height();
    }

    private static Rect inset(Rect original, Insets insets) {
        final int left = original.left + insets.left;
        final int top = original.top + insets.top;
        final int right = original.right - insets.right;
        final int bottom = original.bottom - insets.bottom;
        return new Rect(left, top, right, bottom);
    }

    public static class OnLayoutChangeListener implements View.OnLayoutChangeListener {
        private final CountDownLatch mLayoutLatch = new CountDownLatch(1);

        private volatile Rect mOnLayoutBoundsInScreen;
        private volatile WindowInsets mOnLayoutInsets;

        @Override
        public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft,
                int oldTop, int oldRight, int oldBottom) {
            synchronized (this) {
                mOnLayoutBoundsInScreen = new Rect(left, top, right, bottom);
                // Convert decorView's bounds from window coordinates to screen coordinates.
                final int[] locationOnScreen = new int[2];
                v.getLocationOnScreen(locationOnScreen);
                mOnLayoutBoundsInScreen.offset(locationOnScreen[0], locationOnScreen[1]);

                mOnLayoutInsets = v.getRootWindowInsets();
                mLayoutLatch.countDown();
            }
        }

        public Rect getLayoutBounds() {
            synchronized (this) {
                return mOnLayoutBoundsInScreen;
            }
        }

        public WindowInsets getLayoutInsets() {
            synchronized (this) {
                return mOnLayoutInsets;
            }
        }

        void waitForLayout() {
            try {
                assertTrue("Timed out waiting for layout.",
                        mLayoutLatch.await(4, TimeUnit.SECONDS));
            } catch (InterruptedException e) {
                throw new AssertionError(e);
            }
        }
    }
}
