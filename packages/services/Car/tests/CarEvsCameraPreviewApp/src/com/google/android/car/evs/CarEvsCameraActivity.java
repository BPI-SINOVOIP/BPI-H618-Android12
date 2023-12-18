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

package com.google.android.car.evs;

import android.app.Activity;
import android.car.Car;
import android.car.evs.CarEvsManager;
import android.os.Bundle;
import android.util.Log;

public class CarEvsCameraActivity extends Activity {
    private static final String TAG = CarEvsCameraActivity.class.getSimpleName();
    private static final int CAR_WAIT_TIMEOUT_MS = 3_000;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Car.createCar(getApplicationContext(), /* handler = */ null, CAR_WAIT_TIMEOUT_MS,
                (car, ready) -> {
                    if (!ready) {
                        finish();
                        return;
                    }
                    CarEvsManager evsManager = (CarEvsManager) car.getCarManager(
                            Car.CAR_EVS_SERVICE);
                    int result = evsManager.startActivity(CarEvsManager.SERVICE_TYPE_REARVIEW);
                    Log.i(TAG, "startActivity(): " + result);
                    finish();
                });
    }
}
