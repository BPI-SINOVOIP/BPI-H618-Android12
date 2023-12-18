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
package com.android.car;

import android.annotation.UserIdInt;
import android.content.ComponentName;
import android.content.pm.UserInfo;
import android.os.RemoteException;
import android.util.Log;

import com.android.car.internal.ICarServiceHelper;

import java.util.Arrays;
import java.util.List;

/**
 * Base implementation of {@link ICarServiceHelper.Stub} providing no-ops methods.
 */
abstract class AbstractICarServiceHelperStub extends ICarServiceHelper.Stub {

    private static final String TAG = AbstractICarServiceHelperStub.class.getSimpleName();

    AbstractICarServiceHelperStub() {
        Log.d(TAG, "I am " + this);
    }

    @Override
    public int forceSuspend(int timeoutMs) {
        Log.d(TAG, "forceSuspend(" + timeoutMs + "): returning 0");
        return 0;
    }

    @Override
    public void setDisplayAllowlistForUser(@UserIdInt int userId, int[] displayIds) {
        Log.d(TAG, "setDisplayAllowlistForUser(user=" + userId + ", displayIds="
                + Arrays.toString(displayIds) + ")");
    }

    @Override
    public void setPassengerDisplays(int[] displayIdsForPassenger) {
        Log.d(TAG, "setPassengerDisplays(displayIdsForPassenger="
                + Arrays.toString(displayIdsForPassenger) + ")");
    }

    @Override
    public void setSourcePreferredComponents(boolean enableSourcePreferred,
            List<ComponentName> sourcePreferredComponents) throws RemoteException {
        Log.d(TAG, "setSourcePreferredComponents(enableSourcePreferred=" + enableSourcePreferred
                + ", sourcePreferredComponents=" + sourcePreferredComponents + ")");
    }

    @Override
    public void setSafetyMode(boolean safe) {
        Log.d(TAG, "setSafetyMode(safe=" + safe + ")");
    }

    @Override
    public UserInfo createUserEvenWhenDisallowed(String name, String userType, int flags) {
        Log.d(TAG, "createUserEvenWhenDisallowed(name=" + name + ", userType=" + userType
                + ", flags=" + flags + ")");
        return null;
    }
}
