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

package com.android.car.telemetry;

import static com.google.common.truth.Truth.assertThat;

import android.os.Bundle;
import android.util.Log;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class JniUtilsTest {

    private static final String TAG = JniUtilsTest.class.getSimpleName();

    private static final String BOOLEAN_KEY = "boolean_key";
    private static final String INT_KEY = "int_key";
    private static final String STRING_KEY = "string_key";
    private static final String NUMBER_KEY = "number_key";

    private static final boolean BOOLEAN_VALUE = true;
    private static final double NUMBER_VALUE = 0.1;
    private static final int INT_VALUE = 10;
    private static final String STRING_VALUE = "test";

    // Pointer to Lua Engine instantiated in native space.
    private long mLuaEnginePtr = 0;

    static {
        System.loadLibrary("scriptexecutorjniutils-test");
    }

    @Before
    public void setUp() {
        mLuaEnginePtr = nativeCreateLuaEngine();
    }

    @After
    public void tearDown() {
        nativeDestroyLuaEngine(mLuaEnginePtr);
    }

    // Simply invokes PushBundleToLuaTable native method under test.
    private native void nativePushBundleToLuaTableCaller(long luaEnginePtr, Bundle bundle);

    // Creates an instance of LuaEngine on the heap and returns the pointer.
    private native long nativeCreateLuaEngine();

    // Destroys instance of LuaEngine on the native side at provided memory address.
    private native void nativeDestroyLuaEngine(long luaEnginePtr);

    // Returns size of a Lua object located at the specified position on the stack.
    private native int nativeGetObjectSize(long luaEnginePtr, int index);

    /*
     * Family of methods to check if the table on top of the stack has
     * the given value under provided key.
     */
    private native boolean nativeHasBooleanValue(long luaEnginePtr, String key, boolean value);
    private native boolean nativeHasStringValue(long luaEnginePtr, String key, String value);
    private native boolean nativeHasIntValue(long luaEnginePtr, String key, int value);
    private native boolean nativeHasDoubleValue(long luaEnginePtr, String key, double value);

    @Test
    public void pushBundleToLuaTable_nullBundleMakesEmptyLuaTable() {
        Log.d(TAG, "Using Lua Engine with address " + mLuaEnginePtr);
        nativePushBundleToLuaTableCaller(mLuaEnginePtr, null);
        // Get the size of the object on top of the stack,
        // which is where our table is supposed to be.
        assertThat(nativeGetObjectSize(mLuaEnginePtr, 1)).isEqualTo(0);
    }

    @Test
    public void pushBundleToLuaTable_valuesOfDifferentTypes() {
        Bundle bundle = new Bundle();
        bundle.putBoolean(BOOLEAN_KEY, BOOLEAN_VALUE);
        bundle.putInt(INT_KEY, INT_VALUE);
        bundle.putDouble(NUMBER_KEY, NUMBER_VALUE);
        bundle.putString(STRING_KEY, STRING_VALUE);

        // Invokes the corresponding helper method to convert the bundle
        // to Lua table on Lua stack.
        nativePushBundleToLuaTableCaller(mLuaEnginePtr, bundle);

        // Check contents of Lua table.
        assertThat(nativeHasBooleanValue(mLuaEnginePtr, BOOLEAN_KEY, BOOLEAN_VALUE)).isTrue();
        assertThat(nativeHasIntValue(mLuaEnginePtr, INT_KEY, INT_VALUE)).isTrue();
        assertThat(nativeHasDoubleValue(mLuaEnginePtr, NUMBER_KEY, NUMBER_VALUE)).isTrue();
        assertThat(nativeHasStringValue(mLuaEnginePtr, STRING_KEY, STRING_VALUE)).isTrue();
    }


    @Test
    public void pushBundleToLuaTable_wrongKey() {
        Bundle bundle = new Bundle();
        bundle.putBoolean(BOOLEAN_KEY, BOOLEAN_VALUE);

        // Invokes the corresponding helper method to convert the bundle
        // to Lua table on Lua stack.
        nativePushBundleToLuaTableCaller(mLuaEnginePtr, bundle);

        // Check contents of Lua table.
        assertThat(nativeHasBooleanValue(mLuaEnginePtr, "wrong key", BOOLEAN_VALUE)).isFalse();
    }
}
