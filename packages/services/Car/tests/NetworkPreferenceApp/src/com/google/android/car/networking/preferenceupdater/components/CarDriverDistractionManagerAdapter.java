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
package com.google.android.car.networking.preferenceupdater.components;

import android.car.Car;
import android.car.experimental.CarDriverDistractionManager;
import android.car.experimental.DriverDistractionChangeEvent;
import android.car.experimental.ExperimentalCar;
import android.content.Context;

/**
 * Module that interacts with CarDriverDistractionManager inside the Car service and provides latest
 * information about about driving state to the caller.
 */
public final class CarDriverDistractionManagerAdapter {
    private final CarDriverDistractionManager mCarDriverDistractionManager;
    private final Car mCar;

    public CarDriverDistractionManagerAdapter(Context ctx) {
        // Connect to car service
        mCar = Car.createCar(ctx);
        if (mCar.isFeatureEnabled(
                ExperimentalCar.DRIVER_DISTRACTION_EXPERIMENTAL_FEATURE_SERVICE)) {
            mCarDriverDistractionManager = (CarDriverDistractionManager) mCar.getCarManager(
                    ExperimentalCar.DRIVER_DISTRACTION_EXPERIMENTAL_FEATURE_SERVICE);
        } else {
            mCarDriverDistractionManager = null;
        }
    }

    /** Method that has to be called during destroy. */
    public void destroy() {
        mCar.disconnect();
    }

    /**
     * Returns true/false boolean based on the whether driver can be distracted right now or not
     */
    public boolean allowedToBeDistracted() {
        if (mCarDriverDistractionManager == null) {
            // This means we could not bind to CarDriverDistractionManager. Return true.
            return true;
        }
        DriverDistractionChangeEvent event = mCarDriverDistractionManager.getLastDistractionEvent();
        /**
         * event.getAwarenessPercentage returns the current driver awareness value, a float number
         * between 0.0 -> 1.0, which represents a percentage of the required awareness from driver.
         *  - 0.0 indicates that the driver has no situational awareness of the surrounding
         *    environment, which means driver is allawed to be distracted.
         *  - 1.0 indicates that the driver has the target amount of situational awareness
         *    necessary for the current driving environment, meaning driver can not be distracted at
         *    this moment.
         */
        if (event.getAwarenessPercentage() > 0) {
            // To simplify logic, we consider everything above 0% as dangerous and return false.
            return false;
        }
        return true;
    }
}
