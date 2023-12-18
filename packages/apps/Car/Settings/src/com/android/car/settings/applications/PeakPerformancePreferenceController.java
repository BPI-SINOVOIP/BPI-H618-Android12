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

package com.android.car.settings.applications;

import android.car.Car;
import android.car.drivingstate.CarUxRestrictions;
import android.car.watchdog.CarWatchdogManager;
import android.car.watchdog.PackageKillableState;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.os.UserHandle;

import androidx.annotation.VisibleForTesting;
import androidx.preference.TwoStatePreference;

import com.android.car.settings.R;
import com.android.car.settings.common.ConfirmationDialogFragment;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.Logger;
import com.android.car.settings.common.PreferenceController;
import com.android.internal.annotations.GuardedBy;

import java.util.Objects;

/**
 * Controller for preference which enables / disables I/O overuse killing for an application.
 */
public class PeakPerformancePreferenceController extends PreferenceController<TwoStatePreference> {
    private static final Logger LOG = new Logger(PeakPerformancePreferenceController.class);

    @VisibleForTesting
    static final String TURN_OFF_PEAK_PERFORMANCE_DIALOG_TAG =
            "com.android.car.settings.applications.TurnOffPeakPerformanceDialog";

    private final Object mLock = new Object();
    @GuardedBy("mLock")
    private CarWatchdogManager mCarWatchdogManager;

    private Car mCar;
    private String mPackageName;
    private UserHandle mUserHandle;
    private int mKillableState;

    public PeakPerformancePreferenceController(Context context, String preferenceKey,
            FragmentController fragmentController,
            CarUxRestrictions uxRestrictions) {
        super(context, preferenceKey, fragmentController, uxRestrictions);
    }

    @Override
    protected void onCreateInternal() {
        if (mCar != null && mCar.isConnected()) {
            mCar.disconnect();
            mCar = null;
        }
        mCar = Car.createCar(getContext(), null, Car.CAR_WAIT_TIMEOUT_WAIT_FOREVER,
                (Car car, boolean ready) -> {
                    synchronized (mLock) {
                        if (ready) {
                            mCarWatchdogManager = (CarWatchdogManager) car.getCarManager(
                                    Car.CAR_WATCHDOG_SERVICE);
                        } else {
                            mCarWatchdogManager = null;
                        }
                    }
                });
    }

    @Override
    protected void onDestroyInternal() {
        if (mCar != null) {
            mCar.disconnect();
            mCar = null;
        }
    }

    /**
     * Set the package info of the application.
     */
    public void setPackageInfo(PackageInfo packageInfo) {
        mPackageName = packageInfo.packageName;
        mUserHandle = UserHandle.getUserHandleForUid(packageInfo.applicationInfo.uid);
    }

    @Override
    protected Class<TwoStatePreference> getPreferenceType() {
        return TwoStatePreference.class;
    }

    @Override
    protected void updateState(TwoStatePreference preference) {
        mKillableState = getKillableState();
        preference.setSummary(getContext().getString(R.string.peak_performance_summary));
        preference.setChecked(mKillableState == PackageKillableState.KILLABLE_STATE_YES);
        preference.setEnabled(mKillableState != PackageKillableState.KILLABLE_STATE_NEVER);
    }

    @Override
    protected boolean handlePreferenceChanged(TwoStatePreference preference, Object newValue) {
        if (mKillableState == PackageKillableState.KILLABLE_STATE_NEVER) {
            return false;
        }

        boolean shouldKill = (boolean) newValue;

        // If shouldKill is true, switch toggle going from OFF to ON.
        if (shouldKill) {
            setKillableState(true);
            preference.setChecked(true);
            mKillableState = PackageKillableState.KILLABLE_STATE_YES;
        } else {
            ConfirmationDialogFragment dialogFragment =
                    new ConfirmationDialogFragment.Builder(getContext())
                            .setTitle(R.string.peak_performance_dialog_title)
                            .setMessage(R.string.peak_performance_dialog_text)
                            .setPositiveButton(R.string.peak_performance_dialog_action_off,
                                    arguments -> {
                                        setKillableState(false);
                                        preference.setChecked(false);
                                        mKillableState = PackageKillableState.KILLABLE_STATE_NO;
                                    })
                            .setNegativeButton(
                                    R.string.peak_performance_dialog_action_on,
                                    /* rejectListener= */null)
                            .build();
            getFragmentController().showDialog(dialogFragment,
                    TURN_OFF_PEAK_PERFORMANCE_DIALOG_TAG);
        }
        return shouldKill;
    }

    private int getKillableState() {
        synchronized (mLock) {
            return Objects.requireNonNull(mCarWatchdogManager).getPackageKillableStatesAsUser(
                    mUserHandle).stream()
                    .filter(pks -> pks.getPackageName().equals(mPackageName))
                    .findFirst().map(PackageKillableState::getKillableState).orElse(-1);
        }
    }

    private void setKillableState(boolean isKillable) {
        synchronized (mLock) {
            mCarWatchdogManager.setKillablePackageAsUser(mPackageName, mUserHandle, isKillable);
        }
    }
}
