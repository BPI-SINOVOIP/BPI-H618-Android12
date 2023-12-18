/*
 * Copyright (C) 2019 The Android Open Source Project
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
import static com.google.common.truth.Truth.assertWithMessage;

import static org.testng.Assert.assertThrows;

import android.car.Car;
import android.car.EvConnectorType;
import android.car.FuelType;
import android.car.PortLocationType;
import android.car.VehicleAreaSeat;
import android.car.VehicleAreaType;
import android.car.VehicleAreaWheel;
import android.car.VehicleGear;
import android.car.VehiclePropertyIds;
import android.car.cts.utils.VehiclePropertyVerifier;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.property.CarPropertyManager;
import android.car.hardware.property.CarPropertyManager.CarPropertyEventCallback;
import android.car.hardware.property.VehicleElectronicTollCollectionCardStatus;
import android.car.hardware.property.VehicleElectronicTollCollectionCardType;
import android.platform.test.annotations.AppModeFull;
import android.platform.test.annotations.RequiresDevice;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.ArraySet;
import android.util.SparseArray;

import androidx.annotation.GuardedBy;
import androidx.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.CddTest;

import com.google.common.collect.ImmutableSet;

import org.junit.Assert;
import org.junit.Assume;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

@SmallTest
@RequiresDevice
@RunWith(AndroidJUnit4.class)
@AppModeFull(reason = "Instant apps cannot get car related permissions.")
public class CarPropertyManagerTest extends CarApiTestBase {

    private static final long WAIT_CALLBACK = 1500L;
    private static final int NO_EVENTS = 0;
    private static final int ONCHANGE_RATE_EVENT_COUNTER = 1;
    private static final int UI_RATE_EVENT_COUNTER = 5;
    private static final int FAST_OR_FASTEST_EVENT_COUNTER = 10;
    private static final ImmutableSet<Integer> PORT_LOCATION_TYPES =
            ImmutableSet.<Integer>builder().add(PortLocationType.UNKNOWN,
                    PortLocationType.FRONT_LEFT, PortLocationType.FRONT_RIGHT,
                    PortLocationType.REAR_RIGHT, PortLocationType.REAR_LEFT,
                    PortLocationType.FRONT, PortLocationType.REAR).build();
    private static final ImmutableSet<Integer> VEHICLE_GEARS =
            ImmutableSet.<Integer>builder().add(VehicleGear.GEAR_UNKNOWN,
                    VehicleGear.GEAR_NEUTRAL, VehicleGear.GEAR_REVERSE,
                    VehicleGear.GEAR_PARK, VehicleGear.GEAR_DRIVE,
                    VehicleGear.GEAR_FIRST, VehicleGear.GEAR_SECOND,
                    VehicleGear.GEAR_THIRD, VehicleGear.GEAR_FOURTH,
                    VehicleGear.GEAR_FIFTH, VehicleGear.GEAR_SIXTH,
                    VehicleGear.GEAR_SEVENTH, VehicleGear.GEAR_EIGHTH,
                    VehicleGear.GEAR_NINTH).build();
    private static final ImmutableSet<Integer> DISTANCE_DISPLAY_UNITS =
            ImmutableSet.<Integer>builder().add(/*VehicleUnit.MILLIMETER=*/
                    0x20, /*VehicleUnit.METER=*/ 0x21,
                    /*VehicleUnit.KILOMETER=*/0x23, /*VehicleUnit.MILE=*/0x24).build();
    private static final ImmutableSet<Integer> VOLUME_DISPLAY_UNITS =
            ImmutableSet.<Integer>builder().add(/*VehicleUnit.MILLILITER=*/
                    0x40, /*VehicleUnit.LITER=*/0x41,
                    /*VehicleUnit.US_GALLON=*/0x42, /*VehicleUnit.IMPERIAL_GALLON=*/0x43).build();
    private static final ImmutableSet<Integer> PRESSURE_DISPLAY_UNITS =
            ImmutableSet.<Integer>builder().add(/*VehicleUnit.KILOPASCAL=*/
                    0x70, /*VehicleUnit.PSI=*/0x71,
                    /*VehicleUnit.BAR=*/0x72).build();
    private static final ImmutableSet<Integer> BATTERY_DISPLAY_UNITS =
            ImmutableSet.<Integer>builder().add(
                    /*VehicleUnit.MILLIAMPERE=*/ 0x61,
                    /*VehicleUnit.MILLIVOLT=*/ 0x62,
                    /*VehicleUnit.MILLIWATTS=*/ 0x63,
                    /*VehicleUnit.WATT_HOUR=*/ 0x60,
                    /*VehicleUnit.AMPERE_HOURS=*/ 0x64,
                    /*VehicleUnit.KILOWATT_HOUR=*/ 0x65).build();
    private static final ImmutableSet<Integer> SPEED_DISPLAY_UNITS =
            ImmutableSet.<Integer>builder().add(/*VehicleUnit.METER_PER_SEC=*/0x01,
                    /*VehicleUnit.MILES_PER_HOUR=*/ 0x90,
                    /*VehicleUnit.KILOMETERS_PER_HOUR=*/ 0x91).build();
    /** contains property Ids for the properties required by CDD */
    private final ArraySet<Integer> mPropertyIds = new ArraySet<>();
    private CarPropertyManager mCarPropertyManager;

    private static void verifyWheelTickConfigArray(int supportedWheels, int wheelToVerify,
            int configArrayIndex, int wheelTicksToUm) {
        if ((supportedWheels & wheelToVerify) != 0) {
            assertWithMessage(
                    "WHEEL_TICK configArray[" + configArrayIndex
                            + "] must specify the ticks to micrometers for " + wheelToString(
                            wheelToVerify))
                    .that(wheelTicksToUm)
                    .isGreaterThan(0);
        } else {
            assertWithMessage(
                    "WHEEL_TICK configArray[" + configArrayIndex + "] should be zero since "
                            + wheelToString(wheelToVerify)
                            + "is not supported")
                    .that(wheelTicksToUm)
                    .isEqualTo(0);
        }
    }

    private static void verifyWheelTickValue(int supportedWheels, int wheelToVerify,
            int valueIndex, Long ticks) {
        if ((supportedWheels & wheelToVerify) == 0) {
            assertWithMessage(
                    "WHEEL_TICK value[" + valueIndex + "] should be zero since "
                            + wheelToString(wheelToVerify)
                            + "is not supported")
                    .that(ticks)
                    .isEqualTo(0);
        }
    }

    private static String wheelToString(int wheel) {
        switch (wheel) {
            case VehicleAreaWheel.WHEEL_LEFT_FRONT:
                return "WHEEL_LEFT_FRONT";
            case VehicleAreaWheel.WHEEL_RIGHT_FRONT:
                return "WHEEL_RIGHT_FRONT";
            case VehicleAreaWheel.WHEEL_RIGHT_REAR:
                return "WHEEL_RIGHT_REAR";
            case VehicleAreaWheel.WHEEL_LEFT_REAR:
                return "WHEEL_LEFT_REAR";
            default:
                return Integer.toString(wheel);
        }
    }

    @Before
    public void setUp() throws Exception {
        super.setUp();
        mCarPropertyManager = (CarPropertyManager) getCar().getCarManager(Car.PROPERTY_SERVICE);
        mPropertyIds.add(VehiclePropertyIds.PERF_VEHICLE_SPEED);
        mPropertyIds.add(VehiclePropertyIds.GEAR_SELECTION);
        mPropertyIds.add(VehiclePropertyIds.NIGHT_MODE);
        mPropertyIds.add(VehiclePropertyIds.PARKING_BRAKE_ON);
    }

    /**
     * Test for {@link CarPropertyManager#getPropertyList()}
     */
    @Test
    public void testGetPropertyList() {
        List<CarPropertyConfig> allConfigs = mCarPropertyManager.getPropertyList();
        assertThat(allConfigs).isNotNull();
    }

    /**
     * Test for {@link CarPropertyManager#getPropertyList(ArraySet)}
     */
    @Test
    public void testGetPropertyListWithArraySet() {
        List<CarPropertyConfig> requiredConfigs = mCarPropertyManager.getPropertyList(mPropertyIds);
        // Vehicles need to implement all of those properties
        assertThat(requiredConfigs.size()).isEqualTo(mPropertyIds.size());
    }

    /**
     * Test for {@link CarPropertyManager#getCarPropertyConfig(int)}
     */
    @Test
    public void testGetPropertyConfig() {
        List<CarPropertyConfig> allConfigs = mCarPropertyManager.getPropertyList();
        for (CarPropertyConfig cfg : allConfigs) {
            assertThat(mCarPropertyManager.getCarPropertyConfig(cfg.getPropertyId())).isNotNull();
        }
    }

    /**
     * Test for {@link CarPropertyManager#getAreaId(int, int)}
     */
    @Test
    public void testGetAreaId() {
        // For global properties, getAreaId should always return 0.
        List<CarPropertyConfig> allConfigs = mCarPropertyManager.getPropertyList();
        for (CarPropertyConfig cfg : allConfigs) {
            if (cfg.isGlobalProperty()) {
                assertThat(mCarPropertyManager.getAreaId(cfg.getPropertyId(),
                        VehicleAreaSeat.SEAT_ROW_1_LEFT))
                        .isEqualTo(VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL);
            } else {
                int[] areaIds = cfg.getAreaIds();
                // Because areaId in propConfig must not be overlapped with each other.
                // The result should be itself.
                for (int areaIdInConfig : areaIds) {
                    int areaIdByCarPropertyManager =
                            mCarPropertyManager.getAreaId(cfg.getPropertyId(), areaIdInConfig);
                    assertThat(areaIdByCarPropertyManager).isEqualTo(areaIdInConfig);
                }
            }
        }
    }

    @CddTest(requirement = "2.5.1")
    @Test
    public void testMustSupportGearSelection() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.GEAR_SELECTION,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Integer.class).requireProperty().setPossibleConfigArrayValues(
                VEHICLE_GEARS).requirePropertyValueTobeInConfigArray().build().verify(
                mCarPropertyManager);
    }

    @CddTest(requirement = "2.5.1")
    @Test
    public void testMustSupportNightMode() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.NIGHT_MODE,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Boolean.class).requireProperty().build().verify(mCarPropertyManager);
    }

    @CddTest(requirement = "2.5.1")
    @Test
    public void testMustSupportPerfVehicleSpeed() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.PERF_VEHICLE_SPEED,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_CONTINUOUS,
                Float.class).requireProperty().build().verify(mCarPropertyManager);
    }

    @Test
    public void testPerfVehicleSpeedDisplayIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.PERF_VEHICLE_SPEED_DISPLAY,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_CONTINUOUS,
                Float.class).build().verify(mCarPropertyManager);
    }

    @CddTest(requirement = "2.5.1")
    @Test
    public void testMustSupportParkingBrakeOn() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.PARKING_BRAKE_ON,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Boolean.class).requireProperty().build().verify(mCarPropertyManager);
    }

    @Test
    public void testWheelTickIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.WHEEL_TICK,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_CONTINUOUS,
                Long[].class).setConfigArrayVerifier(
                configArray -> {
                    assertWithMessage("WHEEL_TICK config array must be size 5")
                            .that(configArray.size())
                            .isEqualTo(5);

                    int supportedWheels = configArray.get(0);
                    assertWithMessage(
                            "WHEEL_TICK config array first element specifies which wheels are"
                                    + " supported")
                            .that(supportedWheels).isGreaterThan(
                            VehicleAreaWheel.WHEEL_UNKNOWN);
                    assertWithMessage(
                            "WHEEL_TICK config array first element specifies which wheels are"
                                    + " supported")
                            .that(supportedWheels)
                            .isAtMost(VehicleAreaWheel.WHEEL_LEFT_FRONT
                                    | VehicleAreaWheel.WHEEL_RIGHT_FRONT |
                                    VehicleAreaWheel.WHEEL_LEFT_REAR
                                    | VehicleAreaWheel.WHEEL_RIGHT_REAR);

                    verifyWheelTickConfigArray(supportedWheels,
                            VehicleAreaWheel.WHEEL_LEFT_FRONT, 1, configArray.get(1));
                    verifyWheelTickConfigArray(supportedWheels,
                            VehicleAreaWheel.WHEEL_RIGHT_FRONT, 2, configArray.get(2));
                    verifyWheelTickConfigArray(supportedWheels,
                            VehicleAreaWheel.WHEEL_RIGHT_REAR, 3, configArray.get(3));
                    verifyWheelTickConfigArray(supportedWheels,
                            VehicleAreaWheel.WHEEL_LEFT_REAR, 4, configArray.get(4));
                }).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> {
                    List<Integer> wheelTickConfigArray = carPropertyConfig.getConfigArray();
                    int supportedWheels = wheelTickConfigArray.get(0);

                    Long[] wheelTicks = (Long[]) carPropertyValue.getValue();
                    assertWithMessage("WHEEL_TICK Long[] value must be size 5").that(
                            wheelTicks.length)
                            .isEqualTo(5);

                    verifyWheelTickValue(supportedWheels, VehicleAreaWheel.WHEEL_LEFT_FRONT, 1,
                            wheelTicks[1]);
                    verifyWheelTickValue(supportedWheels, VehicleAreaWheel.WHEEL_RIGHT_FRONT, 2,
                            wheelTicks[2]);
                    verifyWheelTickValue(supportedWheels, VehicleAreaWheel.WHEEL_RIGHT_REAR, 3,
                            wheelTicks[3]);
                    verifyWheelTickValue(supportedWheels, VehicleAreaWheel.WHEEL_LEFT_REAR, 4,
                            wheelTicks[4]);
                }).build().verify(mCarPropertyManager);

    }

    @Test
    public void testInfoMakeIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.INFO_MAKE,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_STATIC,
                String.class).build().verify(mCarPropertyManager);
    }

    @Test
    public void testInfoModelIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.INFO_MODEL,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_STATIC,
                String.class).build().verify(mCarPropertyManager);
    }

    @Test
    public void testInfoModelYearIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.INFO_MODEL_YEAR,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_STATIC,
                Integer.class).build().verify(mCarPropertyManager);
    }

    @Test
    public void testInfoFuelCapacityIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.INFO_FUEL_CAPACITY,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_STATIC,
                Float.class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> assertWithMessage(
                        "INFO_FUEL_CAPACITY Float value must be greater than or equal 0").that(
                        (Float) carPropertyValue.getValue()).isAtLeast(0)).build().verify(
                mCarPropertyManager);
    }

    @Test
    public void testInfoFuelTypeIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.INFO_FUEL_TYPE,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_STATIC,
                Integer[].class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> {
                    Integer[] fuelTypes = (Integer[]) carPropertyValue.getValue();
                    assertWithMessage("INFO_FUEL_TYPE must specify at least 1 fuel type").that(
                            fuelTypes.length).isGreaterThan(0);
                    for (Integer fuelType : fuelTypes) {
                        assertWithMessage(
                                "INFO_FUEL_TYPE must be a defined fuel type: " + fuelType).that(
                                fuelType).isIn(
                                ImmutableSet.builder().add(FuelType.UNKNOWN, FuelType.UNLEADED,
                                        FuelType.LEADED, FuelType.DIESEL_1, FuelType.DIESEL_2,
                                        FuelType.BIODIESEL, FuelType.E85, FuelType.LPG,
                                        FuelType.CNG, FuelType.LNG, FuelType.ELECTRIC,
                                        FuelType.HYDROGEN, FuelType.OTHER).build());
                    }
                }).build().verify(mCarPropertyManager);
    }

    @Test
    public void testInfoEvBatteryCapacityIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.INFO_EV_BATTERY_CAPACITY,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_STATIC,
                Float.class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> assertWithMessage(
                        "INFO_EV_BATTERY_CAPACITY Float value must be greater than or equal to 0")
                        .that((Float) carPropertyValue.getValue()).isAtLeast(0)).build().verify(
                mCarPropertyManager);
    }

    @Test
    public void testInfoEvConnectorTypeIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.INFO_EV_CONNECTOR_TYPE,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_STATIC,
                Integer[].class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> {
                    Integer[] evConnectorTypes = (Integer[]) carPropertyValue.getValue();
                    assertWithMessage(
                            "INFO_EV_CONNECTOR_TYPE must specify at least 1 connection type").that(
                            evConnectorTypes.length).isGreaterThan(0);
                    for (Integer evConnectorType : evConnectorTypes) {
                        assertWithMessage(
                                "INFO_EV_CONNECTOR_TYPE must be a defined connection type: "
                                        + evConnectorType).that(
                                evConnectorType).isIn(
                                ImmutableSet.builder().add(EvConnectorType.UNKNOWN,
                                        EvConnectorType.J1772, EvConnectorType.MENNEKES,
                                        EvConnectorType.CHADEMO, EvConnectorType.COMBO_1,
                                        EvConnectorType.COMBO_2, EvConnectorType.TESLA_ROADSTER,
                                        EvConnectorType.TESLA_HPWC,
                                        EvConnectorType.TESLA_SUPERCHARGER, EvConnectorType.GBT,
                                        EvConnectorType.GBT_DC, EvConnectorType.SCAME,
                                        EvConnectorType.OTHER).build());
                    }
                }).build().verify(mCarPropertyManager);
    }

    @Test
    public void testInfoFuelDoorLocationIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.INFO_FUEL_DOOR_LOCATION,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_STATIC,
                Integer.class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> {
                    Integer fuelDoorLocation = (Integer) carPropertyValue.getValue();
                    assertWithMessage(
                            "INFO_FUEL_DOOR_LOCATION must be a defined port location: "
                                    + fuelDoorLocation).that(
                            fuelDoorLocation).isIn(PORT_LOCATION_TYPES);
                }).build().verify(mCarPropertyManager);
    }

    @Test
    public void testInfoEvPortLocationIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.INFO_EV_PORT_LOCATION,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_STATIC,
                Integer.class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> {
                    Integer evPortLocation = (Integer) carPropertyValue.getValue();
                    assertWithMessage(
                            "INFO_EV_PORT_LOCATION must be a defined port location: "
                                    + evPortLocation).that(
                            evPortLocation).isIn(PORT_LOCATION_TYPES);
                }).build().verify(mCarPropertyManager);
    }

    @Test
    public void testInfoMultiEvPortLocationsIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.INFO_MULTI_EV_PORT_LOCATIONS,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_STATIC,
                Integer[].class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> {
                    Integer[] evPortLocations = (Integer[]) carPropertyValue.getValue();
                    assertWithMessage(
                            "INFO_MULTI_EV_PORT_LOCATIONS must specify at least 1 port location")
                            .that(evPortLocations.length).isGreaterThan(0);
                    for (Integer evPortLocation : evPortLocations) {
                        assertWithMessage(
                                "INFO_MULTI_EV_PORT_LOCATIONS must be a defined port location: "
                                        + evPortLocation).that(
                                evPortLocation).isIn(PORT_LOCATION_TYPES);
                    }
                }).build().verify(mCarPropertyManager);
    }

    @Test
    public void testInfoDriverSeatIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.INFO_DRIVER_SEAT,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_SEAT,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_STATIC,
                Integer.class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> {
                    Integer driverSeat = (Integer) carPropertyValue.getValue();
                    assertWithMessage(
                            "INFO_DRIVER_SEAT must be a defined front seat location: "
                                    + driverSeat).that(
                            driverSeat).isIn(
                            ImmutableSet.builder().add(VehicleAreaSeat.SEAT_UNKNOWN,
                                    VehicleAreaSeat.SEAT_ROW_1_LEFT,
                                    VehicleAreaSeat.SEAT_ROW_1_CENTER,
                                    VehicleAreaSeat.SEAT_ROW_1_RIGHT).build());
                }).setAreaIdsVerifier(areaIds -> assertWithMessage(
                "Even though INFO_DRIVER_SEAT is VEHICLE_AREA_TYPE_SEAT, it is meant to be "
                        + "VEHICLE_AREA_TYPE_GLOBAL, so its AreaIds must contain a single 0")
                .that(areaIds).isEqualTo(
                        new int[]{VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL})).build()
                .verify(mCarPropertyManager);
    }

    @Test
    public void testInfoExteriorDimensionsIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.INFO_EXTERIOR_DIMENSIONS,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_STATIC,
                Integer[].class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> {
                    Integer[] exteriorDimensions = (Integer[]) carPropertyValue.getValue();
                    assertWithMessage(
                            "INFO_EXTERIOR_DIMENSIONS must specify all 8 dimension measurements")
                            .that(exteriorDimensions.length).isEqualTo(8);
                    for (Integer exteriorDimension : exteriorDimensions) {
                        assertWithMessage(
                                "INFO_EXTERIOR_DIMENSIONS measurement must be greater than 0").that(
                                exteriorDimension).isGreaterThan(0);
                    }
                }).build().verify(mCarPropertyManager);
    }

    @Test
    public void testElectronicTollCollectionCardTypeIfSupported() {
        VehiclePropertyVerifier.newBuilder(
                VehiclePropertyIds.ELECTRONIC_TOLL_COLLECTION_CARD_TYPE,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Integer.class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> {
                    Integer electronicTollCollectionCardType =
                            (Integer) carPropertyValue.getValue();
                    assertWithMessage(
                            "ELECTRONIC_TOLL_COLLECTION_CARD_TYPE value must be a valid "
                                    + "VehicleElectronicTollCollectionCardType").that(
                            electronicTollCollectionCardType).isIn(ImmutableSet.builder().add(
                            VehicleElectronicTollCollectionCardType.UNKNOWN,
                            VehicleElectronicTollCollectionCardType.
                                    JP_ELECTRONIC_TOLL_COLLECTION_CARD,
                            VehicleElectronicTollCollectionCardType.
                                    JP_ELECTRONIC_TOLL_COLLECTION_CARD_V2).build());
                }).build().verify(mCarPropertyManager);
    }

    @Test
    public void testElectronicTollCollectionCardStatusIfSupported() {
        VehiclePropertyVerifier.newBuilder(
                VehiclePropertyIds.ELECTRONIC_TOLL_COLLECTION_CARD_STATUS,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Integer.class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> {
                    Integer electronicTollCollectionCardStatus =
                            (Integer) carPropertyValue.getValue();
                    assertWithMessage(
                            "ELECTRONIC_TOLL_COLLECTION_CARD_STATUS value must be a valid "
                                    + "VehicleElectronicTollCollectionCardStatus").that(
                            electronicTollCollectionCardStatus).isIn(ImmutableSet.builder().add(
                            VehicleElectronicTollCollectionCardStatus.UNKNOWN,
                            VehicleElectronicTollCollectionCardStatus.
                                    ELECTRONIC_TOLL_COLLECTION_CARD_VALID,
                            VehicleElectronicTollCollectionCardStatus.
                                    ELECTRONIC_TOLL_COLLECTION_CARD_INVALID,
                            VehicleElectronicTollCollectionCardStatus.
                                    ELECTRONIC_TOLL_COLLECTION_CARD_NOT_INSERTED).build());
                }).build().verify(mCarPropertyManager);
    }

    @Test
    public void testEnvOutsideTemperatureIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.ENV_OUTSIDE_TEMPERATURE,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_CONTINUOUS,
                Float.class).build().verify(mCarPropertyManager);
    }

    @Test
    public void testCurrentGearIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.CURRENT_GEAR,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Integer.class).setPossibleConfigArrayValues(
                VEHICLE_GEARS).requirePropertyValueTobeInConfigArray().build().verify(
                mCarPropertyManager);
    }

    @Test
    public void testParkingBrakeAutoApplyIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.PARKING_BRAKE_AUTO_APPLY,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Boolean.class).build().verify(mCarPropertyManager);
    }

    @Test
    public void testIgnitionStateIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.IGNITION_STATE,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Integer.class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> {
                    Integer ignitionState = (Integer) carPropertyValue.getValue();
                    assertWithMessage(
                            "IGNITION_STATE must be a defined ignition state: "
                                    + ignitionState).that(
                            ignitionState).isIn(ImmutableSet.of(
                            /*VehicleIgnitionState.UNDEFINED=*/0,
                            /*VehicleIgnitionState.LOCK=*/1,
                            /*VehicleIgnitionState.OFF=*/2,
                            /*VehicleIgnitionState.ACC=*/3,
                            /*VehicleIgnitionState.ON=*/4,
                            /*VehicleIgnitionState.START=*/5));
                }).build().verify(mCarPropertyManager);
    }

    @Test
    public void testDistanceDisplayUnitsIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.DISTANCE_DISPLAY_UNITS,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ_WRITE,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Integer.class).setPossibleConfigArrayValues(
                DISTANCE_DISPLAY_UNITS).requirePropertyValueTobeInConfigArray().build().verify(
                mCarPropertyManager);
    }

    @Test
    public void testFuelVolumeDisplayUnitsIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.FUEL_VOLUME_DISPLAY_UNITS,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ_WRITE,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Integer.class).setPossibleConfigArrayValues(
                VOLUME_DISPLAY_UNITS).requirePropertyValueTobeInConfigArray().build().verify(
                mCarPropertyManager);
    }

    @Test
    public void testTirePressureDisplayUnitsIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.TIRE_PRESSURE_DISPLAY_UNITS,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ_WRITE,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Integer.class).setPossibleConfigArrayValues(
                PRESSURE_DISPLAY_UNITS).requirePropertyValueTobeInConfigArray().build().verify(
                mCarPropertyManager);
    }

    @Test
    public void testEvBatteryDisplayUnitsIfSupported() {
        VehiclePropertyVerifier.newBuilder(VehiclePropertyIds.EV_BATTERY_DISPLAY_UNITS,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ_WRITE,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Integer.class).setPossibleConfigArrayValues(
                BATTERY_DISPLAY_UNITS).requirePropertyValueTobeInConfigArray().build().verify(
                mCarPropertyManager);
    }

    @Test
    public void testVehicleSpeedDisplayUnitsIfSupported() {
        VehiclePropertyVerifier.newBuilder(/*VehiclePropertyIds.VEHICLE_SPEED_DISPLAY_UNITS=*/
                289408516,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ_WRITE,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Integer.class).setPossibleConfigArrayValues(
                SPEED_DISPLAY_UNITS).requirePropertyValueTobeInConfigArray().build().verify(
                mCarPropertyManager);
    }

    @Test
    public void testFuelConsumptionUnitsDistanceOverTimeIfSupported() {
        VehiclePropertyVerifier.newBuilder(
                VehiclePropertyIds.FUEL_CONSUMPTION_UNITS_DISTANCE_OVER_VOLUME,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ_WRITE,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Boolean.class).build().verify(mCarPropertyManager);
    }

    @Test
    public void testFuelLevelIfSupported() {
        VehiclePropertyVerifier.newBuilder(
                VehiclePropertyIds.FUEL_LEVEL,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_CONTINUOUS,
                Float.class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> {
                    assertWithMessage(
                            "FUEL_LEVEL Float value must be greater than or equal 0").that(
                            (Float) carPropertyValue.getValue()).isAtLeast(0);

                    if (mCarPropertyManager.getCarPropertyConfig(
                            VehiclePropertyIds.INFO_FUEL_CAPACITY) == null) {
                        return;
                    }

                    CarPropertyValue<?> infoFuelCapacityValue = mCarPropertyManager.getProperty(
                            VehiclePropertyIds.INFO_FUEL_CAPACITY,
                            VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL);

                    assertWithMessage(
                            "FUEL_LEVEL Float value must not exceed INFO_FUEL_CAPACITY Float "
                                    + "value").that(
                            (Float) carPropertyValue.getValue()).isAtMost(
                            (Float) infoFuelCapacityValue.getValue());
                }).build().verify(mCarPropertyManager);
    }

    @Test
    public void testEvBatteryLevelIfSupported() {
        VehiclePropertyVerifier.newBuilder(
                VehiclePropertyIds.EV_BATTERY_LEVEL,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_CONTINUOUS,
                Float.class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> {
                    assertWithMessage(
                            "EV_BATTERY_LEVEL Float value must be greater than or equal 0").that(
                            (Float) carPropertyValue.getValue()).isAtLeast(0);

                    if (mCarPropertyManager.getCarPropertyConfig(
                            VehiclePropertyIds.INFO_EV_BATTERY_CAPACITY) == null) {
                        return;
                    }

                    CarPropertyValue<?> infoEvBatteryCapacityValue =
                            mCarPropertyManager.getProperty(
                                    VehiclePropertyIds.INFO_EV_BATTERY_CAPACITY,
                                    VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL);

                    assertWithMessage(
                            "EV_BATTERY_LEVEL Float value must not exceed "
                                    + "INFO_EV_BATTERY_CAPACITY Float "
                                    + "value").that(
                            (Float) carPropertyValue.getValue()).isAtMost(
                            (Float) infoEvBatteryCapacityValue.getValue());
                }).build().verify(mCarPropertyManager);
    }

    @Test
    public void testEvBatteryInstantaneousChargeRateIfSupported() {
        VehiclePropertyVerifier.newBuilder(
                VehiclePropertyIds.EV_BATTERY_INSTANTANEOUS_CHARGE_RATE,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_CONTINUOUS,
                Float.class).build().verify(mCarPropertyManager);
    }

    @Test
    public void testRangeRemainingIfSupported() {
        VehiclePropertyVerifier.newBuilder(
                VehiclePropertyIds.RANGE_REMAINING,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ_WRITE,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_CONTINUOUS,
                Float.class).setCarPropertyValueVerifier(
                (carPropertyConfig, carPropertyValue) -> {
                    assertWithMessage(
                            "RANGE_REMAINING Float value must be greater than or equal 0").that(
                            (Float) carPropertyValue.getValue()).isAtLeast(0);
                }).build().verify(mCarPropertyManager);
    }

    @Test
    public void testFuelLevelLowIfSupported() {
        VehiclePropertyVerifier.newBuilder(
                VehiclePropertyIds.FUEL_LEVEL_LOW,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Boolean.class).build().verify(mCarPropertyManager);
    }

    @Test
    public void testFuelDoorOpenIfSupported() {
        VehiclePropertyVerifier.newBuilder(
                VehiclePropertyIds.FUEL_DOOR_OPEN,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ_WRITE,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Boolean.class).build().verify(mCarPropertyManager);
    }

    @Test
    public void testEvChargePortOpenIfSupported() {
        VehiclePropertyVerifier.newBuilder(
                VehiclePropertyIds.EV_CHARGE_PORT_OPEN,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ_WRITE,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Boolean.class).build().verify(mCarPropertyManager);
    }

    @Test
    public void testEvChargePortConnectedIfSupported() {
        VehiclePropertyVerifier.newBuilder(
                VehiclePropertyIds.EV_CHARGE_PORT_CONNECTED,
                CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ,
                VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL,
                CarPropertyConfig.VEHICLE_PROPERTY_CHANGE_MODE_ONCHANGE,
                Boolean.class).build().verify(mCarPropertyManager);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testGetProperty() {
        List<CarPropertyConfig> configs = mCarPropertyManager.getPropertyList(mPropertyIds);
        for (CarPropertyConfig cfg : configs) {
            if (cfg.getAccess() == CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ) {
                int[] areaIds = getAreaIdsHelper(cfg);
                int propId = cfg.getPropertyId();
                // no guarantee if we can get values, just call and check if it throws exception.
                if (cfg.getPropertyType() == Boolean.class) {
                    for (int areaId : areaIds) {
                        mCarPropertyManager.getBooleanProperty(propId, areaId);
                    }
                } else if (cfg.getPropertyType() == Integer.class) {
                    for (int areaId : areaIds) {
                        mCarPropertyManager.getIntProperty(propId, areaId);
                    }
                } else if (cfg.getPropertyType() == Float.class) {
                    for (int areaId : areaIds) {
                        mCarPropertyManager.getFloatProperty(propId, areaId);
                    }
                } else if (cfg.getPropertyType() == Integer[].class) {
                    for (int areId : areaIds) {
                        mCarPropertyManager.getIntArrayProperty(propId, areId);
                    }
                } else {
                    for (int areaId : areaIds) {
                        mCarPropertyManager.getProperty(
                                cfg.getPropertyType(), propId, areaId);
                    }
                }
            }
        }
    }

    @Test
    public void testGetIntArrayProperty() {
        List<CarPropertyConfig> allConfigs = mCarPropertyManager.getPropertyList();
        for (CarPropertyConfig cfg : allConfigs) {
            if (cfg.getAccess() == CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_NONE
                    || cfg.getAccess() == CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_WRITE
                    || cfg.getPropertyType() != Integer[].class) {
                // skip the test if the property is not readable or not an int array type property.
                continue;
            }
            switch (cfg.getPropertyId()) {
                case VehiclePropertyIds.INFO_FUEL_TYPE:
                    int[] fuelTypes = mCarPropertyManager.getIntArrayProperty(cfg.getPropertyId(),
                            VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL);
                    verifyEnumsRange(EXPECTED_FUEL_TYPES, fuelTypes);
                    break;
                case VehiclePropertyIds.INFO_MULTI_EV_PORT_LOCATIONS:
                    int[] evPortLocations = mCarPropertyManager.getIntArrayProperty(
                            cfg.getPropertyId(), VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL);
                    verifyEnumsRange(EXPECTED_PORT_LOCATIONS, evPortLocations);
                    break;
                default:
                    int[] areaIds = getAreaIdsHelper(cfg);
                    for (int areaId : areaIds) {
                        mCarPropertyManager.getIntArrayProperty(cfg.getPropertyId(), areaId);
                    }
            }
        }
    }

    private void verifyEnumsRange(List<Integer> expectedResults, int[] results) {
        assertThat(results).isNotNull();
        // If the property is not implemented in cars, getIntArrayProperty returns an empty array.
        if (results.length == 0) {
            return;
        }
        for (int result : results) {
            assertThat(result).isIn(expectedResults);
        }
    }

    @Test
    public void testIsPropertyAvailable() {
        List<CarPropertyConfig> configs = mCarPropertyManager.getPropertyList(mPropertyIds);

        for (CarPropertyConfig cfg : configs) {
            int[] areaIds = getAreaIdsHelper(cfg);
            for (int areaId : areaIds) {
                assertThat(mCarPropertyManager.isPropertyAvailable(cfg.getPropertyId(), areaId))
                        .isTrue();
            }
        }
    }

    @Test
    public void testSetProperty() {
        List<CarPropertyConfig> configs = mCarPropertyManager.getPropertyList();
        for (CarPropertyConfig cfg : configs) {
            if (cfg.getAccess() == CarPropertyConfig.VEHICLE_PROPERTY_ACCESS_READ_WRITE
                    && cfg.getPropertyType() == Boolean.class) {
                // In R, there is no property which is writable for third-party apps.
                for (int areaId : getAreaIdsHelper(cfg)) {
                    assertThrows(SecurityException.class,
                            () -> mCarPropertyManager.setBooleanProperty(
                                    cfg.getPropertyId(), areaId, true));
                }
            }
        }
    }

    @Test
    public void testRegisterCallback() throws Exception {
        //Test on registering a invalid property
        int invalidPropertyId = -1;
        boolean isRegistered = mCarPropertyManager.registerCallback(
                new CarPropertyEventCounter(), invalidPropertyId, 0);
        assertThat(isRegistered).isFalse();

        // Test for continuous properties
        int vehicleSpeed = VehiclePropertyIds.PERF_VEHICLE_SPEED;
        CarPropertyEventCounter speedListenerUI = new CarPropertyEventCounter();
        CarPropertyEventCounter speedListenerFast = new CarPropertyEventCounter();

        assertThat(speedListenerUI.receivedEvent(vehicleSpeed)).isEqualTo(NO_EVENTS);
        assertThat(speedListenerUI.receivedError(vehicleSpeed)).isEqualTo(NO_EVENTS);
        assertThat(speedListenerUI.receivedErrorWithErrorCode(vehicleSpeed)).isEqualTo(NO_EVENTS);
        assertThat(speedListenerFast.receivedEvent(vehicleSpeed)).isEqualTo(NO_EVENTS);
        assertThat(speedListenerFast.receivedError(vehicleSpeed)).isEqualTo(NO_EVENTS);
        assertThat(speedListenerFast.receivedErrorWithErrorCode(vehicleSpeed)).isEqualTo(NO_EVENTS);

        mCarPropertyManager.registerCallback(speedListenerUI, vehicleSpeed,
                CarPropertyManager.SENSOR_RATE_UI);
        mCarPropertyManager.registerCallback(speedListenerFast, vehicleSpeed,
                CarPropertyManager.SENSOR_RATE_FASTEST);
        speedListenerUI.resetCountDownLatch(UI_RATE_EVENT_COUNTER);
        speedListenerUI.assertOnChangeEventCalled();
        assertThat(speedListenerUI.receivedEvent(vehicleSpeed)).isGreaterThan(NO_EVENTS);
        assertThat(speedListenerFast.receivedEvent(vehicleSpeed)).isGreaterThan(
                speedListenerUI.receivedEvent(vehicleSpeed));
        // The test did not change property values, it should not get error with error codes.
        assertThat(speedListenerUI.receivedErrorWithErrorCode(vehicleSpeed)).isEqualTo(NO_EVENTS);
        assertThat(speedListenerFast.receivedErrorWithErrorCode(vehicleSpeed)).isEqualTo(NO_EVENTS);

        mCarPropertyManager.unregisterCallback(speedListenerFast);
        mCarPropertyManager.unregisterCallback(speedListenerUI);

        // Test for on_change properties
        int nightMode = VehiclePropertyIds.NIGHT_MODE;
        CarPropertyEventCounter nightModeListener = new CarPropertyEventCounter();
        nightModeListener.resetCountDownLatch(ONCHANGE_RATE_EVENT_COUNTER);
        mCarPropertyManager.registerCallback(nightModeListener, nightMode, 0);
        nightModeListener.assertOnChangeEventCalled();
        assertThat(nightModeListener.receivedEvent(nightMode)).isEqualTo(1);
        mCarPropertyManager.unregisterCallback(nightModeListener);

    }

    @Test
    public void testUnregisterCallback() throws Exception {

        int vehicleSpeed = VehiclePropertyIds.PERF_VEHICLE_SPEED;
        CarPropertyEventCounter speedListenerNormal = new CarPropertyEventCounter();
        CarPropertyEventCounter speedListenerUI = new CarPropertyEventCounter();

        mCarPropertyManager.registerCallback(speedListenerNormal, vehicleSpeed,
                CarPropertyManager.SENSOR_RATE_NORMAL);

        // test on unregistering a callback that was never registered
        try {
            mCarPropertyManager.unregisterCallback(speedListenerUI);
        } catch (Exception e) {
            Assert.fail();
        }

        mCarPropertyManager.registerCallback(speedListenerUI, vehicleSpeed,
                CarPropertyManager.SENSOR_RATE_UI);
        speedListenerUI.resetCountDownLatch(UI_RATE_EVENT_COUNTER);
        speedListenerUI.assertOnChangeEventCalled();
        mCarPropertyManager.unregisterCallback(speedListenerNormal, vehicleSpeed);

        int currentEventNormal = speedListenerNormal.receivedEvent(vehicleSpeed);
        int currentEventUI = speedListenerUI.receivedEvent(vehicleSpeed);
        speedListenerNormal.assertOnChangeEventNotCalled();

        assertThat(speedListenerNormal.receivedEvent(vehicleSpeed)).isEqualTo(currentEventNormal);
        assertThat(speedListenerUI.receivedEvent(vehicleSpeed)).isNotEqualTo(currentEventUI);

        mCarPropertyManager.unregisterCallback(speedListenerUI);
        speedListenerUI.assertOnChangeEventNotCalled();

        currentEventUI = speedListenerUI.receivedEvent(vehicleSpeed);
        assertThat(speedListenerUI.receivedEvent(vehicleSpeed)).isEqualTo(currentEventUI);
    }

    @Test
    public void testUnregisterWithPropertyId() throws Exception {
        // Ignores the test if wheel_tick property does not exist in the car.
        Assume.assumeTrue("WheelTick is not available, skip unregisterCallback test",
                mCarPropertyManager.isPropertyAvailable(
                        VehiclePropertyIds.WHEEL_TICK, VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL));

        CarPropertyConfig wheelTickConfig = mCarPropertyManager.getCarPropertyConfig(
                VehiclePropertyIds.WHEEL_TICK);
        CarPropertyConfig speedConfig = mCarPropertyManager.getCarPropertyConfig(
                VehiclePropertyIds.PERF_VEHICLE_SPEED);
        float maxSampleRateHz =
                Math.max(wheelTickConfig.getMaxSampleRate(), speedConfig.getMaxSampleRate());
        int eventCounter = getCounterBySampleRate(maxSampleRateHz);

        // Ignores the test if sampleRates for properties are too low.
        Assume.assumeTrue("The SampleRates for properties are too low, "
                + "skip testUnregisterWithPropertyId test", eventCounter != 0);
        CarPropertyEventCounter speedAndWheelTicksListener = new CarPropertyEventCounter();

        // CarService will register them to the maxSampleRate in CarPropertyConfig
        mCarPropertyManager.registerCallback(speedAndWheelTicksListener,
                VehiclePropertyIds.PERF_VEHICLE_SPEED, CarPropertyManager.SENSOR_RATE_FASTEST);
        mCarPropertyManager.registerCallback(speedAndWheelTicksListener,
                VehiclePropertyIds.WHEEL_TICK, CarPropertyManager.SENSOR_RATE_FASTEST);
        speedAndWheelTicksListener.resetCountDownLatch(eventCounter);
        speedAndWheelTicksListener.assertOnChangeEventCalled();

        // Tests unregister the individual property
        mCarPropertyManager.unregisterCallback(speedAndWheelTicksListener,
                VehiclePropertyIds.PERF_VEHICLE_SPEED);

        // Updates counter after unregistering the PERF_VEHICLE_SPEED
        int wheelTickEventCounter = getCounterBySampleRate(wheelTickConfig.getMaxSampleRate());
        speedAndWheelTicksListener.resetCountDownLatch(wheelTickEventCounter);
        speedAndWheelTicksListener.assertOnChangeEventCalled();
        int speedEventCountAfterFirstCountDown = speedAndWheelTicksListener.receivedEvent(
                VehiclePropertyIds.PERF_VEHICLE_SPEED);
        int wheelTickEventCountAfterFirstCountDown = speedAndWheelTicksListener.receivedEvent(
                VehiclePropertyIds.WHEEL_TICK);

        speedAndWheelTicksListener.resetCountDownLatch(wheelTickEventCounter);
        speedAndWheelTicksListener.assertOnChangeEventCalled();
        int speedEventCountAfterSecondCountDown = speedAndWheelTicksListener.receivedEvent(
                VehiclePropertyIds.PERF_VEHICLE_SPEED);
        int wheelTickEventCountAfterSecondCountDown = speedAndWheelTicksListener.receivedEvent(
                VehiclePropertyIds.WHEEL_TICK);

        assertThat(speedEventCountAfterFirstCountDown).isEqualTo(
                speedEventCountAfterSecondCountDown);
        assertThat(wheelTickEventCountAfterSecondCountDown)
                .isGreaterThan(wheelTickEventCountAfterFirstCountDown);
    }

    private int getCounterBySampleRate(float maxSampleRateHz) {
        if (Float.compare(maxSampleRateHz, (float) FAST_OR_FASTEST_EVENT_COUNTER) > 0) {
            return FAST_OR_FASTEST_EVENT_COUNTER;
        } else if (Float.compare(maxSampleRateHz, (float) UI_RATE_EVENT_COUNTER) > 0) {
            return UI_RATE_EVENT_COUNTER;
        } else if (Float.compare(maxSampleRateHz, (float) ONCHANGE_RATE_EVENT_COUNTER) > 0) {
            return ONCHANGE_RATE_EVENT_COUNTER;
        } else {
            return 0;
        }
    }

    // Returns {0} if the property is global property, otherwise query areaId for CarPropertyConfig
    private int[] getAreaIdsHelper(CarPropertyConfig config) {
        if (config.isGlobalProperty()) {
            return new int[]{0};
        } else {
            return config.getAreaIds();
        }
    }

    private static class CarPropertyEventCounter implements CarPropertyEventCallback {
        private final Object mLock = new Object();
        @GuardedBy("mLock")
        private final SparseArray<Integer> mEventCounter = new SparseArray<>();
        @GuardedBy("mLock")
        private final SparseArray<Integer> mErrorCounter = new SparseArray<>();
        @GuardedBy("mLock")
        private final SparseArray<Integer> mErrorWithErrorCodeCounter = new SparseArray<>();
        private int mCounter = FAST_OR_FASTEST_EVENT_COUNTER;
        private CountDownLatch mCountDownLatch = new CountDownLatch(mCounter);

        public int receivedEvent(int propId) {
            int val;
            synchronized (mLock) {
                val = mEventCounter.get(propId, 0);
            }
            return val;
        }

        public int receivedError(int propId) {
            int val;
            synchronized (mLock) {
                val = mErrorCounter.get(propId, 0);
            }
            return val;
        }

        public int receivedErrorWithErrorCode(int propId) {
            int val;
            synchronized (mLock) {
                val = mErrorWithErrorCodeCounter.get(propId, 0);
            }
            return val;
        }

        @Override
        public void onChangeEvent(CarPropertyValue value) {
            synchronized (mLock) {
                int val = mEventCounter.get(value.getPropertyId(), 0) + 1;
                mEventCounter.put(value.getPropertyId(), val);
            }
            mCountDownLatch.countDown();
        }

        @Override
        public void onErrorEvent(int propId, int zone) {
            synchronized (mLock) {
                int val = mErrorCounter.get(propId, 0) + 1;
                mErrorCounter.put(propId, val);
            }
        }

        @Override
        public void onErrorEvent(int propId, int areaId, int errorCode) {
            synchronized (mLock) {
                int val = mErrorWithErrorCodeCounter.get(propId, 0) + 1;
                mErrorWithErrorCodeCounter.put(propId, val);
            }
        }

        public void resetCountDownLatch(int counter) {
            mCountDownLatch = new CountDownLatch(counter);
            mCounter = counter;
        }

        public void assertOnChangeEventCalled() throws InterruptedException {
            if (!mCountDownLatch.await(WAIT_CALLBACK, TimeUnit.MILLISECONDS)) {
                throw new IllegalStateException("Callback is not called:" + mCounter + "times in "
                        + WAIT_CALLBACK + " ms.");
            }
        }

        public void assertOnChangeEventNotCalled() throws InterruptedException {
            // Once get an event, fail the test.
            mCountDownLatch = new CountDownLatch(1);
            if (mCountDownLatch.await(WAIT_CALLBACK, TimeUnit.MILLISECONDS)) {
                throw new IllegalStateException("Callback is called in "
                        + WAIT_CALLBACK + " ms.");
            }
        }

    }
}
