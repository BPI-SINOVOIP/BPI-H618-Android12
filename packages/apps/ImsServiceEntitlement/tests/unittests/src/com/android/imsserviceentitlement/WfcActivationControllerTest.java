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

package com.android.imsserviceentitlement;

import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.runner.AndroidJUnit4;

import com.android.imsserviceentitlement.entitlement.EntitlementResult;
import com.android.imsserviceentitlement.ts43.Ts43VowifiStatus;
import com.android.imsserviceentitlement.ts43.Ts43VowifiStatus.AddrStatus;
import com.android.imsserviceentitlement.ts43.Ts43VowifiStatus.EntitlementStatus;
import com.android.imsserviceentitlement.ts43.Ts43VowifiStatus.ProvStatus;
import com.android.imsserviceentitlement.ts43.Ts43VowifiStatus.TcStatus;
import com.android.imsserviceentitlement.utils.Executors;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import java.lang.reflect.Field;

// TODO(b/176127289) add tests
@RunWith(AndroidJUnit4.class)
public class WfcActivationControllerTest {
    @Rule public final MockitoRule rule = MockitoJUnit.rule();
    @Mock private TelephonyManager mTelephonyManager;
    @Mock private ImsEntitlementApi mActivationApi;
    @Mock private WfcActivationUi mActivationUi;
    @Mock private ConnectivityManager mConnectivityManager;
    @Mock private NetworkInfo mNetworkInfo;

    private static final int SUB_ID = 1;
    private static final String EMERGENCY_ADDRESS_WEB_URL = "webUrl";
    private static final String EMERGENCY_ADDRESS_WEB_DATA = "webData";
    private static final String TERMS_AND_CONDITION_WEB_URL = "tncUrl";
    private static final String WEBVIEW_JS_CONTROLLER_NAME = "webviewJsControllerName";

    private WfcActivationController mWfcActivationController;
    private Context mContext;
    private Instrumentation mInstrumentation;

    @Before
    public void setUp() throws Exception {
        mContext = spy(ApplicationProvider.getApplicationContext());

        when(mContext.getSystemService(TelephonyManager.class)).thenReturn(mTelephonyManager);
        when(mTelephonyManager.createForSubscriptionId(SUB_ID)).thenReturn(mTelephonyManager);
        setNetworkConnected(true);

        Field field = Executors.class.getDeclaredField("sUseDirectExecutorForTest");
        field.setAccessible(true);
        field.set(null, true);
    }

    @Test
    public void startFlow_launchAppForActivation_setPurposeActivation() {
        InOrder mOrderVerifier = inOrder(mActivationUi);
        setNetworkConnected(false);
        buildActivity(ActivityConstants.LAUNCH_APP_ACTIVATE);

        mWfcActivationController.startFlow();

        verifyGeneralWaitingUiInOrder(mOrderVerifier, R.string.activate_title);
        verifyErrorUiInOrder(
                mOrderVerifier,
                R.string.activate_title,
                R.string.wfc_activation_error);
    }

    @Test
    public void startFlow_launchAppForUpdate_setPurposeUpdate() {
        InOrder mOrderVerifier = inOrder(mActivationUi);
        setNetworkConnected(false);
        buildActivity(ActivityConstants.LAUNCH_APP_UPDATE);

        mWfcActivationController.startFlow();

        verifyGeneralWaitingUiInOrder(mOrderVerifier, R.string.e911_title);
        verifyErrorUiInOrder(mOrderVerifier, R.string.e911_title, R.string.address_update_error);
    }

    @Test
    public void startFlow_launchAppForShowTc_setPurposeUpdate() {
        InOrder mOrderVerifier = inOrder(mActivationUi);
        setNetworkConnected(false);
        buildActivity(ActivityConstants.LAUNCH_APP_SHOW_TC);

        mWfcActivationController.startFlow();

        verifyGeneralWaitingUiInOrder(mOrderVerifier, R.string.tos_title);
        verifyErrorUiInOrder(
                mOrderVerifier,
                R.string.tos_title,
                R.string.show_terms_and_condition_error);
    }

    @Test
    public void finishFlow_isFinishing_showGeneralWaitingUi() {
        InOrder mOrderVerifier = inOrder(mActivationUi);
        when(mActivationApi.checkEntitlementStatus()).thenReturn(null);
        buildActivity(ActivityConstants.LAUNCH_APP_ACTIVATE);

        mWfcActivationController.finishFlow();

        mOrderVerifier
                .verify(mActivationUi)
                .showActivationUi(
                        R.string.activate_title,
                        R.string.progress_text,
                        true,
                        0,
                        Activity.RESULT_CANCELED,
                        0);
        mOrderVerifier
                .verify(mActivationUi)
                .showActivationUi(
                        R.string.activate_title,
                        R.string.wfc_activation_error,
                        false,
                        R.string.ok,
                        WfcActivationUi.RESULT_FAILURE,
                        0);
    }

    @Test
    public void handleEntitlementStatusForActivation_isVowifiEntitledTrue_setActivityResultOk() {
        EntitlementResult mEntitlementResult =
                EntitlementResult.builder()
                        .setVowifiStatus(
                                Ts43VowifiStatus.builder()
                                        .setEntitlementStatus(EntitlementStatus.ENABLED)
                                        .setTcStatus(TcStatus.AVAILABLE)
                                        .setAddrStatus(AddrStatus.AVAILABLE)
                                        .setProvStatus(ProvStatus.PROVISIONED)
                                        .build())
                        .build();
        when(mActivationApi.checkEntitlementStatus()).thenReturn(mEntitlementResult);
        buildActivity(ActivityConstants.LAUNCH_APP_ACTIVATE);

        mWfcActivationController.evaluateEntitlementStatus();

        verify(mActivationUi).setResultAndFinish(Activity.RESULT_OK);
    }

    @Test
    public void handleEntitlementStatusForActivation_isServerDataMissingTrue_showWebview() {
        EntitlementResult mEntitlementResult =
                EntitlementResult.builder()
                        .setVowifiStatus(
                                Ts43VowifiStatus.builder()
                                        .setEntitlementStatus(EntitlementStatus.DISABLED)
                                        .setTcStatus(TcStatus.NOT_AVAILABLE)
                                        .setAddrStatus(AddrStatus.NOT_AVAILABLE)
                                        .build())
                        .setEmergencyAddressWebUrl(EMERGENCY_ADDRESS_WEB_URL)
                        .setEmergencyAddressWebData(EMERGENCY_ADDRESS_WEB_DATA)
                        .build();
        when(mActivationApi.checkEntitlementStatus()).thenReturn(mEntitlementResult);
        buildActivity(ActivityConstants.LAUNCH_APP_ACTIVATE);

        mWfcActivationController.evaluateEntitlementStatus();

        verify(mActivationUi).showWebview(EMERGENCY_ADDRESS_WEB_URL, EMERGENCY_ADDRESS_WEB_DATA);
    }

    @Test
    public void handleEntitlementStatusForActivation_isIncompatibleTrue_showErrorUi() {
        EntitlementResult mEntitlementResult =
                EntitlementResult.builder()
                        .setVowifiStatus(
                                Ts43VowifiStatus.builder()
                                        .setEntitlementStatus(EntitlementStatus.INCOMPATIBLE)
                                        .build())
                        .build();
        when(mActivationApi.checkEntitlementStatus()).thenReturn(mEntitlementResult);
        buildActivity(ActivityConstants.LAUNCH_APP_ACTIVATE);

        mWfcActivationController.evaluateEntitlementStatus();

        verifyErrorUi(R.string.activate_title, R.string.failure_contact_carrier);
    }

    @Test
    public void handleEntitlementStatusForActivation_unexpectedStatus_showGeneralErrorUi() {
        EntitlementResult mEntitlementResult =
                EntitlementResult.builder()
                        .setVowifiStatus(
                                Ts43VowifiStatus.builder()
                                        .setEntitlementStatus(EntitlementStatus.DISABLED)
                                        .setTcStatus(TcStatus.IN_PROGRESS)
                                        .setAddrStatus(AddrStatus.IN_PROGRESS)
                                        .build())
                        .build();
        when(mActivationApi.checkEntitlementStatus()).thenReturn(mEntitlementResult);
        buildActivity(ActivityConstants.LAUNCH_APP_ACTIVATE);

        mWfcActivationController.evaluateEntitlementStatus();

        verifyErrorUi(R.string.activate_title, R.string.wfc_activation_error);
    }

    @Test
    public void handleEntitlementStatusAfterActivation_isVowifiEntitledTrue_setActivityResultOk() {
        EntitlementResult mEntitlementResult =
                EntitlementResult.builder()
                        .setVowifiStatus(
                                Ts43VowifiStatus.builder()
                                        .setEntitlementStatus(EntitlementStatus.ENABLED)
                                        .setTcStatus(TcStatus.AVAILABLE)
                                        .setAddrStatus(AddrStatus.AVAILABLE)
                                        .setProvStatus(ProvStatus.PROVISIONED)
                                        .build())
                        .build();
        when(mActivationApi.checkEntitlementStatus()).thenReturn(mEntitlementResult);
        buildActivity(ActivityConstants.LAUNCH_APP_ACTIVATE);

        mWfcActivationController.reevaluateEntitlementStatus();

        verify(mActivationUi).setResultAndFinish(Activity.RESULT_OK);
    }

    @Test
    public void handleEntitlementStatusAfterActivation_unexpectedStatus_showGeneralErrorUi() {
        EntitlementResult mEntitlementResult =
                EntitlementResult.builder()
                        .setVowifiStatus(
                                Ts43VowifiStatus.builder()
                                        .setEntitlementStatus(EntitlementStatus.DISABLED)
                                        .setTcStatus(TcStatus.IN_PROGRESS)
                                        .setAddrStatus(AddrStatus.IN_PROGRESS)
                                        .build())
                        .build();
        when(mActivationApi.checkEntitlementStatus()).thenReturn(mEntitlementResult);
        buildActivity(ActivityConstants.LAUNCH_APP_ACTIVATE);

        mWfcActivationController.reevaluateEntitlementStatus();

        verifyErrorUi(R.string.activate_title, R.string.wfc_activation_error);
    }

    private void buildActivity(int extraLaunchCarrierApp) {
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.putExtra(SubscriptionManager.EXTRA_SUBSCRIPTION_INDEX, SUB_ID);
        intent.putExtra(ActivityConstants.EXTRA_LAUNCH_CARRIER_APP, extraLaunchCarrierApp);
        mWfcActivationController =
                new WfcActivationController(mContext, mActivationUi, mActivationApi, intent);
    }

    private void setNetworkConnected(boolean isConnected) {
        when(mNetworkInfo.isConnected()).thenReturn(isConnected);
        when(mContext.getSystemService(Context.CONNECTIVITY_SERVICE)).thenReturn(
                mConnectivityManager);
        when(mConnectivityManager.getActiveNetworkInfo()).thenReturn(mNetworkInfo);
        when(mNetworkInfo.isConnected()).thenReturn(isConnected);
    }

    private void verifyErrorUi(int title, int errorMesssage) {
        verify(mActivationUi)
                .showActivationUi(
                        title,
                        errorMesssage,
                        false, R.string.ok,
                        WfcActivationUi.RESULT_FAILURE,
                        0);
    }

    private void verifyErrorUiInOrder(InOrder inOrder, int title, int errorMesssage) {
        inOrder.verify(mActivationUi)
                .showActivationUi(
                        title,
                        errorMesssage,
                        false, R.string.ok,
                        WfcActivationUi.RESULT_FAILURE,
                        0);
    }

    private void verifyGeneralWaitingUiInOrder(InOrder inOrder, int title) {
        inOrder.verify(mActivationUi)
                .showActivationUi(title, R.string.progress_text, true, 0, 0, 0);
    }
}
