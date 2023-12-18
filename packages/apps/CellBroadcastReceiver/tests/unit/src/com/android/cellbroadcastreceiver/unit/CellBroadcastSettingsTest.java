/**
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.cellbroadcastreceiver.unit;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.os.Looper;
import android.os.RemoteException;
import android.support.test.uiautomator.UiDevice;

import androidx.preference.PreferenceManager;
import androidx.test.InstrumentationRegistry;
import androidx.test.filters.FlakyTest;
import androidx.test.runner.AndroidJUnit4;

import com.android.cellbroadcastreceiver.CellBroadcastSettings;

import junit.framework.Assert;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class CellBroadcastSettingsTest extends
        CellBroadcastActivityTestCase<CellBroadcastSettings> {
    private Instrumentation mInstrumentation;
    private Context mContext;
    private UiDevice mDevice;
    private static final long DEVICE_WAIT_TIME = 1000L;

    public CellBroadcastSettingsTest() {
        super(CellBroadcastSettings.class);
    }

    @Before
    public void setUp() throws Exception {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mContext = mInstrumentation.getTargetContext();
        mDevice = UiDevice.getInstance(mInstrumentation);
    }

    @InstrumentationTest
    // This test has a module dependency, so it is disabled for OEM testing because it is not a true
    // unit test
    @FlakyTest
    @Test
    public void testRotateAlertReminderDialogOpen() throws InterruptedException {
        try {
            mDevice.wakeUp();
            mDevice.pressMenu();
        } catch (RemoteException exception) {
            Assert.fail("Exception " + exception);
        }

        mInstrumentation.startActivitySync(createActivityIntent());
        int w = mDevice.getDisplayWidth();
        int h = mDevice.getDisplayHeight();

        waitUntilDialogOpens(()-> {
            mDevice.swipe(w / 2 /* start X */,
                    h / 2 /* start Y */,
                    w / 2 /* end X */,
                    0 /* end Y */,
                    100 /* steps */);

            openAlertReminderDialog();
        }, DEVICE_WAIT_TIME);

        try {
            mDevice.setOrientationLeft();
            mDevice.setOrientationNatural();
            mDevice.setOrientationRight();
        } catch (Exception e) {
            Assert.fail("Exception " + e);
        }
    }

    @Test
    public void testResetAllPreferences() throws Throwable {
        Looper.prepare();
        // set a few preferences so we can verify they are reset to the default
        PreferenceManager.getDefaultSharedPreferences(mContext).edit()
                .putBoolean(CellBroadcastSettings.KEY_RECEIVE_CMAS_IN_SECOND_LANGUAGE, true)
                .putBoolean(CellBroadcastSettings.KEY_ENABLE_ALERT_VIBRATE, false).apply();
        assertTrue("receive_cmas_in_second_language was not set to true",
                PreferenceManager.getDefaultSharedPreferences(mContext)
                        .getBoolean(CellBroadcastSettings.KEY_RECEIVE_CMAS_IN_SECOND_LANGUAGE,
                                false));
        assertFalse("enable_alert_vibrate was not set to false",
                PreferenceManager.getDefaultSharedPreferences(mContext)
                        .getBoolean(CellBroadcastSettings.KEY_ENABLE_ALERT_VIBRATE, true));

        // see preferences.xml for default values.
        // receive_cmas_in_second_language is false by default
        // enable_alert_vibrate is true by default
        CellBroadcastSettings.resetAllPreferences(mContext);

        assertFalse("receive_cmas_in_second_language was not reset to the default (false)",
                PreferenceManager.getDefaultSharedPreferences(mContext)
                .getBoolean(CellBroadcastSettings.KEY_RECEIVE_CMAS_IN_SECOND_LANGUAGE, true));
        assertTrue("enable_alert_vibrate was not reset to the default (true)",
                PreferenceManager.getDefaultSharedPreferences(mContext)
                .getBoolean(CellBroadcastSettings.KEY_ENABLE_ALERT_VIBRATE, false));
    }

    public void waitUntilDialogOpens(Runnable r, long maxWaitMs) {
        long waitTime = 0;
        while (waitTime < maxWaitMs) {
            try {
                r.run();
                // if the assert succeeds, return
                return;
            } catch (Exception e) {
                waitTime += 100;
                waitForMs(100);
            }
        }
        // if timed out, run one last time without catching exception
        r.run();
    }

    @Override
    protected Intent createActivityIntent() {
        Intent intent = new Intent(mContext, CellBroadcastSettings.class);
        intent.setPackage("com.android.cellbroadcastreceiver");
        intent.setAction("android.intent.action.MAIN");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    private void openAlertReminderDialog() {
        onView(withText(mContext.getString(com.android.cellbroadcastreceiver.R
                .string.alert_reminder_interval_title))).perform(click());
    }
}
