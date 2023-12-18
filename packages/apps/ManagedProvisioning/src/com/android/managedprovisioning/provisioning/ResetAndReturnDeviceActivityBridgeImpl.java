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

package com.android.managedprovisioning.provisioning;

import android.app.Activity;

import com.android.managedprovisioning.R;
import com.android.managedprovisioning.common.InitializeLayoutConsumerHandler;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.CustomizationParams;
import com.android.managedprovisioning.model.ProvisioningParams;

import com.google.android.setupdesign.GlifLayout;
import com.google.auto.value.AutoValue;

@AutoValue
abstract class ResetAndReturnDeviceActivityBridgeImpl
        implements ResetAndReturnDeviceActivityBridge {

    abstract Utils getUtils();
    abstract ProvisioningParams getParams();
    abstract ResetAndReturnDeviceActivityBridgeCallback getBridgeCallback();
    abstract InitializeLayoutConsumerHandler getInitializeLayoutParamsConsumer();

    @Override
    public void initiateUi(Activity activity) {
        CustomizationParams customizationParams =
                CustomizationParams.createInstance(getParams(), activity, getUtils());
        getInitializeLayoutParamsConsumer()
                .initializeLayoutParams(R.layout.return_device_screen, null, customizationParams);

        GlifLayout layout = activity.findViewById(R.id.setup_wizard_layout);
        layout.setIcon(activity.getDrawable(R.drawable.ic_error_outline));
        Utils.addResetButton(layout, v -> getBridgeCallback().onResetButtonClicked());
    }

    static Builder builder() {
        return new AutoValue_ResetAndReturnDeviceActivityBridgeImpl.Builder();
    }

    @AutoValue.Builder
    abstract static class Builder {
        abstract Builder setUtils(Utils utils);
        abstract Builder setParams(ProvisioningParams params);
        abstract Builder setBridgeCallback(ResetAndReturnDeviceActivityBridgeCallback callback);
        abstract Builder setInitializeLayoutParamsConsumer(
                InitializeLayoutConsumerHandler initializeLayoutParamsConsumer);
        abstract ResetAndReturnDeviceActivityBridgeImpl build();
    }
}
