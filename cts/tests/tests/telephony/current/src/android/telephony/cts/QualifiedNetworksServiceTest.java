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

package android.telephony.cts;

import static org.junit.Assert.assertEquals;

import android.telephony.data.QualifiedNetworksService;

import org.junit.Test;

import java.util.Collections;

public class QualifiedNetworksServiceTest {
    private static final int SLOT_INDEX = 0;
    private static final int APN_TYPES = 0;

    private class TestQns extends QualifiedNetworksService {
        TestQns() {
            super();
        }

        public TestNap onCreateNetworkAvailabilityProvider(int slotIndex) {
            return new TestNap(slotIndex);
        }

        public class TestNap extends QualifiedNetworksService.NetworkAvailabilityProvider {
            TestNap(int slotIndex) {
                super(slotIndex);
            }

            public void close() {
                // Do nothing
            }
        }
    }

    @Test
    public void testConstructorsAndGetters() {
        QualifiedNetworksService qns = new TestQns();
        QualifiedNetworksService.NetworkAvailabilityProvider nap =
                qns.onCreateNetworkAvailabilityProvider(SLOT_INDEX);
        assertEquals(SLOT_INDEX, nap.getSlotIndex());
    }

    @Test
    public void testNetworkAvailabilityProvider() {
        QualifiedNetworksService qns = new TestQns();
        QualifiedNetworksService.NetworkAvailabilityProvider nap =
                qns.onCreateNetworkAvailabilityProvider(SLOT_INDEX);
        nap.updateQualifiedNetworkTypes(APN_TYPES, Collections.emptyList());
        nap.reportThrottleStatusChanged(Collections.emptyList());
    }
}
