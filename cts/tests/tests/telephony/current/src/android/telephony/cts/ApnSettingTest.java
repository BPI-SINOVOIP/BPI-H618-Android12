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

package android.telephony.cts;

import static org.junit.Assert.assertEquals;

import android.telephony.data.ApnSetting;
import android.util.ArrayMap;

import org.junit.Test;

import java.util.Map;

public class ApnSettingTest {
    private static final Map<String, Integer> EXPECTED_STRING_TO_INT_MAP;
    private static final Map<Integer, String> EXPECTED_INT_TO_STRING_MAP;
    static {
        EXPECTED_STRING_TO_INT_MAP = new ArrayMap<>();
        EXPECTED_STRING_TO_INT_MAP.put(ApnSetting.TYPE_DEFAULT_STRING, ApnSetting.TYPE_DEFAULT);
        EXPECTED_STRING_TO_INT_MAP.put(ApnSetting.TYPE_MMS_STRING, ApnSetting.TYPE_MMS);
        EXPECTED_STRING_TO_INT_MAP.put(ApnSetting.TYPE_SUPL_STRING, ApnSetting.TYPE_SUPL);
        EXPECTED_STRING_TO_INT_MAP.put(ApnSetting.TYPE_DUN_STRING, ApnSetting.TYPE_DUN);
        EXPECTED_STRING_TO_INT_MAP.put(ApnSetting.TYPE_HIPRI_STRING, ApnSetting.TYPE_HIPRI);
        EXPECTED_STRING_TO_INT_MAP.put(ApnSetting.TYPE_FOTA_STRING, ApnSetting.TYPE_FOTA);
        EXPECTED_STRING_TO_INT_MAP.put(ApnSetting.TYPE_IMS_STRING, ApnSetting.TYPE_IMS);
        EXPECTED_STRING_TO_INT_MAP.put(ApnSetting.TYPE_CBS_STRING, ApnSetting.TYPE_CBS);
        EXPECTED_STRING_TO_INT_MAP.put(ApnSetting.TYPE_IA_STRING, ApnSetting.TYPE_IA);
        EXPECTED_STRING_TO_INT_MAP.put(ApnSetting.TYPE_EMERGENCY_STRING, ApnSetting.TYPE_EMERGENCY);
        EXPECTED_STRING_TO_INT_MAP.put(ApnSetting.TYPE_MCX_STRING, ApnSetting.TYPE_MCX);
        EXPECTED_STRING_TO_INT_MAP.put(ApnSetting.TYPE_XCAP_STRING, ApnSetting.TYPE_XCAP);

        EXPECTED_INT_TO_STRING_MAP = new ArrayMap<>();
        EXPECTED_INT_TO_STRING_MAP.put(ApnSetting.TYPE_DEFAULT, ApnSetting.TYPE_DEFAULT_STRING);
        EXPECTED_INT_TO_STRING_MAP.put(ApnSetting.TYPE_MMS, ApnSetting.TYPE_MMS_STRING);
        EXPECTED_INT_TO_STRING_MAP.put(ApnSetting.TYPE_SUPL, ApnSetting.TYPE_SUPL_STRING);
        EXPECTED_INT_TO_STRING_MAP.put(ApnSetting.TYPE_DUN, ApnSetting.TYPE_DUN_STRING);
        EXPECTED_INT_TO_STRING_MAP.put(ApnSetting.TYPE_HIPRI, ApnSetting.TYPE_HIPRI_STRING);
        EXPECTED_INT_TO_STRING_MAP.put(ApnSetting.TYPE_FOTA, ApnSetting.TYPE_FOTA_STRING);
        EXPECTED_INT_TO_STRING_MAP.put(ApnSetting.TYPE_IMS, ApnSetting.TYPE_IMS_STRING);
        EXPECTED_INT_TO_STRING_MAP.put(ApnSetting.TYPE_CBS, ApnSetting.TYPE_CBS_STRING);
        EXPECTED_INT_TO_STRING_MAP.put(ApnSetting.TYPE_IA, ApnSetting.TYPE_IA_STRING);
        EXPECTED_INT_TO_STRING_MAP.put(ApnSetting.TYPE_EMERGENCY, ApnSetting.TYPE_EMERGENCY_STRING);
        EXPECTED_INT_TO_STRING_MAP.put(ApnSetting.TYPE_MCX, ApnSetting.TYPE_MCX_STRING);
        EXPECTED_INT_TO_STRING_MAP.put(ApnSetting.TYPE_XCAP, ApnSetting.TYPE_XCAP_STRING);
    }

    @Test
    public void testIntToString() {
        for (Map.Entry<Integer, String> e : EXPECTED_INT_TO_STRING_MAP.entrySet()) {
            assertEquals(e.getValue(), ApnSetting.getApnTypeString(e.getKey()));
        }
    }

    @Test
    public void testStringToInt() {
        for (Map.Entry<String, Integer> e : EXPECTED_STRING_TO_INT_MAP.entrySet()) {
            assertEquals((int) e.getValue(), ApnSetting.getApnTypeInt(e.getKey()));
        }
    }
}
