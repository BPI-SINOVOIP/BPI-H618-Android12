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

package com.android.car.dialer.testing;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.net.Uri;
import android.telecom.Call;
import android.telecom.GatewayInfo;

/** Helper class to create mock entities for testing. */
public final class MockEntityFactory {

    /** Create a mock {@link Call.Details} with given number. */
    public static Call.Details createMockCallDetails(String number) {
        Call.Details callDetails = mock(Call.Details.class);
        Uri uri = Uri.fromParts("tel", number, null);
        GatewayInfo gatewayInfo = new GatewayInfo("", uri, uri);
        when(callDetails.getHandle()).thenReturn(uri);
        when(callDetails.getGatewayInfo()).thenReturn(gatewayInfo);
        return callDetails;
    }
}
