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

package com.android.car.cluster;

import static android.car.settings.CarSettings.Global.DISABLE_INSTRUMENTATION_SERVICE;

import static com.android.dx.mockito.inline.extended.ExtendedMockito.doAnswer;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.doReturn;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.car.CarAppFocusManager;
import android.car.cluster.renderer.IInstrumentCluster;
import android.car.cluster.renderer.IInstrumentClusterNavigation;
import android.car.navigation.CarNavigationInstrumentCluster;
import android.car.test.mocks.AbstractExtendedMockitoTestCase;
import android.content.ContentResolver;
import android.content.Context;
import android.os.Bundle;
import android.os.Looper;
import android.os.Process;
import android.view.KeyEvent;

import androidx.test.InstrumentationRegistry;

import com.android.car.AppFocusService;
import com.android.car.CarInputService;
import com.android.car.CarLocalServices;
import com.android.car.CarServiceUtils;
import com.android.car.R;
import com.android.car.user.CarUserService;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.stubbing.Answer;

public class InstrumentClusterServiceTest extends AbstractExtendedMockitoTestCase {

    private static final String DEFAULT_RENDERER_SERVICE =
            "com.android.car.carservice_unittest/.FakeService";

    private InstrumentClusterService mService;
    private ClusterNavigationService mNavigationService;

    @Mock
    private Context mContext;
    @Mock
    private AppFocusService mAppFocusService;
    @Mock
    private CarInputService mCarInputService;
    @Mock
    private CarUserService mCarUserService;

    private final IInstrumentClusterNavigationImpl mInstrumentClusterNavigation =
            new IInstrumentClusterNavigationImpl();

    private final IInstrumentCluster mInstrumentClusterRenderer = new IInstrumentCluster.Stub() {

        @Override
        public IInstrumentClusterNavigation getNavigationService() {
            return mInstrumentClusterNavigation;
        }

        @Override
        public void setNavigationContextOwner(int uid, int pid) {
        }

        @Override
        public void onKeyEvent(KeyEvent keyEvent) {
        }
    };

    @Before
    public void setUp() {
        doReturn(DEFAULT_RENDERER_SERVICE).when(mContext).getString(
                R.string.instrumentClusterRendererService);
        doReturn(true).when(mContext).bindServiceAsUser(any(), any(), anyInt(), any());
        ContentResolver cr = InstrumentationRegistry.getTargetContext().getContentResolver();
        doReturn(cr).when(mContext).getContentResolver();
        putSettingsString(DISABLE_INSTRUMENTATION_SERVICE, "false");
        doAnswer((Answer<Void>) invocationOnMock -> {
                    Runnable r = invocationOnMock.getArgument(0);
                    r.run();
                    return null;
                }
        ).when(mCarUserService).runOnUser0Unlock(any());
        CarLocalServices.removeServiceForTest(CarUserService.class);
        CarLocalServices.addService(CarUserService.class, mCarUserService);

        setNewService();
    }

    private void setNewService() {
        // Must prepare Looper (once) otherwise InstrumentClusterService constructor will fail.
        Looper looper = Looper.myLooper();
        if (looper == null) {
            Looper.prepare();
        }
        mNavigationService = new ClusterNavigationService(mContext, mAppFocusService);
        mService = new InstrumentClusterService(mContext, mNavigationService, mCarInputService);
    }

    @After
    public void tearDown() {
        CarLocalServices.removeServiceForTest(CarUserService.class);
    }

    private void initService(boolean connect) {
        mService.init();
        if (connect) {
            notifyRendererServiceConnection();
        }
        // Give nav focus to the test
        mNavigationService.onFocusAcquired(CarAppFocusManager.APP_FOCUS_TYPE_NAVIGATION,
                Process.myUid(), Process.myPid());
    }

    private void notifyRendererServiceConnection() {
        mService.mRendererServiceConnection.onServiceConnected(null,
                mInstrumentClusterRenderer.asBinder());
    }

    @Test
    public void testNonNullManager() throws Exception {
        initService(/* connect= */ true);
        checkValidClusterNavigation();
        verify(mContext).bindServiceAsUser(any(), any(), anyInt(), any());
    }

    @Test
    public void testDelayedConnection() throws Exception {
        initService(/* connect= */ false);
        CarServiceUtils.runOnMain(() -> {
            // need to delay notification
            try {
                Thread.sleep(50);
            } catch (InterruptedException e) {
            }
            notifyRendererServiceConnection();
        });
        checkValidClusterNavigation();
        verify(mContext).bindServiceAsUser(any(), any(), anyInt(), any());
    }

    @Test
    public void testNoConfig() throws Exception {
        doReturn("").when(mContext).getString(R.string.instrumentClusterRendererService);
        setNewService();
        initService(/* connect= */ false);
        verify(mContext, times(0)).bindServiceAsUser(any(), any(), anyInt(), any());
    }

    private void checkValidClusterNavigation() throws Exception {
        assertThat(mNavigationService.getInstrumentClusterInfo()).isEqualTo(
                mInstrumentClusterNavigation.mClusterInfo);
        Bundle bundle = new Bundle();
        mNavigationService.onNavigationStateChanged(bundle);
        assertThat(bundle).isEqualTo(mInstrumentClusterNavigation.mLastBundle);
    }

    private static class IInstrumentClusterNavigationImpl
            extends IInstrumentClusterNavigation.Stub {

        private final CarNavigationInstrumentCluster mClusterInfo =
                CarNavigationInstrumentCluster.createCustomImageCluster(/*minIntervalMs= */ 100,
                        /* imageWidth= */ 800, /* imageHeight= */ 480,
                        /* imageColorDepthBits= */ 32);

        private Bundle mLastBundle;

        @Override
        public void onNavigationStateChanged(Bundle bundle) {
            mLastBundle = bundle;
        }

        @Override
        public CarNavigationInstrumentCluster getInstrumentClusterInfo() {
            return mClusterInfo;
        }
    }
}
