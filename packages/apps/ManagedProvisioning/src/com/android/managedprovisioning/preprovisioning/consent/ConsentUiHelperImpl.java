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
package com.android.managedprovisioning.preprovisioning.consent;

import static java.util.Objects.requireNonNull;

import android.annotation.DrawableRes;
import android.app.Activity;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.RawRes;

import com.android.managedprovisioning.R;
import com.android.managedprovisioning.common.ProvisionLogger;
import com.android.managedprovisioning.common.ThemeHelper;
import com.android.managedprovisioning.common.TouchTargetEnforcer;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.CustomizationParams;
import com.android.managedprovisioning.preprovisioning.PreProvisioningActivityBridgeCallbacks;
import com.android.managedprovisioning.preprovisioning.PreProvisioningActivityController.UiParams;

import com.airbnb.lottie.LottieAnimationView;
import com.google.android.setupdesign.GlifLayout;


/**
 * Implements functionality for the consent screen.
 */
class ConsentUiHelperImpl implements ConsentUiHelper {
    private final Activity mActivity;
    private final TouchTargetEnforcer mTouchTargetEnforcer;
    private final ConsentUiHelperCallback mCallback;
    private final Utils mUtils;
    private final PreProvisioningActivityBridgeCallbacks mBridgeCallbacks;
    private final ThemeHelper mThemeHelper;

    ConsentUiHelperImpl(Activity activity, ConsentUiHelperCallback callback, Utils utils,
            PreProvisioningActivityBridgeCallbacks bridgeCallbacks,
            ThemeHelper themeHelper) {
        mActivity = requireNonNull(activity);
        mCallback = requireNonNull(callback);
        mTouchTargetEnforcer =
            new TouchTargetEnforcer(activity.getResources().getDisplayMetrics().density);
        mUtils = requireNonNull(utils);
        mBridgeCallbacks = requireNonNull(bridgeCallbacks);
        mThemeHelper = requireNonNull(themeHelper);
    }

    @Override
    public void initiateUi(UiParams uiParams) {
        int titleResId = 0;
        int headerResId = 0;
        int animationResId = 0;
        if (mUtils.isProfileOwnerAction(uiParams.provisioningAction)) {
            titleResId = R.string.setup_profile;
            headerResId = R.string.work_profile_provisioning_accept_header_post_suw;
            animationResId = R.raw.consent_animation_po;
        } else if (mUtils.isDeviceOwnerAction(uiParams.provisioningAction)) {
            titleResId = R.string.setup_device;
            headerResId = R.string.fully_managed_device_provisioning_accept_header;
            animationResId = R.raw.consent_animation_do;
        }

        final CustomizationParams customization = uiParams.customization;
        mCallback.onInitiateUi(
                R.layout.intro,
                headerResId,
                customization);

        setupAnimation(animationResId);
        setupAcceptAndContinueButton();

        // set the activity title
        mActivity.setTitle(titleResId);

        // set up terms headers
        setupViewTermsButton();
    }

    @Override
    public void onStart() {}

    @Override
    public void onStop() {}

    private void setupAnimation(@RawRes int animationResId) {
        final GlifLayout layout = mActivity.findViewById(R.id.setup_wizard_layout);
        LottieAnimationView lottieAnimationView = layout.findViewById(R.id.animation);
        lottieAnimationView.setAnimation(animationResId);
        mThemeHelper.setupAnimationDynamicColors(
                mActivity, lottieAnimationView, mActivity.getIntent());
    }

    private void setupAcceptAndContinueButton() {
        final GlifLayout layout = mActivity.findViewById(R.id.setup_wizard_layout);
        Utils.addAcceptAndContinueButton(layout, v -> onNextButtonClicked());
    }

    private void onNextButtonClicked() {
        ProvisionLogger.logi("Next button (next_button) is clicked.");
        mBridgeCallbacks.onTermsAccepted();
    }

    private void setupViewTermsButton() {
        final GlifLayout layout = mActivity.findViewById(R.id.setup_wizard_layout);
        layout.setDescriptionText(R.string.view_terms);
        TextView subtitle = layout.findViewById(R.id.sud_layout_subtitle);
        subtitle.setTextColor(mUtils.getAccentColor(mActivity));
        subtitle.setOnClickListener(v -> mBridgeCallbacks.onTermsButtonClicked());
        mTouchTargetEnforcer.enforce(subtitle, (View) subtitle.getParent());
    }
}
