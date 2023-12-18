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

package com.android.car;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.when;
import static org.testng.Assert.expectThrows;

import android.car.ICarBugreportCallback;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.ParcelFileDescriptor;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.junit.MockitoRule;

/**
 * Unit tests for {@link CarBugreportManagerService}.
 *
 * <p>Run {@code atest CarServiceUnitTest:CarBugreportManagerServiceTest}.
 */
@SmallTest
@RunWith(MockitoJUnitRunner.class)
public class CarBugreportManagerServiceTest {
    private static final boolean DUMPSTATE_DRY_RUN = true;

    @Rule public MockitoRule rule = MockitoJUnit.rule();

    private CarBugreportManagerService mService;

    @Mock private Context mMockContext;
    @Mock private Resources mMockResources;
    @Mock private PackageManager mMockPackageManager;
    @Mock private ICarBugreportCallback mMockCallback;
    @Mock private ParcelFileDescriptor mMockOutput;
    @Mock private ParcelFileDescriptor mMockExtraOutput;

    @Before
    public void setUp() {
        when(mMockContext.getResources()).thenReturn(mMockResources);
        when(mMockContext.getPackageManager()).thenReturn(mMockPackageManager);
    }

    @After
    public void tearDown() {
        if (mService != null) {
            mService.release();
        }
    }

    @Test
    public void test_requestBugreport_failsIfNotDesignatedAppOnUserBuild() {
        mService = new CarBugreportManagerService(mMockContext, /* isUserBuild= */ true);
        mService.init();
        when(mMockPackageManager.checkSignatures(anyInt(), anyInt()))
                .thenReturn(PackageManager.SIGNATURE_MATCH);
        when(mMockPackageManager.getNameForUid(anyInt())).thenReturn("current_app_name");
        when(mMockResources.getString(
                R.string.config_car_bugreport_application)).thenReturn("random_app_name");

        SecurityException expected =
                expectThrows(SecurityException.class,
                        () -> mService.requestBugreport(mMockOutput, mMockExtraOutput,
                                mMockCallback, DUMPSTATE_DRY_RUN));

        assertThat(expected).hasMessageThat().contains(
                "Caller current_app_name is not a designated bugreport app");
    }

    @Test
    public void test_requestBugreport_failsIfNotSignedWithPlatformKeys() {
        mService = new CarBugreportManagerService(mMockContext);
        mService.init();
        when(mMockPackageManager.checkSignatures(anyInt(), anyInt()))
                .thenReturn(PackageManager.SIGNATURE_NO_MATCH);
        when(mMockPackageManager.getNameForUid(anyInt())).thenReturn("current_app_name");

        SecurityException expected =
                expectThrows(SecurityException.class,
                        () -> mService.requestBugreport(mMockOutput, mMockExtraOutput,
                                mMockCallback, DUMPSTATE_DRY_RUN));

        assertThat(expected).hasMessageThat().contains(
                "Caller current_app_name does not have the right signature");
    }
}
