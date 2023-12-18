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

package com.android.car.messenger.impl;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.IBinder;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.PreferenceManager;

import com.android.car.messenger.core.interfaces.AppFactory;
import com.android.car.messenger.core.interfaces.DataModel;
import com.android.car.messenger.core.service.MessengerService;
import com.android.car.messenger.impl.datamodels.TelephonyDataModel;

/* App Factory Implementation */
class AppFactoryImpl extends AppFactory {
    @NonNull private Context mApplicationContext;
    @NonNull private DataModel mDataModel;
    @NonNull private SharedPreferences mSharedPreferences;
    @Nullable private MessengerService mMessengerService;

    @NonNull
    private final ServiceConnection mServiceConnection =
            new ServiceConnection() {
                @Override
                public void onServiceConnected(
                        @NonNull ComponentName className, @NonNull IBinder service) {
                    MessengerService.LocalBinder binder = (MessengerService.LocalBinder) service;
                    mMessengerService = binder.getService();
                }

                @Override
                public void onServiceDisconnected(@NonNull ComponentName arg0) {
                    mMessengerService = null;
                }
            };

    private AppFactoryImpl() {}

    public static void register(@NonNull final CarMessengerApp application) {
        if (sRegistered && sInitialized) {
            return;
        }

        final AppFactoryImpl factory = new AppFactoryImpl();
        AppFactory.setInstance(factory);
        sRegistered = true;

        // At this point Factory is published. Services can now get initialized and depend on
        // Factory.get().
        factory.mApplicationContext = application.getApplicationContext();
        factory.mDataModel = new TelephonyDataModel();
        factory.mSharedPreferences =
                PreferenceManager.getDefaultSharedPreferences(factory.mApplicationContext);

        // Create Messenger Service
        Intent intent = new Intent(factory.mApplicationContext, MessengerService.class);
        factory.mApplicationContext.bindService(
                intent, factory.mServiceConnection, Context.BIND_AUTO_CREATE);
    }

    @Override
    @NonNull
    public Context getContext() {
        // prefer the messenger service context
        // to avoid warnings on using app context for UI constants
        if (mMessengerService != null) {
            return mMessengerService;
        } else {
            return mApplicationContext;
        }
    }

    @Override
    @NonNull
    public DataModel getDataModel() {
        return mDataModel;
    }

    @Override
    @NonNull
    public SharedPreferences getSharedPreferences() {
        return mSharedPreferences;
    }
}
