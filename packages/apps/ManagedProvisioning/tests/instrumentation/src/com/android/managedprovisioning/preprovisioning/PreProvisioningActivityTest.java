/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.managedprovisioning.preprovisioning;

import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_COMPONENT_NAME;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_LOGO_URI;

import static com.android.managedprovisioning.e2eui.ManagedProfileAdminReceiver.COMPONENT_NAME;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;

import androidx.test.filters.SmallTest;
import androidx.test.rule.ActivityTestRule;

import com.android.managedprovisioning.TestInstrumentationRunner;
import com.android.managedprovisioning.TestUtils;
import com.android.managedprovisioning.analytics.TimeLogger;
import com.android.managedprovisioning.common.CustomizationVerifier;
import com.android.managedprovisioning.common.GetProvisioningModeUtils;
import com.android.managedprovisioning.common.ManagedProvisioningSharedPreferences;
import com.android.managedprovisioning.common.PolicyComplianceUtils;
import com.android.managedprovisioning.common.SettingsFacade;
import com.android.managedprovisioning.common.ThemeHelper;
import com.android.managedprovisioning.common.ThemeHelper.DefaultNightModeChecker;
import com.android.managedprovisioning.common.ThemeHelper.DefaultSetupWizardBridge;
import com.android.managedprovisioning.common.UriBitmap;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.parser.MessageParser;
import com.android.managedprovisioning.preprovisioning.terms.TermsActivity;

import org.junit.AfterClass;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import java.io.IOException;

@SmallTest
@RunWith(MockitoJUnitRunner.class)
// TODO: Currently only color and logo functionality are covered. Fill in the rest (b/32131665).
public class PreProvisioningActivityTest {
    private static final int DEFAULT_LOGO_COLOR = Color.rgb(99, 99, 99);

    @Mock
    private Utils mUtils;

    @Rule
    public ActivityTestRule<PreProvisioningActivity> mActivityRule = new ActivityTestRule<>(
            PreProvisioningActivity.class, true, false);

    @Before
    public void setup() {
        when(mUtils.getAccentColor(any())).thenReturn(DEFAULT_LOGO_COLOR);

        TestInstrumentationRunner.registerReplacedActivity(PreProvisioningActivity.class,
                (classLoader, className, intent) -> new PreProvisioningActivity(
                        activity -> new PreProvisioningActivityController(
                                activity,
                                activity,
                                mUtils,
                                new SettingsFacade(),
                                new ManagedProvisioningSharedPreferences(activity),
                                new PolicyComplianceUtils(),
                                new GetProvisioningModeUtils(),
                                new PreProvisioningViewModel(
                                        new TimeLogger(activity, 0 /* category */),
                                        new MessageParser(activity),
                                        TestUtils.createEncryptionController(activity))
                        ) {
                            @Override
                            protected boolean checkDevicePolicyPreconditions() {
                                return true;
                            }

                            @Override
                            protected boolean verifyActionAndCaller(Intent intent,
                                    String caller) {
                                return true;
                            }
                        }, null,
                        mUtils,
                        new SettingsFacade(),
                        new ThemeHelper(
                                new DefaultNightModeChecker(), new DefaultSetupWizardBridge())));
    }

    @AfterClass
    public static void tearDownClass() {
        TestInstrumentationRunner.unregisterReplacedActivity(TermsActivity.class);
    }

    @Ignore("b/181323689")
    @Test
    public void deviceOwnerDefaultLogo() {
        Activity activity = mActivityRule.launchActivity(
                createIntent(ACTION_PROVISION_MANAGED_DEVICE));
        CustomizationVerifier v = new CustomizationVerifier(activity);

        v.assertDefaultLogoCorrect(DEFAULT_LOGO_COLOR);
    }

    @Ignore("b/181323689")
    @Test
    public void deviceOwnerCustomLogo() throws IOException {
        UriBitmap expectedLogo = UriBitmap.createSimpleInstance();

        Activity activity = mActivityRule.launchActivity(
                createIntent(ACTION_PROVISION_MANAGED_DEVICE)
                        .putExtra(EXTRA_PROVISIONING_LOGO_URI, expectedLogo.getUri()));
        CustomizationVerifier v = new CustomizationVerifier(activity);

        v.assertCustomLogoCorrect(expectedLogo.getBitmap());
    }

    private Intent createIntent(String provisioningAction) {
        Intent intent = new Intent(provisioningAction).putExtra(
                EXTRA_PROVISIONING_DEVICE_ADMIN_COMPONENT_NAME, COMPONENT_NAME);
        return intent;
    }
}