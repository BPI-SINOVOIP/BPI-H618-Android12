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

import android.app.Service;
import android.car.telemetry.IScriptExecutor;
import android.car.telemetry.IScriptExecutorListener;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.util.Log;

import com.android.car.CarServiceUtils;

/**
 * Executes Lua code in an isolated process with provided source code
 * and input arguments.
 */
public final class ScriptExecutor extends Service {

    static {
        System.loadLibrary("scriptexecutorjni");
    }

    private static final String TAG = ScriptExecutor.class.getSimpleName();

    // Dedicated "worker" thread to handle all calls related to native code.
    private HandlerThread mNativeHandlerThread;
    // Handler associated with the native worker thread.
    private Handler mNativeHandler;

    private final class IScriptExecutorImpl extends IScriptExecutor.Stub {
        @Override
        public void invokeScript(String scriptBody, String functionName, Bundle publishedData,
                Bundle savedState, IScriptExecutorListener listener) {
            mNativeHandler.post(() ->
                    nativeInvokeScript(mLuaEnginePtr, scriptBody, functionName, publishedData,
                            savedState, listener));
        }
    }
    private IScriptExecutorImpl mScriptExecutorBinder;

    // Memory location of Lua Engine object which is allocated in native code.
    private long mLuaEnginePtr;

    @Override
    public void onCreate() {
        super.onCreate();

        mNativeHandlerThread = CarServiceUtils.getHandlerThread(
            ScriptExecutor.class.getSimpleName());
        // TODO(b/192284628): Remove this once start handling all recoverable errors via onError
        // callback.
        mNativeHandlerThread.setUncaughtExceptionHandler((t, e) -> Log.e(TAG, e.getMessage()));
        mNativeHandler = new Handler(mNativeHandlerThread.getLooper());

        mLuaEnginePtr = nativeInitLuaEngine();
        mScriptExecutorBinder = new IScriptExecutorImpl();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        nativeDestroyLuaEngine(mLuaEnginePtr);
        mNativeHandlerThread.quit();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mScriptExecutorBinder;
    }

    /**
    * Initializes Lua Engine.
    *
    * <p>Returns memory location of Lua Engine.
    */
    private native long nativeInitLuaEngine();

    /**
     * Destroys LuaEngine at the provided memory address.
     */
    private native void nativeDestroyLuaEngine(long luaEnginePtr);

    /**
     * Calls provided Lua function.
     *
     * @param luaEnginePtr memory address of the stored LuaEngine instance.
     * @param scriptBody complete body of Lua script that also contains the function to be invoked.
     * @param functionName the name of the function to execute.
     * @param publishedData input data provided by the source which the function handles.
     * @param savedState key-value pairs preserved from the previous invocation of the function.
     * @param listener callback for the sandboxed environent to report back script execution results
     * and errors.
     */
    private native void nativeInvokeScript(long luaEnginePtr, String scriptBody,
            String functionName, Bundle publishedData, Bundle savedState,
            IScriptExecutorListener listener);
}
