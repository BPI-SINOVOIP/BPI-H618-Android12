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

package android.usb.cts;

import static android.Manifest.permission.MANAGE_USB;

import android.app.UiAutomation;
import android.hardware.usb.UsbManager;
import android.util.Log;

import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import java.util.List;
import java.util.ArrayList;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Unit tests for {@link android.hardware.usb.UsbManager}.
 * Note: MUST claimed MANAGE_USB permission in Manifest
 */
@RunWith(AndroidJUnit4.class)
public class UsbManagerApiTest {
    private static final String TAG = UsbManagerApiTest.class.getSimpleName();

    private UsbManager mUsbManagerSys =
        InstrumentationRegistry.getContext().getSystemService(UsbManager.class);
    private UiAutomation mUiAutomation =
        InstrumentationRegistry.getInstrumentation().getUiAutomation();

    // Update latest HAL version here
    private int USB_HAL_LATEST_VERSION = UsbManager.USB_HAL_V1_3;

    @Before
    public void setUp() {
        Assert.assertNotNull(mUsbManagerSys);
    }

    /**
     * Verify NO SecurityException.
     * Go through System Server.
     */
    @Test
    public void test_UsbApiSetGetCurrentFunctionsSys() throws Exception {
        // Adopt MANAGE_USB permission.
        mUiAutomation.adoptShellPermissionIdentity(MANAGE_USB);

        // Should pass with permission.
        mUsbManagerSys.setCurrentFunctions(UsbManager.FUNCTION_NONE);
        Assert.assertEquals("CurrentFunctions mismatched: ", UsbManager.FUNCTION_NONE,
                mUsbManagerSys.getCurrentFunctions());

        // Drop MANAGE_USB permission.
        mUiAutomation.dropShellPermissionIdentity();

        try {
            mUsbManagerSys.getCurrentFunctions();
            Assert.fail("Expecting SecurityException on getCurrentFunctions.");
        } catch (SecurityException secEx) {
            Log.d(TAG, "Expected SecurityException on getCurrentFunctions");
        }

        try {
            mUsbManagerSys.setCurrentFunctions(UsbManager.FUNCTION_NONE);
            Assert.fail("Expecting SecurityException on setCurrentFunctions.");
        } catch (SecurityException secEx) {
            Log.d(TAG, "Expected SecurityException on setCurrentFunctions");
        }
    }

    /**
     * Verify NO SecurityException.
     */
    @Test
    public void test_UsbApiForUsbGadgetHal() throws Exception {
        // Adopt MANAGE_USB permission.
        mUiAutomation.adoptShellPermissionIdentity(MANAGE_USB);

        // Should pass with permission.
        int version = mUsbManagerSys.getGadgetHalVersion();
        int usbBandwidth = mUsbManagerSys.getUsbBandwidthMbps();
        if (version > UsbManager.GADGET_HAL_V1_1) {
            Assert.assertTrue(usbBandwidth > UsbManager.USB_DATA_TRANSFER_RATE_UNKNOWN);
        } else {
            Assert.assertEquals(usbBandwidth, UsbManager.USB_DATA_TRANSFER_RATE_UNKNOWN);
        }

        // Drop MANAGE_USB permission.
        mUiAutomation.dropShellPermissionIdentity();

        try {
            mUsbManagerSys.getGadgetHalVersion();
            Assert.fail("Expecting SecurityException on getGadgetHalVersion.");
        } catch (SecurityException secEx) {
            Log.d(TAG, "Expected SecurityException on getGadgetHalVersion.");
        }
    }

    /**
     * Verify NO SecurityException.
     */
    @Test
    public void test_UsbApiForUsbHal() throws Exception {
        // Adopt MANAGE_USB permission.
        mUiAutomation.adoptShellPermissionIdentity(MANAGE_USB);

        // Should pass with permission.
        int version = mUsbManagerSys.getUsbHalVersion();
        if (version == USB_HAL_LATEST_VERSION) {
            Log.d(TAG, "Running with the latest HAL version");
        } else if (version == UsbManager.USB_HAL_NOT_SUPPORTED) {
            Log.d(TAG, "Not supported HAL version");
        }
        else {
            Log.d(TAG, "Not the latest HAL version");
        }

        // Drop MANAGE_USB permission.
        mUiAutomation.dropShellPermissionIdentity();

        try {
            mUsbManagerSys.getUsbHalVersion();
            Assert.fail("Expecting SecurityException on getUsbHalVersion.");
        } catch (SecurityException secEx) {
            Log.d(TAG, "Expected SecurityException on getUsbHalVersion.");
        }
    }
}
