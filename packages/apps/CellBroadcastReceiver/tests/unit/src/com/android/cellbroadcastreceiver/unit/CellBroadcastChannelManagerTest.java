/**
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.cellbroadcastreceiver.unit;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import android.telephony.SubscriptionManager;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.cellbroadcastreceiver.CellBroadcastAlertService.AlertType;
import com.android.cellbroadcastreceiver.CellBroadcastChannelManager;
import com.android.cellbroadcastreceiver.CellBroadcastChannelManager.CellBroadcastChannelRange;
import com.android.cellbroadcastreceiver.unit.CellBroadcastTest;

import java.util.Arrays;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.ArrayList;

/**
 * APN retry manager tests
 */
public class CellBroadcastChannelManagerTest extends CellBroadcastTest {

    @Before
    public void setUp() throws Exception {
        super.setUp(getClass().getSimpleName());
    }

    @After
    public void tearDown() throws Exception {
        super.tearDown();
    }

    @Test
    @SmallTest
    public void testGetCellBroadcastChannelRanges() throws Exception {
        putResources(com.android.cellbroadcastreceiver.R.array.additional_cbs_channels_strings,
                new String[]{
                        "12:type=etws_earthquake, emergency=true, display=false, always_on=true",
                        "456:type=etws_tsunami, emergency=true, alert_duration=60000, "
                                + "scope=domestic", "0xAC00-0xAFED:type=other, emergency=false, "
                        + "override_dnd=true, scope=carrier", "54-60:emergency=true, "
                        + "testing_mode=true", "100-200", "0xA804:type=test, "
                        + "emergency=true, exclude_from_sms_inbox=true, vibration=0|350|250|350"});

        CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(mContext,
                SubscriptionManager.DEFAULT_SUBSCRIPTION_ID);

        ArrayList<CellBroadcastChannelRange> list = channelManager.getCellBroadcastChannelRanges(
                com.android.cellbroadcastreceiver.R.array.additional_cbs_channels_strings);

        assertEquals(12, list.get(0).mStartId);
        assertEquals(12, list.get(0).mEndId);
        assertEquals(AlertType.ETWS_EARTHQUAKE, list.get(0).mAlertType);
        assertEquals(CellBroadcastChannelRange.LEVEL_EMERGENCY, list.get(0).mEmergencyLevel);
        assertTrue(list.get(0).mAlwaysOn);
        assertFalse(list.get(0).mDisplay);
        assertFalse(list.get(0).mOverrideDnd);
        assertTrue(list.get(0).mWriteToSmsInbox);
        assertFalse(list.get(0).mTestMode);

        assertEquals(456, list.get(1).mStartId);
        assertEquals(456, list.get(1).mEndId);
        assertEquals(AlertType.ETWS_TSUNAMI, list.get(1).mAlertType);
        assertEquals(CellBroadcastChannelRange.LEVEL_EMERGENCY, list.get(1).mEmergencyLevel);
        assertFalse(list.get(1).mAlwaysOn);
        assertTrue(list.get(1).mDisplay);
        assertFalse(list.get(1).mOverrideDnd);
        assertTrue(list.get(1).mWriteToSmsInbox);
        assertFalse(list.get(1).mTestMode);
        assertEquals(60000, list.get(1).mAlertDuration);

        assertEquals(0xAC00, list.get(2).mStartId);
        assertEquals(0xAFED, list.get(2).mEndId);
        assertEquals(AlertType.OTHER, list.get(2).mAlertType);
        assertEquals(CellBroadcastChannelRange.LEVEL_NOT_EMERGENCY, list.get(2).mEmergencyLevel);
        assertFalse(list.get(2).mAlwaysOn);
        assertTrue(list.get(2).mDisplay);
        assertTrue(list.get(2).mOverrideDnd);
        assertTrue(list.get(2).mWriteToSmsInbox);
        assertFalse(list.get(2).mTestMode);
        assertEquals(list.get(2).mScope, CellBroadcastChannelRange.SCOPE_CARRIER);

        assertEquals(54, list.get(3).mStartId);
        assertEquals(60, list.get(3).mEndId);
        assertEquals(AlertType.DEFAULT, list.get(3).mAlertType);
        assertEquals(CellBroadcastChannelRange.LEVEL_EMERGENCY, list.get(3).mEmergencyLevel);
        assertFalse(list.get(3).mAlwaysOn);
        assertTrue(list.get(3).mDisplay);
        assertFalse(list.get(3).mOverrideDnd);
        assertTrue(list.get(3).mWriteToSmsInbox);
        assertTrue(list.get(3).mTestMode);

        assertEquals(100, list.get(4).mStartId);
        assertEquals(200, list.get(4).mEndId);
        assertEquals(AlertType.DEFAULT, list.get(4).mAlertType);
        assertEquals(CellBroadcastChannelRange.LEVEL_UNKNOWN, list.get(4).mEmergencyLevel);
        assertFalse(list.get(4).mAlwaysOn);
        assertTrue(list.get(4).mDisplay);
        assertFalse(list.get(4).mOverrideDnd);
        assertTrue(list.get(4).mWriteToSmsInbox);
        assertFalse(list.get(4).mTestMode);

        assertEquals(0xA804, list.get(5).mStartId);
        assertEquals(0xA804, list.get(5).mEndId);
        assertEquals(AlertType.TEST, list.get(5).mAlertType);
        assertEquals(CellBroadcastChannelRange.LEVEL_EMERGENCY, list.get(5).mEmergencyLevel);
        assertFalse(list.get(5).mAlwaysOn);
        assertTrue(list.get(5).mDisplay);
        assertFalse(list.get(5).mOverrideDnd);
        assertFalse(list.get(5).mWriteToSmsInbox);
        assertFalse(list.get(5).mTestMode);
        assertTrue(Arrays.equals(new int[]{0, 350, 250, 350}, list.get(5).mVibrationPattern));
        assertNotEquals(list.get(4).toString(), list.get(5).toString());
    }
}
