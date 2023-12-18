/*
 * Copyright (C) 2019 The Android Open Source Project
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

package com.android.net.module.util;

import static com.android.dx.mockito.inline.extended.ExtendedMockito.doReturn;
import static com.android.dx.mockito.inline.extended.ExtendedMockito.mockitoSession;
import static com.android.net.module.util.DeviceConfigUtils.FIXED_PACKAGE_VERSION;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.content.Context;
import android.content.pm.ModuleInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.provider.DeviceConfig;

import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.MockitoSession;


/**
 * Tests for DeviceConfigUtils.
 *
 */
@RunWith(AndroidJUnit4.class)
@SmallTest
public class DeviceConfigUtilsTest {
    private static final String TEST_NAME_SPACE = "connectivity";
    private static final String TEST_EXPERIMENT_FLAG = "experiment_flag";
    private static final int TEST_FLAG_VALUE = 28;
    private static final String TEST_FLAG_VALUE_STRING = "28";
    private static final int TEST_DEFAULT_FLAG_VALUE = 0;
    private static final int TEST_MAX_FLAG_VALUE = 1000;
    private static final int TEST_MIN_FLAG_VALUE = 100;
    private static final long TEST_PACKAGE_VERSION = 290000000;
    private static final String TEST_PACKAGE_NAME = "test.package.name";
    private static final String TETHERING_AOSP_PACKAGE_NAME = "com.android.networkstack.tethering";
    private static final String TEST_APEX_NAME = "test.apex.name";
    private MockitoSession mSession;

    @Mock private Context mContext;
    @Mock private PackageManager mPm;
    @Mock private ModuleInfo mMi;
    @Mock private PackageInfo mPi;
    @Mock private Resources mResources;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mSession = mockitoSession().spyStatic(DeviceConfig.class).startMocking();

        final PackageInfo pi = new PackageInfo();
        pi.setLongVersionCode(TEST_PACKAGE_VERSION);

        doReturn(mPm).when(mContext).getPackageManager();
        doReturn(TEST_PACKAGE_NAME).when(mContext).getPackageName();
        doReturn(mMi).when(mPm).getModuleInfo(eq(TEST_APEX_NAME), anyInt());
        doReturn(TEST_PACKAGE_NAME).when(mMi).getPackageName();
        doReturn(pi).when(mPm).getPackageInfo(anyString(), anyInt());
        doReturn(mResources).when(mContext).getResources();
    }

    @After
    public void tearDown() {
        mSession.finishMocking();
        DeviceConfigUtils.resetPackageVersionCacheForTest();
    }

    @Test
    public void testGetDeviceConfigPropertyInt_Null() {
        doReturn(null).when(() -> DeviceConfig.getProperty(eq(TEST_NAME_SPACE),
                eq(TEST_EXPERIMENT_FLAG)));
        assertEquals(TEST_DEFAULT_FLAG_VALUE, DeviceConfigUtils.getDeviceConfigPropertyInt(
                TEST_NAME_SPACE, TEST_EXPERIMENT_FLAG,
                TEST_DEFAULT_FLAG_VALUE /* default value */));
    }

    @Test
    public void testGetDeviceConfigPropertyInt_NotNull() {
        doReturn(TEST_FLAG_VALUE_STRING).when(() -> DeviceConfig.getProperty(eq(TEST_NAME_SPACE),
                eq(TEST_EXPERIMENT_FLAG)));
        assertEquals(TEST_FLAG_VALUE, DeviceConfigUtils.getDeviceConfigPropertyInt(
                TEST_NAME_SPACE, TEST_EXPERIMENT_FLAG,
                TEST_DEFAULT_FLAG_VALUE /* default value */));
    }

    @Test
    public void testGetDeviceConfigPropertyInt_NormalValue() {
        doReturn(TEST_FLAG_VALUE_STRING).when(() -> DeviceConfig.getProperty(eq(TEST_NAME_SPACE),
                eq(TEST_EXPERIMENT_FLAG)));
        assertEquals(TEST_FLAG_VALUE, DeviceConfigUtils.getDeviceConfigPropertyInt(
                TEST_NAME_SPACE, TEST_EXPERIMENT_FLAG, 0 /* minimum value */,
                TEST_MAX_FLAG_VALUE /* maximum value */,
                TEST_DEFAULT_FLAG_VALUE /* default value */));
    }

    @Test
    public void testGetDeviceConfigPropertyInt_NullValue() {
        doReturn(null).when(() -> DeviceConfig.getProperty(
                eq(TEST_NAME_SPACE), eq(TEST_EXPERIMENT_FLAG)));
        assertEquals(TEST_DEFAULT_FLAG_VALUE, DeviceConfigUtils.getDeviceConfigPropertyInt(
                TEST_NAME_SPACE, TEST_EXPERIMENT_FLAG, 0 /* minimum value */,
                TEST_MAX_FLAG_VALUE /* maximum value */,
                TEST_DEFAULT_FLAG_VALUE /* default value */));
    }

    @Test
    public void testGetDeviceConfigPropertyInt_OverMaximumValue() {
        doReturn(Integer.toString(TEST_MAX_FLAG_VALUE + 10)).when(() -> DeviceConfig.getProperty(
                eq(TEST_NAME_SPACE), eq(TEST_EXPERIMENT_FLAG)));
        assertEquals(TEST_DEFAULT_FLAG_VALUE, DeviceConfigUtils.getDeviceConfigPropertyInt(
                TEST_NAME_SPACE, TEST_EXPERIMENT_FLAG, TEST_MIN_FLAG_VALUE /* minimum value */,
                TEST_MAX_FLAG_VALUE /* maximum value */,
                TEST_DEFAULT_FLAG_VALUE /* default value */));
    }

    @Test
    public void testGetDeviceConfigPropertyInt_EqualsMaximumValue() {
        doReturn(Integer.toString(TEST_MAX_FLAG_VALUE)).when(() -> DeviceConfig.getProperty(
                eq(TEST_NAME_SPACE), eq(TEST_EXPERIMENT_FLAG)));
        assertEquals(TEST_MAX_FLAG_VALUE, DeviceConfigUtils.getDeviceConfigPropertyInt(
                TEST_NAME_SPACE, TEST_EXPERIMENT_FLAG, TEST_MIN_FLAG_VALUE /* minimum value */,
                TEST_MAX_FLAG_VALUE /* maximum value */,
                TEST_DEFAULT_FLAG_VALUE /* default value */));
    }

    @Test
    public void testGetDeviceConfigPropertyInt_BelowMinimumValue() {
        doReturn(Integer.toString(TEST_MIN_FLAG_VALUE - 10)).when(() -> DeviceConfig.getProperty(
                eq(TEST_NAME_SPACE), eq(TEST_EXPERIMENT_FLAG)));
        assertEquals(TEST_DEFAULT_FLAG_VALUE, DeviceConfigUtils.getDeviceConfigPropertyInt(
                TEST_NAME_SPACE, TEST_EXPERIMENT_FLAG, TEST_MIN_FLAG_VALUE /* minimum value */,
                TEST_MAX_FLAG_VALUE /* maximum value */,
                TEST_DEFAULT_FLAG_VALUE /* default value */));
    }

    @Test
    public void testGetDeviceConfigPropertyInt_EqualsMinimumValue() {
        doReturn(Integer.toString(TEST_MIN_FLAG_VALUE)).when(() -> DeviceConfig.getProperty(
                eq(TEST_NAME_SPACE), eq(TEST_EXPERIMENT_FLAG)));
        assertEquals(TEST_MIN_FLAG_VALUE, DeviceConfigUtils.getDeviceConfigPropertyInt(
                TEST_NAME_SPACE, TEST_EXPERIMENT_FLAG, TEST_MIN_FLAG_VALUE /* minimum value */,
                TEST_MAX_FLAG_VALUE /* maximum value */,
                TEST_DEFAULT_FLAG_VALUE /* default value */));
    }

    @Test
    public void testGetDeviceConfigPropertyBoolean_Null() {
        doReturn(null).when(() -> DeviceConfig.getProperty(eq(TEST_NAME_SPACE),
                eq(TEST_EXPERIMENT_FLAG)));
        assertFalse(DeviceConfigUtils.getDeviceConfigPropertyBoolean(
                TEST_NAME_SPACE, TEST_EXPERIMENT_FLAG,
                false /* default value */));
    }

    @Test
    public void testGetDeviceConfigPropertyBoolean_NotNull() {
        doReturn("true").when(() -> DeviceConfig.getProperty(eq(TEST_NAME_SPACE),
                eq(TEST_EXPERIMENT_FLAG)));
        assertTrue(DeviceConfigUtils.getDeviceConfigPropertyBoolean(
                TEST_NAME_SPACE, TEST_EXPERIMENT_FLAG,
                false /* default value */));
    }

    @Test
    public void testFeatureIsEnabled() {
        doReturn(TEST_FLAG_VALUE_STRING).when(() -> DeviceConfig.getProperty(eq(TEST_NAME_SPACE),
                eq(TEST_EXPERIMENT_FLAG)));
        assertTrue(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG));
        assertTrue(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG, TEST_APEX_NAME, false /* defaultEnabled */));
    }

    @Test
    public void testFeatureDefaultEnabled() {
        doReturn(null).when(() -> DeviceConfig.getProperty(eq(TEST_NAME_SPACE),
                eq(TEST_EXPERIMENT_FLAG)));
        assertFalse(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG));
        assertFalse(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG, TEST_APEX_NAME, false /* defaultEnabled */));
        assertTrue(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG, TEST_APEX_NAME, true /* defaultEnabled */));
    }

    @Test
    public void testFeatureIsEnabledWithException() throws Exception {
        doThrow(NameNotFoundException.class).when(mPm).getPackageInfo(anyString(), anyInt());
        assertFalse(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG));
        assertFalse(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG, TEST_APEX_NAME, false /* defaultEnabled */));
        doThrow(NameNotFoundException.class).when(mPm).getModuleInfo(anyString(), anyInt());
        assertFalse(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG, TEST_APEX_NAME, false /* defaultEnabled */));
    }


    @Test
    public void testFeatureIsEnabledUsingFixedVersion() throws Exception {
        doReturn(TETHERING_AOSP_PACKAGE_NAME).when(mContext).getPackageName();
        doThrow(NameNotFoundException.class).when(mPm).getModuleInfo(anyString(), anyInt());

        doReturn(Long.toString(FIXED_PACKAGE_VERSION)).when(() -> DeviceConfig.getProperty(
                eq(TEST_NAME_SPACE), eq(TEST_EXPERIMENT_FLAG)));
        assertTrue(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG, TEST_APEX_NAME, false /* defaultEnabled */));

        doReturn(Long.toString(FIXED_PACKAGE_VERSION + 1)).when(() -> DeviceConfig.getProperty(
                eq(TEST_NAME_SPACE), eq(TEST_EXPERIMENT_FLAG)));
        assertFalse(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG, TEST_APEX_NAME, false /* defaultEnabled */));

        doReturn(Long.toString(FIXED_PACKAGE_VERSION - 1)).when(() -> DeviceConfig.getProperty(
                eq(TEST_NAME_SPACE), eq(TEST_EXPERIMENT_FLAG)));
        assertTrue(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG, TEST_APEX_NAME, false /* defaultEnabled */));
    }

    @Test
    public void testFeatureIsEnabledCaching() throws Exception {
        doReturn(TEST_FLAG_VALUE_STRING).when(() -> DeviceConfig.getProperty(eq(TEST_NAME_SPACE),
                eq(TEST_EXPERIMENT_FLAG)));
        assertTrue(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG));
        assertTrue(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG));

        // Package info is only queried once
        verify(mContext, times(1)).getPackageManager();
        verify(mContext, times(1)).getPackageName();
        verify(mPm, times(1)).getPackageInfo(anyString(), anyInt());

        assertTrue(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG, TEST_APEX_NAME, false /* defaultEnabled */));
        assertTrue(DeviceConfigUtils.isFeatureEnabled(mContext, TEST_NAME_SPACE,
                TEST_EXPERIMENT_FLAG, TEST_APEX_NAME, false /* defaultEnabled */));

        // Module info is only queried once
        verify(mPm, times(1)).getModuleInfo(anyString(), anyInt());
    }

    @Test
    public void testGetResBooleanConfig() {
        final int someResId = 1234;
        doReturn(true).when(mResources).getBoolean(someResId);
        assertTrue(DeviceConfigUtils.getResBooleanConfig(mContext, someResId, false));
        doReturn(false).when(mResources).getBoolean(someResId);
        assertFalse(DeviceConfigUtils.getResBooleanConfig(mContext, someResId, false));
        doThrow(new Resources.NotFoundException()).when(mResources).getBoolean(someResId);
        assertFalse(DeviceConfigUtils.getResBooleanConfig(mContext, someResId, false));
    }
}
