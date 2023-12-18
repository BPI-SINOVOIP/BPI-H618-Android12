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

package com.android.imsserviceentitlement;

import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import androidx.test.runner.AndroidJUnit4;

import com.android.imsserviceentitlement.WfcActivationController.EntitlementResultCallback;
import com.android.imsserviceentitlement.entitlement.EntitlementResult;
import com.android.imsserviceentitlement.utils.Executors;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import java.lang.reflect.Field;

@RunWith(AndroidJUnit4.class)
public class EntitlementUtilsTest {
    @Rule public final MockitoRule rule = MockitoJUnit.rule();
    @Mock private ImsEntitlementApi mMockImsEntitlementApi;
    @Mock private EntitlementResultCallback mEntitlementResultCallback;
    @Mock private EntitlementResult mEntitlementResult;

    @Before
    public void setup() throws Exception {
        Field field = Executors.class.getDeclaredField("sUseDirectExecutorForTest");
        field.setAccessible(true);
        field.set(null, true);
    }

    @Test
    public void entitlementCheck_checkEntitlementStatusPass_onEntitlementResult() {
        when(mMockImsEntitlementApi.checkEntitlementStatus()).thenReturn(mEntitlementResult);

        EntitlementUtils.entitlementCheck(mMockImsEntitlementApi, mEntitlementResultCallback);

        verify(mEntitlementResultCallback).onEntitlementResult(mEntitlementResult);
    }

    @Test
    public void entitlementCheck_checkEntitlementStatusWithRuntimeException_onFailure() {
        when(mMockImsEntitlementApi.checkEntitlementStatus()).thenThrow(new RuntimeException());

        EntitlementUtils.entitlementCheck(mMockImsEntitlementApi, mEntitlementResultCallback);

        verify(mEntitlementResultCallback, never()).onEntitlementResult(mEntitlementResult);
    }
}
