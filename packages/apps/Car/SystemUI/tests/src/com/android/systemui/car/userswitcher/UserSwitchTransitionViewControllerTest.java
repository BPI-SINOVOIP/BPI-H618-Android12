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

package com.android.systemui.car.userswitcher;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;

import android.content.Context;
import android.os.UserManager;
import android.test.suitebuilder.annotation.SmallTest;
import android.testing.AndroidTestingRunner;
import android.testing.TestableLooper;
import android.testing.TestableResources;
import android.view.IWindowManager;
import android.view.LayoutInflater;
import android.view.ViewGroup;

import com.android.systemui.R;
import com.android.systemui.SysuiTestCase;
import com.android.systemui.car.CarSystemUiTest;
import com.android.systemui.car.window.OverlayViewGlobalStateController;
import com.android.systemui.util.concurrency.FakeExecutor;
import com.android.systemui.util.time.FakeSystemClock;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@CarSystemUiTest
@RunWith(AndroidTestingRunner.class)
@TestableLooper.RunWithLooper
@SmallTest
public class UserSwitchTransitionViewControllerTest extends SysuiTestCase {
    private static final int TEST_USER_1 = 100;
    private static final int TEST_USER_2 = 110;

    private UserSwitchTransitionViewController mCarUserSwitchingDialogController;
    private TestableResources mTestableResources;
    private FakeExecutor mExecutor;
    @Mock
    private OverlayViewGlobalStateController mOverlayViewGlobalStateController;
    @Mock
    private IWindowManager mWindowManagerService;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mTestableResources = mContext.getOrCreateTestableResources();
        mExecutor = new FakeExecutor(new FakeSystemClock());
        mCarUserSwitchingDialogController = new UserSwitchTransitionViewController(
                mContext,
                mTestableResources.getResources(),
                mExecutor,
                (UserManager) mContext.getSystemService(Context.USER_SERVICE),
                mWindowManagerService,
                mOverlayViewGlobalStateController
        );

        mCarUserSwitchingDialogController.inflate((ViewGroup) LayoutInflater.from(mContext).inflate(
                R.layout.sysui_overlay_window, /* root= */ null));
    }

    @Test
    public void onHandleShow_newUserSelected_showsDialog() {
        mCarUserSwitchingDialogController.handleShow(/* currentUserId= */ TEST_USER_1);
        mExecutor.advanceClockToLast();
        mExecutor.runAllReady();

        verify(mOverlayViewGlobalStateController).showView(eq(mCarUserSwitchingDialogController),
                any());
    }

    @Test
    public void onHandleShow_alreadyShowing_ignoresRequest() {
        mCarUserSwitchingDialogController.handleShow(/* currentUserId= */ TEST_USER_1);
        mExecutor.advanceClockToLast();
        mExecutor.runAllReady();
        mCarUserSwitchingDialogController.handleShow(/* currentUserId= */ TEST_USER_2);
        mExecutor.advanceClockToLast();
        mExecutor.runAllReady();

        // Verify that the request was processed only once.
        verify(mOverlayViewGlobalStateController).showView(eq(mCarUserSwitchingDialogController),
                any());
    }

    @Test
    public void onHandleShow_sameUserSelected_ignoresRequest() {
        mCarUserSwitchingDialogController.handleShow(/* currentUserId= */ TEST_USER_1);
        mExecutor.advanceClockToLast();
        mExecutor.runAllReady();
        mCarUserSwitchingDialogController.handleHide();
        mExecutor.advanceClockToLast();
        mExecutor.runAllReady();
        mCarUserSwitchingDialogController.handleShow(/* currentUserId= */ TEST_USER_1);
        mExecutor.advanceClockToLast();
        mExecutor.runAllReady();

        // Verify that the request was processed only once.
        verify(mOverlayViewGlobalStateController).showView(eq(mCarUserSwitchingDialogController),
                any());
    }

    @Test
    public void onHide_currentlyShowing_hidesDialog() {
        mCarUserSwitchingDialogController.handleShow(/* currentUserId= */ TEST_USER_1);
        mExecutor.advanceClockToLast();
        mExecutor.runAllReady();
        mCarUserSwitchingDialogController.handleHide();
        mExecutor.advanceClockToLast();
        mExecutor.runAllReady();

        verify(mOverlayViewGlobalStateController).hideView(eq(mCarUserSwitchingDialogController),
                any());
    }

    @Test
    public void onHide_notShowing_ignoresRequest() {
        mCarUserSwitchingDialogController.handleShow(/* currentUserId= */ TEST_USER_1);
        mExecutor.advanceClockToLast();
        mExecutor.runAllReady();
        mCarUserSwitchingDialogController.handleHide();
        mExecutor.advanceClockToLast();
        mExecutor.runAllReady();
        mCarUserSwitchingDialogController.handleHide();
        mExecutor.advanceClockToLast();
        mExecutor.runAllReady();

        // Verify that the request was processed only once.
        verify(mOverlayViewGlobalStateController).hideView(eq(mCarUserSwitchingDialogController),
                any());
    }

    @Test
    public void onWindowShownTimeoutPassed_viewNotHidden_hidesUserSwitchTransitionView() {
        mCarUserSwitchingDialogController.handleShow(/* currentUserId= */ TEST_USER_1);
        mExecutor.advanceClockToLast();
        mExecutor.runAllReady();
        reset(mOverlayViewGlobalStateController);

        getContext().getMainThreadHandler().postDelayed(() -> {
            verify(mOverlayViewGlobalStateController).hideView(
                    eq(mCarUserSwitchingDialogController), any());
        }, mCarUserSwitchingDialogController.getWindowShownTimeoutMs() + 10);
    }

    @Test
    public void onWindowShownTimeoutPassed_viewHidden_doesNotHideUserSwitchTransitionViewAgain() {
        mCarUserSwitchingDialogController.handleShow(/* currentUserId= */ TEST_USER_1);
        mExecutor.advanceClockToLast();
        mExecutor.runAllReady();
        mCarUserSwitchingDialogController.handleHide();
        mExecutor.advanceClockToLast();
        mExecutor.runAllReady();
        reset(mOverlayViewGlobalStateController);

        getContext().getMainThreadHandler().postDelayed(() -> {
            verify(mOverlayViewGlobalStateController, never()).hideView(
                    eq(mCarUserSwitchingDialogController), any());
        }, mCarUserSwitchingDialogController.getWindowShownTimeoutMs() + 10);
    }
}
