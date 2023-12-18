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

package android.net.ipsec.test.ike.ike3gpp;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.net.ipsec.test.ike.exceptions.IkeProtocolException;

import org.junit.Test;

public class Ike3gppBackoffTimerTest {
    private static final byte BACKOFF_TIMER = (byte) 0xAF;
    private static final int BACKOFF_CAUSE = Ike3gppBackoffTimer.ERROR_TYPE_NETWORK_FAILURE;

    @Test
    public void testIke3gppBackoffTimer() {
        Ike3gppBackoffTimer backoffTimer = new Ike3gppBackoffTimer(BACKOFF_TIMER, BACKOFF_CAUSE);

        assertEquals(Ike3gppData.DATA_TYPE_NOTIFY_BACKOFF_TIMER, backoffTimer.getDataType());
        assertEquals(BACKOFF_TIMER, backoffTimer.getBackoffTimer());
        assertEquals(BACKOFF_CAUSE, backoffTimer.getBackoffCause());
    }

    @Test
    public void testIsValidErrorNotifyCause() {
        assertTrue(
                Ike3gppBackoffTimer.isValidErrorNotifyCause(
                        Ike3gppBackoffTimer.ERROR_TYPE_NO_APN_SUBSCRIPTION));
        assertTrue(
                Ike3gppBackoffTimer.isValidErrorNotifyCause(
                        Ike3gppBackoffTimer.ERROR_TYPE_NETWORK_FAILURE));
        assertFalse(
                Ike3gppBackoffTimer.isValidErrorNotifyCause(
                        IkeProtocolException.ERROR_TYPE_AUTHENTICATION_FAILED));
    }
}
