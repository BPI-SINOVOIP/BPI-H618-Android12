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

package android.car.cts;

import static com.google.common.truth.Truth.assertThat;

import android.car.VehiclePropertyIds;
import android.platform.test.annotations.RequiresDevice;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Log;

import androidx.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;

@SmallTest
@RequiresDevice
@RunWith(AndroidJUnit4.class)
public class VehiclePropertyIdsTest {
    private static final String TAG = "VehiclePropertyIdsTest";

    // Get all enums from the class.
    private static List<Integer> getIntegersFromDataEnums() {
        Field[] fields = VehiclePropertyIds.class.getDeclaredFields();
        List<Integer> integerList = new ArrayList<>(5);
        for (Field f : fields) {
            if (f.getType() == int.class) {
                try {
                    integerList.add(f.getInt(VehiclePropertyIds.class));
                } catch (IllegalAccessException | RuntimeException e) {
                    Log.w(TAG, "Failed to get value");
                }
            }
        }
        return integerList;
    }

    /**
     * Test for {@link VehiclePropertyIds#toString()}
     */
    @Test
    public void testToString() {

        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.INVALID))
                .isEqualTo("INVALID");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.IGNITION_STATE))
                .isEqualTo("IGNITION_STATE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.INFO_DRIVER_SEAT))
                .isEqualTo("INFO_DRIVER_SEAT");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.INFO_EV_BATTERY_CAPACITY))
                .isEqualTo("INFO_EV_BATTERY_CAPACITY");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.INFO_EV_CONNECTOR_TYPE))
                .isEqualTo("INFO_EV_CONNECTOR_TYPE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.INFO_EV_PORT_LOCATION))
                .isEqualTo("INFO_EV_PORT_LOCATION");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.INFO_EXTERIOR_DIMENSIONS))
                .isEqualTo("INFO_EXTERIOR_DIMENSIONS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.INFO_FUEL_CAPACITY))
                .isEqualTo("INFO_FUEL_CAPACITY");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.INFO_FUEL_DOOR_LOCATION))
                .isEqualTo("INFO_FUEL_DOOR_LOCATION");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.INFO_FUEL_TYPE))
                .isEqualTo("INFO_FUEL_TYPE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.INFO_MAKE))
                .isEqualTo("INFO_MAKE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.INFO_MODEL))
                .isEqualTo("INFO_MODEL");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.INFO_MODEL_YEAR))
                .isEqualTo("INFO_MODEL_YEAR");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.INFO_MULTI_EV_PORT_LOCATIONS))
                .isEqualTo("INFO_MULTI_EV_PORT_LOCATIONS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.INFO_VIN))
                .isEqualTo("INFO_VIN");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.PERF_ODOMETER))
                .isEqualTo("PERF_ODOMETER");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.PERF_REAR_STEERING_ANGLE))
                .isEqualTo("PERF_REAR_STEERING_ANGLE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.PERF_STEERING_ANGLE))
                .isEqualTo("PERF_STEERING_ANGLE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.PERF_VEHICLE_SPEED))
                .isEqualTo("PERF_VEHICLE_SPEED");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.PERF_VEHICLE_SPEED_DISPLAY))
                .isEqualTo("PERF_VEHICLE_SPEED_DISPLAY");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.ENGINE_COOLANT_TEMP))
                .isEqualTo("ENGINE_COOLANT_TEMP");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.ENGINE_OIL_LEVEL))
                .isEqualTo("ENGINE_OIL_LEVEL");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.ENGINE_OIL_TEMP))
                .isEqualTo("ENGINE_OIL_TEMP");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.ENGINE_RPM))
                .isEqualTo("ENGINE_RPM");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.WHEEL_TICK))
                .isEqualTo("WHEEL_TICK");
        assertThat(VehiclePropertyIds.toString(
                VehiclePropertyIds.FUEL_CONSUMPTION_UNITS_DISTANCE_OVER_VOLUME))
                .isEqualTo("FUEL_CONSUMPTION_UNITS_DISTANCE_OVER_VOLUME");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.FUEL_DOOR_OPEN))
                .isEqualTo("FUEL_DOOR_OPEN");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.FUEL_LEVEL))
                .isEqualTo("FUEL_LEVEL");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.FUEL_LEVEL_LOW))
                .isEqualTo("FUEL_LEVEL_LOW");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.FUEL_VOLUME_DISPLAY_UNITS))
                .isEqualTo("FUEL_VOLUME_DISPLAY_UNITS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.EV_BATTERY_DISPLAY_UNITS))
                .isEqualTo("EV_BATTERY_DISPLAY_UNITS");
        assertThat(VehiclePropertyIds.toString(
                VehiclePropertyIds.EV_BATTERY_INSTANTANEOUS_CHARGE_RATE))
                .isEqualTo("EV_BATTERY_INSTANTANEOUS_CHARGE_RATE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.EV_BATTERY_LEVEL))
                .isEqualTo("EV_BATTERY_LEVEL");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.EV_CHARGE_PORT_CONNECTED))
                .isEqualTo("EV_CHARGE_PORT_CONNECTED");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.EV_CHARGE_PORT_OPEN))
                .isEqualTo("EV_CHARGE_PORT_OPEN");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.RANGE_REMAINING))
                .isEqualTo("RANGE_REMAINING");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.TIRE_PRESSURE)).
                isEqualTo("TIRE_PRESSURE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.TIRE_PRESSURE_DISPLAY_UNITS))
                .isEqualTo("TIRE_PRESSURE_DISPLAY_UNITS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.GEAR_SELECTION))
                .isEqualTo("GEAR_SELECTION");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.CURRENT_GEAR))
                .isEqualTo("CURRENT_GEAR");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.PARKING_BRAKE_ON))
                .isEqualTo("PARKING_BRAKE_ON");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.PARKING_BRAKE_AUTO_APPLY))
                .isEqualTo("PARKING_BRAKE_AUTO_APPLY");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.NIGHT_MODE))
                .isEqualTo("NIGHT_MODE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.TURN_SIGNAL_STATE))
                .isEqualTo("TURN_SIGNAL_STATE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.ABS_ACTIVE))
                .isEqualTo("ABS_ACTIVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.TRACTION_CONTROL_ACTIVE))
                .isEqualTo("TRACTION_CONTROL_ACTIVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_AC_ON))
                .isEqualTo("HVAC_AC_ON");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_ACTUAL_FAN_SPEED_RPM))
                .isEqualTo("HVAC_ACTUAL_FAN_SPEED_RPM");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_AUTO_ON))
                .isEqualTo("HVAC_AUTO_ON");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_AUTO_RECIRC_ON))
                .isEqualTo("HVAC_AUTO_RECIRC_ON");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_DEFROSTER))
                .isEqualTo("HVAC_DEFROSTER");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_DUAL_ON))
                .isEqualTo("HVAC_DUAL_ON");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_FAN_DIRECTION))
                .isEqualTo("HVAC_FAN_DIRECTION");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_FAN_DIRECTION_AVAILABLE))
                .isEqualTo("HVAC_FAN_DIRECTION_AVAILABLE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_FAN_SPEED))
                .isEqualTo("HVAC_FAN_SPEED");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_MAX_AC_ON))
                .isEqualTo("HVAC_MAX_AC_ON");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_MAX_DEFROST_ON))
                .isEqualTo("HVAC_MAX_DEFROST_ON");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_POWER_ON))
                .isEqualTo("HVAC_POWER_ON");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_RECIRC_ON))
                .isEqualTo("HVAC_RECIRC_ON");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_SEAT_TEMPERATURE))
                .isEqualTo("HVAC_SEAT_TEMPERATURE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_SEAT_VENTILATION))
                .isEqualTo("HVAC_SEAT_VENTILATION");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_SIDE_MIRROR_HEAT))
                .isEqualTo("HVAC_SIDE_MIRROR_HEAT");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_STEERING_WHEEL_HEAT))
                .isEqualTo("HVAC_STEERING_WHEEL_HEAT");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_TEMPERATURE_CURRENT))
                .isEqualTo("HVAC_TEMPERATURE_CURRENT");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_TEMPERATURE_DISPLAY_UNITS))
                .isEqualTo("HVAC_TEMPERATURE_DISPLAY_UNITS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HVAC_TEMPERATURE_SET))
                .isEqualTo("HVAC_TEMPERATURE_SET");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.DISTANCE_DISPLAY_UNITS))
                .isEqualTo("DISTANCE_DISPLAY_UNITS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.ENV_OUTSIDE_TEMPERATURE))
                .isEqualTo("ENV_OUTSIDE_TEMPERATURE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.AP_POWER_BOOTUP_REASON))
                .isEqualTo("AP_POWER_BOOTUP_REASON");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.AP_POWER_STATE_REPORT))
                .isEqualTo("AP_POWER_STATE_REPORT");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.AP_POWER_STATE_REQ))
                .isEqualTo("AP_POWER_STATE_REQ");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.DISPLAY_BRIGHTNESS))
                .isEqualTo("DISPLAY_BRIGHTNESS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HW_KEY_INPUT))
                .isEqualTo("HW_KEY_INPUT");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.DOOR_LOCK))
                .isEqualTo("DOOR_LOCK");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.DOOR_MOVE))
                .isEqualTo("DOOR_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.DOOR_POS))
                .isEqualTo("DOOR_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.MIRROR_FOLD))
                .isEqualTo("MIRROR_FOLD");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.MIRROR_LOCK))
                .isEqualTo("MIRROR_LOCK");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.MIRROR_Y_MOVE))
                .isEqualTo("MIRROR_Y_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.MIRROR_Y_POS))
                .isEqualTo("MIRROR_Y_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.MIRROR_Z_MOVE))
                .isEqualTo("MIRROR_Z_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.MIRROR_Z_POS))
                .isEqualTo("MIRROR_Z_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_BACKREST_ANGLE_1_MOVE))
                .isEqualTo("SEAT_BACKREST_ANGLE_1_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_BACKREST_ANGLE_1_POS))
                .isEqualTo("SEAT_BACKREST_ANGLE_1_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_BACKREST_ANGLE_2_MOVE))
                .isEqualTo("SEAT_BACKREST_ANGLE_2_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_BACKREST_ANGLE_2_POS))
                .isEqualTo("SEAT_BACKREST_ANGLE_2_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_BELT_BUCKLED))
                .isEqualTo("SEAT_BELT_BUCKLED");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_BELT_HEIGHT_MOVE))
                .isEqualTo("SEAT_BELT_HEIGHT_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_BELT_HEIGHT_POS))
                .isEqualTo("SEAT_BELT_HEIGHT_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_DEPTH_MOVE))
                .isEqualTo("SEAT_DEPTH_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_DEPTH_POS))
                .isEqualTo("SEAT_DEPTH_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_FORE_AFT_MOVE))
                .isEqualTo("SEAT_FORE_AFT_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_FORE_AFT_POS))
                .isEqualTo("SEAT_FORE_AFT_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_HEADREST_ANGLE_MOVE))
                .isEqualTo("SEAT_HEADREST_ANGLE_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_HEADREST_ANGLE_POS))
                .isEqualTo("SEAT_HEADREST_ANGLE_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_HEADREST_FORE_AFT_MOVE))
                .isEqualTo("SEAT_HEADREST_FORE_AFT_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_HEADREST_FORE_AFT_POS))
                .isEqualTo("SEAT_HEADREST_FORE_AFT_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_HEADREST_HEIGHT_MOVE))
                .isEqualTo("SEAT_HEADREST_HEIGHT_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_HEADREST_HEIGHT_POS))
                .isEqualTo("SEAT_HEADREST_HEIGHT_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_HEIGHT_MOVE))
                .isEqualTo("SEAT_HEIGHT_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_HEIGHT_POS))
                .isEqualTo("SEAT_HEIGHT_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_LUMBAR_FORE_AFT_MOVE))
                .isEqualTo("SEAT_LUMBAR_FORE_AFT_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_LUMBAR_FORE_AFT_POS))
                .isEqualTo("SEAT_LUMBAR_FORE_AFT_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_LUMBAR_SIDE_SUPPORT_MOVE))
                .isEqualTo("SEAT_LUMBAR_SIDE_SUPPORT_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_LUMBAR_SIDE_SUPPORT_POS))
                .isEqualTo("SEAT_LUMBAR_SIDE_SUPPORT_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_MEMORY_SELECT))
                .isEqualTo("SEAT_MEMORY_SELECT");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_MEMORY_SET))
                .isEqualTo("SEAT_MEMORY_SET");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_OCCUPANCY))
                .isEqualTo("SEAT_OCCUPANCY");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_TILT_MOVE))
                .isEqualTo("SEAT_TILT_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.SEAT_TILT_POS))
                .isEqualTo("SEAT_TILT_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.WINDOW_LOCK))
                .isEqualTo("WINDOW_LOCK");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.WINDOW_MOVE))
                .isEqualTo("WINDOW_MOVE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.WINDOW_POS))
                .isEqualTo("WINDOW_POS");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.VEHICLE_MAP_SERVICE))
                .isEqualTo("VEHICLE_MAP_SERVICE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.OBD2_FREEZE_FRAME))
                .isEqualTo("OBD2_FREEZE_FRAME");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.OBD2_FREEZE_FRAME_CLEAR))
                .isEqualTo("OBD2_FREEZE_FRAME_CLEAR");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.OBD2_FREEZE_FRAME_INFO))
                .isEqualTo("OBD2_FREEZE_FRAME_INFO");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.OBD2_LIVE_FRAME))
                .isEqualTo("OBD2_LIVE_FRAME");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HEADLIGHTS_STATE))
                .isEqualTo("HEADLIGHTS_STATE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HEADLIGHTS_SWITCH))
                .isEqualTo("HEADLIGHTS_SWITCH");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HIGH_BEAM_LIGHTS_STATE))
                .isEqualTo("HIGH_BEAM_LIGHTS_STATE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HIGH_BEAM_LIGHTS_SWITCH))
                .isEqualTo("HIGH_BEAM_LIGHTS_SWITCH");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.FOG_LIGHTS_STATE))
                .isEqualTo("FOG_LIGHTS_STATE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.FOG_LIGHTS_SWITCH))
                .isEqualTo("FOG_LIGHTS_SWITCH");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HAZARD_LIGHTS_STATE))
                .isEqualTo("HAZARD_LIGHTS_STATE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.HAZARD_LIGHTS_SWITCH))
                .isEqualTo("HAZARD_LIGHTS_SWITCH");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.CABIN_LIGHTS_STATE))
                .isEqualTo("CABIN_LIGHTS_STATE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.CABIN_LIGHTS_SWITCH))
                .isEqualTo("CABIN_LIGHTS_SWITCH");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.READING_LIGHTS_STATE))
                .isEqualTo("READING_LIGHTS_STATE");
        assertThat(VehiclePropertyIds.toString(VehiclePropertyIds.READING_LIGHTS_SWITCH))
                .isEqualTo("READING_LIGHTS_SWITCH");
        assertThat(VehiclePropertyIds.toString(3)).isEqualTo("0x3");
        assertThat(VehiclePropertyIds.toString(12)).isEqualTo("0xc");
    }

    /**
     * Test if all system properties have a mapped string value.
     */
    @Test
    public void testAllPropertiesAreMappedInToString() {
        List<Integer> systemProperties = getIntegersFromDataEnums();
        for (int propertyId : systemProperties) {
            String propertyString = VehiclePropertyIds.toString(propertyId);
            assertThat(propertyString.startsWith("0x")).isFalse();
        }
    }
}
