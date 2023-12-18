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
package com.android.car.ui.core;

import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThrows;

import com.android.car.ui.test.R;

import org.junit.Test;

/** A test for {@link com.android.car.ui.core.CarUi} */
public class CarUiTest {

    @Test
    public void test_findCarUiComponentById_returnsNullWithNullInput() {
        assertNull(CarUi.findCarUiComponentById(null, R.id.car_ui_recycler_view));
    }

    @Test
    public void test_requireCarUiComponentById_throwsWithNullInput() {
        assertThrows(NullPointerException.class,
                () -> CarUi.requireCarUiComponentById(null, R.id.car_ui_recycler_view));
    }
}
