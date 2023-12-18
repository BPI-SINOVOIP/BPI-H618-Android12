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

package com.android.car.ui;

import static android.view.View.FOCUS_DOWN;
import static android.view.View.FOCUS_LEFT;
import static android.view.View.FOCUS_RIGHT;
import static android.view.View.FOCUS_UP;
import static android.view.View.LAYOUT_DIRECTION_LTR;
import static android.view.View.LAYOUT_DIRECTION_RTL;
import static android.view.accessibility.AccessibilityNodeInfo.ACTION_FOCUS;

import static com.android.car.ui.RotaryCache.CACHE_TYPE_DISABLED;
import static com.android.car.ui.RotaryCache.CACHE_TYPE_NEVER_EXPIRE;
import static com.android.car.ui.utils.RotaryConstants.ACTION_NUDGE_SHORTCUT;
import static com.android.car.ui.utils.RotaryConstants.ACTION_NUDGE_TO_ANOTHER_FOCUS_AREA;
import static com.android.car.ui.utils.RotaryConstants.FOCUS_AREA_BOTTOM_BOUND_OFFSET;
import static com.android.car.ui.utils.RotaryConstants.FOCUS_AREA_LEFT_BOUND_OFFSET;
import static com.android.car.ui.utils.RotaryConstants.FOCUS_AREA_RIGHT_BOUND_OFFSET;
import static com.android.car.ui.utils.RotaryConstants.FOCUS_AREA_TOP_BOUND_OFFSET;
import static com.android.car.ui.utils.RotaryConstants.NUDGE_DIRECTION;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.os.Bundle;
import android.view.View;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.Button;

import androidx.annotation.NonNull;
import androidx.test.rule.ActivityTestRule;

import com.android.car.rotary.test.R;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/** Unit tests for {@link FocusArea} not in touch mode. */
// TODO(b/187553946): Improve this test.
public class FocusAreaTest {
    private static final long WAIT_TIME_MS = 3000;

    @Rule
    public ActivityTestRule<FocusAreaTestActivity> mActivityRule =
            new ActivityTestRule<>(FocusAreaTestActivity.class);

    private FocusAreaTestActivity mActivity;
    private TestFocusArea mFocusArea1;
    private TestFocusArea mFocusArea2;
    private TestFocusArea mFocusArea3;
    private TestFocusArea mFocusArea4;
    private TestFocusArea mFocusArea5;
    private TestFocusArea mFocusArea6;
    private FocusParkingView mFpv;
    private View mView1;
    private Button mButton1;
    private View mView2;
    private View mDefaultFocus2;
    private View mView3;
    private View mNudgeShortcut3;
    private View mView4;
    private View mView5;
    private Button mButton5;
    private View mView6;
    private View mNudgeShortcut6;

    @Before
    public void setUp() {
        mActivity = mActivityRule.getActivity();
        mFocusArea1 = mActivity.findViewById(R.id.focus_area1);
        mFocusArea2 = mActivity.findViewById(R.id.focus_area2);
        mFocusArea3 = mActivity.findViewById(R.id.focus_area3);
        mFocusArea4 = mActivity.findViewById(R.id.focus_area4);
        mFocusArea5 = mActivity.findViewById(R.id.focus_area5);
        mFocusArea6 = mActivity.findViewById(R.id.focus_area6);
        mFpv = mActivity.findViewById(R.id.fpv);
        mView1 = mActivity.findViewById(R.id.view1);
        mButton1 = mActivity.findViewById(R.id.button1);
        mView2 = mActivity.findViewById(R.id.view2);
        mDefaultFocus2 = mActivity.findViewById(R.id.default_focus2);
        mView3 = mActivity.findViewById(R.id.view3);
        mNudgeShortcut3 = mActivity.findViewById(R.id.nudge_shortcut3);
        mView4 = mActivity.findViewById(R.id.view4);
        mView5 = mActivity.findViewById(R.id.view5);
        mButton5 = mActivity.findViewById(R.id.button5);
        mView6 = mActivity.findViewById(R.id.view6);
        mNudgeShortcut6 = mActivity.findViewById(R.id.nudge_shortcut6);
    }

    @Test
    public void testDrawMethodsCalled() throws Exception {
        CountDownLatch latch = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mFocusArea1.enableForegroundHighlight();
            mFocusArea2.enableForegroundHighlight();
            mFocusArea1.setOnDrawCalled(false);
            mFocusArea1.setDrawCalled(false);
            mFocusArea2.setOnDrawCalled(false);
            mFocusArea2.setDrawCalled(false);

            mView2.requestFocus();
            mView2.post(() -> latch.countDown());
        });

        // The methods should be called when a FocusArea gains or loses focus.
        assertDrawMethodsCalled(mFocusArea1, latch);
        assertDrawMethodsCalled(mFocusArea2, latch);
    }

    @Test
    public void testPerformAccessibilityAction_actionNudgeShortcut_legacy() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView3.post(() -> {
            mView3.requestFocus();
            mView3.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView3.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        Bundle arguments = new Bundle();
        mFocusArea3.post(() -> {
            // Nudge to the nudgeShortcut view.
            arguments.putInt(NUDGE_DIRECTION, FOCUS_RIGHT);
            mFocusArea3.performAccessibilityAction(ACTION_NUDGE_SHORTCUT, arguments);
            mFocusArea3.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mNudgeShortcut3.isFocused()).isTrue();

        CountDownLatch latch3 = new CountDownLatch(1);
        mView3.post(() -> {
            mView3.requestFocus();
            mView3.post(() -> latch3.countDown());
        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView3.isFocused()).isTrue();

        CountDownLatch latch4 = new CountDownLatch(1);
        mFocusArea3.post(() -> {
            arguments.putInt(NUDGE_DIRECTION, FOCUS_DOWN);
            mFocusArea3.performAccessibilityAction(ACTION_NUDGE_SHORTCUT, arguments);
            mFocusArea3.post(() -> latch4.countDown());
        });
        latch4.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        // nudgeShortcutDirection doesn't match. The focus should stay the same.
        assertThat(mView3.isFocused()).isTrue();

        CountDownLatch latch5 = new CountDownLatch(1);
        mFocusArea1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch5.countDown());
        });
        latch5.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch6 = new CountDownLatch(1);
        mFocusArea1.post(() -> {
            arguments.putInt(NUDGE_DIRECTION, FOCUS_RIGHT);
            mFocusArea1.performAccessibilityAction(ACTION_NUDGE_SHORTCUT, arguments);
            mFocusArea1.post(() -> latch6.countDown());
        });
        latch6.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        // No nudgeShortcut view in the current FocusArea. The focus should stay the same.
        assertThat(mView1.isFocused()).isTrue();
    }

    @Test
    public void testPerformAccessibilityAction_actionNudgeShortcut_new()throws Exception  {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView6.post(() -> {
            mView6.requestFocus();
            mView6.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView6.isFocused()).isTrue();

        Bundle arguments = new Bundle();
        CountDownLatch latch2 = new CountDownLatch(1);
        mFocusArea6.post(() -> {
            // Nudge to the nudgeShortcut view.
            arguments.putInt(NUDGE_DIRECTION, FOCUS_RIGHT);
            mFocusArea6.performAccessibilityAction(ACTION_NUDGE_SHORTCUT, arguments);
            mFocusArea6.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mNudgeShortcut6.isFocused()).isTrue();

        CountDownLatch latch3 = new CountDownLatch(1);
        mView6.post(() -> {
            mView6.requestFocus();
            mView6.post(() -> latch3.countDown());
        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView6.isFocused()).isTrue();

        CountDownLatch latch4 = new CountDownLatch(1);
        mFocusArea6.post(() -> {
            arguments.putInt(NUDGE_DIRECTION, FOCUS_DOWN);
            mFocusArea6.performAccessibilityAction(ACTION_NUDGE_SHORTCUT, arguments);
            mFocusArea6.post(() -> latch4.countDown());
        });
        latch4.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        // No shortcut specified for given direction. The focus should stay the same.
        assertThat(mView6.isFocused()).isTrue();

        CountDownLatch latch5 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch5.countDown());
        });
        latch5.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch6 = new CountDownLatch(1);
        mFocusArea1.post(() -> {
            arguments.putInt(NUDGE_DIRECTION, FOCUS_RIGHT);
            mFocusArea1.performAccessibilityAction(ACTION_NUDGE_SHORTCUT, arguments);
            mFocusArea1.post(() -> latch6.countDown());
        });
        latch6.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        // No shortcut specified for any direction. The focus should stay the same.
        assertThat(mView1.isFocused()).isTrue();
    }

    @Test
    public void testPerformAccessibilityAction_actionNudgeShortcut_programmatic()throws Exception  {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView6.post(() -> {
            // Programmatically change the nudge shortcut from right to left.
            mFocusArea6.setNudgeShortcut(FOCUS_RIGHT, null);
            mFocusArea6.setNudgeShortcut(FOCUS_LEFT, mNudgeShortcut3);

            mView6.requestFocus();
            mView6.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView6.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        Bundle arguments = new Bundle();
        mFocusArea6.post(() -> {
            // Nudge to the nudgeShortcut view.
            arguments.putInt(NUDGE_DIRECTION, FOCUS_LEFT);
            mFocusArea6.performAccessibilityAction(ACTION_NUDGE_SHORTCUT, arguments);
            mFocusArea6.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mNudgeShortcut3.isFocused()).isTrue();

        CountDownLatch latch3 = new CountDownLatch(1);
        mView6.post(() -> {
            mView6.requestFocus();
            mView6.post(() -> latch3.countDown());
        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView6.isFocused()).isTrue();

        CountDownLatch latch4 = new CountDownLatch(1);
        mFocusArea6.post(() -> {
            arguments.putInt(NUDGE_DIRECTION, FOCUS_RIGHT);
            mFocusArea6.performAccessibilityAction(ACTION_NUDGE_SHORTCUT, arguments);
            mFocusArea6.post(() -> latch4.countDown());
        });
        latch4.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        // No shortcut specified for given direction. The focus should stay the same.
        assertThat(mView6.isFocused()).isTrue();

        CountDownLatch latch5 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch5.countDown());
        });
        latch5.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch6 = new CountDownLatch(1);
        mFocusArea1.post(() -> {
            arguments.putInt(NUDGE_DIRECTION, FOCUS_RIGHT);
            mFocusArea1.performAccessibilityAction(ACTION_NUDGE_SHORTCUT, arguments);
            mFocusArea1.post(() -> latch6.countDown());
        });
        latch6.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        // No shortcut specified for any direction. The focus should stay the same.
        assertThat(mView1.isFocused()).isTrue();
    }

    @Test
    public void testPerformAccessibilityAction_actionFocus() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mFocusArea1.post(() -> {
            mFocusArea1.performAccessibilityAction(ACTION_FOCUS, null);
            mFocusArea1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mFocusArea2.post(() -> {
            mFocusArea2.performAccessibilityAction(ACTION_FOCUS, null);
            mFocusArea2.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        // It should focus on the default or the first view in the FocusArea.
        assertThat(mDefaultFocus2.isFocused()).isTrue();
    }

    @Test
    public void testPerformAccessibilityAction_actionFocus_enabledFocusCache() throws Exception {
        RotaryCache cache =
                new RotaryCache(CACHE_TYPE_NEVER_EXPIRE, 0, CACHE_TYPE_NEVER_EXPIRE, 0);
        CountDownLatch latch1 = new CountDownLatch(1);
        mButton1.post(() -> {
            mFocusArea1.setRotaryCache(cache);
            mButton1.requestFocus();
            mButton1.post(() -> latch1.countDown());

        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mButton1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mView2.post(() -> {
            mView2.requestFocus();
            mView2.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView2.isFocused()).isTrue();

        CountDownLatch latch3 = new CountDownLatch(1);
        mFocusArea1.post(() -> {
            mFocusArea1.performAccessibilityAction(ACTION_FOCUS, null);
            mFocusArea1.post(() -> latch3.countDown());

        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        // With cache, it should focus on the lastly focused view in the FocusArea.
        assertThat(mButton1.isFocused()).isTrue();
    }

    @Test
    public void testPerformAccessibilityAction_actionFocus_disabledFocusCache() throws Exception {
        RotaryCache cache = new RotaryCache(CACHE_TYPE_DISABLED, 0, CACHE_TYPE_NEVER_EXPIRE, 0);
        CountDownLatch latch1 = new CountDownLatch(1);
        mButton1.post(() -> {
            mFocusArea1.setRotaryCache(cache);
            mButton1.requestFocus();
            mButton1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mButton1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mView2.post(() -> {
            mView2.requestFocus();
            mView2.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView2.isFocused()).isTrue();

        CountDownLatch latch3 = new CountDownLatch(1);
        mFocusArea1.post(() -> {
            mFocusArea1.performAccessibilityAction(ACTION_FOCUS, null);
            mFocusArea1.post(() -> latch3.countDown());
        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        // Without cache, it should focus on the default or the first view in the FocusArea.
        assertThat(mView1.isFocused()).isTrue();
    }

    @Test
    public void testPerformAccessibilityAction_actionFocus_lastFocusedViewRemoved()
            throws Exception {
        // Focus on mDefaultFocus2 in mFocusArea2, then mView1 in mFocusArea1.
        CountDownLatch latch1 = new CountDownLatch(1);
        mDefaultFocus2.post(() -> {
            mDefaultFocus2.requestFocus();
            mDefaultFocus2.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mDefaultFocus2.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch3 = new CountDownLatch(1);
        mFocusArea2.post(() -> {
            // Remove mDefaultFocus2, then Perform ACTION_FOCUS on mFocusArea2.
            mFocusArea2.removeView(mDefaultFocus2);
            mFocusArea2.performAccessibilityAction(ACTION_FOCUS, null);
            mFocusArea2.post(() -> latch3.countDown());
        });
        // mView2 in mFocusArea2 should get focused.
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView2.isFocused()).isTrue();
    }

    @Test
    public void testPerformAccessibilityAction_actionNudgeToAnotherFocusArea_enabledCache()
            throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mButton1.post(() -> {
            RotaryCache cache1 =
                    new RotaryCache(CACHE_TYPE_NEVER_EXPIRE, 0, CACHE_TYPE_NEVER_EXPIRE, 0);
            mFocusArea1.setRotaryCache(cache1);
            RotaryCache cache2 =
                    new RotaryCache(CACHE_TYPE_NEVER_EXPIRE, 0, CACHE_TYPE_NEVER_EXPIRE, 0);
            mFocusArea2.setRotaryCache(cache2);

            // Focus on the second view in mFocusArea1.
            mButton1.requestFocus();
            mButton1.post(() -> latch1.countDown());

        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mButton1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        Bundle arguments = new Bundle();
        mFocusArea2.post(() -> {
            // Nudge to mFocusArea2.
            arguments.putInt(NUDGE_DIRECTION, FOCUS_DOWN);
            mFocusArea2.performAccessibilityAction(ACTION_FOCUS, arguments);
            mFocusArea2.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mDefaultFocus2.isFocused()).isTrue();

        CountDownLatch latch3 = new CountDownLatch(1);
        mFocusArea2.post(() -> {
            // Nudge back. It should focus on the cached view (mButton1) in the cached
            // FocusArea (mFocusArea1).
            arguments.putInt(NUDGE_DIRECTION, FOCUS_UP);
            mFocusArea2.performAccessibilityAction(ACTION_NUDGE_TO_ANOTHER_FOCUS_AREA, arguments);
            mFocusArea2.post(() -> latch3.countDown());
        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mButton1.isFocused()).isTrue();

        CountDownLatch latch4 = new CountDownLatch(1);
        mFocusArea1.post(() -> {
            // Nudge back. It should fail and the focus should stay the same because of one-way
            // nudge history.
            arguments.putInt(NUDGE_DIRECTION, FOCUS_DOWN);
            mFocusArea1.performAccessibilityAction(ACTION_NUDGE_TO_ANOTHER_FOCUS_AREA, arguments);
            mFocusArea1.post(() -> latch4.countDown());
        });
        latch4.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mButton1.isFocused()).isTrue();
    }

    @Test
    public void testPerformAccessibilityAction_actionNudgeToAnotherFocusArea_mixedCache()
            throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mButton1.post(() -> {
            // Disabled FocusCache but enabled FocusAreaCache.
            RotaryCache cache1 =
                    new RotaryCache(CACHE_TYPE_DISABLED, 0, CACHE_TYPE_NEVER_EXPIRE, 0);
            mFocusArea1.setRotaryCache(cache1);
            RotaryCache cache2 =
                    new RotaryCache(CACHE_TYPE_DISABLED, 0, CACHE_TYPE_NEVER_EXPIRE, 0);
            mFocusArea2.setRotaryCache(cache2);

            // Focus on the second view in mFocusArea1.
            mButton1.requestFocus();
            mButton1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mButton1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        Bundle arguments = new Bundle();
        mFocusArea2.post(() -> {
            // Nudge to mFocusArea2.
            arguments.putInt(NUDGE_DIRECTION, FOCUS_DOWN);
            mFocusArea2.performAccessibilityAction(ACTION_FOCUS, arguments);
            mFocusArea2.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mDefaultFocus2.isFocused()).isTrue();

        CountDownLatch latch3 = new CountDownLatch(1);
        mFocusArea2.post(() -> {
            // Nudge back. Since FocusCache is disabled, it should focus on the default or the first
            // view (mView1) in the cached FocusArea (mFocusArea1).
            arguments.putInt(NUDGE_DIRECTION, FOCUS_UP);
            mFocusArea2.performAccessibilityAction(ACTION_NUDGE_TO_ANOTHER_FOCUS_AREA, arguments);
            mFocusArea2.post(() -> latch3.countDown());
        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();
    }

    @Test
    public void testPerformAccessibilityAction_actionNudgeToAnotherFocusArea_mixedCache2()
            throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mButton1.post(() -> {
            // Enabled FocusCache but disabled FocusAreaCache.
            RotaryCache cache1 =
                    new RotaryCache(CACHE_TYPE_NEVER_EXPIRE, 0, CACHE_TYPE_DISABLED, 0);
            mFocusArea1.setRotaryCache(cache1);
            RotaryCache cache2 =
                    new RotaryCache(CACHE_TYPE_NEVER_EXPIRE, 0, CACHE_TYPE_DISABLED, 0);
            mFocusArea2.setRotaryCache(cache2);

            // Focus on the second view in mFocusArea1, then nudge to mFocusArea2.
            mButton1.requestFocus();
            mButton1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mButton1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        Bundle arguments = new Bundle();
        mFocusArea2.post(() -> {
            arguments.putInt(NUDGE_DIRECTION, FOCUS_DOWN);
            mFocusArea2.performAccessibilityAction(ACTION_FOCUS, arguments);
            mFocusArea2.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mDefaultFocus2.isFocused()).isTrue();

        CountDownLatch latch3 = new CountDownLatch(1);
        mFocusArea2.post(() -> {
            // Nudge back. Since FocusAreaCache is disabled, nudge should fail and the focus should
            // stay the same.
            arguments.putInt(NUDGE_DIRECTION, FOCUS_UP);
            mFocusArea2.performAccessibilityAction(ACTION_NUDGE_TO_ANOTHER_FOCUS_AREA, arguments);
            mFocusArea2.post(() -> latch3.countDown());
        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mDefaultFocus2.isFocused()).isTrue();
    }

    @Test
    public void testPerformAccessibilityAction_actionNudgeToAnotherFocusArea_specifiedTarget()
            throws Exception {
        // Nudge to specified FocusArea.
        CountDownLatch latch1 = new CountDownLatch(1);
        mView4.post(() -> {
            mView4.requestFocus();
            mView4.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView4.isFocused()).isTrue();
        CountDownLatch latch2 = new CountDownLatch(1);
        Bundle arguments = new Bundle();
        mFocusArea4.post(() -> {
            arguments.putInt(NUDGE_DIRECTION, FOCUS_LEFT);
            mFocusArea4.performAccessibilityAction(ACTION_NUDGE_TO_ANOTHER_FOCUS_AREA, arguments);
            mFocusArea4.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mDefaultFocus2.isFocused()).isTrue();

        // Direction doesn't match specified FocusArea. The focus should stay the same.
        CountDownLatch latch3 = new CountDownLatch(1);
        mView4.post(() -> {
            mView4.requestFocus();
            mView4.post(() -> latch3.countDown());
        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView4.isFocused()).isTrue();
        CountDownLatch latch4 = new CountDownLatch(1);
        mFocusArea4.post(() -> {
            arguments.putInt(NUDGE_DIRECTION, FOCUS_UP);
            mFocusArea4.performAccessibilityAction(ACTION_NUDGE_TO_ANOTHER_FOCUS_AREA, arguments);
            mFocusArea4.post(() -> latch4.countDown());
        });
        latch4.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView4.isFocused()).isTrue();

        // The FocusArea doesn't specify a target FocusArea. The focus should stay the same.
        CountDownLatch latch5 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch5.countDown());
        });
        latch5.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();
        CountDownLatch latch6 = new CountDownLatch(1);
        mFocusArea1.post(() -> {
            arguments.putInt(NUDGE_DIRECTION, FOCUS_LEFT);
            mFocusArea1.performAccessibilityAction(ACTION_NUDGE_TO_ANOTHER_FOCUS_AREA, arguments);
            mFocusArea1.post(() -> latch6.countDown());
        });
        latch6.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();
    }

    @Test
    public void testDefaultFocusOverridesHistory_override() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView2.post(() -> {
            RotaryCache cache =
                    new RotaryCache(CACHE_TYPE_NEVER_EXPIRE, 0, CACHE_TYPE_NEVER_EXPIRE, 0);
            mFocusArea2.setRotaryCache(cache);
            mFocusArea2.setDefaultFocusOverridesHistory(true);
            mView2.requestFocus();
            mView2.post(() -> latch1.countDown());

        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView2.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch3 = new CountDownLatch(1);
        mFocusArea2.post(() -> {
            mFocusArea2.performAccessibilityAction(ACTION_FOCUS, null);
            mFocusArea2.post(() -> latch3.countDown());
        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        // The focused view should be the default focus view rather than the cached view.
        assertThat(mDefaultFocus2.isFocused()).isTrue();
    }

    @Test
    public void testDefaultFocusOverridesHistory_notOverride() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView2.post(() -> {
            RotaryCache cache =
                    new RotaryCache(CACHE_TYPE_NEVER_EXPIRE, 0, CACHE_TYPE_NEVER_EXPIRE, 0);
            mFocusArea2.setRotaryCache(cache);
            mView2.requestFocus();
            mView2.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView2.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch3 = new CountDownLatch(1);
        mFocusArea2.post(() -> {
            mFocusArea2.performAccessibilityAction(ACTION_FOCUS, null);
            mFocusArea2.post(() -> latch3.countDown());
        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        // The focused view should be the cached view rather than the default focus view.
        assertThat(mView2.isFocused()).isTrue();
    }

    @Test
    public void testClearFocusAreaHistoryWhenRotating_clear() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView1.post(() -> {
            RotaryCache cache1 =
                    new RotaryCache(CACHE_TYPE_NEVER_EXPIRE, 0, CACHE_TYPE_NEVER_EXPIRE, 0);
            mFocusArea1.setRotaryCache(cache1);
            mFocusArea1.setClearFocusAreaHistoryWhenRotating(true);
            RotaryCache cache2 =
                    new RotaryCache(CACHE_TYPE_NEVER_EXPIRE, 0, CACHE_TYPE_NEVER_EXPIRE, 0);
            mFocusArea2.setRotaryCache(cache2);
            mFocusArea2.setClearFocusAreaHistoryWhenRotating(true);
            mView1.requestFocus();
            mView1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        Bundle arguments = new Bundle();
        mFocusArea2.post(() -> {
            // Nudging down from mFocusArea1 to mFocusArea2.
            arguments.putInt(NUDGE_DIRECTION, FOCUS_DOWN);
            mFocusArea2.performAccessibilityAction(ACTION_FOCUS, arguments);
            mFocusArea2.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mDefaultFocus2.isFocused()).isTrue();

        CountDownLatch latch3 = new CountDownLatch(1);
        mView2.post(() -> {
            // Rotate.
            mView2.requestFocus();
            mView2.post(() -> latch3.countDown());
        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView2.isFocused()).isTrue();

        CountDownLatch latch4 = new CountDownLatch(1);
        mFocusArea2.post(() -> {
            arguments.putInt(NUDGE_DIRECTION, FOCUS_UP);
            mFocusArea2.performAccessibilityAction(ACTION_NUDGE_TO_ANOTHER_FOCUS_AREA, arguments);
            mFocusArea2.post(() -> latch4.countDown());
        });
        latch4.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        // Since nudge history is cleared, nudging up should fail and the focus should stay
        // the same.
        assertThat(mView2.isFocused()).isTrue();
    }

    @Test
    public void testClearFocusAreaHistoryWhenRotating_notClear() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView1.post(() -> {
            RotaryCache cache1 =
                    new RotaryCache(CACHE_TYPE_NEVER_EXPIRE, 0, CACHE_TYPE_NEVER_EXPIRE, 0);
            mFocusArea1.setRotaryCache(cache1);
            mFocusArea1.setClearFocusAreaHistoryWhenRotating(false);
            RotaryCache cache2 =
                    new RotaryCache(CACHE_TYPE_NEVER_EXPIRE, 0, CACHE_TYPE_NEVER_EXPIRE, 0);
            mFocusArea2.setRotaryCache(cache2);
            mFocusArea2.setClearFocusAreaHistoryWhenRotating(false);
            mView1.requestFocus();
            mView1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        Bundle arguments = new Bundle();
        mFocusArea2.post(() -> {
            // Nudging down from mFocusArea1 to mFocusArea2.
            arguments.putInt(NUDGE_DIRECTION, FOCUS_DOWN);
            mFocusArea2.performAccessibilityAction(ACTION_FOCUS, arguments);
            mFocusArea2.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mDefaultFocus2.isFocused()).isTrue();

        CountDownLatch latch3 = new CountDownLatch(1);
        mView2.post(() -> {
            // Rotate.
            mView2.requestFocus();
            mView2.post(() -> latch3.countDown());
        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView2.isFocused()).isTrue();

        CountDownLatch latch4 = new CountDownLatch(1);
        mFocusArea2.post(() -> {
            // Nudging up should move focus to mFocusArea1 according to nudge history.
            arguments.putInt(NUDGE_DIRECTION, FOCUS_UP);
            mFocusArea2.performAccessibilityAction(ACTION_NUDGE_TO_ANOTHER_FOCUS_AREA, arguments);
            mFocusArea2.post(() -> latch4.countDown());
        });
        latch4.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();
    }

    @Test
    public void testBoundsOffset() {
        assertThat(mFocusArea1.getLayoutDirection()).isEqualTo(LAYOUT_DIRECTION_LTR);

        // FocusArea's bounds offset specified in layout file:
        // 10dp(start), 20dp(end), 30dp(top), 40dp(bottom).
        int left = dp2Px(10);
        int right = dp2Px(20);
        int top = dp2Px(30);
        int bottom = dp2Px(40);
        AccessibilityNodeInfo node = mFocusArea1.createAccessibilityNodeInfo();
        assertBoundsOffset(node, left, top, right, bottom);
        node.recycle();
    }

    @Test
    public void testBoundsOffsetWithRtl() throws Exception {
        CountDownLatch latch = new CountDownLatch(1);
        mFocusArea1.post(() -> {
            mFocusArea1.setLayoutDirection(LAYOUT_DIRECTION_RTL);
            mFocusArea1.post(() -> latch.countDown());
        });
        latch.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFocusArea1.getLayoutDirection()).isEqualTo(LAYOUT_DIRECTION_RTL);

        // FocusArea highlight padding specified in layout file:
        // 10dp(start), 20dp(end), 30dp(top), 40dp(bottom).
        int left = dp2Px(20);
        int right = dp2Px(10);
        int top = dp2Px(30);
        int bottom = dp2Px(40);
        AccessibilityNodeInfo node = mFocusArea1.createAccessibilityNodeInfo();
        assertBoundsOffset(node, left, top, right, bottom);
        node.recycle();
    }

    @Test
    public void testSetBoundsOffset() {
        mFocusArea1.setBoundsOffset(50, 60, 70, 80);
        AccessibilityNodeInfo node = mFocusArea1.createAccessibilityNodeInfo();
        assertBoundsOffset(node, 50, 60, 70, 80);
        node.recycle();
    }

    @Test
    public void testHighlightPadding() {
        assertThat(mFocusArea2.getLayoutDirection()).isEqualTo(LAYOUT_DIRECTION_LTR);

        int left = dp2Px(50);
        int right = dp2Px(10);
        int top = dp2Px(40);
        int bottom = dp2Px(20);
        AccessibilityNodeInfo node = mFocusArea2.createAccessibilityNodeInfo();
        assertBoundsOffset(node, left, top, right, bottom);
        node.recycle();
    }

    @Test
    public void testBug170423337() throws Exception {
        Bundle arguments = new Bundle();
        CountDownLatch latch1 = new CountDownLatch(1);
        mFocusArea2.post(() -> {
            // Focus on app bar (assume mFocusArea1 is app bar).
            mView1.requestFocus();

            // Nudge down to browse list (assume mFocusArea2 is browse list).
            arguments.putInt(NUDGE_DIRECTION, FOCUS_DOWN);
            mFocusArea2.performAccessibilityAction(ACTION_FOCUS, arguments);
            mFocusArea2.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mDefaultFocus2.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mFocusArea3.post(() -> {
            // Nudge down to playback control bar (assume mFocusArea3 is playback control bar).
            mFocusArea3.performAccessibilityAction(ACTION_FOCUS, arguments);
            mFocusArea3.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView3.isFocused()).isTrue();

        CountDownLatch latch3 = new CountDownLatch(1);
        mFocusArea3.post(() -> {
            // Nudge down to navigation bar (navigation bar is in system window without FocusAreas).
            mFpv.performAccessibilityAction(ACTION_FOCUS, null);

            // Nudge up to playback control bar.
            arguments.putInt(NUDGE_DIRECTION, FOCUS_UP);
            mFocusArea3.performAccessibilityAction(ACTION_FOCUS, arguments);
            mFocusArea3.post(() -> latch3.countDown());
        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView3.isFocused()).isTrue();

        CountDownLatch latch4 = new CountDownLatch(1);
        mFocusArea3.post(() -> {
            // Nudge up to browse list.
            arguments.putInt(NUDGE_DIRECTION, FOCUS_UP);
            mFocusArea3.performAccessibilityAction(ACTION_NUDGE_TO_ANOTHER_FOCUS_AREA, arguments);
            mFocusArea3.post(() -> latch4.countDown());
        });
        latch4.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mDefaultFocus2.isFocused()).isTrue();

        CountDownLatch latch5 = new CountDownLatch(1);
        mFocusArea2.post(() -> {
            // Nudge up, and it should focus on app bar.
            mFocusArea2.performAccessibilityAction(ACTION_NUDGE_TO_ANOTHER_FOCUS_AREA, arguments);
            mFocusArea2.post(() -> latch5.countDown());
        });
        latch5.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();
    }

    @Test
    public void testWrapAround() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView5.post(() -> {
            mView5.requestFocus();
            mView5.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView5.isFocused()).isTrue();

        View focusSearch = mView5.focusSearch(View.FOCUS_FORWARD);
        assertWithMessage("Forward wrap-around").that(focusSearch).isEqualTo(mButton5);

        CountDownLatch latch2 = new CountDownLatch(1);
        mButton5.post(() -> {
            mButton5.requestFocus();
            mButton5.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mButton5.isFocused()).isTrue();

        focusSearch = mButton5.focusSearch(View.FOCUS_BACKWARD);
        assertWithMessage("Backward wrap-around").that(focusSearch).isEqualTo(mView5);
    }

    @Test
    public void testNoWrapAround() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mButton1.post(() -> {
            mButton1.requestFocus();
            mButton1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mButton1.isFocused()).isTrue();

        View focusSearch = mButton1.focusSearch(View.FOCUS_FORWARD);
        assertWithMessage("Forward wrap-around").that(focusSearch).isNotEqualTo(mView1);

        CountDownLatch latch2 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        focusSearch = mView1.focusSearch(View.FOCUS_BACKWARD);
        assertWithMessage("Backward wrap-around").that(focusSearch).isNotEqualTo(mButton1);
    }

    private void assertBoundsOffset(
            @NonNull AccessibilityNodeInfo node, int leftPx, int topPx, int rightPx, int bottomPx) {
        Bundle extras = node.getExtras();
        assertThat(extras.getInt(FOCUS_AREA_LEFT_BOUND_OFFSET)).isEqualTo(leftPx);
        assertThat(extras.getInt(FOCUS_AREA_RIGHT_BOUND_OFFSET)).isEqualTo(rightPx);
        assertThat(extras.getInt(FOCUS_AREA_TOP_BOUND_OFFSET)).isEqualTo(topPx);
        assertThat(extras.getInt(FOCUS_AREA_BOTTOM_BOUND_OFFSET)).isEqualTo(bottomPx);
    }

    /** Converts dp unit to equivalent pixels. */
    private int dp2Px(int dp) {
        return (int) (dp * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }

    private void assertDrawMethodsCalled(@NonNull TestFocusArea focusArea, CountDownLatch latch)
            throws Exception {
        latch.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(focusArea.onDrawCalled()).isTrue();
        assertThat(focusArea.drawCalled()).isTrue();
    }
}
