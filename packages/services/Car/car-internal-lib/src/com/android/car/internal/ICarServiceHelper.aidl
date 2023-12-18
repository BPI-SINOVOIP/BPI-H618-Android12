/*
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

package com.android.car.internal;

import android.content.ComponentName;
import android.content.pm.UserInfo;

import java.util.List;

/**
 * Helper API for car service. Only for interaction between system server and car service.
 */
interface ICarServiceHelper {
    int forceSuspend(int timeoutMs);
    /**
    * Check
    * {@link com.android.server.wm.CarLaunchParamsModifier#setDisplayAllowlistForUser(int, int[]).
    */
    void setDisplayAllowlistForUser(int userId, in int[] displayIds);

    /**
     * Check
     * {@link com.android.server.wm.CarLaunchParamsModifier#setPassengerDisplays(int[])}.
     */
    void setPassengerDisplays(in int[] displayIds);

    /**
     * Check
     * {@link com.android.server.wm.CarLaunchParamsModifier#setSourcePreferredComponents(
     *         boolean, List<ComponentName>)}.
     */
    void setSourcePreferredComponents(
            boolean enableSourcePreferred, in List<ComponentName> sourcePreferredComponents);

    /**
     * Sets whether it's safe to run operations (like DevicePolicyManager.lockNow()).
     */
    void setSafetyMode(boolean safe);

    /**
     * Creates the given user, even when it's disallowed by DevicePolicyManager.
     */
    UserInfo createUserEvenWhenDisallowed(String name, String userType, int flags);
}
