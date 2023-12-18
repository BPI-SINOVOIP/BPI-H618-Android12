/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.compatibility.common.util;

import static com.android.compatibility.common.util.SettingsUtils.putGlobalSetting;
import static com.android.compatibility.common.util.TestUtils.waitUntil;

import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.BatteryManager;
import android.os.PowerManager;
import android.provider.Settings.Global;
import android.util.Log;

import androidx.test.InstrumentationRegistry;

import org.junit.Assume;

public class BatteryUtils {
    private static final String TAG = "CtsBatteryUtils";

    private BatteryUtils() {
    }

    public static BatteryManager getBatteryManager() {
        return InstrumentationRegistry.getContext().getSystemService(BatteryManager.class);
    }

    public static PowerManager getPowerManager() {
        return InstrumentationRegistry.getContext().getSystemService(PowerManager.class);
    }

    public static boolean hasBattery() {
        final Intent batteryInfo = InstrumentationRegistry.getContext()
                .registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        return batteryInfo.getBooleanExtra(BatteryManager.EXTRA_PRESENT, true);
    }

    /** Make the target device think it's off charger. */
    public static void runDumpsysBatteryUnplug() throws Exception {
        SystemUtil.runShellCommandForNoOutput("cmd battery unplug");

        waitForPlugStatus(false);

        Log.d(TAG, "Battery UNPLUGGED");
    }

    /**
     * Set the battery level to {@code level} percent. The valid range is [0, 100].
     */
    public static void runDumpsysBatterySetLevel(int level) throws Exception {
        SystemUtil.runShellCommandForNoOutput(("cmd battery set level " + level));

        Log.d(TAG, "Battery level set to " + level);
    }

    /**
     * Set whether the device is plugged in to a charger or not.
     */
    public static void runDumpsysBatterySetPluggedIn(boolean pluggedIn) throws Exception {
        SystemUtil.runShellCommandForNoOutput(("cmd battery set ac " + (pluggedIn ? "1" : "0")));

        waitForPlugStatus(pluggedIn);

        Log.d(TAG, "Battery AC set to " + pluggedIn);
    }

    private static void waitForPlugStatus(boolean pluggedIn) throws Exception {
        if (InstrumentationRegistry.getContext().getPackageManager().isInstantApp()) {
            // Instant apps are not allowed to query ACTION_BATTERY_CHANGED. Add short sleep as
            // best-effort wait for status.
            Thread.sleep(2000);
            return;
        }
        IntentFilter ifilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        waitUntil("Device still " + (pluggedIn ? " not plugged" : " plugged"),
                () -> {
                    Intent batteryStatus =
                            InstrumentationRegistry.getContext().registerReceiver(null, ifilter);
                    int chargePlug = batteryStatus.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);
                    return pluggedIn == (chargePlug != 0);
                });
    }

    /** Reset the effect of all the previous {@code runDumpsysBattery*} call */
    public static void runDumpsysBatteryReset() {
        SystemUtil.runShellCommandForNoOutput(("cmd battery reset"));

        Log.d(TAG, "Battery RESET");
    }

    public static void enableAdaptiveBatterySaver(boolean enabled) {
        final String setting = enabled ? "true" : "false";
        SystemUtil.runShellCommandForNoOutput(
                "cmd power set-adaptive-power-saver-enabled " + setting);
    }

    /**
     * Enable / disable battery saver. Note {@link #runDumpsysBatteryUnplug} must have been
     * executed before enabling BS.
     */
    public static void enableBatterySaver(boolean enabled) throws Exception {
        if (enabled) {
            SystemUtil.runShellCommandForNoOutput("cmd power set-mode 1");
            putGlobalSetting(Global.LOW_POWER_MODE, "1");
            waitUntil("Battery saver still off", () -> getPowerManager().isPowerSaveMode());
        } else {
            SystemUtil.runShellCommandForNoOutput("cmd power set-mode 0");
            putGlobalSetting(Global.LOW_POWER_MODE, "0");
            putGlobalSetting(Global.LOW_POWER_MODE_STICKY, "0");
            waitUntil("Battery saver still on", () -> !getPowerManager().isPowerSaveMode());
        }

        AmUtils.waitForBroadcastIdle();
        Log.d(TAG, "Battery saver turned " + (enabled ? "ON" : "OFF"));
    }

    /**
     * Turn on/off screen.
     */
    public static void turnOnScreen(boolean on) throws Exception {
        if (on) {
            SystemUtil.runShellCommandForNoOutput("input keyevent KEYCODE_WAKEUP");
            waitUntil("Device still not interactive", () -> getPowerManager().isInteractive());

        } else {
            SystemUtil.runShellCommandForNoOutput("input keyevent KEYCODE_SLEEP");
            waitUntil("Device still interactive", () -> !getPowerManager().isInteractive());
        }
        AmUtils.waitForBroadcastIdle();
        Log.d(TAG, "Screen turned " + (on ? "ON" : "OFF"));
    }

    /** @return true if the device supports battery saver. */
    public static boolean isBatterySaverSupported() {
        if (!hasBattery()) {
            // Devices without a battery don't support battery saver.
            return false;
        }

        final PackageManager pm = InstrumentationRegistry.getContext().getPackageManager();
        return !(pm.hasSystemFeature(PackageManager.FEATURE_WATCH) ||
            pm.hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE));
    }

    /** "Assume" the current device supports battery saver. */
    public static void assumeBatterySaverFeature() {
        Assume.assumeTrue("Device doesn't support battery saver", isBatterySaverSupported());
    }
}
