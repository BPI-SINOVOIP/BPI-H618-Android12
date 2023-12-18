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

package com.android.systemui.car.systembar;

import static android.hardware.SensorPrivacyManager.Sensors.MICROPHONE;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.car.Car;
import android.car.user.CarUserManager;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.hardware.SensorPrivacyManager;
import android.testing.AndroidTestingRunner;
import android.testing.TestableLooper;
import android.view.View;
import android.widget.FrameLayout;

import androidx.test.filters.SmallTest;

import com.android.systemui.R;
import com.android.systemui.SysuiTestCase;
import com.android.systemui.broadcast.BroadcastDispatcher;
import com.android.systemui.car.CarDeviceProvisionedController;
import com.android.systemui.car.CarServiceProvider;
import com.android.systemui.car.CarSystemUiTest;
import com.android.systemui.car.privacy.MicPrivacyChip;
import com.android.systemui.car.privacy.MicPrivacyChipDialogController;
import com.android.systemui.privacy.PrivacyItem;
import com.android.systemui.privacy.PrivacyItemController;
import com.android.systemui.privacy.PrivacyType;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.Collections;
import java.util.concurrent.Executor;

@CarSystemUiTest
@RunWith(AndroidTestingRunner.class)
@TestableLooper.RunWithLooper
@SmallTest
public class PrivacyChipViewControllerTest extends SysuiTestCase {

    private PrivacyChipViewController mPrivacyChipViewController;
    private FrameLayout mFrameLayout;
    private MicPrivacyChip mMicPrivacyChip;

    @Captor
    private ArgumentCaptor<Intent> mIntentArgumentCaptor;
    @Captor
    private ArgumentCaptor<Runnable> mRunnableArgumentCaptor;
    @Captor
    private ArgumentCaptor<PrivacyItemController.Callback> mPicCallbackArgumentCaptor;
    @Captor
    private ArgumentCaptor<SensorPrivacyManager.OnSensorPrivacyChangedListener>
            mOnSensorPrivacyChangedListenerArgumentCaptor;
    @Captor
    private ArgumentCaptor<BroadcastReceiver> mBroadcastReceiverArgumentCaptor;
    @Captor
    private ArgumentCaptor<CarUserManager.UserLifecycleListener>
            mUserLifecycleListenerArgumentCaptor;

    @Mock
    private PrivacyItemController mPrivacyItemController;
    @Mock
    private PrivacyItem mPrivacyItem;
    @Mock
    private Executor mExecutor;
    @Mock
    private BroadcastDispatcher mBroadcastDispatcher;
    @Mock
    private SensorPrivacyManager mSensorPrivacyManager;
    @Mock
    private CarDeviceProvisionedController mCarDeviceProvisionedController;
    @Mock
    private CarUserManager mCarUserManager;
    @Mock
    private Car mCar;
    @Mock
    private MicPrivacyChipDialogController mMicPrivacyChipDialogController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(/* testClass= */ this);

        mFrameLayout = new FrameLayout(mContext);
        mMicPrivacyChip = spy(new MicPrivacyChip(mContext));
        mContext = spy(mContext);

        when(mContext.getMainExecutor()).thenReturn(mExecutor);
        when(mCar.isConnected()).thenReturn(true);
        when(mCar.getCarManager(Car.CAR_USER_SERVICE)).thenReturn(mCarUserManager);

        CarServiceProvider carServiceProvider = new CarServiceProvider(mContext, mCar);

        mPrivacyChipViewController =
                new PrivacyChipViewController(mContext, mPrivacyItemController, carServiceProvider,
                        mBroadcastDispatcher, mSensorPrivacyManager,
                        mCarDeviceProvisionedController, mMicPrivacyChipDialogController);
        mMicPrivacyChip.setId(R.id.privacy_chip);
        mFrameLayout.addView(mMicPrivacyChip);
        when(mCarDeviceProvisionedController.getCurrentUser()).thenReturn(0);
    }

    @Test
    public void addPrivacyChipView_privacyChipViewPresent_addCallbackCalled() {
        mPrivacyChipViewController.addPrivacyChipView(mFrameLayout);

        verify(mPrivacyItemController).addCallback(any());
    }

    @Test
    public void addPrivacyChipView_privacyChipViewPresent_micStatusSet() {
        when(mSensorPrivacyManager.isSensorPrivacyEnabled(MICROPHONE, /* userId= */ 0))
                .thenReturn(false);
        mPrivacyChipViewController.addPrivacyChipView(mFrameLayout);
        verify(mExecutor).execute(mRunnableArgumentCaptor.capture());

        mRunnableArgumentCaptor.getValue().run();

        verify(mMicPrivacyChip).setMicrophoneEnabled(eq(true));
    }

    @Test
    public void addPrivacyChipView_privacyChipViewNotPresent_addCallbackNotCalled() {
        mPrivacyChipViewController.addPrivacyChipView(new View(getContext()));

        verify(mPrivacyItemController, never()).addCallback(any());
    }

    @Test
    public void onClick_dialogShowCalled() {
        mPrivacyChipViewController.onClick(/* view= */ null);

        verify(mMicPrivacyChipDialogController).show();
    }

    @Test
    public void onPrivacyItemsChanged_micIsPartOfPrivacyItems_animateInCalled() {
        when(mPrivacyItem.getPrivacyType()).thenReturn(PrivacyType.TYPE_MICROPHONE);
        mPrivacyChipViewController.addPrivacyChipView(mFrameLayout);
        verify(mPrivacyItemController).addCallback(mPicCallbackArgumentCaptor.capture());
        mPicCallbackArgumentCaptor.getValue().onFlagAllChanged(true);
        mPicCallbackArgumentCaptor.getValue().onFlagMicCameraChanged(true);

        mPicCallbackArgumentCaptor.getValue()
                .onPrivacyItemsChanged(Collections.singletonList(mPrivacyItem));
        verify(mExecutor, times(2)).execute(mRunnableArgumentCaptor.capture());
        mRunnableArgumentCaptor.getAllValues().forEach(Runnable::run);

        verify(mMicPrivacyChip).animateIn();
    }

    @Test
    public void onPrivacyItemsChanged_micIsPartOfPrivacyItemsTwice_animateInCalledOnce() {
        when(mPrivacyItem.getPrivacyType()).thenReturn(PrivacyType.TYPE_MICROPHONE);
        mPrivacyChipViewController.addPrivacyChipView(mFrameLayout);
        verify(mPrivacyItemController).addCallback(mPicCallbackArgumentCaptor.capture());
        mPicCallbackArgumentCaptor.getValue().onFlagAllChanged(true);
        mPicCallbackArgumentCaptor.getValue().onFlagMicCameraChanged(true);

        mPicCallbackArgumentCaptor.getValue()
                .onPrivacyItemsChanged(Collections.singletonList(mPrivacyItem));
        mPicCallbackArgumentCaptor.getValue()
                .onPrivacyItemsChanged(Collections.singletonList(mPrivacyItem));
        verify(mExecutor, times(2)).execute(mRunnableArgumentCaptor.capture());
        mRunnableArgumentCaptor.getAllValues().forEach(Runnable::run);

        verify(mMicPrivacyChip).animateIn();
    }

    @Test
    public void onPrivacyItemsChanged_micIsNotPartOfPrivacyItems_animateOutCalled() {
        when(mPrivacyItem.getPrivacyType()).thenReturn(PrivacyType.TYPE_MICROPHONE);
        mPrivacyChipViewController.addPrivacyChipView(mFrameLayout);
        verify(mPrivacyItemController).addCallback(mPicCallbackArgumentCaptor.capture());
        mPicCallbackArgumentCaptor.getValue().onFlagAllChanged(true);
        mPicCallbackArgumentCaptor.getValue().onFlagMicCameraChanged(true);
        mPicCallbackArgumentCaptor.getValue()
                .onPrivacyItemsChanged(Collections.singletonList(mPrivacyItem));

        mPicCallbackArgumentCaptor.getValue().onPrivacyItemsChanged(Collections.emptyList());
        verify(mExecutor, times(3))
                .execute(mRunnableArgumentCaptor.capture());
        mRunnableArgumentCaptor.getAllValues().forEach(Runnable::run);

        verify(mMicPrivacyChip).animateOut();
    }

    @Test
    public void onPrivacyItemsChanged_micIsNotPartOfPrivacyItemsTwice_animateOutCalledOnce() {
        when(mPrivacyItem.getPrivacyType()).thenReturn(PrivacyType.TYPE_MICROPHONE);
        mPrivacyChipViewController.addPrivacyChipView(mFrameLayout);
        verify(mPrivacyItemController).addCallback(mPicCallbackArgumentCaptor.capture());
        mPicCallbackArgumentCaptor.getValue().onFlagAllChanged(true);
        mPicCallbackArgumentCaptor.getValue().onFlagMicCameraChanged(true);
        mPicCallbackArgumentCaptor.getValue()
                .onPrivacyItemsChanged(Collections.singletonList(mPrivacyItem));

        mPicCallbackArgumentCaptor.getValue().onPrivacyItemsChanged(Collections.emptyList());
        mPicCallbackArgumentCaptor.getValue().onPrivacyItemsChanged(Collections.emptyList());
        verify(mExecutor, times(3))
                .execute(mRunnableArgumentCaptor.capture());
        mRunnableArgumentCaptor.getAllValues().forEach(Runnable::run);

        verify(mMicPrivacyChip).animateOut();
    }

    @Test
    public void onSensorPrivacyChanged_argTrue_setMicrophoneEnabledWithFalseCalled() {
        mPrivacyChipViewController.addPrivacyChipView(mFrameLayout);
        verify(mSensorPrivacyManager).addSensorPrivacyListener(eq(MICROPHONE),
                /* userId= */ eq(0), mOnSensorPrivacyChangedListenerArgumentCaptor.capture());
        reset(mMicPrivacyChip);
        reset(mExecutor);
        mOnSensorPrivacyChangedListenerArgumentCaptor.getValue()
                .onSensorPrivacyChanged(MICROPHONE, /* enabled= */ true);
        verify(mExecutor).execute(mRunnableArgumentCaptor.capture());

        mRunnableArgumentCaptor.getAllValues().forEach(Runnable::run);

        verify(mMicPrivacyChip).setMicrophoneEnabled(eq(false));
    }

    @Test
    public void onSensorPrivacyChanged_argFalse_setMicrophoneEnabledWithTrueCalled() {
        mPrivacyChipViewController.addPrivacyChipView(mFrameLayout);
        verify(mSensorPrivacyManager).addSensorPrivacyListener(eq(MICROPHONE),
                /* userId= */ eq(0), mOnSensorPrivacyChangedListenerArgumentCaptor.capture());
        reset(mMicPrivacyChip);
        reset(mExecutor);
        mOnSensorPrivacyChangedListenerArgumentCaptor.getValue()
                .onSensorPrivacyChanged(MICROPHONE, /* enabled= */ false);
        verify(mExecutor).execute(mRunnableArgumentCaptor.capture());

        mRunnableArgumentCaptor.getAllValues().forEach(Runnable::run);

        verify(mMicPrivacyChip).setMicrophoneEnabled(eq(true));
    }

    @Test
    public void onReceive_setMicrophoneEnabledCalled() {
        when(mSensorPrivacyManager.isSensorPrivacyEnabled(MICROPHONE, /* userId= */ 1))
                .thenReturn(true);
        mPrivacyChipViewController.addPrivacyChipView(mFrameLayout);
        verify(mBroadcastDispatcher).registerReceiver(mBroadcastReceiverArgumentCaptor.capture(),
                any(), any(), any());
        reset(mExecutor);
        when(mCarDeviceProvisionedController.getCurrentUser()).thenReturn(1);
        mBroadcastReceiverArgumentCaptor.getValue().onReceive(mContext,
                new Intent(Intent.ACTION_USER_INFO_CHANGED));
        verify(mExecutor).execute(mRunnableArgumentCaptor.capture());

        mRunnableArgumentCaptor.getValue().run();

        verify(mMicPrivacyChip).setMicrophoneEnabled(eq(false));
    }

    @Test
    public void onEvent_setMicrophoneEnabledCalled() {
        when(mSensorPrivacyManager.isSensorPrivacyEnabled(MICROPHONE, /* userId= */ 1))
                .thenReturn(false);
        mPrivacyChipViewController.addPrivacyChipView(mFrameLayout);
        verify(mCarUserManager).addListener(any(), mUserLifecycleListenerArgumentCaptor.capture());
        CarUserManager.UserLifecycleEvent event = new CarUserManager.UserLifecycleEvent(
                CarUserManager.USER_LIFECYCLE_EVENT_TYPE_SWITCHING, /* from= */ 0,
                /* to= */ 1);
        CarUserManager.UserLifecycleListener userLifecycleListener =
                mUserLifecycleListenerArgumentCaptor.getValue();
        assertThat(userLifecycleListener).isNotNull();
        reset(mExecutor);
        userLifecycleListener.onEvent(event);
        verify(mExecutor).execute(mRunnableArgumentCaptor.capture());

        mRunnableArgumentCaptor.getValue().run();

        verify(mMicPrivacyChip).setMicrophoneEnabled(eq(true));
    }
}
