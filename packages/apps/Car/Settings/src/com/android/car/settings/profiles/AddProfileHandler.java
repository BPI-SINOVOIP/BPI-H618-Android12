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

package com.android.car.settings.profiles;

import android.car.Car;
import android.car.user.CarUserManager;
import android.content.Context;
import android.os.UserManager;

import androidx.annotation.VisibleForTesting;
import androidx.preference.Preference;

import com.android.car.settings.R;
import com.android.car.settings.common.ConfirmationDialogFragment;
import com.android.car.settings.common.ErrorDialog;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceController;

/**
 * Consolidates adding profile logic into one handler so we can have consistent logic across various
 * parts of the Settings app.
 */
public class AddProfileHandler implements AddNewProfileTask.AddNewProfileListener {

    @VisibleForTesting
    static final String CONFIRM_CREATE_NEW_PROFILE_DIALOG_TAG =
            "com.android.car.settings.profiles.ConfirmCreateNewProfileDialog";

    @VisibleForTesting
    AddNewProfileTask mAddNewProfileTask;
    /** Indicates that a task is running. */
    private boolean mIsBusy;

    private final Context mContext;
    private final FragmentController mFragmentController;
    private final PreferenceController mPreferenceController;
    private final Car mCar;
    private CarUserManager mCarUserManager;

    @VisibleForTesting
    ConfirmationDialogFragment.ConfirmListener mConfirmCreateNewProfileListener;

    public AddProfileHandler(Context context, FragmentController fragmentController,
            PreferenceController preferenceController) {
        mContext = context;
        mFragmentController = fragmentController;
        mPreferenceController = preferenceController;
        mCar = Car.createCar(context);
        mCarUserManager = (CarUserManager) mCar.getCarManager(Car.CAR_USER_SERVICE);

        mConfirmCreateNewProfileListener = arguments -> {
            mAddNewProfileTask = new AddNewProfileTask(mContext,
                    mCarUserManager, /* addNewProfileListener= */ this);
            mAddNewProfileTask.execute(mContext.getString(R.string.user_new_user_name));

            mIsBusy = true;
            mPreferenceController.refreshUi();
        };
    }

    /**
     * Handles operations that should happen in host's onCreateInternal().
     * Resets listeners as they can get unregistered with certain configuration changes.
     */
    public void onCreateInternal() {
        ConfirmationDialogFragment.resetListeners(
                (ConfirmationDialogFragment) mFragmentController.findDialogByTag(
                        CONFIRM_CREATE_NEW_PROFILE_DIALOG_TAG),
                mConfirmCreateNewProfileListener,
                /* rejectListener= */ null,
                /* neutralListener= */ null);
    }

    /**
     * Handles operations that should happen in host's onStopInternal().
     */
    public void onStopInternal() {
        mFragmentController.showProgressBar(false);
    }

    /**
     * Handles events that should happen in host's onDestroyInternal().
     */
    public void onDestroyInternal() {
        if (mAddNewProfileTask != null) {
            mAddNewProfileTask.cancel(/* mayInterruptIfRunning= */ false);
        }
        if (mCar != null) {
            mCar.disconnect();
        }
    }

    /**
     * Handles events that should happen in host's updateState().
     */
    public void updateState(Preference preference) {
        preference.setEnabled(!mIsBusy);
        mFragmentController.showProgressBar(mIsBusy);
    }

    @Override
    public void onProfileAddedSuccess() {
        mIsBusy = false;
        mPreferenceController.refreshUi();
    }

    @Override
    public void onProfileAddedFailure() {
        mIsBusy = false;
        mPreferenceController.refreshUi();
        // Display failure dialog.
        mFragmentController.showDialog(
                ErrorDialog.newInstance(R.string.add_user_error_title), /* tag= */ null);
    }

    /**
     *  Determines whether the user manager instance has the permission to add profiles
     *
     * @param userManager UserManager instance to evaluate
     * @return whether the user has permissions to add profiles
     */
    public boolean canAddProfiles(UserManager userManager) {
        return !userManager.hasUserRestriction(UserManager.DISALLOW_ADD_USER);
    }

    /**
     * Display dialog to add a profile
     */
    public void showAddProfileDialog() {
        ConfirmationDialogFragment dialogFragment =
                ProfilesDialogProvider.getConfirmCreateNewProfileDialogFragment(
                        mContext, mConfirmCreateNewProfileListener, null);

        mFragmentController.showDialog(dialogFragment, CONFIRM_CREATE_NEW_PROFILE_DIALOG_TAG);
    }

    @VisibleForTesting
    void setCarUserManager(CarUserManager carUserManager) {
        mCarUserManager = carUserManager;
    }
}
