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

package android.app.cts;

import static androidx.test.InstrumentationRegistry.getInstrumentation;

import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeFalse;

import android.app.StatusBarManager;
import android.app.StatusBarManager.DisableInfo;
import android.app.UiAutomation;
import android.content.Context;
import android.content.pm.PackageManager;
import android.view.KeyEvent;

import androidx.test.InstrumentationRegistry;
import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class StatusBarManagerTest {
    private static final String PERMISSION_STATUS_BAR = "android.permission.STATUS_BAR";

    private StatusBarManager mStatusBarManager;
    private Context mContext;
    private UiAutomation mUiAutomation;

    private boolean isWatch() {
        return mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_WATCH);
    }

    /**
     * Setup
     * @throws Exception
     */
    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getContext();
        assumeFalse("Status bar service not supported", isWatch());
        mStatusBarManager = (StatusBarManager) mContext.getSystemService(
                Context.STATUS_BAR_SERVICE);
        mUiAutomation = getInstrumentation().getUiAutomation();
        mUiAutomation.adoptShellPermissionIdentity(PERMISSION_STATUS_BAR);
    }

    @After
    public void tearDown() {

        if (mStatusBarManager != null) {
            // Adopt again since tests could've dropped it
            mUiAutomation.adoptShellPermissionIdentity(PERMISSION_STATUS_BAR);
            mStatusBarManager.setDisabledForSetup(false);
        }
        mUiAutomation.dropShellPermissionIdentity();
    }


    /**
     * Test StatusBarManager.setDisabledForSetup(true)
     * @throws Exception
     */
    @Test
    public void testDisableForSetup_setDisabledTrue() throws Exception {
        mStatusBarManager.setDisabledForSetup(true);

        // Check for the default set of disable flags
        assertDefaultFlagsArePresent(mStatusBarManager.getDisableInfo());
    }

    private void assertDefaultFlagsArePresent(DisableInfo info) {
        assertTrue(info.isNotificationPeekingDisabled());
        assertTrue(info.isNavigateToHomeDisabled());
        assertTrue(info.isStatusBarExpansionDisabled());
        assertTrue(info.isRecentsDisabled());
        assertTrue(info.isSearchDisabled());
    }

    /**
     * Test StatusBarManager.setDisabledForSetup(false)
     * @throws Exception
     */
    @Test
    public void testDisableForSetup_setDisabledFalse() throws Exception {
        // First disable, then re-enable
        mStatusBarManager.setDisabledForSetup(true);
        mStatusBarManager.setDisabledForSetup(false);

        DisableInfo info = mStatusBarManager.getDisableInfo();

        assertTrue("Invalid disableFlags", info.areAllComponentsEnabled());
    }

    @Test
    public void testDisableForSimLock_setDisabledTrue() throws Exception {
        mStatusBarManager.setExpansionDisabledForSimNetworkLock(true);

        // Check for the default set of disable flags
        assertTrue(mStatusBarManager.getDisableInfo().isStatusBarExpansionDisabled());
    }

    @Test
    public void testDisableForSimLock_setDisabledFalse() throws Exception {
        // First disable, then re-enable
        mStatusBarManager.setExpansionDisabledForSimNetworkLock(true);
        mStatusBarManager.setExpansionDisabledForSimNetworkLock(false);

        DisableInfo info = mStatusBarManager.getDisableInfo();
        assertTrue("Invalid disableFlags", info.areAllComponentsEnabled());
    }

    @Test(expected = SecurityException.class)
    public void testCollapsePanels_withoutStatusBarPermission_throws() throws Exception {
        // We've adopted shell identity for STATUS_BAR in setUp(), so drop it now
        mUiAutomation.dropShellPermissionIdentity();

        mStatusBarManager.collapsePanels();
    }

    @Test
    public void testCollapsePanels_withStatusBarPermission_doesNotThrow() throws Exception {
        // We've adopted shell identity for STATUS_BAR in setUp()

        mStatusBarManager.collapsePanels();

        // Nothing thrown, passed
    }

    @Test(expected = SecurityException.class)
    public void testTogglePanel_withoutStatusBarPermission_throws() throws Exception {
        // We've adopted shell identity for STATUS_BAR in setUp(), so drop it now
        mUiAutomation.dropShellPermissionIdentity();

        mStatusBarManager.togglePanel();
    }

    @Test
    public void testTogglePanel_withStatusBarPermission_doesNotThrow() throws Exception {
        // We've adopted shell identity for STATUS_BAR in setUp()

        mStatusBarManager.togglePanel();

        // Nothing thrown, passed
    }

    @Test(expected = SecurityException.class)
    public void testHandleSystemKey_withoutStatusBarPermission_throws() throws Exception {
        // We've adopted shell identity for STATUS_BAR in setUp(), so drop it now
        mUiAutomation.dropShellPermissionIdentity();

        mStatusBarManager.handleSystemKey(KeyEvent.KEYCODE_SYSTEM_NAVIGATION_UP);
    }

    @Test
    public void testHandleSystemKey_withStatusBarPermission_doesNotThrow() throws Exception {
        // We've adopted shell identity for STATUS_BAR in setUp()

        mStatusBarManager.handleSystemKey(KeyEvent.KEYCODE_SYSTEM_NAVIGATION_UP);

        // Nothing thrown, passed
    }
}
