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

package com.android.net.module.util;

import static com.android.testutils.MiscAsserts.assertThrows;

import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.net.INetd;
import android.net.InterfaceConfigurationParcel;

import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.Arrays;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class NetdUtilsTest {
    @Mock private INetd mNetd;

    private static final String IFACE = "TEST_IFACE";

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
    }

    private void setupFlagsForInterfaceConfiguration(String[] flags) throws Exception {
        final InterfaceConfigurationParcel config = new InterfaceConfigurationParcel();
        config.flags = flags;
        when(mNetd.interfaceGetCfg(eq(IFACE))).thenReturn(config);
    }

    private void verifyMethodsAndArgumentsOfSetInterface(boolean ifaceUp) throws Exception {
        final String[] flagsContainDownAndUp = new String[] {"flagA", "down", "flagB", "up"};
        final String[] flagsForInterfaceDown = new String[] {"flagA", "down", "flagB"};
        final String[] flagsForInterfaceUp = new String[] {"flagA", "up", "flagB"};
        final String[] expectedFinalFlags;
        setupFlagsForInterfaceConfiguration(flagsContainDownAndUp);
        if (ifaceUp) {
            // "down" flag will be removed from flagsContainDownAndUp when interface is up. Set
            // expectedFinalFlags to flagsForInterfaceUp.
            expectedFinalFlags = flagsForInterfaceUp;
            NetdUtils.setInterfaceUp(mNetd, IFACE);
        } else {
            // "up" flag will be removed from flagsContainDownAndUp when interface is down. Set
            // expectedFinalFlags to flagsForInterfaceDown.
            expectedFinalFlags = flagsForInterfaceDown;
            NetdUtils.setInterfaceDown(mNetd, IFACE);
        }
        verify(mNetd).interfaceSetCfg(
                argThat(config ->
                        // Check if actual flags are the same as expected flags.
                        // TODO: Have a function in MiscAsserts to check if two arrays are the same.
                        CollectionUtils.all(Arrays.asList(expectedFinalFlags),
                                flag -> Arrays.asList(config.flags).contains(flag))
                        && CollectionUtils.all(Arrays.asList(config.flags),
                                flag -> Arrays.asList(expectedFinalFlags).contains(flag))));
    }

    @Test
    public void testSetInterfaceUp() throws Exception {
        verifyMethodsAndArgumentsOfSetInterface(true /* ifaceUp */);
    }

    @Test
    public void testSetInterfaceDown() throws Exception {
        verifyMethodsAndArgumentsOfSetInterface(false /* ifaceUp */);
    }

    @Test
    public void testRemoveAndAddFlags() throws Exception {
        final String[] flags = new String[] {"flagA", "down", "flagB"};
        // Add an invalid flag and expect to get an IllegalStateException.
        assertThrows(IllegalStateException.class,
                () -> NetdUtils.removeAndAddFlags(flags, "down" /* remove */, "u p" /* add */));
    }
}

