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
package android.time.cts.host;

import static android.app.time.cts.shell.DeviceConfigKeys.NAMESPACE_SYSTEM_TIME;
import static android.app.time.cts.shell.LocationTimeZoneManagerShellHelper.PRIMARY_PROVIDER_INDEX;
import static android.app.time.cts.shell.LocationTimeZoneManagerShellHelper.PROVIDER_MODE_SIMULATED;
import static android.app.time.cts.shell.LocationTimeZoneManagerShellHelper.SECONDARY_PROVIDER_INDEX;

import android.app.time.LocationTimeZoneManagerServiceStateProto;
import android.app.time.cts.shell.DeviceConfigShellHelper;
import android.app.time.cts.shell.DeviceShellCommandExecutor;
import android.app.time.cts.shell.LocationShellHelper;
import android.app.time.cts.shell.LocationTimeZoneManagerShellHelper;
import android.app.time.cts.shell.TimeZoneDetectorShellHelper;
import android.app.time.cts.shell.host.HostShellCommandExecutor;

import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;
import com.google.protobuf.Parser;

import org.junit.After;
import org.junit.Before;

/** A base class for tests that interact with the location_time_zone_manager via adb. */
public abstract class BaseLocationTimeZoneManagerHostTest extends BaseHostJUnit4Test {

    private boolean mOriginalLocationEnabled;

    private boolean mOriginalAutoDetectionEnabled;

    private boolean mOriginalGeoDetectionEnabled;

    protected TimeZoneDetectorShellHelper mTimeZoneDetectorShellHelper;
    private LocationTimeZoneManagerShellHelper mLocationTimeZoneManagerShellHelper;
    private LocationShellHelper mLocationShellHelper;
    private DeviceConfigShellHelper mDeviceConfigShellHelper;
    private DeviceConfigShellHelper.PreTestState mDeviceConfigPreTestState;

    @Before
    public void setUp() throws Exception {
        DeviceShellCommandExecutor shellCommandExecutor = new HostShellCommandExecutor(getDevice());
        mTimeZoneDetectorShellHelper = new TimeZoneDetectorShellHelper(shellCommandExecutor);
        mLocationTimeZoneManagerShellHelper =
                new LocationTimeZoneManagerShellHelper(shellCommandExecutor);
        mLocationShellHelper = new LocationShellHelper(shellCommandExecutor);
        mDeviceConfigShellHelper = new DeviceConfigShellHelper(shellCommandExecutor);

        // Confirm the service being tested is present. It can be turned off, in which case there's
        // nothing to test.
        mLocationTimeZoneManagerShellHelper.assumeLocationTimeZoneManagerIsPresent();

        // All tests start with the location_time_zone_manager disabled so that providers can be
        // configured.
        stopLocationTimeZoneManagerService();

        mDeviceConfigPreTestState = mDeviceConfigShellHelper.setSyncModeForTest(
                DeviceConfigShellHelper.SYNC_DISABLED_MODE_UNTIL_REBOOT, NAMESPACE_SYSTEM_TIME);

        // Configure two simulated providers. At least one is needed to be able to turn on
        // geo detection below. Tests may override these values for their own use.
        setProviderModeOverride(PRIMARY_PROVIDER_INDEX, PROVIDER_MODE_SIMULATED);
        setProviderModeOverride(SECONDARY_PROVIDER_INDEX, PROVIDER_MODE_SIMULATED);

        // Make sure locations is enabled, otherwise the geo detection feature will be disabled
        // whatever the geolocation detection setting is set to.
        mOriginalLocationEnabled = mLocationShellHelper.isLocationEnabledForCurrentUser();
        if (!mOriginalLocationEnabled) {
            mLocationShellHelper.setLocationEnabledForCurrentUser(true);
        }

        // Make sure automatic time zone detection is enabled, otherwise the geo detection feature
        // will be disabled whatever the geolocation detection setting is set to
        mOriginalAutoDetectionEnabled = mTimeZoneDetectorShellHelper.isAutoDetectionEnabled();
        if (!mOriginalAutoDetectionEnabled) {
            mTimeZoneDetectorShellHelper.setAutoDetectionEnabled(true);
        }

        // Make sure geolocation time zone detection is enabled.
        mOriginalGeoDetectionEnabled = mTimeZoneDetectorShellHelper.isGeoDetectionEnabled();
        if (!mOriginalGeoDetectionEnabled) {
            mTimeZoneDetectorShellHelper.setGeoDetectionEnabled(true);
        }
    }

    @After
    public void tearDown() throws Exception {
        if (!mLocationTimeZoneManagerShellHelper.isLocationTimeZoneManagerPresent()) {
            // Setup didn't do anything, no need to tearDown.
            return;
        }
        // Turn off the service before we reset configuration, otherwise it will restart itself
        // repeatedly.
        stopLocationTimeZoneManagerService();

        // Reset settings and server flags as best we can.
        mTimeZoneDetectorShellHelper.setGeoDetectionEnabled(mOriginalGeoDetectionEnabled);
        mTimeZoneDetectorShellHelper.setAutoDetectionEnabled(mOriginalAutoDetectionEnabled);
        mLocationShellHelper.setLocationEnabledForCurrentUser(mOriginalLocationEnabled);
        setLocationTimeZoneManagerStateRecordingMode(false);

        mDeviceConfigShellHelper.restoreDeviceConfigStateForTest(mDeviceConfigPreTestState);

        // Attempt to start the service. It may not start if there are no providers configured,
        // but that is ok.
        startLocationTimeZoneManagerService();
    }

    protected LocationTimeZoneManagerServiceStateProto dumpLocationTimeZoneManagerServiceState()
            throws Exception {
        byte[] protoBytes = mLocationTimeZoneManagerShellHelper.dumpState();
        Parser<LocationTimeZoneManagerServiceStateProto> parser =
                LocationTimeZoneManagerServiceStateProto.parser();
        return parser.parseFrom(protoBytes);
    }

    protected void setLocationTimeZoneManagerStateRecordingMode(boolean enabled) throws Exception {
        mLocationTimeZoneManagerShellHelper.recordProviderStates(enabled);
    }

    protected void startLocationTimeZoneManagerService() throws Exception {
        mLocationTimeZoneManagerShellHelper.start();
    }

    protected void stopLocationTimeZoneManagerService() throws Exception {
        mLocationTimeZoneManagerShellHelper.stop();
    }

    protected void setProviderModeOverride(int providerIndex, String mode) throws Exception {
        mLocationTimeZoneManagerShellHelper.setProviderModeOverride(providerIndex, mode);
    }

    protected void simulateProviderSuggestion(int providerIndex, String... zoneIds)
            throws Exception {
        mLocationTimeZoneManagerShellHelper.simulateProviderSuggestion(providerIndex, zoneIds);
    }

    protected void simulateProviderBind(int providerIndex) throws Exception {
        mLocationTimeZoneManagerShellHelper.simulateProviderBind(providerIndex);
    }
}
