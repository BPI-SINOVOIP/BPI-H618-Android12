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

package com.android.car;

import static com.google.common.truth.Truth.assertThat;

import android.bluetooth.BluetoothProfile;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.Arrays;
import java.util.Collection;

@RunWith(Parameterized.class)
public final class UtilsGetProfilePriorityNameTest {

    private final int mPriority;
    private final String mName;

    public UtilsGetProfilePriorityNameTest(int priority, String name) {
        mPriority = priority;
        mName = name;
    }

    @Test
    public void testGetProfilePriorityName() {
        String result = Utils.getProfilePriorityName(mPriority);

        assertThat(result).contains(String.valueOf(mPriority));
        assertThat(result).ignoringCase().contains(mName);
    }

    @Parameterized.Parameters
    public static Collection provideParams() {
        return Arrays.asList(
            new Object[][] {
                // Any value >= PRIORITY_AUTO_CONNECT maps to "PRIORITY_AUTO_CONNECT".
                {1001, "PRIORITY_AUTO_CONNECT"},
                {BluetoothProfile.PRIORITY_AUTO_CONNECT, "PRIORITY_AUTO_CONNECT"},
                // PRIORITY_AUTO_CONNECT > value >= PRIORITY_ON maps to "PRIORITY_ON".
                {999, "PRIORITY_ON"},
                {BluetoothProfile.PRIORITY_ON, "PRIORITY_ON"},
                // PRIORITY_ON > value >= PRIORITY_OFF mpas to "PRIORITY_OFF".
                {99, "PRIORITY_OFF"},
                {BluetoothProfile.PRIORITY_OFF, "PRIORITY_OFF"},
                // value < PRIORITY_OFF maps to "PRIORITY_UNDEFINED".
                {BluetoothProfile.PRIORITY_UNDEFINED, "PRIORITY_UNDEFINED"},
                {-2, "PRIORITY_UNDEFINED"}
            });
    }
}
