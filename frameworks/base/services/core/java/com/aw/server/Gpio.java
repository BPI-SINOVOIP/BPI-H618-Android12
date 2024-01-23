/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.softwinner;
import android.util.Log;
import java.io.File;
import android.content.Context;
import android.content.res.Resources;
import com.android.internal.R;
/**
 * Class that provides access to some of the gpio management functions.
 *
 * {@hide}
 */
public final class Gpio {
    public static final String TAG = "GPIO";
    private static boolean ServiceReady = false;
    private static boolean mHasNormalLed = false;
    private static boolean mHasStandbyLed = false;
    private static boolean mHasNetworkLed = false;
    private static boolean DEBUG = false;
    private static boolean mLedBrightInLowLevel;
    // can't instantiate this class
    private Gpio() {
    }

    static {
        String lib_name = "libgpio_jni.so";
        String lib_path[] = {"/system/lib/", "/system/lib64/", "/vendor/lib/", "/vendor/lib64/"};
        for (int i = 0; i < lib_path.length; i++) {
            File file = new File(lib_path[i], lib_name);
            if (file.exists()) {
                System.loadLibrary("gpio_jni");
                int ret = nativeInit();
                if (ret == 0) ServiceReady = true;
                break;
            }
        }
        checkLed();
    }

    private static native int nativeInit();
    private static native int nativeWriteGpio(String path, String value);
    private static native int nativeReadGpio(String path);

    private static final String  PATHSTR      = "/sys/class/gpio_sw/P";
    private static final String  DATA_NAME   = "/data";
    private static final String  PULL_NAME     = "/pull";
    private static final String  DRV_LEVEL_NAME = "/drv_level";
    private static final String  MUL_SEL_NAME   = "/mul_sel";
    private static final String  LIGHT_NAME   = "/light";

    private static final String NORMAL_LED_PATH   = "/sys/class/gpio_sw/normal_led";
    private static final String STANDBY_LED_PATH  = "/sys/class/gpio_sw/standby_led";
    private static final String NETWORK_LED_PATH  = "/sys/class/gpio_sw/network_led";

    public static boolean configLevel(Context mContext) {
        mLedBrightInLowLevel = mContext.getResources().getBoolean(
                com.android.internal.R.bool.config_ledBrightInLowLevel);
        return mLedBrightInLowLevel;
    }
    private static boolean checkLed() {
        String led[] = {NORMAL_LED_PATH, STANDBY_LED_PATH};
        File normal = new File(NORMAL_LED_PATH);
        if (normal.exists()) {
            mHasNormalLed = true;
        }
        File standby = new File(STANDBY_LED_PATH);
        if (standby.exists()) {
            mHasStandbyLed = true;
        }
        File network = new File(NETWORK_LED_PATH);
        if (network.exists()) {
            mHasNetworkLed = true;
        }
        return mHasNormalLed || mHasStandbyLed || mHasNetworkLed;
    }

    public static int setNormalLedOn(boolean on) {
        if (!ServiceReady || !mHasNormalLed) {
            if (DEBUG) Log.d(TAG, "GpioService is not exists or normal led is not exists");
            return -1;
        }
        String dataPath = NORMAL_LED_PATH + LIGHT_NAME;
        if (mLedBrightInLowLevel) {
            return nativeWriteGpio(dataPath, on ? "0" : "1");
        } else {
            return nativeWriteGpio(dataPath, on ? "1" : "0");
        }
    }

    public static int setStandbyLedOn(boolean on) {
        if (!ServiceReady || !mHasStandbyLed) {
            if (DEBUG) Log.d(TAG, "GpioService is not exists or standby led is not exists");
            return -1;
        }
        String dataPath = STANDBY_LED_PATH + LIGHT_NAME;
        if (mLedBrightInLowLevel) {
            return nativeWriteGpio(dataPath, on ? "0" : "1");
        } else {
            return nativeWriteGpio(dataPath, on ? "1" : "0");
        }
    }

    public static int setNetworkLedOn(boolean on) {
        if (!ServiceReady || !mHasNetworkLed) {
            if (DEBUG) Log.d(TAG, "GpioService is not exists or network led is not exists");
            return -1;
        }
        String dataPath = NETWORK_LED_PATH + LIGHT_NAME;
        return nativeWriteGpio(dataPath, on ? "1" : "0");
    }
    public static int writeGpio(char group, int num, int value) {
        if (!ServiceReady) {
            if (DEBUG) Log.d(TAG, "GpioService is not exist");
            return -1;
        }
        String dataPath = composePinPath(group, num).concat(DATA_NAME);

        return nativeWriteGpio(dataPath, Integer.toString(value));
    }

    public static int readGpio(char group, int num) {
        if (!ServiceReady) {
            if (DEBUG) Log.d(TAG, "GpioService is not exist");
            return -1;
        }
        String dataPath = composePinPath(group, num).concat(DATA_NAME);

        return nativeReadGpio(dataPath);
    }

    public static int setPull(char group, int num, int value) {
        if (!ServiceReady) {
            if (DEBUG) Log.d(TAG, "GpioService is not exist");
            return -1;
        }
        String dataPath = composePinPath(group, num).concat(PULL_NAME);

        return nativeWriteGpio(dataPath, Integer.toString(value));
    }

    public static int getPull(char group, int num) {
        if (!ServiceReady) {
            if (DEBUG) Log.d(TAG, "GpioService is not exist");
            return -1;
        }
        String dataPath = composePinPath(group, num).concat(PULL_NAME);

        return nativeReadGpio(dataPath);
    }

    public static int setDrvLevel(char group, int num, int value) {
        if (!ServiceReady) {
            if (DEBUG) Log.d(TAG, "GpioService is not exist");
            return -1;
        }
        String dataPath = composePinPath(group, num).concat(DRV_LEVEL_NAME);

        return nativeWriteGpio(dataPath, Integer.toString(value));
    }

    public static int getDrvLevel(char group, int num) {
        if (!ServiceReady) {
            if (DEBUG) Log.d(TAG, "GpioService is not exist");
            return -1;
        }
        String dataPath = composePinPath(group, num).concat(DRV_LEVEL_NAME);

        return nativeReadGpio(dataPath);
    }

    public static int setMulSel(char group, int num, int value) {
        if (!ServiceReady) {
            if (DEBUG) Log.d(TAG, "GpioService is not exist");
            return -1;
        }
        String dataPath = composePinPath(group, num).concat(MUL_SEL_NAME);

        return nativeWriteGpio(dataPath, Integer.toString(value));
    }

    public static int getMulSel(char group, int num) {
        if (!ServiceReady) {
            if (DEBUG) Log.d(TAG, "GpioService is not exist");
            return -1;
        }
        String dataPath = composePinPath(group, num).concat(MUL_SEL_NAME);

        return nativeReadGpio(dataPath);
    }

    private static String composePinPath(char group, int num) {
        String  numstr;
        String  groupstr;

        groupstr = String.valueOf(group).toUpperCase();
        numstr = Integer.toString(num);
        return PATHSTR.concat(groupstr).concat(numstr);
    }
}

