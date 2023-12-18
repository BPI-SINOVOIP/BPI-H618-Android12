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

import static android.view.accessibility.AccessibilityNodeInfo.ACTION_FOCUS;

import static com.android.car.ui.utils.RotaryConstants.ACTION_RESTORE_DEFAULT_FOCUS;
import static com.android.car.ui.utils.ViewUtils.setRotaryScrollEnabled;

import static com.google.common.truth.Truth.assertThat;

import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.test.rule.ActivityTestRule;

import com.android.car.rotary.test.R;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/** Unit test for {@link FocusParkingView} not in touch mode. */
// TODO(b/187553946): Improve this test.
public class FocusParkingViewTest {

    private static final long WAIT_TIME_MS = 3000;
    private static final int NUM_ITEMS = 40;

    @Rule
    public ActivityTestRule<FocusParkingViewTestActivity> mActivityRule =
            new ActivityTestRule<>(FocusParkingViewTestActivity.class);

    private FocusParkingViewTestActivity mActivity;
    private FocusParkingView mFpv;
    private ViewGroup mParent1;
    private View mView1;
    private View mFocusedByDefault;
    private RecyclerView mList;

    @Before
    public void setUp() {
        mActivity = mActivityRule.getActivity();
        mFpv = mActivity.findViewById(R.id.fpv);
        mParent1 = mActivity.findViewById(R.id.parent1);
        mView1 = mActivity.findViewById(R.id.view1);
        mFocusedByDefault = mActivity.findViewById(R.id.focused_by_default);
        mList = mActivity.findViewById(R.id.list);

        mList.post(() -> {
            mList.setLayoutManager(new LinearLayoutManager(mActivity));
            mList.setAdapter(new TestAdapter(NUM_ITEMS));
            setRotaryScrollEnabled(mList, /* isVertical= */ true);
        });
    }

    @Test
    public void testGetWidthAndHeight() {
        assertThat(mFpv.getWidth()).isEqualTo(1);
        assertThat(mFpv.getHeight()).isEqualTo(1);
    }

    @Test
    public void testRequestFocus_focusOnDefaultFocus() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.performAccessibilityAction(ACTION_FOCUS, null);
            mFpv.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFpv.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.requestFocus();
            mFpv.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFocusedByDefault.isFocused()).isTrue();
    }

    @Test
    public void testRequestFocus_doNothing() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.requestFocus();
            mFpv.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();
    }

    @Test
    public void testRestoreDefaultFocus_focusOnDefaultFocus() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.performAccessibilityAction(ACTION_FOCUS, null);
            mFpv.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFpv.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.restoreDefaultFocus();
            mFpv.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFocusedByDefault.isFocused()).isTrue();
    }

    @Test
    public void testRestoreDefaultFocus_doNothing() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.restoreDefaultFocus();
            mFpv.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();
    }

    @Test
    public void testOnWindowFocusChanged_loseFocus() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.onWindowFocusChanged(false);
            mFpv.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFpv.isFocused()).isTrue();
    }

    @Test
    public void testOnWindowFocusChanged_focusOnDefaultFocus() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.performAccessibilityAction(ACTION_FOCUS, null);
            mFpv.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFpv.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.onWindowFocusChanged(true);
            mFpv.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFocusedByDefault.isFocused()).isTrue();
    }

    @Test
    public void testPerformAccessibilityAction_actionRestoreDefaultFocus() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.performAccessibilityAction(ACTION_FOCUS, null);
            mFpv.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFpv.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.performAccessibilityAction(ACTION_RESTORE_DEFAULT_FOCUS, null);
            mFpv.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFocusedByDefault.isFocused()).isTrue();
    }

    @Test
    public void testPerformAccessibilityAction_doNothing() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.performAccessibilityAction(ACTION_RESTORE_DEFAULT_FOCUS, null);
            mFpv.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();
    }

    @Test
    public void testPerformAccessibilityAction_actionFocus() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.performAccessibilityAction(ACTION_FOCUS, null);
            mFpv.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFpv.isFocused()).isTrue();
    }

    @Test
    public void testRestoreFocusInRoot_recyclerViewItemRemoved() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mList.post(() -> mList.getViewTreeObserver().addOnGlobalLayoutListener(
                new ViewTreeObserver.OnGlobalLayoutListener() {
                    @Override
                    public void onGlobalLayout() {
                        mList.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                        mList.post(() -> latch1.countDown());
                    }
                })
        );
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);

        View firstItem = mList.getLayoutManager().findViewByPosition(0);
        CountDownLatch latch2 = new CountDownLatch(1);
        mList.post(() -> {
            firstItem.requestFocus();
            mList.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(firstItem.isFocused()).isTrue();

        ViewGroup parent = (ViewGroup) firstItem.getParent();
        CountDownLatch latch3 = new CountDownLatch(1);
        parent.post(() -> {
            parent.removeView(firstItem);
            parent.post(() -> latch3.countDown());
        });
        latch3.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFocusedByDefault.isFocused()).isTrue();
    }

    // TODO(b/179734335) Reenable this test, and remove the asserts inside of layout listeners.
    // When an assert fails inside of a layout listener, it causes a whole bunch of tests in the
    // test suite to fail with "test did not run due to instrumentation issue.
    // See run level error for reason.", making it hard to debug.
//    @Test
//    public void testRestoreFocusInRoot_recyclerViewItemScrolledOffScreen() {
//        mList.post(() -> mList.getViewTreeObserver().addOnGlobalLayoutListener(
//                new ViewTreeObserver.OnGlobalLayoutListener() {
//                    @Override
//                    public void onGlobalLayout() {
//                        mList.getViewTreeObserver().removeOnGlobalLayoutListener(this);
//                        View firstItem = mList.getLayoutManager().findViewByPosition(0);
//                        firstItem.requestFocus();
//                        assertThat(firstItem.isFocused()).isTrue();
//
//                        mList.scrollToPosition(NUM_ITEMS - 1);
//                        mList.getViewTreeObserver().addOnGlobalLayoutListener(
//                                new ViewTreeObserver.OnGlobalLayoutListener() {
//                                    @Override
//                                    public void onGlobalLayout() {
//                                        mList.getViewTreeObserver()
//                                                .removeOnGlobalLayoutListener(this);
//                                        assertThat(mList.isFocused()).isTrue();
//                                    }
//                                });
//                    }
//                }));
//    }

    @Test
    public void testRestoreFocusInRoot_focusedViewRemoved() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mView1.post(() -> {
            ViewGroup parent = (ViewGroup) mView1.getParent();
            parent.removeView(mView1);
            mView1.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFocusedByDefault.isFocused()).isTrue();
    }

    @Test
    public void testRestoreFocusInRoot_focusedViewDisabled() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.setEnabled(false);
            mView1.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFocusedByDefault.isFocused()).isTrue();
    }

    @Test
    public void testRestoreFocusInRoot_focusedViewBecomesInvisible() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.setVisibility(View.INVISIBLE);
            mView1.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFocusedByDefault.isFocused()).isTrue();
    }

    @Test
    public void testRestoreFocusInRoot_focusedViewParentBecomesInvisible() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mParent1.post(() -> {
            mParent1.setVisibility(View.INVISIBLE);
            mParent1.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFocusedByDefault.isFocused()).isTrue();
    }

    @Test
    public void testRequestFocus_focusesFpvWhenShouldRestoreFocusIsFalse() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.setShouldRestoreFocus(false);
            mFpv.requestFocus();
            mFpv.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFpv.isFocused()).isTrue();
    }

    @Test
    public void testRestoreDefaultFocus_focusesFpvWhenShouldRestoreFocusIsFalse() throws Exception {
        CountDownLatch latch1 = new CountDownLatch(1);
        mView1.post(() -> {
            mView1.requestFocus();
            mView1.post(() -> latch1.countDown());
        });
        latch1.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mView1.isFocused()).isTrue();

        CountDownLatch latch2 = new CountDownLatch(1);
        mFpv.post(() -> {
            mFpv.setShouldRestoreFocus(false);
            mFpv.restoreDefaultFocus();
            mFpv.post(() -> latch2.countDown());
        });
        latch2.await(WAIT_TIME_MS, TimeUnit.MILLISECONDS);
        assertThat(mFpv.isFocused()).isTrue();
    }
}
