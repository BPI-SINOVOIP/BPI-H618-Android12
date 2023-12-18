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

package com.android.tests.apex.sdkextensions;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.ext.SdkExtensions;
import android.os.ext.test.Test;
import android.util.Log;

import com.android.modules.utils.build.SdkLevel;

public class Receiver extends BroadcastReceiver {

    private static final String ACTION_GET_SDK_VERSION =
            "com.android.tests.apex.sdkextensions.GET_SDK_VERSION";
    private static final String ACTION_IS_AT_LEAST =
            "com.android.tests.apex.sdkextensions.IS_AT_LEAST";
    private static final String ACTION_MAKE_CALLS_DEFAULT =
            "com.android.tests.apex.sdkextensions.MAKE_CALLS_DEFAULT";
    private static final String ACTION_MAKE_CALLS_45 =
            "com.android.tests.apex.sdkextensions.MAKE_CALLS_45";

    private static int dessertLetterToInt(char letter) {
        switch (letter) {
            case 'r': return Build.VERSION_CODES.R;
            case 's': return Build.VERSION_CODES.S;
        }
        throw new IllegalArgumentException(String.valueOf(letter));
    }

    private static boolean isAtLeast(char letter) {
        switch (letter) {
            case 'r': return true; // our min sdk version is 30
            case 's': return SdkLevel.isAtLeastS();
        }
        return false;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        try {
            switch (intent.getAction()) {
                case ACTION_GET_SDK_VERSION:
                    int extension = dessertLetterToInt(intent.getStringExtra("extra").charAt(0));
                    int sdkVersion = SdkExtensions.getExtensionVersion(extension);
                    setResultData(String.valueOf(sdkVersion));
                    break;
                case ACTION_IS_AT_LEAST:
                    boolean value = isAtLeast(intent.getStringExtra("extra").charAt(0));
                    setResultData(String.valueOf(value));
                    break;
                case ACTION_MAKE_CALLS_DEFAULT:
                    makeCallsDefault();
                    setResultData("true");
                    break;
                case ACTION_MAKE_CALLS_45:
                    makeCallsVersion45();
                    setResultData("true");
                    break;
            }
        } catch (Throwable e) {
            Log.e("SdkExtensionsE2E", "Unexpected error/exception", e);
            setResultData(e.toString());
        }
    }

    private static void makeCallsDefault() {
        expectException(NoClassDefFoundError.class, "test class", () -> new Test() );
        expectException(NoClassDefFoundError.class, "test class", () -> Test.staticPublicMethod() );
        expectException(NoClassDefFoundError.class, "test class",
            () -> Test.staticSystemApiMethod() );
        expectException(NoClassDefFoundError.class, "test class",
            () -> Test.staticModuleLibsApiMethod() );
        expectException(NoClassDefFoundError.class, "test class", () -> Test.staticHiddenMethod() );
    }

    private static void makeCallsVersion45() {
        Test test = new Test();

        test.publicMethod();
        test.systemApiMethod();
        expectException(NoSuchMethodError.class, "module method", () -> test.moduleLibsApiMethod());
        expectException(NoSuchMethodError.class, "testapi method", () -> test.testApiMethod());
        expectException(NoSuchMethodError.class, "hidden method", () -> test.hiddenMethod());

        Test.staticPublicMethod();
        Test.staticSystemApiMethod();
        expectException(NoSuchMethodError.class, "static module-libs method",
            () -> Test.staticModuleLibsApiMethod());
        expectException(NoSuchMethodError.class, "static testapi method",
            () -> Test.staticTestApiMethod());
        expectException(NoSuchMethodError.class, "static hidden method",
            () -> Test.staticHiddenMethod());
    }

    private static void expectException(Class exceptionClass, String type, Runnable runnable) {
        boolean success = false;
        try {
            runnable.run();
            success = true;
        } catch (Throwable e) {
            if (!e.getClass().equals(exceptionClass)) {
                throw new IllegalStateException("Saw unexpected exception", e);
            }
        }
        if (success) {
            throw new IllegalStateException("Accessed " + type + ", but shouldn't be able to");
        }
    }
}
