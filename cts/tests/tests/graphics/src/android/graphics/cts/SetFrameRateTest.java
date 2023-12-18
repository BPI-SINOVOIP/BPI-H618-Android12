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

package android.graphics.cts;

import static androidx.test.platform.app.InstrumentationRegistry.getInstrumentation;

import static org.junit.Assert.assertTrue;

import android.app.UiAutomation;
import android.content.Context;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceControl;

import androidx.test.filters.MediumTest;
import androidx.test.rule.ActivityTestRule;
import androidx.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.DisplayUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class SetFrameRateTest {
    private static String TAG = "SetFrameRateTest";

    @Rule
    public ActivityTestRule<FrameRateCtsActivity> mActivityRule =
            new ActivityTestRule<>(FrameRateCtsActivity.class);
    private long mFrameRateFlexibilityToken;

    @Before
    public void setUp() throws Exception {
        // Surface flinger requires the ACCESS_SURFACE_FLINGER permission to acquire a frame
        // rate flexibility token. Switch to shell permission identity so we'll have the
        // necessary permission when surface flinger checks.
        UiAutomation uiAutomation = getInstrumentation().getUiAutomation();
        uiAutomation.adoptShellPermissionIdentity();

        Context context = getInstrumentation().getTargetContext();
        assertTrue("Physical display is expected.", DisplayUtil.isDisplayConnected(context));

        try {
            // Take ownership of the frame rate flexibility token, if we were able
            // to get one - we'll release it in tearDown().
            mFrameRateFlexibilityToken = SurfaceControl.acquireFrameRateFlexibilityToken();
        } finally {
            uiAutomation.dropShellPermissionIdentity();
        }

        if (mFrameRateFlexibilityToken == 0) {
            Log.e(TAG, "Failed to acquire frame rate flexibility token."
                    + " SetFrameRate tests may fail.");
        }
    }

    @After
    public void tearDown() {
        if (mFrameRateFlexibilityToken != 0) {
            SurfaceControl.releaseFrameRateFlexibilityToken(mFrameRateFlexibilityToken);
            mFrameRateFlexibilityToken = 0;
        }
    }

    @Test
    public void testExactFrameRateMatch_Seamless() throws InterruptedException {
        FrameRateCtsActivity activity = mActivityRule.getActivity();
        activity.testExactFrameRateMatch(Surface.CHANGE_FRAME_RATE_ONLY_IF_SEAMLESS);
    }

    @Test
    public void testExactFrameRateMatch_NonSeamless() throws InterruptedException {
        FrameRateCtsActivity activity = mActivityRule.getActivity();
        activity.testExactFrameRateMatch(Surface.CHANGE_FRAME_RATE_ALWAYS);
    }

    @Test
    public void testFixedSource_Seamless() throws InterruptedException {
        FrameRateCtsActivity activity = mActivityRule.getActivity();
        activity.testFixedSource(Surface.CHANGE_FRAME_RATE_ONLY_IF_SEAMLESS);
    }

    @Test
    public void testFixedSource_NonSeamless() throws InterruptedException {
        FrameRateCtsActivity activity = mActivityRule.getActivity();
        activity.testFixedSource(Surface.CHANGE_FRAME_RATE_ALWAYS);
    }

    @Test
    public void testInvalidParams() throws InterruptedException {
        FrameRateCtsActivity activity = mActivityRule.getActivity();
        activity.testInvalidParams();
    }
}
