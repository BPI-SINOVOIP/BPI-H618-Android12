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

package com.android.car.ui.recyclerview;

import static android.car.drivingstate.CarUxRestrictions.UX_RESTRICTIONS_LIMIT_CONTENT;

import static androidx.core.math.MathUtils.clamp;
import static androidx.recyclerview.widget.RecyclerView.SCROLL_STATE_DRAGGING;
import static androidx.recyclerview.widget.RecyclerView.SCROLL_STATE_IDLE;
import static androidx.recyclerview.widget.RecyclerView.SCROLL_STATE_SETTLING;
import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.assertion.PositionAssertions.isBottomAlignedWith;
import static androidx.test.espresso.assertion.PositionAssertions.isCompletelyAbove;
import static androidx.test.espresso.assertion.PositionAssertions.isCompletelyRightOf;
import static androidx.test.espresso.assertion.PositionAssertions.isLeftAlignedWith;
import static androidx.test.espresso.assertion.PositionAssertions.isRightAlignedWith;
import static androidx.test.espresso.assertion.PositionAssertions.isTopAlignedWith;
import static androidx.test.espresso.assertion.ViewAssertions.doesNotExist;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.contrib.RecyclerViewActions.scrollToPosition;
import static androidx.test.espresso.matcher.ViewMatchers.assertThat;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.isEnabled;
import static androidx.test.espresso.matcher.ViewMatchers.isRoot;
import static androidx.test.espresso.matcher.ViewMatchers.withContentDescription;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static com.android.car.ui.actions.LowLevelActions.performDrag;
import static com.android.car.ui.actions.LowLevelActions.pressAndHold;
import static com.android.car.ui.actions.LowLevelActions.release;
import static com.android.car.ui.actions.LowLevelActions.touchDownAndUp;
import static com.android.car.ui.actions.ViewActions.waitForView;
import static com.android.car.ui.recyclerview.CarUiRecyclerView.ItemCap.UNLIMITED;
import static com.android.car.ui.utils.ViewUtils.setRotaryScrollEnabled;

import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.greaterThan;
import static org.hamcrest.Matchers.greaterThanOrEqualTo;
import static org.hamcrest.Matchers.is;
import static org.hamcrest.Matchers.lessThan;
import static org.hamcrest.Matchers.lessThanOrEqualTo;
import static org.hamcrest.Matchers.not;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import static java.lang.Math.max;

import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.GridLayoutManager.SpanSizeLookup;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.LinearSnapHelper;
import androidx.recyclerview.widget.OrientationHelper;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.LayoutManager;
import androidx.recyclerview.widget.RecyclerView.LayoutParams;
import androidx.test.core.app.ActivityScenario;
import androidx.test.espresso.IdlingRegistry;
import androidx.test.espresso.IdlingResource;
import androidx.test.ext.junit.rules.ActivityScenarioRule;

import com.android.car.ui.TestActivity;
import com.android.car.ui.recyclerview.decorations.grid.GridDividerItemDecoration;
import com.android.car.ui.test.R;
import com.android.car.ui.utils.CarUxRestrictionsUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

/**
 * Unit tests for {@link CarUiRecyclerView}.
 */
public class CarUiRecyclerViewTest {

    @Rule
    public ActivityScenarioRule<TestActivity> mActivityRule =
            new ActivityScenarioRule<>(TestActivity.class);

    ActivityScenario<TestActivity> mScenario;

    private TestActivity mActivity;
    private Context mTestableContext;
    private Resources mTestableResources;

    @Before
    public void setUp() {
        mScenario = mActivityRule.getScenario();
        mScenario.onActivity(activity -> {
            mActivity = activity;
            mTestableContext = spy(mActivity);
            mTestableResources = spy(mActivity.getResources());
        });

        when(mTestableContext.getResources()).thenReturn(mTestableResources);
    }

    @After
    public void tearDown() {
        for (IdlingResource idlingResource : IdlingRegistry.getInstance().getResources()) {
            IdlingRegistry.getInstance().unregister(idlingResource);
        }
    }

    @Test
    public void testIsScrollbarPresent_scrollbarEnabled() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);
        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setAdapter(new TestAdapter(100));
        });

        onView(withId(R.id.car_ui_scroll_bar)).check(matches(isDisplayed()));
    }

    @Test
    public void testIsScrollbarPresent_scrollbarDisabled() {
        doReturn(false).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);
        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setAdapter(new TestAdapter(100));
        });

        onView(withId(R.id.car_ui_scroll_bar)).check(doesNotExist());
    }

    @Test
    public void testPadding() {
        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        int padding = 100;
        carUiRecyclerView.setPadding(padding, 0, padding, 0);
        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setAdapter(new TestAdapter(100));
        });

        assertEquals(padding, carUiRecyclerView.getPaddingLeft());
        assertEquals(padding, carUiRecyclerView.getPaddingRight());
    }

    @Test
    public void testGridLayout() {
        TypedArray typedArray = spy(mActivity.getBaseContext().obtainStyledAttributes(
                null, R.styleable.CarUiRecyclerView));

        doReturn(typedArray).when(mTestableContext).obtainStyledAttributes(
                any(),
                eq(R.styleable.CarUiRecyclerView),
                anyInt(),
                anyInt());
        when(typedArray.getInt(eq(R.styleable.CarUiRecyclerView_layoutStyle), anyInt()))
                .thenReturn(CarUiRecyclerView.CarUiRecyclerViewLayout.GRID);
        when(typedArray.getInt(eq(R.styleable.CarUiRecyclerView_numOfColumns), anyInt()))
                .thenReturn(3);

        // Ensure the CarUiRecyclerViewLayout constant matches the styleable attribute enum value
        assertEquals(CarUiRecyclerView.CarUiRecyclerViewLayout.GRID, 1);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(4);
        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setAdapter(adapter);
        });

        assertTrue(carUiRecyclerView.getLayoutManager() instanceof GridLayoutManager);

        // Check that all items in the first row are top-aligned.
        onView(withText(adapter.getItemText(0))).check(
                isTopAlignedWith(withText(adapter.getItemText(1))));
        onView(withText(adapter.getItemText(1))).check(
                isTopAlignedWith(withText(adapter.getItemText(2))));

        // Check that all items in the first row are bottom-aligned.
        onView(withText(adapter.getItemText(0))).check(
                isBottomAlignedWith(withText(adapter.getItemText(1))));
        onView(withText(adapter.getItemText(1))).check(
                isBottomAlignedWith(withText(adapter.getItemText(2))));

        // Check that items in second row are rendered correctly below the first row.
        onView(withText(adapter.getItemText(0))).check(
                isCompletelyAbove(withText(adapter.getItemText(3))));
        onView(withText(adapter.getItemText(0))).check(
                isLeftAlignedWith(withText(adapter.getItemText(3))));
        onView(withText(adapter.getItemText(0))).check(
                isRightAlignedWith(withText(adapter.getItemText(3))));
    }

    @Test
    public void testLinearLayout() {
        TypedArray typedArray = spy(mActivity.getBaseContext().obtainStyledAttributes(
                null, R.styleable.CarUiRecyclerView));

        doReturn(typedArray).when(mTestableContext).obtainStyledAttributes(
                any(),
                eq(R.styleable.CarUiRecyclerView),
                anyInt(),
                anyInt());
        when(typedArray.getInt(eq(R.styleable.CarUiRecyclerView_layoutStyle), anyInt()))
                .thenReturn(CarUiRecyclerView.CarUiRecyclerViewLayout.LINEAR);

        // Ensure the CarUiRecyclerViewLayout constant matches the styleable attribute enum value
        assertEquals(CarUiRecyclerView.CarUiRecyclerViewLayout.LINEAR, 0);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(4);
        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setAdapter(adapter);
        });

        assertTrue(carUiRecyclerView.getLayoutManager() instanceof LinearLayoutManager);

        // Check that item views are laid out linearly.
        onView(withText(adapter.getItemText(0))).check(
                isCompletelyAbove(withText(adapter.getItemText(1))));
        onView(withText(adapter.getItemText(1))).check(
                isCompletelyAbove(withText(adapter.getItemText(2))));
        onView(withText(adapter.getItemText(2))).check(
                isCompletelyAbove(withText(adapter.getItemText(3))));
    }

    @Test
    public void testLayoutManagerSetInXml() {
        // Inflate activity where a LayoutManger is set for a CarUiRecyclerView through a
        // styleable attribute.
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(
                        R.layout.car_ui_recycler_view_layout_manager_xml_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        TestAdapter adapter = new TestAdapter(3);
        mActivity.runOnUiThread(() -> {
            setRotaryScrollEnabled(carUiRecyclerView, /* isVertical= */ true);
            carUiRecyclerView.setAdapter(adapter);
            carUiRecyclerView.setVisibility(View.VISIBLE);
        });

        // Check that items in are displayed.
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));
        onView(withText(adapter.getItemText(1))).check(matches(isDisplayed()));
        onView(withText(adapter.getItemText(2))).check(matches(isDisplayed()));

        assertTrue(carUiRecyclerView.getLayoutManager() instanceof GridLayoutManager);
    }

    @Test
    public void testSetLayoutManager_shouldUpdateItemDecorations() {
        TypedArray typedArray = spy(mActivity.getBaseContext().obtainStyledAttributes(
                null, R.styleable.CarUiRecyclerView));

        doReturn(typedArray).when(mTestableContext).obtainStyledAttributes(
                any(),
                eq(R.styleable.CarUiRecyclerView),
                anyInt(),
                anyInt());
        when(typedArray.getBoolean(eq(R.styleable.CarUiRecyclerView_enableDivider), anyBoolean()))
                .thenReturn(true);
        when(typedArray.getInt(eq(R.styleable.CarUiRecyclerView_layoutStyle), anyInt()))
                .thenReturn(CarUiRecyclerView.CarUiRecyclerViewLayout.GRID);
        when(typedArray.getInt(eq(R.styleable.CarUiRecyclerView_numOfColumns), anyInt()))
                .thenReturn(3);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(4);
        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setAdapter(adapter);
        });

        assertTrue(carUiRecyclerView.getLayoutManager() instanceof GridLayoutManager);
        assertEquals(carUiRecyclerView.getItemDecorationCount(), 3);
        assertTrue(carUiRecyclerView.getItemDecorationAt(0) instanceof GridDividerItemDecoration);

        carUiRecyclerView.setLayoutManager(new LinearLayoutManager(mTestableContext));

        assertTrue(carUiRecyclerView.getLayoutManager() instanceof LinearLayoutManager);
        assertEquals(carUiRecyclerView.getItemDecorationCount(), 3);
        assertFalse(carUiRecyclerView.getItemDecorationAt(0) instanceof GridDividerItemDecoration);
    }

    @Test
    public void testVisibility_goneAtInflationWithChangeToVisible() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(
                        R.layout.car_ui_recycler_view_gone_test_activity));

        onView(withId(R.id.list)).check(matches(not(isDisplayed())));

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        TestAdapter adapter = new TestAdapter(3);
        mActivity.runOnUiThread(() -> {
            carUiRecyclerView.setAdapter(adapter);
            carUiRecyclerView.setVisibility(View.VISIBLE);
        });

        // Check that items in are displayed.
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));
        onView(withText(adapter.getItemText(1))).check(matches(isDisplayed()));
        onView(withText(adapter.getItemText(2))).check(matches(isDisplayed()));
    }

    @Test
    public void testVisibility_invisibleAtInflationWithChangeToVisible() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(
                        R.layout.car_ui_recycler_view_invisible_test_activity));

        onView(withId(R.id.list)).check(matches(not(isDisplayed())));

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        TestAdapter adapter = new TestAdapter(3);
        mActivity.runOnUiThread(() -> {
            carUiRecyclerView.setAdapter(adapter);
            carUiRecyclerView.setVisibility(View.VISIBLE);
        });

        // Check that items in are displayed.
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));
        onView(withText(adapter.getItemText(1))).check(matches(isDisplayed()));
        onView(withText(adapter.getItemText(2))).check(matches(isDisplayed()));
    }

    @Test
    public void testFirstItemAtTop_onInitialLoad() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        TestAdapter adapter = new TestAdapter(25);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        LinearLayoutManager layoutManager =
                (LinearLayoutManager) carUiRecyclerView.getLayoutManager();
        assertEquals(layoutManager.findFirstVisibleItemPosition(), 0);
    }

    @Test
    public void testPageUpAndDownMoveSameDistance() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);

        // Can't use OrientationHelper here, because it returns 0 when calling getTotalSpace methods
        // until LayoutManager's onLayoutComplete is called. In this case waiting until the first
        // item of the list is displayed guarantees that OrientationHelper is initialized properly.
        int totalSpace = carUiRecyclerView.getHeight()
                - carUiRecyclerView.getPaddingTop()
                - carUiRecyclerView.getPaddingBottom();
        PerfectFitTestAdapter adapter = new PerfectFitTestAdapter(5, totalSpace);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        LinearLayoutManager layoutManager =
                (LinearLayoutManager) carUiRecyclerView.getLayoutManager();

        OrientationHelper orientationHelper = OrientationHelper.createVerticalHelper(layoutManager);
        assertEquals(totalSpace, orientationHelper.getTotalSpace());

        // Move down one page so there will be sufficient pages for up and downs.
        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());

        int topPosition = layoutManager.findFirstVisibleItemPosition();

        for (int i = 0; i < 3; i++) {
            onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());
            onView(withId(R.id.car_ui_scrollbar_page_up)).perform(click());
        }

        assertEquals(layoutManager.findFirstVisibleItemPosition(), topPosition);
    }

    @Test
    public void testContinuousScroll() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        TestAdapter adapter = new TestAdapter(50);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        LinearLayoutManager layoutManager =
                (LinearLayoutManager) carUiRecyclerView.getLayoutManager();

        // Press and hold the down button for 2 seconds to scroll the list to bottom.
        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(pressAndHold());
        onView(isRoot()).perform(waitForView(withText("Sample item #49"), 3000));
        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(release());

        assertEquals(layoutManager.findLastCompletelyVisibleItemPosition(), 49);
    }

    @Test
    public void testAlphaJumpToMiddleForThumbWhenTrackClicked() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        TestAdapter adapter = new TestAdapter(50);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        View trackView = mActivity.requireViewById(R.id.car_ui_scrollbar_track);
        // scroll to the middle
        onView(withId(R.id.car_ui_scrollbar_track)).perform(
                touchDownAndUp(0f, (trackView.getHeight() / 2f)));
        onView(withText(adapter.getItemText(25))).check(matches(isDisplayed()));
    }

    @Test
    public void testAlphaJumpToEndAndStartForThumbWhenTrackClicked() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        TestAdapter adapter = new TestAdapter(50);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        View trackView = mActivity.requireViewById(R.id.car_ui_scrollbar_track);
        View thumbView = mActivity.requireViewById(R.id.car_ui_scrollbar_thumb);
        // scroll to the end
        onView(withId(R.id.car_ui_scrollbar_track)).perform(
                touchDownAndUp(0f, trackView.getHeight() - 1));
        onView(withText(adapter.getItemText(49))).check(matches(isDisplayed()));

        // scroll to the start
        onView(withId(R.id.car_ui_scrollbar_track)).perform(
                touchDownAndUp(0f, 1));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));
    }

    @Test
    public void testThumbDragToCenter() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        TestAdapter adapter = new TestAdapter(50);
        mActivity.runOnUiThread(() -> {
            carUiRecyclerView.setAdapter(adapter);
        });

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        View trackView = mActivity.requireViewById(R.id.car_ui_scrollbar_track);
        View thumbView = mActivity.requireViewById(R.id.car_ui_scrollbar_thumb);
        // if you drag too far in a single step you'll stop selecting the thumb view. Hence, drag
        // 5 units at a time for 200 intervals and stop at the center of the track by limitY.

        onView(withId(R.id.car_ui_scrollbar_track)).perform(
                performDrag(0f, (thumbView.getHeight() / 2f), 0,
                        5, 200, Float.MAX_VALUE,
                        trackView.getHeight() / 2f));
        onView(withText(adapter.getItemText(25))).check(matches(isDisplayed()));
    }

    @Test
    public void testPageUpButtonDisabledAtTop() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        //  50, because needs to be big enough to make sure content is scrollable.
        TestAdapter adapter = new TestAdapter(50);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));

        // Initially page_up button is disabled.
        onView(withId(R.id.car_ui_scrollbar_page_up)).check(matches(not(isEnabled())));

        // Moving down, should enable the up bottom.
        onView(withId(R.id.car_ui_scrollbar_page_down)).check(matches(isEnabled()));
        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());
        onView(withId(R.id.car_ui_scrollbar_page_up)).check(matches(isEnabled()));

        // Move back up; this should disable the up button again.
        onView(withId(R.id.car_ui_scrollbar_page_up)).perform(click()).check(
                matches(not(isEnabled())));
    }

    @Test
    public void testPageUpScrollsWithoutSnap() {
        RecyclerView.OnScrollListener scrollListener = mock(RecyclerView.OnScrollListener.class);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setAdapter(new TestAdapter(100));
        });

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));

        // Scroll down a few pages so that you can perform page up operations.
        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());
        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());
        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());
        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());

        // Set a mocked scroll listener on the CarUiRecyclerView
        carUiRecyclerView.addOnScrollListener(scrollListener);

        onView(withId(R.id.car_ui_scrollbar_page_up)).perform(click());

        // Verify that scroll operation only settles on the destination once. This means a single
        // smooth scroll to the destination. If the scroll includes a secondary snap after an
        // initial scroll, this callback will have more than one invocation.
        verify(scrollListener, times(1)).onScrollStateChanged(carUiRecyclerView,
                SCROLL_STATE_SETTLING);

        onView(withId(R.id.car_ui_scrollbar_page_up)).perform(click());

        // Make same verification as above for a second page up operation.
        verify(scrollListener, times(2)).onScrollStateChanged(carUiRecyclerView,
                SCROLL_STATE_SETTLING);
    }

    @Test
    public void testPageDownScrollsWithoutSnap() {
        RecyclerView.OnScrollListener scrollListener = mock(RecyclerView.OnScrollListener.class);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setAdapter(new TestAdapter(100));
        });

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));

        // Set a mocked scroll listener on the CarUiRecyclerView
        carUiRecyclerView.addOnScrollListener(scrollListener);

        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());

        // Verify that scroll operation only settles on the destination once. This means a single
        // smooth scroll to the destination. If the scroll includes a secondary snap after an
        // initial scroll, this callback will have more than one invocation.
        verify(scrollListener, times(1)).onScrollStateChanged(carUiRecyclerView,
                SCROLL_STATE_SETTLING);

        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());

        // Make same verification as above for a second page down operation.
        verify(scrollListener, times(2)).onScrollStateChanged(carUiRecyclerView,
                SCROLL_STATE_SETTLING);
    }

    @Test
    public void testPageDownScrollsOverLongItem() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        int itemCount = 100;
        // Position the long item in the middle.
        int longItemPosition = itemCount / 2;

        Map<Integer, TestAdapter.ItemHeight> heightOverrides = new HashMap<>();
        heightOverrides.put(longItemPosition, TestAdapter.ItemHeight.TALL);
        TestAdapter adapter = new TestAdapter(itemCount, heightOverrides);

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        OrientationHelper orientationHelper =
                OrientationHelper.createVerticalHelper(carUiRecyclerView.getLayoutManager());
        int screenHeight = orientationHelper.getTotalSpace();
        // Scroll to a position where long item is partially visible.
        // Scrolling from top, scrollToPosition() aligns the pos-1 item to bottom.
        onView(withId(R.id.list)).perform(scrollToPosition(longItemPosition - 1));
        // Scroll by half the height of the screen so the long item is partially visible.
        mActivity.runOnUiThread(() -> carUiRecyclerView.scrollBy(0, screenHeight / 2));
        // This is needed to make sure scroll is finished before looking for the long item.
        onView(withText(adapter.getItemText(longItemPosition - 1))).check(matches(isDisplayed()));

        // Verify long item is partially shown.
        View longItem = getLongItem(carUiRecyclerView);
        assertThat(
                orientationHelper.getDecoratedStart(longItem),
                is(greaterThan(orientationHelper.getStartAfterPadding())));

        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());

        // Verify long item is snapped to top.
        assertThat(orientationHelper.getDecoratedStart(longItem),
                is(equalTo(orientationHelper.getStartAfterPadding())));
        assertThat(orientationHelper.getDecoratedEnd(longItem),
                is(greaterThan(orientationHelper.getEndAfterPadding())));

        // Set a limit to avoid test stuck in non-moving state.
        while (orientationHelper.getDecoratedEnd(longItem)
                > orientationHelper.getEndAfterPadding()) {
            onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());
        }

        // Verify long item end is aligned to bottom.
        assertThat(orientationHelper.getDecoratedEnd(longItem),
                is(equalTo(orientationHelper.getEndAfterPadding())));

        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());
        // Verify that the long item is no longer visible; Should be on the next child
        assertThat(
                orientationHelper.getDecoratedStart(longItem),
                is(lessThan(orientationHelper.getStartAfterPadding())));
    }

    @Test
    public void testPageDownScrollsOverLongItemAtTheEnd() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        int itemCount = 100;
        // Position the long item at the end.
        int longItemPosition = itemCount - 1;

        Map<Integer, TestAdapter.ItemHeight> heightOverrides = new HashMap<>();
        heightOverrides.put(longItemPosition, TestAdapter.ItemHeight.TALL);
        TestAdapter adapter = new TestAdapter(itemCount, heightOverrides);

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        mActivity.runOnUiThread(() -> {
            // Setting top padding to any number greater than 0.
            // Not having padding will make this test pass all the time.
            // Also adding bottom padding to make sure the padding
            // after the last content is considered in calculations.
            carUiRecyclerView.setPadding(0, 1, 0, 1);
            carUiRecyclerView.setAdapter(adapter);
        });

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        OrientationHelper orientationHelper =
                OrientationHelper.createVerticalHelper(carUiRecyclerView.getLayoutManager());

        // 20 is just an arbitrary number to make sure we reach the end of the recyclerview.
        for (int i = 0; i < 20; i++) {
            onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());
        }

        onView(withId(R.id.car_ui_scrollbar_page_down)).check(matches(not(isEnabled())));

        View longItem = getLongItem(carUiRecyclerView);
        // Making sure we've reached end of the recyclerview, after
        // adding bottom padding
        assertThat(orientationHelper.getDecoratedEnd(longItem),
                is(equalTo(orientationHelper.getEndAfterPadding())));
    }

    @Test
    public void testPageUpScrollsOverLongItem() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        int itemCount = 100;
        // Position the long item in the middle.
        int longItemPosition = itemCount / 2;

        Map<Integer, TestAdapter.ItemHeight> heightOverrides = new HashMap<>();
        heightOverrides.put(longItemPosition, TestAdapter.ItemHeight.TALL);
        TestAdapter adapter = new TestAdapter(itemCount, heightOverrides);

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        OrientationHelper orientationHelper =
                OrientationHelper.createVerticalHelper(carUiRecyclerView.getLayoutManager());

        // Scroll to a position just below the long item.
        onView(withId(R.id.list)).perform(scrollToPosition(longItemPosition + 1));

        // Verify long item is off-screen.
        View longItem = getLongItem(carUiRecyclerView);

        assertThat(
                orientationHelper.getDecoratedEnd(longItem),
                is(lessThanOrEqualTo(orientationHelper.getEndAfterPadding())));

        if (orientationHelper.getStartAfterPadding() - orientationHelper.getDecoratedStart(longItem)
                < orientationHelper.getTotalSpace()) {
            onView(withId(R.id.car_ui_scrollbar_page_up)).perform(click());
            assertThat(orientationHelper.getDecoratedStart(longItem),
                    is(greaterThanOrEqualTo(orientationHelper.getStartAfterPadding())));
        } else {
            int topBeforeClick = orientationHelper.getDecoratedStart(longItem);

            onView(withId(R.id.car_ui_scrollbar_page_up)).perform(click());

            // Verify we scrolled 1 screen
            assertThat(orientationHelper.getStartAfterPadding() - topBeforeClick,
                    is(equalTo(orientationHelper.getTotalSpace())));
        }
    }

    @Test
    public void testPageDownScrollsOverVeryLongItem() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        int itemCount = 100;
        // Position the long item in the middle.
        int longItemPosition = itemCount / 2;

        Map<Integer, TestAdapter.ItemHeight> heightOverrides = new HashMap<>();
        heightOverrides.put(longItemPosition, TestAdapter.ItemHeight.EXTRA_TALL);
        TestAdapter adapter = new TestAdapter(itemCount, heightOverrides);

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        OrientationHelper orientationHelper =
                OrientationHelper.createVerticalHelper(carUiRecyclerView.getLayoutManager());

        int screenHeight = Resources.getSystem().getDisplayMetrics().heightPixels;
        // Scroll to a position where long item is partially visible.
        // Scrolling from top, scrollToPosition() aligns the pos-1 item to bottom.
        onView(withId(R.id.list)).perform(scrollToPosition(longItemPosition - 1));
        // Scroll by half the height of the screen so the long item is partially visible.
        mActivity.runOnUiThread(() -> carUiRecyclerView.scrollBy(0, screenHeight / 2));

        onView(withText(adapter.getItemText(longItemPosition))).check(matches(isDisplayed()));

        // Verify long item is partially shown.
        View longItem = getLongItem(carUiRecyclerView);
        assertThat(
                orientationHelper.getDecoratedStart(longItem),
                is(greaterThan(orientationHelper.getStartAfterPadding())));

        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());

        // Verify long item is snapped to top.
        assertThat(orientationHelper.getDecoratedStart(longItem),
                is(equalTo(orientationHelper.getStartAfterPadding())));
        assertThat(orientationHelper.getDecoratedEnd(longItem),
                is(greaterThan(orientationHelper.getEndAfterPadding())));

        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());

        // Verify long item does not snap to bottom.
        assertThat(orientationHelper.getDecoratedEnd(longItem),
                not(equalTo(orientationHelper.getEndAfterPadding())));
    }

    @Test
    public void testPageDownScrollsOverVeryLongItemAtTheEnd() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        int itemCount = 100;
        // Position the long item at the end.
        int longItemPosition = itemCount - 1;

        Map<Integer, TestAdapter.ItemHeight> heightOverrides = new HashMap<>();
        heightOverrides.put(longItemPosition, TestAdapter.ItemHeight.EXTRA_TALL);
        TestAdapter adapter = new TestAdapter(itemCount, heightOverrides);

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        mActivity.runOnUiThread(() -> {
            // Setting top padding to any number greater than 0.
            // Not having padding will make this test pass all the time.
            // Also adding bottom padding to make sure the padding
            // after the last content is considered in calculations.
            carUiRecyclerView.setPadding(0, 1, 0, 1);
            carUiRecyclerView.setAdapter(adapter);
        });

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        OrientationHelper orientationHelper =
                OrientationHelper.createVerticalHelper(carUiRecyclerView.getLayoutManager());

        // 20 is just an arbitrary number to make sure we reach the end of the recyclerview.
        for (int i = 0; i < 20; i++) {
            onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());
        }

        onView(withId(R.id.car_ui_scrollbar_page_down)).check(matches(not(isEnabled())));

        View longItem = getLongItem(carUiRecyclerView);
        // Making sure we've reached end of the recyclerview, after
        // adding bottom padding
        assertThat(orientationHelper.getDecoratedEnd(longItem),
                is(equalTo(orientationHelper.getEndAfterPadding())));
    }

    @Test
    public void testPageDownMaintainsMinimumScrollThumbTrackHeight() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        int itemCount = 25000;
        TestAdapter adapter = new TestAdapter(itemCount);

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));
        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        mActivity.runOnUiThread(() -> carUiRecyclerView.requestLayout());

        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());

        // Check that thumb track maintains minimum height
        int minThumbViewHeight = mActivity.getResources()
                .getDimensionPixelOffset(R.dimen.car_ui_scrollbar_min_thumb_height);
        View thumbView = mActivity.requireViewById(R.id.car_ui_scrollbar_thumb);
        assertThat(thumbView.getHeight(), is(equalTo(minThumbViewHeight)));
    }

    @Test
    public void testSetPaddingToRecyclerViewContainerWithScrollbar() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);

        assertThat(carUiRecyclerView.getPaddingBottom(), is(equalTo(0)));
        assertThat(carUiRecyclerView.getPaddingTop(), is(equalTo(0)));
        assertThat(carUiRecyclerView.getPaddingStart(), is(equalTo(0)));
        assertThat(carUiRecyclerView.getPaddingEnd(), is(equalTo(0)));

        carUiRecyclerView.setPadding(10, 10, 10, 10);

        assertThat(carUiRecyclerView.getPaddingTop(), is(equalTo(10)));
        assertThat(carUiRecyclerView.getPaddingBottom(), is(equalTo(10)));
        assertThat(carUiRecyclerView.getPaddingStart(), is(equalTo(0)));
        assertThat(carUiRecyclerView.getPaddingEnd(), is(equalTo(0)));
    }

    @Test
    public void testSetPaddingToRecyclerViewContainerWithoutScrollbar() {
        doReturn(false).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);

        assertThat(carUiRecyclerView.getPaddingBottom(), is(equalTo(0)));
        assertThat(carUiRecyclerView.getPaddingTop(), is(equalTo(0)));
        assertThat(carUiRecyclerView.getPaddingStart(), is(equalTo(0)));
        assertThat(carUiRecyclerView.getPaddingEnd(), is(equalTo(0)));

        carUiRecyclerView.setPadding(10, 10, 10, 10);

        assertThat(carUiRecyclerView.getPaddingTop(), is(equalTo(10)));
        assertThat(carUiRecyclerView.getPaddingBottom(), is(equalTo(10)));
        assertThat(carUiRecyclerView.getPaddingStart(), is(equalTo(10)));
        assertThat(carUiRecyclerView.getPaddingEnd(), is(equalTo(10)));
    }

    @Test
    public void testSetPaddingRelativeToRecyclerViewContainerWithScrollbar() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);

        assertThat(carUiRecyclerView.getPaddingBottom(), is(equalTo(0)));
        assertThat(carUiRecyclerView.getPaddingTop(), is(equalTo(0)));
        assertThat(carUiRecyclerView.getPaddingStart(), is(equalTo(0)));
        assertThat(carUiRecyclerView.getPaddingEnd(), is(equalTo(0)));

        carUiRecyclerView.setPaddingRelative(10, 10, 10, 10);

        assertThat(carUiRecyclerView.getPaddingTop(), is(equalTo(10)));
        assertThat(carUiRecyclerView.getPaddingBottom(), is(equalTo(10)));
        assertThat(carUiRecyclerView.getPaddingStart(), is(equalTo(0)));
        assertThat(carUiRecyclerView.getPaddingEnd(), is(equalTo(0)));
    }

    @Test
    public void testSetPaddingRelativeToRecyclerViewContainerWithoutScrollbar() {
        doReturn(false).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);

        assertThat(carUiRecyclerView.getPaddingBottom(), is(equalTo(0)));
        assertThat(carUiRecyclerView.getPaddingTop(), is(equalTo(0)));
        assertThat(carUiRecyclerView.getPaddingStart(), is(equalTo(0)));
        assertThat(carUiRecyclerView.getPaddingEnd(), is(equalTo(0)));

        carUiRecyclerView.setPaddingRelative(10, 10, 10, 10);

        assertThat(carUiRecyclerView.getPaddingTop(), is(equalTo(10)));
        assertThat(carUiRecyclerView.getPaddingBottom(), is(equalTo(10)));
        assertThat(carUiRecyclerView.getPaddingStart(), is(equalTo(10)));
        assertThat(carUiRecyclerView.getPaddingEnd(), is(equalTo(10)));
    }

    @Test
    public void testSetAlphaToRecyclerViewWithoutScrollbar() {
        doReturn(false).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);

        assertThat(carUiRecyclerView.getAlpha(), is(equalTo(1.0f)));

        carUiRecyclerView.setAlpha(0.5f);

        assertThat(carUiRecyclerView.getAlpha(), is(equalTo(0.5f)));
    }

    @Test
    public void testSetAlphaToRecyclerViewWithScrollbar() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(
                        R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);

        ViewGroup container = (ViewGroup) carUiRecyclerView.getParent().getParent();

        assertThat(carUiRecyclerView.getAlpha(), is(equalTo(1.0f)));
        assertThat(container.getAlpha(), is(equalTo(1.0f)));

        carUiRecyclerView.setAlpha(0.5f);

        assertThat(carUiRecyclerView.getAlpha(), is(equalTo(1.0f)));
        assertThat(container.getAlpha(), is(equalTo(0.5f)));
    }

    @Test
    public void testCallAnimateOnRecyclerViewWithScrollbar() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);
        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);

        ViewGroup container = mActivity.findViewById(R.id.test_container);
        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setAdapter(new TestAdapter(100));
        });

        onView(withId(R.id.car_ui_scroll_bar)).check(matches(isDisplayed()));

        ViewGroup recyclerViewContainer = (ViewGroup) carUiRecyclerView.getParent().getParent();

        assertThat(carUiRecyclerView.animate(), is(equalTo(recyclerViewContainer.animate())));
    }

    @Test
    public void testScrollbarVisibility_tooSmallHeight() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);

        // Set to anything less than 2 * (minTouchSize + margin)
        // minTouchSize = R.dimen.car_ui_touch_target_size
        // margin is button up top margin or button down bottom margin
        int recyclerviewHeight = 1;

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);

        ViewGroup container = mActivity.findViewById(R.id.test_container);
        container.post(() -> {
            FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(-1, recyclerviewHeight);
            container.addView(carUiRecyclerView, lp);
            carUiRecyclerView.setAdapter(new TestAdapter(100));
        });

        onView(withId(R.id.car_ui_scroll_bar)).check(matches(not(isDisplayed())));

        OrientationHelper orientationHelper =
                OrientationHelper.createVerticalHelper(carUiRecyclerView.getLayoutManager());
        int screenHeight = orientationHelper.getTotalSpace();

        assertEquals(recyclerviewHeight, screenHeight);
    }

    @Test
    public void testScrollbarVisibility_justEnoughToShowOnlyButtons() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);

        // R.dimen.car_ui_touch_target_size
        float minTouchSize = mTestableResources.getDimension(R.dimen.car_ui_touch_target_size);
        // This value is hardcoded to 15dp in the layout.
        int margin = (int) dpToPixel(mTestableContext, 15)
                + (int) mTestableResources.getDimension(R.dimen.car_ui_scrollbar_separator_margin);
        // Set to 2 * (minTouchSize + margin)
        int recyclerviewHeight = 2 * (int) (minTouchSize + margin);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);

        ViewGroup container = mActivity.findViewById(R.id.test_container);
        container.post(() -> {
            FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(-1, recyclerviewHeight);
            container.addView(carUiRecyclerView, lp);
            carUiRecyclerView.setAdapter(new TestAdapter(100));
        });

        onView(withId(R.id.car_ui_scroll_bar)).check(matches(isDisplayed()));
        onView(withId(R.id.car_ui_scrollbar_thumb)).check(matches(not(isDisplayed())));
        onView(withId(R.id.car_ui_scrollbar_track)).check(matches(not(isDisplayed())));
        onView(withId(R.id.car_ui_scrollbar_page_down)).check(matches(isDisplayed()));
        onView(withId(R.id.car_ui_scrollbar_page_up)).check(matches(isDisplayed()));

        OrientationHelper orientationHelper =
                OrientationHelper.createVerticalHelper(carUiRecyclerView.getLayoutManager());
        int screenHeight = orientationHelper.getTotalSpace();

        assertEquals(recyclerviewHeight, screenHeight);
    }

    @Test
    public void testScrollbarVisibility_enoughToShowEverything() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);

        int minTouchSize = (int) mTestableResources.getDimension(R.dimen.car_ui_touch_target_size);
        int mScrollbarThumbMinHeight = (int) mTestableResources
                .getDimension(R.dimen.car_ui_scrollbar_min_thumb_height);
        // This value is hardcoded to 15dp in the layout.
        int margin = (int) dpToPixel(mTestableContext, 15)
                + (int) mTestableResources.getDimension(R.dimen.car_ui_scrollbar_separator_margin);
        int trackMargin = 2 * (int) mTestableResources
                .getDimension(R.dimen.car_ui_scrollbar_separator_margin);
        // Set to anything greater or equal to
        // 2 * minTouchSize + max(minTouchSize, mScrollbarThumbMinHeight) + 2 * margin
        int recyclerviewHeight =
                2 *  minTouchSize
                + max(minTouchSize, mScrollbarThumbMinHeight)
                + 2 * margin + trackMargin;

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);

        ViewGroup container = mActivity.findViewById(R.id.test_container);
        container.post(() -> {
            FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(-1, recyclerviewHeight);
            container.addView(carUiRecyclerView, lp);
            carUiRecyclerView.setAdapter(new TestAdapter(100));
        });

        onView(withId(R.id.car_ui_scroll_bar)).check(matches(isDisplayed()));
        onView(withId(R.id.car_ui_scrollbar_thumb)).check(matches(isDisplayed()));
        onView(withId(R.id.car_ui_scrollbar_track)).check(matches(isDisplayed()));
        onView(withId(R.id.car_ui_scrollbar_page_down)).check(matches(isDisplayed()));
        onView(withId(R.id.car_ui_scrollbar_page_up)).check(matches(isDisplayed()));

        OrientationHelper orientationHelper =
                OrientationHelper.createVerticalHelper(carUiRecyclerView.getLayoutManager());
        int screenHeight = orientationHelper.getTotalSpace();

        assertEquals(recyclerviewHeight, screenHeight);
    }

    @Test
    public void testDefaultSize_noScrollbar() {
        doReturn(false).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        int listId = View.generateViewId();
        TestAdapter adapter = new TestAdapter(50);
        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setId(listId);
            carUiRecyclerView.setAdapter(adapter);
        });

        onView(withId(R.id.car_ui_scroll_bar)).check(doesNotExist());
        onView(withText(adapter.getItemText(0))).check(isLeftAlignedWith(withId(listId)));
        onView(withText(adapter.getItemText(0))).check(isRightAlignedWith(withId(listId)));
    }

    @Test
    public void testLargeSize_withScrollbar() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);

        TypedArray typedArray = spy(mActivity.getBaseContext().obtainStyledAttributes(
                null, R.styleable.CarUiRecyclerView));

        doReturn(typedArray).when(mTestableContext).obtainStyledAttributes(
                any(),
                eq(R.styleable.CarUiRecyclerView),
                anyInt(),
                anyInt());
        when(typedArray.getInt(eq(R.styleable.CarUiRecyclerView_carUiSize),
                anyInt())).thenReturn(2); // Large size

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);
        int listId = View.generateViewId();
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(50);
        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setId(listId);
            carUiRecyclerView.setAdapter(adapter);
        });

        onView(withId(R.id.car_ui_scroll_bar)).check(matches(isDisplayed()));
        onView(withText(adapter.getItemText(0))).check(
                isCompletelyRightOf(withId(R.id.car_ui_scroll_bar)));
        onView(withText(adapter.getItemText(0))).check(
                matches(not(isRightAlignedWith(withId(listId)))));
    }

    @Test
    public void testMediumSize_withScrollbar() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);

        TypedArray typedArray = spy(mActivity.getBaseContext().obtainStyledAttributes(
                null, R.styleable.CarUiRecyclerView));

        doReturn(typedArray).when(mTestableContext).obtainStyledAttributes(
                any(),
                eq(R.styleable.CarUiRecyclerView),
                anyInt(),
                anyInt());
        when(typedArray.getInt(eq(R.styleable.CarUiRecyclerView_carUiSize),
                anyInt())).thenReturn(1); // Medium size

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);
        int listId = View.generateViewId();
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(50);
        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setId(listId);
            carUiRecyclerView.setAdapter(adapter);
        });

        onView(withId(R.id.car_ui_scroll_bar)).check(matches(isDisplayed()));
        onView(withText(adapter.getItemText(0))).check(
                isCompletelyRightOf(withId(R.id.car_ui_scroll_bar)));
        onView(withText(adapter.getItemText(0))).check(isRightAlignedWith(withId(listId)));
    }

    @Test
    public void testSmallSize_withScrollbar() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);

        TypedArray typedArray = spy(mActivity.getBaseContext().obtainStyledAttributes(
                null, R.styleable.CarUiRecyclerView));

        doReturn(typedArray).when(mTestableContext).obtainStyledAttributes(
                any(),
                eq(R.styleable.CarUiRecyclerView),
                anyInt(),
                anyInt());
        when(typedArray.getInt(eq(R.styleable.CarUiRecyclerView_carUiSize),
                anyInt())).thenReturn(0); // Small size

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);
        int listId = View.generateViewId();
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(50);
        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setId(listId);
            carUiRecyclerView.setAdapter(adapter);
        });

        onView(withId(R.id.car_ui_scroll_bar)).check(doesNotExist());
        onView(withText(adapter.getItemText(0))).check(isLeftAlignedWith(withId(listId)));
        onView(withText(adapter.getItemText(0))).check(isRightAlignedWith(withId(listId)));
    }

    @Test
    public void testSameSizeItems_estimateNextPositionDiffForScrollDistance() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        int itemCount = 100;
        TestAdapter adapter = new TestAdapter(itemCount);
        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        LayoutManager layoutManager = carUiRecyclerView.getLayoutManager();
        OrientationHelper orientationHelper = OrientationHelper.createVerticalHelper(layoutManager);

        int firstViewHeight = layoutManager.getChildAt(0).getHeight();
        int itemsToScroll = 10;
        CarUiSnapHelper snapHelper  = new CarUiSnapHelper(mActivity);
        // Get an estimate of how many items CarUiSnaphelpwer says we need to scroll. The scroll
        // distance is set to 10 * height of the first item. Since all items have the items have
        // the same height, we're expecting to get exactly 10 back from CarUiSnapHelper.
        int estimate = snapHelper.estimateNextPositionDiffForScrollDistance(orientationHelper,
                itemsToScroll * firstViewHeight);

        assertEquals(estimate, itemsToScroll);
    }

    @Test
    public void testSameSizeItems_estimateNextPositionDiffForScrollDistance_zeroDistance() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        int itemCount = 100;
        TestAdapter adapter = new TestAdapter(itemCount);
        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        LayoutManager layoutManager = carUiRecyclerView.getLayoutManager();
        OrientationHelper orientationHelper = OrientationHelper.createVerticalHelper(layoutManager);

        int firstViewHeight = layoutManager.getChildAt(0).getHeight();
        // the scroll distance has to be less than half of the size of the first view so that
        // recyclerview doesn't snap to the next view
        int distantToScroll = (firstViewHeight / 2) - 1;
        CarUiSnapHelper snapHelper  = new CarUiSnapHelper(mActivity);
        int estimate = snapHelper.estimateNextPositionDiffForScrollDistance(orientationHelper,
                distantToScroll);

        assertEquals(estimate, 0);
    }

    @Test
    public void testSameSizeItems_estimateNextPositionDiffForScrollDistance_zeroHeight() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        int itemCount = 1;
        Map<Integer, TestAdapter.ItemHeight> heightOverrides = new HashMap<>();
        heightOverrides.put(0, TestAdapter.ItemHeight.ZERO);
        TestAdapter adapter = new TestAdapter(itemCount, heightOverrides);

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withContentDescription("ZERO")).check(matches(isEnabled()));

        LayoutManager layoutManager = carUiRecyclerView.getLayoutManager();
        OrientationHelper orientationHelper = OrientationHelper.createVerticalHelper(layoutManager);

        // 10 is an arbitrary number
        int distantToScroll = 10;
        CarUiSnapHelper snapHelper  = new CarUiSnapHelper(mActivity);
        int estimate = snapHelper.estimateNextPositionDiffForScrollDistance(orientationHelper,
                distantToScroll);

        assertEquals(estimate, 0);
    }

    @Test
    public void testSameSizeItems_estimateNextPositionDiffForScrollDistance_zeroItems() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        int itemCount = 0;
        TestAdapter adapter = new TestAdapter(itemCount);

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));

        LayoutManager layoutManager = carUiRecyclerView.getLayoutManager();
        OrientationHelper orientationHelper = OrientationHelper.createVerticalHelper(layoutManager);
        CarUiSnapHelper snapHelper  = new CarUiSnapHelper(mActivity);
        int estimate = snapHelper.estimateNextPositionDiffForScrollDistance(orientationHelper, 50);

        assertEquals(estimate, 50);
    }

    @Test
    public void testEmptyList_calculateScrollDistanceClampToScreenSize() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        int itemCount = 0;
        TestAdapter adapter = new TestAdapter(itemCount);

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));

        LayoutManager layoutManager = carUiRecyclerView.getLayoutManager();

        LinearSnapHelper linearSnapHelper = new LinearSnapHelper();
        carUiRecyclerView.setOnFlingListener(null);
        linearSnapHelper.attachToRecyclerView(carUiRecyclerView);
        // 200 is just an arbitrary number. the intent is to make sure the return value is smaller
        // than the layoutmanager height.
        int[] baseOutDist = linearSnapHelper.calculateScrollDistance(200, -200);

        CarUiSnapHelper carUiSnapHelper = new CarUiSnapHelper(mTestableContext);
        carUiRecyclerView.setOnFlingListener(null);
        carUiSnapHelper.attachToRecyclerView(carUiRecyclerView);
        int[] outDist = carUiSnapHelper.calculateScrollDistance(200, -200);

        assertEquals(outDist[0], baseOutDist[0]);
        assertEquals(outDist[1], baseOutDist[1]);
    }

    @Test
    public void testCalculateScrollDistanceClampToScreenSize() {
        mActivity.runOnUiThread(
                () -> mActivity.setContentView(R.layout.car_ui_recycler_view_test_activity));

        onView(withId(R.id.list)).check(matches(isDisplayed()));

        int itemCount = 100;
        TestAdapter adapter = new TestAdapter(itemCount);

        CarUiRecyclerView carUiRecyclerView = mActivity.requireViewById(R.id.list);
        mActivity.runOnUiThread(() -> carUiRecyclerView.setAdapter(adapter));

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        LayoutManager layoutManager = carUiRecyclerView.getLayoutManager();
        OrientationHelper orientationHelper = OrientationHelper.createVerticalHelper(layoutManager);

        LinearSnapHelper linearSnapHelper = new LinearSnapHelper();
        carUiRecyclerView.setOnFlingListener(null);
        linearSnapHelper.attachToRecyclerView(carUiRecyclerView);
        // 8000 is just an arbitrary number. the intent is to make sure the return value is bigger
        // than the layoutmanager height.
        int[] baseOutDist = linearSnapHelper.calculateScrollDistance(8000, -8000);

        CarUiSnapHelper carUiSnapHelper = new CarUiSnapHelper(mTestableContext);
        carUiRecyclerView.setOnFlingListener(null);
        carUiSnapHelper.attachToRecyclerView(carUiRecyclerView);
        int[] outDist = carUiSnapHelper.calculateScrollDistance(8000, -8000);

        int lastChildPosition = carUiSnapHelper.isAtEnd(layoutManager)
                ? 0 : layoutManager.getChildCount() - 1;
        View lastChild = Objects.requireNonNull(layoutManager.getChildAt(lastChildPosition));
        float percentageVisible = CarUiSnapHelper
                .getPercentageVisible(lastChild, orientationHelper);

        int maxDistance = layoutManager.getHeight();
        if (percentageVisible > 0.f) {
            // The max and min distance is the total height of the RecyclerView minus the height of
            // the last child. This ensures that each scroll will never scroll more than a single
            // page on the RecyclerView. That is, the max scroll will make the last child the
            // first child and vice versa when scrolling the opposite way.
            maxDistance -= layoutManager.getDecoratedMeasuredHeight(lastChild);
        }
        int minDistance = -maxDistance;

        assertEquals(clamp(baseOutDist[0], minDistance, maxDistance), outDist[0]);
        assertEquals(clamp(baseOutDist[1], minDistance, maxDistance), outDist[1]);
    }

    @Test
    public void testContinuousScrollListenerConstructor_negativeInitialDelay() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);
        doReturn(-1).when(mTestableResources)
            .getInteger(R.integer.car_ui_scrollbar_longpress_initial_delay);

        CarUiRecyclerView carUiRecyclerView = CarUiRecyclerView.create(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(50);

        container.post(() -> carUiRecyclerView.setAdapter(adapter));
        container.post(() -> assertThrows("negative intervals are not allowed",
                IllegalArgumentException.class, () -> container.addView(carUiRecyclerView)));
    }

    @Test
    public void testContinuousScrollListenerConstructor_negativeRepeatInterval() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);
        doReturn(-1).when(mTestableResources)
                .getInteger(R.integer.car_ui_scrollbar_longpress_repeat_interval);

        CarUiRecyclerView carUiRecyclerView = CarUiRecyclerView.create(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(50);

        container.post(() -> carUiRecyclerView.setAdapter(adapter));
        container.post(() -> assertThrows("negative intervals are not allowed",
                IllegalArgumentException.class, () -> container.addView(carUiRecyclerView)));
    }

    @Test
    public void testUnknownClass_createScrollBarFromConfig() {
        doReturn("random.class").when(mTestableResources)
                .getString(R.string.car_ui_scrollbar_component);

        CarUiRecyclerView carUiRecyclerView = CarUiRecyclerView.create(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(50);

        container.post(() -> carUiRecyclerView.setAdapter(adapter));
        container.post(() -> assertThrows("Error loading scroll bar component: random.class",
                RuntimeException.class, () -> container.addView(carUiRecyclerView)));
    }

    @Test
    public void testWrongType_createScrollBarFromConfig() {
        // Basically return any class that exists but doesn't extend ScrollBar
        doReturn(CarUiRecyclerViewImpl.class.getName()).when(mTestableResources)
            .getString(R.string.car_ui_scrollbar_component);

        CarUiRecyclerView carUiRecyclerView = CarUiRecyclerView.create(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(50);

        container.post(() -> carUiRecyclerView.setAdapter(adapter));
        container.post(() -> assertThrows("Error loading scroll bar component: "
                + CarUiRecyclerViewImpl.class.getName(),
                RuntimeException.class, () -> container.addView(carUiRecyclerView)));
    }

    @Test
    public void testSetLinearLayoutStyle_setsLayoutManager() {
        CarUiLayoutStyle layoutStyle = new CarUiLinearLayoutStyle();
        CarUiRecyclerView carUiRecyclerView = CarUiRecyclerView.create(mTestableContext);
        carUiRecyclerView.setLayoutStyle(layoutStyle);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(50);

        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setAdapter(adapter);
        });

        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        assertTrue(carUiRecyclerView.getLayoutManager() instanceof LinearLayoutManager);
        assertEquals(((LinearLayoutManager) carUiRecyclerView.getLayoutManager()).getOrientation(),
                layoutStyle.getOrientation());
        assertEquals(((LinearLayoutManager) carUiRecyclerView.getLayoutManager())
                .getReverseLayout(), layoutStyle.getReverseLayout());
    }

    @Test
    public void testSetGridLayoutStyle_setsLayoutManager() {
        CarUiLayoutStyle layoutStyle = new CarUiGridLayoutStyle();
        CarUiRecyclerView carUiRecyclerView = CarUiRecyclerView.create(mTestableContext);
        carUiRecyclerView.setLayoutStyle(layoutStyle);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(50);

        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setAdapter(adapter);
        });

        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        assertTrue(carUiRecyclerView.getLayoutManager() instanceof GridLayoutManager);
        assertEquals(((GridLayoutManager) carUiRecyclerView.getLayoutManager()).getOrientation(),
                layoutStyle.getOrientation());
        assertEquals(((GridLayoutManager) carUiRecyclerView.getLayoutManager()).getReverseLayout(),
                layoutStyle.getReverseLayout());
        assertEquals(((GridLayoutManager) carUiRecyclerView.getLayoutManager()).getSpanCount(),
                layoutStyle.getSpanCount());
    }

    @Test
    public void testNegativeSpanCount_setSpanCount() {
        CarUiLayoutStyle layoutStyle = new CarUiGridLayoutStyle();
        assertThrows(AssertionError.class, () -> ((CarUiGridLayoutStyle) layoutStyle)
                .setSpanCount((-1)));
    }

    @Test
    public void testSetGridLayoutStyle_setsLayoutManagerSpanSizeLookup() {
        CarUiGridLayoutStyle layoutStyle = new CarUiGridLayoutStyle();
        // has to bigger than span sizes for all the rows
        layoutStyle.setSpanCount(20);
        SpanSizeLookup spanSizeLookup = new SpanSizeLookup() {
            @Override
            public int getSpanSize(int position) {
                switch(position) {
                    case 0:
                        return 10;
                    case 1:
                        return 20;
                    default:
                        return 15;
                }
            }
        };
        layoutStyle.setSpanSizeLookup(spanSizeLookup);
        CarUiRecyclerView carUiRecyclerView = CarUiRecyclerView.create(mTestableContext);
        carUiRecyclerView.setLayoutStyle(layoutStyle);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(50);

        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setAdapter(adapter);
        });

        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        assertTrue(carUiRecyclerView.getLayoutManager() instanceof GridLayoutManager);
        assertEquals(((GridLayoutManager) carUiRecyclerView.getLayoutManager()).getSpanSizeLookup()
                .getSpanSize(0), spanSizeLookup.getSpanSize(0));
        assertEquals(((GridLayoutManager) carUiRecyclerView.getLayoutManager()).getSpanSizeLookup()
                .getSpanSize(1), spanSizeLookup.getSpanSize(1));
        assertEquals(((GridLayoutManager) carUiRecyclerView.getLayoutManager()).getSpanSizeLookup()
                .getSpanSize(2), spanSizeLookup.getSpanSize(2));
    }

    @Test
    public void testCarUiGridLayoutStyle_LayoutManagerFrom() {
        GridLayoutManager layoutManager =
                new GridLayoutManager(mTestableContext, 20, RecyclerView.VERTICAL, true);
        layoutManager.setSpanSizeLookup(new SpanSizeLookup() {
            @Override
            public int getSpanSize(int position) {
                switch(position) {
                    case 0:
                        return 10;
                    case 1:
                        return 20;
                    default:
                        return 15;
                }
            }
        });
        CarUiGridLayoutStyle layoutStyle = CarUiGridLayoutStyle.from(layoutManager);
        CarUiRecyclerView carUiRecyclerView = CarUiRecyclerView.create(mTestableContext);
        carUiRecyclerView.setLayoutStyle(layoutStyle);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(50);

        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setAdapter(adapter);
        });

        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        assertTrue(carUiRecyclerView.getLayoutManager() instanceof GridLayoutManager);
        assertEquals(((GridLayoutManager) carUiRecyclerView.getLayoutManager()).getOrientation(),
                layoutManager.getOrientation());
        assertEquals(((GridLayoutManager) carUiRecyclerView.getLayoutManager()).getReverseLayout(),
                layoutManager.getReverseLayout());
        assertEquals(((GridLayoutManager) carUiRecyclerView.getLayoutManager()).getSpanCount(),
                layoutManager.getSpanCount());
        assertEquals(((GridLayoutManager) carUiRecyclerView.getLayoutManager()).getSpanSizeLookup()
                .getSpanSize(0), layoutManager.getSpanSizeLookup().getSpanSize(0));
        assertEquals(((GridLayoutManager) carUiRecyclerView.getLayoutManager()).getSpanSizeLookup()
                .getSpanSize(1), layoutManager.getSpanSizeLookup().getSpanSize(1));
        assertEquals(((GridLayoutManager) carUiRecyclerView.getLayoutManager()).getSpanSizeLookup()
                .getSpanSize(2), layoutManager.getSpanSizeLookup().getSpanSize(2));
    }

    @Test
    public void testCarUiGridLayoutStyle_fromLinearLayout_throwsException() {
        LinearLayoutManager layoutManager = new LinearLayoutManager(mTestableContext);
        assertThrows(AssertionError.class, () -> CarUiGridLayoutStyle.from(layoutManager));
    }

    @Test
    public void testCarUiGridLayoutStyle_fromNull() {
        assertNull(CarUiGridLayoutStyle.from(null));
    }

    @Test
    public void testCarUiLinearLayoutStyle_LayoutManagerFrom() {
        LinearLayoutManager layoutManager =
                new LinearLayoutManager(mTestableContext, RecyclerView.VERTICAL, true);
        CarUiLinearLayoutStyle layoutStyle = CarUiLinearLayoutStyle.from(layoutManager);
        CarUiRecyclerView carUiRecyclerView = CarUiRecyclerView.create(mTestableContext);
        carUiRecyclerView.setLayoutStyle(layoutStyle);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(50);

        container.post(() -> {
            container.addView(carUiRecyclerView);
            carUiRecyclerView.setAdapter(adapter);
        });

        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        assertTrue(carUiRecyclerView.getLayoutManager() instanceof LinearLayoutManager);
        assertEquals(((LinearLayoutManager) carUiRecyclerView.getLayoutManager()).getOrientation(),
                layoutManager.getOrientation());
        assertEquals(((LinearLayoutManager) carUiRecyclerView.getLayoutManager())
                .getReverseLayout(), layoutManager.getReverseLayout());
    }

    @Test
    public void testCarUiLinearLayoutStyle_fromGridLayout_throwsException() {
        NotLinearLayoutManager layoutManager = new NotLinearLayoutManager(mTestableContext);
        assertThrows(AssertionError.class, () -> CarUiLinearLayoutStyle.from(layoutManager));
    }

    @Test
    public void testCarUiLinearLayoutStyle_fromNull() {
        assertNull(CarUiGridLayoutStyle.from(null));
    }

    @Test
    public void testOnContinuousScrollListener_cancelCallback() {
        doReturn(0).when(mTestableResources)
                .getInteger(R.integer.car_ui_scrollbar_longpress_initial_delay);
        View view = mock(View.class);
        OnClickListener clickListener = mock(OnClickListener.class);
        OnContinuousScrollListener listener = new OnContinuousScrollListener(
                mTestableContext, clickListener);
        MotionEvent motionEvent = mock(MotionEvent.class);
        when(motionEvent.getAction()).thenReturn(MotionEvent.ACTION_DOWN);
        listener.onTouch(view, motionEvent);
        when(view.isEnabled()).thenReturn(false);
        when(motionEvent.getAction()).thenReturn(MotionEvent.ACTION_UP);
        listener.onTouch(view, motionEvent);
        verify(clickListener, times(1)).onClick(view);
    }

    @Test
    public void testUxRestriction_withLimitedContent_setsMaxItems() {
        CarUxRestrictionsUtil uxRestriction = CarUxRestrictionsUtil.getInstance(mTestableContext);
        CarUxRestrictions restriction = mock(CarUxRestrictions.class);
        when(restriction.getActiveRestrictions()).thenReturn(UX_RESTRICTIONS_LIMIT_CONTENT);
        when(restriction.getMaxCumulativeContentItems()).thenReturn(10);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter.WithItemCap adapter = spy(new TestAdapter.WithItemCap(100));
        container.post(() -> {
            uxRestriction.setUxRestrictions(restriction);
            carUiRecyclerView.setAdapter(adapter);
            container.addView(carUiRecyclerView);
        });

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        verify(adapter, atLeastOnce()).setMaxItems(10);
    }

    @Test
    public void testUxRestriction_withoutLimitedContent_setsUnlimitedMaxItems() {
        CarUxRestrictionsUtil uxRestriction = CarUxRestrictionsUtil.getInstance(mTestableContext);
        CarUxRestrictions restriction = mock(CarUxRestrictions.class);
        when(restriction.getMaxCumulativeContentItems()).thenReturn(10);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);
        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter.WithItemCap adapter = spy(new TestAdapter.WithItemCap(100));
        container.post(() -> {
            uxRestriction.setUxRestrictions(restriction);
            carUiRecyclerView.setAdapter(adapter);
            container.addView(carUiRecyclerView);
        });

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        verify(adapter, atLeastOnce()).setMaxItems(UNLIMITED);
    }

    @Test
    public void testPageUp_returnsWhen_verticalScrollOffsetIsZero() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);
        doReturn(TestScrollBar.class.getName()).when(mTestableResources)
            .getString(R.string.car_ui_scrollbar_component);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);

        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(100);
        container.post(() -> {
            carUiRecyclerView.setAdapter(adapter);
            container.addView(carUiRecyclerView);
        });

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        // Scroll to a position so page up is enabled.
        container.post(() -> carUiRecyclerView.scrollToPosition(20));

        onView(withId(R.id.car_ui_scrollbar_page_up)).check(matches(isEnabled()));

        View v = mActivity.findViewById(R.id.car_ui_scroll_bar);
        TestScrollBar sb = (TestScrollBar) v.getTag();
        // We set this to simulate a case where layout manager is null
        sb.mReturnZeroVerticalScrollOffset = true;

        onView(withId(R.id.car_ui_scrollbar_page_up)).perform(click());

        assertFalse(sb.mScrollWasCalled);
    }

    @Test
    public void testPageUp_returnsWhen_layoutManagerIsNull() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);
        doReturn(TestScrollBar.class.getName()).when(mTestableResources)
                .getString(R.string.car_ui_scrollbar_component);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);

        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(100);
        container.post(() -> {
            carUiRecyclerView.setAdapter(adapter);
            container.addView(carUiRecyclerView);
        });

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        // Scroll to a position so page up is enabled.
        container.post(() -> carUiRecyclerView.scrollToPosition(20));

        onView(withId(R.id.car_ui_scrollbar_page_up)).check(matches(isEnabled()));

        View v = mActivity.findViewById(R.id.car_ui_scroll_bar);
        TestScrollBar sb = (TestScrollBar) v.getTag();
        // We set this to simulate a case where layout manager is null
        sb.mReturnMockLayoutManager = true;

        onView(withId(R.id.car_ui_scrollbar_page_up)).perform(click());

        assertFalse(sb.mScrollWasCalled);
    }

    @Test
    public void testPageDown_returnsWhen_layoutManagerIsNullOrEmpty() {
        doReturn(true).when(mTestableResources).getBoolean(R.bool.car_ui_scrollbar_enable);
        doReturn(TestScrollBar.class.getName()).when(mTestableResources)
            .getString(R.string.car_ui_scrollbar_component);

        CarUiRecyclerView carUiRecyclerView = new CarUiRecyclerViewImpl(mTestableContext);

        ViewGroup container = mActivity.findViewById(R.id.test_container);
        TestAdapter adapter = new TestAdapter(100);
        container.post(() -> {
            carUiRecyclerView.setAdapter(adapter);
            container.addView(carUiRecyclerView);
        });

        IdlingRegistry.getInstance().register(new ScrollIdlingResource(carUiRecyclerView));
        onView(withText(adapter.getItemText(0))).check(matches(isDisplayed()));

        // Scroll to a position so page up is enabled.
        container.post(() -> carUiRecyclerView.scrollToPosition(20));

        onView(withId(R.id.car_ui_scrollbar_page_down)).check(matches(isEnabled()));

        View v = mActivity.findViewById(R.id.car_ui_scroll_bar);
        TestScrollBar sb = (TestScrollBar) v.getTag();
        // We set this to simulate a case where layout manager is empty
        sb.mReturnMockLayoutManager = true;
        sb.mMockLayoutManager = spy(carUiRecyclerView.getLayoutManager());
        when(sb.mMockLayoutManager.getChildCount()).thenReturn(0);

        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());

        assertFalse(sb.mScrollWasCalled);

        // We set this to simulate a case where layout manager is null
        sb.mReturnMockLayoutManager = true;
        sb.mMockLayoutManager = null;

        onView(withId(R.id.car_ui_scrollbar_page_down)).perform(click());

        assertFalse(sb.mScrollWasCalled);
    }

    static class TestScrollBar extends DefaultScrollBar {

        boolean mReturnMockLayoutManager = false;
        LayoutManager mMockLayoutManager = null;
        boolean mScrollWasCalled = false;
        boolean mReturnZeroVerticalScrollOffset = false;

        @Override
        public void initialize(RecyclerView rv, View scrollView) {
            super.initialize(rv, scrollView);

            scrollView.setTag(this);
        }

        @Override
        public LayoutManager getLayoutManager() {
            return mReturnMockLayoutManager ? mMockLayoutManager : super.getLayoutManager();
        }

        @Override
        void smoothScrollBy(int dx, int dy) {
            mScrollWasCalled = true;
        }

        @Override
        void smoothScrollToPosition(int max) {
            mScrollWasCalled = true;
        }

        @Override
        int computeVerticalScrollOffset() {
            return mReturnZeroVerticalScrollOffset ? 0 : super.computeVerticalScrollOffset();
        }
    }

    private static float dpToPixel(Context context, int dp) {
        return TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                dp,
                context.getResources().getDisplayMetrics());
    }

    /**
     * Returns an item in the current list view whose height is taller than that of the
     * CarUiRecyclerView. If that item exists, then it is returned; otherwise an {@link
     * IllegalStateException} is thrown.
     *
     * @return An item that is taller than the CarUiRecyclerView.
     */
    private View getLongItem(CarUiRecyclerView recyclerView) {
        OrientationHelper orientationHelper =
                OrientationHelper.createVerticalHelper(recyclerView.getLayoutManager());
        for (int i = 0; i < recyclerView.getLayoutManager().getChildCount(); i++) {
            View item = recyclerView.getLayoutManager().getChildAt(i);

            if (item.getHeight() > orientationHelper.getTotalSpace()) {
                return item;
            }
        }

        throw new IllegalStateException(
                "No item found that is longer than the height of the CarUiRecyclerView.");
    }

    /**
     * A test adapter that handles inflating test views and binding data to it.
     */
    private static class TestAdapter extends RecyclerView.Adapter<TestViewHolder> {

        public enum ItemHeight {
            STANDARD,
            TALL,
            EXTRA_TALL,
            ZERO
        }

        protected final List<String> mData;
        private final Map<Integer, ItemHeight> mHeightOverrides;

        TestAdapter(int itemCount, Map<Integer, ItemHeight> overrides) {
            mHeightOverrides = overrides;
            mData = new ArrayList<>(itemCount);

            for (int i = 0; i < itemCount; i++) {
                mData.add(getItemText(i));
            }
        }

        TestAdapter(int itemCount) {
            this(itemCount, new HashMap<>());
        }

        String getItemText(int position) {
            if (position > mData.size()) {
                return null;
            }

            return String.format("Sample item #%d", position);
        }

        @NonNull
        @Override
        public TestViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            LayoutInflater inflater = LayoutInflater.from(parent.getContext());
            return new TestViewHolder(inflater, parent);
        }

        @Override
        public void onBindViewHolder(@NonNull TestViewHolder holder, int position) {
            ItemHeight height = ItemHeight.STANDARD;

            if (mHeightOverrides.containsKey(position)) {
                height = mHeightOverrides.get(position);
            }

            int screenHeight = Resources.getSystem().getDisplayMetrics().heightPixels;

            switch (height) {
                case ZERO:
                    ViewGroup.LayoutParams lp = holder.itemView.getLayoutParams();
                    holder.itemView.setContentDescription("ZERO");
                    lp.height = 0;
                    holder.itemView.setLayoutParams(lp);
                    break;
                case STANDARD:
                    break;
                case TALL:
                    holder.itemView.setMinimumHeight(screenHeight);
                    break;
                case EXTRA_TALL:
                    holder.itemView.setMinimumHeight(screenHeight * 2);
                    break;
                default:
                    throw new IllegalStateException("Unexpected value: " + height);
            }

            holder.bind(mData.get(position));
        }

        @Override
        public int getItemCount() {
            return mData.size();
        }

        static class WithItemCap extends TestAdapter implements CarUiRecyclerView.ItemCap {

            private int mMaxitems = -1;

            WithItemCap(int itemCount,
                    Map<Integer, ItemHeight> overrides) {
                super(itemCount, overrides);
            }

            WithItemCap(int itemCount) {
                super(itemCount);
            }

            @Override
            public void setMaxItems(int maxItems) {
                mMaxitems = maxItems;
            }

            @Override
            public int getItemCount() {
                return mMaxitems >= 0 ? mMaxitems : mData.size();
            }
        }
    }

    private static class PerfectFitTestAdapter extends RecyclerView.Adapter<TestViewHolder> {

        private static final int MIN_HEIGHT = 30;
        private final List<String> mData;
        private final int mItemHeight;

        private int getMinHeightPerItemToFitScreen(int screenHeight) {
            // When the height is a prime number, there can only be 1 item per page
            int minHeight = screenHeight;
            for (int i = screenHeight; i >= 1; i--) {
                if (screenHeight % i == 0 && screenHeight / i >= MIN_HEIGHT) {
                    minHeight = screenHeight / i;
                    break;
                }
            }
            return minHeight;
        }

        PerfectFitTestAdapter(int numOfPages, int recyclerViewHeight) {
            mItemHeight = getMinHeightPerItemToFitScreen(recyclerViewHeight);
            int itemsPerPage = recyclerViewHeight / mItemHeight;
            int itemCount = itemsPerPage * numOfPages;
            mData = new ArrayList<>(itemCount);
            for (int i = 0; i < itemCount; i++) {
                mData.add(getItemText(i));
            }
        }

        String getItemText(int position) {
            return String.format("Sample item #%d", position);
        }

        @NonNull
        @Override
        public TestViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            LayoutInflater inflater = LayoutInflater.from(parent.getContext());
            return new TestViewHolder(inflater, parent);
        }

        @Override
        public void onBindViewHolder(@NonNull TestViewHolder holder, int position) {
            holder.itemView.setMinimumHeight(mItemHeight);
            holder.bind(mData.get(position));
        }

        @Override
        public int getItemCount() {
            return mData.size();
        }
    }

    private static class TestViewHolder extends RecyclerView.ViewHolder {
        private TextView mTextView;

        TestViewHolder(LayoutInflater inflater, ViewGroup parent) {
            super(inflater.inflate(R.layout.test_list_item, parent, false));
            mTextView = itemView.findViewById(R.id.text);
        }

        void bind(String text) {
            mTextView.setText(text);
        }
    }

    /**
     * An {@link IdlingResource} that will prevent assertions from running while the {@link
     * CarUiRecyclerView} is scrolling.
     */
    private static class ScrollIdlingResource implements IdlingResource {
        private boolean mIdle = true;
        private ResourceCallback mResourceCallback;

        ScrollIdlingResource(CarUiRecyclerView carUiRecyclerView) {
            carUiRecyclerView
                    .addOnScrollListener(
                            new RecyclerView.OnScrollListener() {
                                @Override
                                public void onScrollStateChanged(@NonNull RecyclerView recyclerView,
                                        int newState) {
                                    super.onScrollStateChanged(recyclerView, newState);
                                    mIdle = (newState == SCROLL_STATE_IDLE
                                            // Treat dragging as idle, or Espresso will
                                            // block itself when swiping.
                                            || newState == SCROLL_STATE_DRAGGING);
                                    if (mIdle && mResourceCallback != null) {
                                        mResourceCallback.onTransitionToIdle();
                                    }
                                }

                                @Override
                                public void onScrolled(@NonNull RecyclerView recyclerView, int dx,
                                        int dy) {
                                }
                            });
        }

        @Override
        public String getName() {
            return ScrollIdlingResource.class.getName();
        }

        @Override
        public boolean isIdleNow() {
            return mIdle;
        }

        @Override
        public void registerIdleTransitionCallback(ResourceCallback callback) {
            mResourceCallback = callback;
        }
    }

    private static class NotLinearLayoutManager extends LayoutManager {

        NotLinearLayoutManager(Context mTestableContext) {}

        @Override
        public LayoutParams generateDefaultLayoutParams() {
            return null;
        }
    }
}
