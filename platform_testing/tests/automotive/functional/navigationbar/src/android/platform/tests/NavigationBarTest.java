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

package android.platform.tests;

import android.app.Instrumentation;
import android.platform.helpers.AutoUtility;
import android.platform.helpers.HelperAccessor;
import android.platform.helpers.IAutoAppGridHelper;
import android.platform.helpers.IAutoDialHelper;
import android.platform.helpers.IAutoHomeHelper;
import android.platform.helpers.IAutoNotificationHelper;
import android.platform.helpers.IAutoSettingHelper;
import android.support.test.uiautomator.UiDevice;

import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;

@RunWith(AndroidJUnit4.class)
public class NavigationBarTest {

    private Instrumentation mInstrumentation;
    private UiDevice mDevice;
    private HelperAccessor<IAutoAppGridHelper> mAppGridHelper;
    private HelperAccessor<IAutoDialHelper> mDialerHelper;
    private HelperAccessor<IAutoNotificationHelper> mNotificationHelper;
    private HelperAccessor<IAutoSettingHelper> mSettingHelper;
    private HelperAccessor<IAutoHomeHelper> mHomeHelper;

    public NavigationBarTest() {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mDevice = UiDevice.getInstance(mInstrumentation);
        mHomeHelper = new HelperAccessor<>(IAutoHomeHelper.class);
        mAppGridHelper = new HelperAccessor<>(IAutoAppGridHelper.class);
        mNotificationHelper = new HelperAccessor<>(IAutoNotificationHelper.class);
        mSettingHelper = new HelperAccessor<>(IAutoSettingHelper.class);
        mDialerHelper = new HelperAccessor<>(IAutoDialHelper.class);
    }

    @BeforeClass
    public static void exitSuw() {
        AutoUtility.exitSuw();
    }

    @After
    public void goBackToHomeScreen() {
        mDevice.pressHome();
    }

    @Test
    public void testHomeButton() {
        mHomeHelper.get().open();
        assertTrue(mHomeHelper.get().hasMediaWidget());
    }

    @Test
    public void testDialButton() {
        mDialerHelper.get().open();
        assertFalse("Phone is paired.", mDialerHelper.get().isPhonePaired());
    }

    @Test
    public void testAppGridButton() {
        mAppGridHelper.get().open();
        assertTrue("App Grid is not open.", mAppGridHelper.get().isAppInForeground());
    }

    @Test
    public void testNotificationButton() {
        mNotificationHelper.get().open();
        assertTrue("Notification did not open.", mNotificationHelper.get().isAppInForeground());
    }

    @Test
    public void testQuickSetting() {
        mSettingHelper.get().openQuickSettings();
    }
}
