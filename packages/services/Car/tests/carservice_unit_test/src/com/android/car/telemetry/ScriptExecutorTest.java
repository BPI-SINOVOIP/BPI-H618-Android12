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

import static org.junit.Assert.fail;

import android.car.telemetry.IScriptExecutor;
import android.car.telemetry.IScriptExecutorListener;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;

import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

@RunWith(JUnit4.class)
public final class ScriptExecutorTest {

    private IScriptExecutor mScriptExecutor;
    private ScriptExecutor mInstance;
    private Context mContext = InstrumentationRegistry.getInstrumentation().getTargetContext();


    private static final class ScriptExecutorListener extends IScriptExecutorListener.Stub {
        @Override
        public void onScriptFinished(byte[] result) {
        }

        @Override
        public void onSuccess(Bundle stateToPersist) {
        }

        @Override
        public void onError(int errorType, String message, String stackTrace) {
        }
    }

    private final IScriptExecutorListener mFakeScriptExecutorListener =
            new ScriptExecutorListener();

    // TODO(b/189241508). Parsing of publishedData is not implemented yet.
    // Null is the only accepted input.
    private final Bundle mPublishedData = null;
    private final Bundle mSavedState = new Bundle();

    private static final String LUA_SCRIPT =
            "function hello(data, state)\n"
            + "    print(\"Hello World\")\n"
            + "end\n";

    private static final String LUA_METHOD = "hello";

    private final CountDownLatch mBindLatch = new CountDownLatch(1);

    private static final int BIND_SERVICE_TIMEOUT_SEC = 5;

    private final ServiceConnection mScriptExecutorConnection =
            new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName className, IBinder service) {
                    mScriptExecutor = IScriptExecutor.Stub.asInterface(service);
                    mBindLatch.countDown();
                }

                @Override
                public void onServiceDisconnected(ComponentName className) {
                    fail("Service unexpectedly disconnected");
                }
            };

    @Before
    public void setUp() throws InterruptedException {
        mContext.bindIsolatedService(new Intent(mContext, ScriptExecutor.class),
                Context.BIND_AUTO_CREATE, "scriptexecutor", mContext.getMainExecutor(),
                mScriptExecutorConnection);
        if (!mBindLatch.await(BIND_SERVICE_TIMEOUT_SEC, TimeUnit.SECONDS)) {
            fail("Failed to bind to ScripExecutor service");
        }
    }

    @Test
    public void invokeScript_helloWorld() throws RemoteException {
        // Expect to load "hello world" script successfully and push the function.
        mScriptExecutor.invokeScript(LUA_SCRIPT, LUA_METHOD, mPublishedData, mSavedState,
                mFakeScriptExecutorListener);
        // Sleep, otherwise the test case will complete before the script loads
        // because invokeScript is non-blocking.
        try {
            // TODO(b/192285332): Replace sleep logic with waiting for specific callbacks
            // to be called once they are implemented. Otherwise, this could be a flaky test.
            TimeUnit.SECONDS.sleep(10);
        } catch (InterruptedException e) {
            e.printStackTrace();
            fail(e.getMessage());
        }
    }
}

