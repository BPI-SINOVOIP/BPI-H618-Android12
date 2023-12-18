/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.app.stubs;;

import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.AsyncTask;
import android.os.Binder;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Parcel;
import android.os.RemoteException;

import java.util.concurrent.CountDownLatch;

public class TrimMemService extends Service {
    private static final int COMMAND_TRIM_MEMORY_LEVEL = IBinder.FIRST_CALL_TRANSACTION;
    private Binder mRemote = new Binder();
    private IBinder mCallback;

    @Override
    public IBinder onBind(Intent intent) {
        final Bundle extras = intent.getExtras();
        mCallback = extras.getBinder(CommandReceiver.EXTRA_CALLBACK);
        return mRemote;
    }

    @Override
    public void onTrimMemory(int level) {
        if (mCallback != null) {
            Parcel data = Parcel.obtain();
            Parcel reply = Parcel.obtain();
            data.writeInt(level);
            try {
                mCallback.transact(COMMAND_TRIM_MEMORY_LEVEL, data, reply, 0);
            } catch (RemoteException e) {
            } finally {
                data.recycle();
                reply.recycle();
            }
        }
    }

    private static class MyMemFactorCallback extends Binder {
        private CountDownLatch[] mLatchHolder;
        private int[] mLevelHolder;

        MyMemFactorCallback(CountDownLatch[] latchHolder, int[] levelHolder) {
            mLatchHolder = latchHolder;
            mLevelHolder = levelHolder;
        }

        @Override
        protected boolean onTransact(int code, Parcel data, Parcel reply, int flags)
                throws RemoteException {
            switch (code) {
                case COMMAND_TRIM_MEMORY_LEVEL:
                    mLevelHolder[0] = data.readInt();
                    mLatchHolder[0].countDown();
                    return true;
                default:
                    return false;
            }
        }
    }

    private static class MyServiceConnection implements ServiceConnection {
        private CountDownLatch mLatch;

        MyServiceConnection(CountDownLatch latch) {
            mLatch = latch;
        }

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mLatch.countDown();
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
        }
    }

    public static ServiceConnection bindToTrimMemService(String packageName, String instanceName,
            CountDownLatch[] latchHolder, int[] levelHolder, Context context) throws Exception {
        final Intent intent = new Intent();
        intent.setClassName(packageName, "android.app.stubs.TrimMemService");
        final Bundle extras = new Bundle();
        extras.putBinder(CommandReceiver.EXTRA_CALLBACK,
                new MyMemFactorCallback(latchHolder, levelHolder));
        intent.putExtras(extras);
        final CountDownLatch latch = new CountDownLatch(1);
        final MyServiceConnection conn = new MyServiceConnection(latch);
        context.bindIsolatedService(intent, Context.BIND_AUTO_CREATE | Context.BIND_WAIVE_PRIORITY,
                instanceName, AsyncTask.THREAD_POOL_EXECUTOR, conn);
        latch.await();
        return conn;
    }
}

