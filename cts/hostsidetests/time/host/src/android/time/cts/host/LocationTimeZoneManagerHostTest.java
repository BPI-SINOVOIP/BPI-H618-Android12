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

package android.time.cts.host;


import static android.app.time.cts.shell.LocationTimeZoneManagerShellHelper.PROVIDER_MODE_DISABLED;
import static android.app.time.cts.shell.LocationTimeZoneManagerShellHelper.PROVIDER_MODE_SIMULATED;
import static android.app.time.cts.shell.LocationTimeZoneManagerShellHelper.PRIMARY_PROVIDER_INDEX;
import static android.app.time.cts.shell.LocationTimeZoneManagerShellHelper.SECONDARY_PROVIDER_INDEX;

import static org.junit.Assert.assertEquals;

import android.app.time.LocationTimeZoneManagerServiceStateProto;
import android.app.time.TimeZoneProviderStateEnum;
import android.app.time.TimeZoneProviderStateProto;

import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.List;

/** Host-side CTS tests for the location time zone manager service. */
@RunWith(DeviceJUnit4ClassRunner.class)
public class LocationTimeZoneManagerHostTest extends BaseLocationTimeZoneManagerHostTest {

    @Test
    public void testSecondarySuggestion() throws Exception {
        setProviderModeOverride(PRIMARY_PROVIDER_INDEX, PROVIDER_MODE_DISABLED);
        setProviderModeOverride(SECONDARY_PROVIDER_INDEX, PROVIDER_MODE_SIMULATED);
        startLocationTimeZoneManagerService();
        setLocationTimeZoneManagerStateRecordingMode(true);

        simulateProviderBind(SECONDARY_PROVIDER_INDEX);
        simulateProviderSuggestion(SECONDARY_PROVIDER_INDEX, "Europe/London");

        LocationTimeZoneManagerServiceStateProto serviceState =
                dumpLocationTimeZoneManagerServiceState();
        assertEquals(Arrays.asList("Europe/London"),
                serviceState.getLastSuggestion().getZoneIdsList());

        List<TimeZoneProviderStateProto> secondaryStates =
                serviceState.getSecondaryProviderStatesList();
        assertEquals(1, secondaryStates.size());
        assertEquals(TimeZoneProviderStateEnum.TIME_ZONE_PROVIDER_STATE_CERTAIN,
                secondaryStates.get(0).getState());
    }
}
