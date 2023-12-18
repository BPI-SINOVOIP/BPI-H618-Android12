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

import static android.server.wm.WindowManagerTestBase.startActivity;
import static android.view.Display.DEFAULT_DISPLAY;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION;
import static android.view.WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.content.ComponentCallbacks;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.res.Configuration;
import android.graphics.Rect;
import android.os.Binder;
import android.os.IBinder;
import android.platform.test.annotations.AppModeFull;
import android.platform.test.annotations.Presubmit;
import android.server.wm.WindowContextTests.TestWindowService.TestToken;
import android.view.View;
import android.view.WindowManager;
import android.window.WindowProviderService;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.rule.ServiceTestRule;

import org.junit.Test;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Tests that verify the behavior of window context
 *
 * Build/Install/Run:
 *     atest CtsWindowManagerDeviceTestCases:WindowContextTests
 */
@Presubmit
public class WindowContextTests extends WindowContextTestBase {
    @Test
    @AppModeFull
    public void testWindowContextConfigChanges() {
        createAllowSystemAlertWindowAppOpSession();
        final WindowManagerState.DisplayContent display = createManagedVirtualDisplaySession()
                .setSimulateDisplay(true).createDisplay();
        final Context windowContext = createWindowContext(display.mId);
        mInstrumentation.runOnMainSync(() -> {
            final View view = new View(windowContext);
            WindowManager wm = windowContext.getSystemService(WindowManager.class);
            wm.addView(view, new WindowManager.LayoutParams(TYPE_APPLICATION_OVERLAY));
        });
        final DisplayMetricsSession displayMetricsSession =
                createManagedDisplayMetricsSession(display.mId);

        mWmState.computeState();

        Rect bounds = windowContext.getSystemService(WindowManager.class).getCurrentWindowMetrics()
                .getBounds();
        assertBoundsEquals(displayMetricsSession.getDisplayMetrics(), bounds);

        displayMetricsSession.changeDisplayMetrics(1.2 /* sizeRatio */, 1.1 /* densityRatio */);

        mWmState.computeState();

        bounds = windowContext.getSystemService(WindowManager.class).getCurrentWindowMetrics()
                .getBounds();
        assertBoundsEquals(displayMetricsSession.getDisplayMetrics(), bounds);
    }

    private void assertBoundsEquals(ReportedDisplayMetrics expectedMetrics,
            Rect bounds) {
        assertEquals(expectedMetrics.getSize().getWidth(), bounds.width());
        assertEquals(expectedMetrics.getSize().getHeight(), bounds.height());
    }

    @Test
    @AppModeFull
    public void testWindowContextBindService() {
        createAllowSystemAlertWindowAppOpSession();
        final Context windowContext = createWindowContext(DEFAULT_DISPLAY);
        final ServiceConnection serviceConnection = new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {}

            @Override
            public void onServiceDisconnected(ComponentName name) {}
        };
        try {
            assertTrue("WindowContext must bind service successfully.",
                    windowContext.bindService(new Intent(windowContext, TestLogService.class),
                            serviceConnection, Context.BIND_AUTO_CREATE));
        } finally {
            windowContext.unbindService(serviceConnection);
        }
    }

    /**
     * Verify if the {@link ComponentCallbacks#onConfigurationChanged(Configuration)} callback
     * is received when the window context configuration changes.
     */
    @Test
    public void testWindowContextRegisterComponentCallbacks() throws Exception {
        final TestComponentCallbacks callbacks = new TestComponentCallbacks();
        final WindowManagerState.DisplayContent display = createManagedVirtualDisplaySession()
                .setSimulateDisplay(true).createDisplay();
        final Context windowContext = createWindowContext(display.mId);
        final DisplayMetricsSession displayMetricsSession =
                createManagedDisplayMetricsSession(display.mId);

        windowContext.registerComponentCallbacks(callbacks);

        callbacks.mLatch = new CountDownLatch(1);

        displayMetricsSession.changeDisplayMetrics(1.2 /* sizeRatio */, 1.1 /* densityRatio */);

        // verify if there is a callback from the window context configuration change.
        assertTrue(callbacks.mLatch.await(4, TimeUnit.SECONDS));
        Rect bounds = callbacks.mConfiguration.windowConfiguration.getBounds();
        assertBoundsEquals(displayMetricsSession.getDisplayMetrics(), bounds);

        windowContext.unregisterComponentCallbacks(callbacks);
    }

    /**
     * Verifies if window context on the secondary display receives global configuration changes.
     */
    @Test
    public void testWindowContextGlobalConfigChanges() {
        final TestComponentCallbacks callbacks = new TestComponentCallbacks();
        final WindowManagerState.DisplayContent display = createManagedVirtualDisplaySession()
                .setPublicDisplay(true).createDisplay();
        final FontScaleSession fontScaleSession = createManagedFontScaleSession();
        final Context windowContext = createWindowContext(display.mId);
        windowContext.registerComponentCallbacks(callbacks);
        final float expectedFontScale = fontScaleSession.get() + 0.3f;
        fontScaleSession.set(expectedFontScale);

        // Wait for TestComponentCallbacks#mConfiguration to be assigned.
        callbacks.waitForConfigChanged();

        // We don't rely on latch to verify the result because we may receive two configuration
        // changes. One may from that WindowContext attaches to a DisplayArea although it is before
        // ComponentCallback registration), the other is from font the scale change, which is what
        // we want to verify.
        waitForOrFail("font scale to match " + expectedFontScale, () ->
                expectedFontScale == callbacks.mConfiguration.fontScale);

        windowContext.unregisterComponentCallbacks(callbacks);
    }

    /**
     * Verify the {@link WindowProviderService} lifecycle:
     * <ul>
     *     <li>In {@link WindowProviderService#onCreate()}, register to the DisplayArea with
     *     given value from {@link WindowProviderService#getWindowType()} and
     *     {@link WindowProviderService#getWindowContextOptions()}} and receive a
     *     {@link Configuration} update which matches DisplayArea's metrics.</li>
     *     <li>After {@link WindowProviderService#attachToWindowToken(IBinder)}, the
     *     {@link WindowProviderService} must be switched to register to the Window Token and
     *     receive a configuration update which matches Window Token's metrics.</li>
     * </ul>
     */
    @Test
    public void testWindowProviderServiceLifecycle() throws Exception {
        // Start an activity for WindowProviderService to attach
        TestActivity activity = startActivity(TestActivity.class);
        final ComponentName activityName = activity.getComponentName();

        // If the device supports multi-window, make this Activity to multi-window mode.
        // In this way, we can verify if the WindowProviderService's metrics matches
        // the split-screen Activity's metrics, which is different from TaskDisplayArea's metrics.
        if (supportsSplitScreenMultiWindow()) {
            mWmState.computeState(activityName);

            putActivityInPrimarySplit(activityName);

            activity.waitAndAssertConfigurationChanged();
        }

        // Obtain the TestWindowService instance.
        final Context context = ApplicationProvider.getApplicationContext();
        final Intent intent = new Intent(context, TestWindowService.class);
        final ServiceTestRule serviceRule = new ServiceTestRule();
        try {
            TestToken token = (TestToken) serviceRule.bindService(intent);
            final TestWindowService service = token.getService();

            mWmState.computeState(activityName);
            final WindowManagerState.DisplayArea da = mWmState.getTaskDisplayArea(activityName);
            final Rect daBounds = da.mFullConfiguration.windowConfiguration.getBounds();
            final Rect maxDaBounds = da.mFullConfiguration.windowConfiguration.getMaxBounds();

            waitAndAssertWindowMetricsBoundsMatches(service, daBounds, maxDaBounds,
                    "WindowProviderService bounds must match DisplayArea bounds.");

            // Obtain the Activity's token and attach it to TestWindowService.
            final IBinder windowToken = activity.getWindow().getAttributes().token;
            service.attachToWindowToken(windowToken);

            final WindowManager wm = activity.getWindowManager();
            final Rect currentBounds = wm.getCurrentWindowMetrics().getBounds();
            final Rect maxBounds = wm.getMaximumWindowMetrics().getBounds();

            // After TestWindowService attaches the Activity's token, which is also a WindowToken,
            // it is expected to receive a config update which matches the WindowMetrics of
            // the Activity.
            waitAndAssertWindowMetricsBoundsMatches(service, currentBounds, maxBounds,
                    "WindowProviderService bounds must match WindowToken bounds.");
        } finally {
            serviceRule.unbindService();
        }
    }

    private void waitAndAssertWindowMetricsBoundsMatches(Context context, Rect currentBounds,
            Rect maxBounds, String message) {
        final WindowManager wm = context.getSystemService(WindowManager.class);
        waitForOrFail(message, () -> {
            final Rect currentWindowBounds = wm.getCurrentWindowMetrics().getBounds();
            final Rect maxWindowBounds = wm.getMaximumWindowMetrics().getBounds();
            return currentBounds.equals(currentWindowBounds) && maxBounds.equals(maxWindowBounds);
        });
    }

    public static class TestActivity extends WindowManagerTestBase.FocusableActivity {
        final CountDownLatch mLatch = new CountDownLatch(1);

        @Override
        public void onConfigurationChanged(@NonNull Configuration newConfig) {
            super.onConfigurationChanged(newConfig);
            mLatch.countDown();
        }

        private void waitAndAssertConfigurationChanged() throws Exception {
            assertThat(mLatch.await(4, TimeUnit.SECONDS)).isTrue();
        }
    }

    public static class TestWindowService extends WindowProviderService {
        private final IBinder mToken = new TestToken();

        @Override
        public int getWindowType() {
            return TYPE_APPLICATION;
        }

        @Nullable
        @Override
        public IBinder onBind(Intent intent) {
            return mToken;
        }

        public class TestToken extends Binder {
            private TestWindowService getService() {
                return TestWindowService.this;
            }
        }
    }

    private static class TestComponentCallbacks implements ComponentCallbacks {
        private Configuration mConfiguration;
        private CountDownLatch mLatch = new CountDownLatch(1);

        private void waitForConfigChanged() {
            try {
                assertThat(mLatch.await(4, TimeUnit.SECONDS)).isTrue();
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        }

        @Override
        public void onConfigurationChanged(@NonNull Configuration newConfig) {
            mConfiguration = newConfig;
            mLatch.countDown();
        }

        @Override
        public void onLowMemory() {}
    }
}
