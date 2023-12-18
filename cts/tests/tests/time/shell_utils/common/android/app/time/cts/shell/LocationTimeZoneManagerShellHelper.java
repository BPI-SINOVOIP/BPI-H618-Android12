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
package android.app.time.cts.shell;

import static android.app.time.cts.shell.DeviceConfigKeys.LocationTimeZoneManager.KEY_PRIMARY_LOCATION_TIME_ZONE_PROVIDER_MODE_OVERRIDE;
import static android.app.time.cts.shell.DeviceConfigKeys.LocationTimeZoneManager.KEY_SECONDARY_LOCATION_TIME_ZONE_PROVIDER_MODE_OVERRIDE;
import static android.app.time.cts.shell.DeviceConfigKeys.NAMESPACE_SYSTEM_TIME;

import static org.junit.Assume.assumeTrue;

import java.io.BufferedReader;
import java.io.StringReader;
import java.util.Objects;

/**
 * A class for interacting with the {@code location_time_zone_manager} service via the shell "cmd"
 * command-line interface.
 */
public class LocationTimeZoneManagerShellHelper {
    /**
     * The index of the primary location time zone provider, used for shell commands.
     */
    public static final int PRIMARY_PROVIDER_INDEX = 0;

    /**
     * The index of the secondary location time zone provider, used for shell commands.
     */
    public static final int SECONDARY_PROVIDER_INDEX = 1;

    /**
     * The "disabled" provider mode (equivalent to there being no provider configured).
     */
    public static final String PROVIDER_MODE_DISABLED =
            DeviceConfigKeys.LocationTimeZoneManager.PROVIDER_MODE_DISABLED;

    /**
     * The "simulated" provider mode.
     */
    public static final String PROVIDER_MODE_SIMULATED =
            DeviceConfigKeys.LocationTimeZoneManager.PROVIDER_MODE_SIMULATED;

    /**
     * Simulated provider test command that simulates the bind succeeding.
     */
    public static final String SIMULATED_PROVIDER_TEST_COMMAND_ON_BIND = "on_bind";

    /**
     * Simulated provider test command that simulates the provider reporting uncertainty.
     */
    public static final String SIMULATED_PROVIDER_TEST_COMMAND_UNCERTAIN = "uncertain";

    /**
     * Simulated provider test command that simulates a successful time zone detection.
     */
    public static final String SIMULATED_PROVIDER_TEST_COMMAND_SUCCESS = "success";

    /**
     * Argument for {@link #SIMULATED_PROVIDER_TEST_COMMAND_SUCCESS} to specify TZDB time zone IDs.
     */
    public static final String SIMULATED_PROVIDER_TEST_COMMAND_SUCCESS_ARG_KEY_TZ = "tz";

    /**
     * The name of the service for shell commands.
     */
    private static final String SERVICE_NAME = "location_time_zone_manager";

    /**
     * A shell command that starts the service (after stop).
     */
    private static final String SHELL_COMMAND_START = "start";

    /**
     * A shell command that stops the service.
     */
    private static final String SHELL_COMMAND_STOP = "stop";

    /**
     * A shell command that tells the service to record state information during tests. The next
     * argument value is "true" or "false".
     */
    private static final String SHELL_COMMAND_RECORD_PROVIDER_STATES = "record_provider_states";

    /**
     * A shell command that tells the service to dump its current state.
     */
    private static final String SHELL_COMMAND_DUMP_STATE = "dump_state";

    /**
     * Option for {@link #SHELL_COMMAND_DUMP_STATE} that tells it to dump state as a binary proto.
     */
    private static final String DUMP_STATE_OPTION_PROTO = "proto";

    /**
     * A shell command that sends test commands to a provider
     */
    private static final String SHELL_COMMAND_SEND_PROVIDER_TEST_COMMAND =
            "send_provider_test_command";

    private static final String SHELL_CMD_PREFIX = "cmd " + SERVICE_NAME + " ";

    private final DeviceShellCommandExecutor mShellCommandExecutor;
    private final DeviceConfigShellHelper mDeviceConfigShellHelper;

    public LocationTimeZoneManagerShellHelper(DeviceShellCommandExecutor shellCommandExecutor) {
        mShellCommandExecutor = Objects.requireNonNull(shellCommandExecutor);
        mDeviceConfigShellHelper = new DeviceConfigShellHelper(shellCommandExecutor);
    }

    /**
     * Throws an {@link org.junit.AssumptionViolatedException} if the location_time_zone_manager
     * service is not found. The service can be turned off in config, so this can be used to prevent
     * CTS tests that need it from running.
     */
    public void assumeLocationTimeZoneManagerIsPresent() throws Exception {
        assumeTrue(isLocationTimeZoneManagerPresent());
    }

    /**
     * Returns {@code false} if the location_time_zone_manager service is not found.
     */
    public boolean isLocationTimeZoneManagerPresent() throws Exception {
        // Look for the service name in "cmd -l".
        String serviceList = mShellCommandExecutor.executeToString("cmd -l");
        try (BufferedReader reader = new BufferedReader(new StringReader(serviceList))) {
            String serviceName;
            while ((serviceName = reader.readLine()) != null) {
                serviceName = serviceName.trim();
                if (SERVICE_NAME.equals(serviceName)) {
                    return true;
                }
            }
            return false;
        }
    }

    /** Executes "start". Starts the service. */
    public void start() throws Exception {
        mShellCommandExecutor.executeToTrimmedString(SHELL_CMD_PREFIX + SHELL_COMMAND_START);
    }

    /** Executes "stop". Stops the service. */
    public void stop() throws Exception {
        mShellCommandExecutor.executeToTrimmedString(SHELL_CMD_PREFIX + SHELL_COMMAND_STOP);
    }

    /** Executes "record_provider_states". */
    public void recordProviderStates(boolean enabled) throws Exception {
        String cmd = String.format("%s %s", SHELL_COMMAND_RECORD_PROVIDER_STATES, enabled);
        mShellCommandExecutor.executeToTrimmedString(SHELL_CMD_PREFIX + cmd);
    }

    /** Executes "dump_state". */
    public byte[] dumpState() throws Exception {
        String cmd = String.format("%s --%s", SHELL_COMMAND_DUMP_STATE, DUMP_STATE_OPTION_PROTO);
        return mShellCommandExecutor.executeToBytes(SHELL_CMD_PREFIX + cmd);
    }

    /** Modifies a provider's mode using "device_config" commands. */
    public void setProviderModeOverride(int providerIndex, String mode) throws Exception {
        String deviceConfigKey;
        if (providerIndex == PRIMARY_PROVIDER_INDEX) {
            deviceConfigKey = KEY_PRIMARY_LOCATION_TIME_ZONE_PROVIDER_MODE_OVERRIDE;
        } else {
            deviceConfigKey = KEY_SECONDARY_LOCATION_TIME_ZONE_PROVIDER_MODE_OVERRIDE;
        }

        if (mode == null) {
            mDeviceConfigShellHelper.delete(NAMESPACE_SYSTEM_TIME, deviceConfigKey);
        } else {
            mDeviceConfigShellHelper.put(NAMESPACE_SYSTEM_TIME, deviceConfigKey, mode);
        }
    }

    /**
     * Simulates a provider successfully binding using the "send_provider_test_command" command.
     */
    public void simulateProviderBind(int providerIndex) throws Exception {
        sendProviderTestCommand(providerIndex, SIMULATED_PROVIDER_TEST_COMMAND_ON_BIND);
    }

    /**
     * Simulates a provider generating an uncertain report using the "send_provider_test_command"
     * command.
     */
    public void simulateProviderUncertain(int providerIndex) throws Exception {
        sendProviderTestCommand(providerIndex, SIMULATED_PROVIDER_TEST_COMMAND_UNCERTAIN);
    }

    /**
     * Simulates a provider generating a suggestion using the "send_provider_test_command" command.
     */
    public void simulateProviderSuggestion(int providerIndex, String... zoneIds)
            throws Exception {
        String timeZoneIds = String.join("&", zoneIds);
        String testCommand = String.format("%s %s=string_array:%s",
                SIMULATED_PROVIDER_TEST_COMMAND_SUCCESS,
                SIMULATED_PROVIDER_TEST_COMMAND_SUCCESS_ARG_KEY_TZ,
                timeZoneIds);
        sendProviderTestCommand(providerIndex, testCommand);
    }

    /** Executes "send_provider_test_command". */
    private void sendProviderTestCommand(int providerIndex, String testCommand) throws Exception {
        String cmd = String.format("%s %s %s",
                SHELL_COMMAND_SEND_PROVIDER_TEST_COMMAND, providerIndex, testCommand);
        mShellCommandExecutor.executeToTrimmedString(SHELL_CMD_PREFIX + cmd);
    }
}
