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
package com.android.car.dialer.integration;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.swipeLeft;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.intent.Intents.intended;
import static androidx.test.espresso.intent.matcher.IntentMatchers.hasAction;
import static androidx.test.espresso.intent.matcher.IntentMatchers.hasCategories;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static org.hamcrest.Matchers.allOf;

import android.content.Context;
import android.content.Intent;

import androidx.test.core.app.ActivityScenario;
import androidx.test.espresso.intent.Intents;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.car.dialer.R;
import com.android.car.dialer.bluetooth.BluetoothState;
import com.android.car.dialer.framework.FakeHfpManager;
import com.android.car.dialer.ui.TelecomActivity;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Collections;

import javax.inject.Inject;

import dagger.hilt.android.testing.HiltAndroidRule;
import dagger.hilt.android.testing.HiltAndroidTest;

@RunWith(AndroidJUnit4.class)
@HiltAndroidTest
public class NoHfpFragmentTest {

    // Consistent with the constants defined in NoHfpFragment.
    private static final String Bluetooth_Setting_ACTION = "android.settings.BLUETOOTH_SETTINGS";
    private static final String Bluetooth_Setting_CATEGORY = "android.intent.category.DEFAULT";

    @Rule
    public final HiltAndroidRule mHiltAndroidRule = new HiltAndroidRule(this);

    @Inject FakeHfpManager mFakeHfpManager;

    @Before
    public void setup() {
        mHiltAndroidRule.inject();

        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        ActivityScenario.launch(new Intent(context, TelecomActivity.class));
    }

    @Test
    public void bluetoothDisabled_displayErrorMsg() {
        mFakeHfpManager.getBluetoothStateLiveData().postValue(BluetoothState.DISABLED);

        onView(withId(R.id.error_string)).check(
                matches(allOf(isDisplayed(), withText(R.string.bluetooth_disabled))));
    }

    @Test
    public void noPairedDevices_displayErrorMsg() {
        mFakeHfpManager.getBluetoothStateLiveData().postValue(BluetoothState.ENABLED);
        mFakeHfpManager.getBluetoothPairListLiveData().postValue(Collections.EMPTY_SET);

        onView(withId(R.id.error_string)).check(
                matches(allOf(isDisplayed(), withText(R.string.bluetooth_unpaired))));
    }

    @Test
    public void clickButton_navigateToBluetoothSettings() {
        onView(withId(R.id.connect_bluetooth_button)).check(matches(isDisplayed()));

        Intents.init();
        // Use swipe action to perform the click action since the UI messes up and overlaps with
        // each other.
        onView(withId(R.id.connect_bluetooth_button)).perform(swipeLeft());
        intended(allOf(hasAction(Bluetooth_Setting_ACTION),
                hasCategories(Collections.singleton(Bluetooth_Setting_CATEGORY))));
        Intents.release();
    }
}
