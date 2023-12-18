/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.ims.rcs.uce.presence.publish;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.verify;

import android.content.BroadcastReceiver;
import android.content.Intent;
import android.os.Handler;
import android.telecom.TelecomManager;
import android.telephony.ims.ImsMmTelManager;
import android.telephony.ims.ImsRcsManager;
import android.telephony.ims.ImsReasonInfo;
import android.telephony.ims.ImsRegistrationAttributes;
import android.telephony.ims.ProvisioningManager;
import android.telephony.ims.RegistrationManager.RegistrationCallback;
import android.telephony.ims.feature.MmTelFeature;
import android.telephony.ims.stub.ImsRegistrationImplBase;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SmallTest;

import com.android.ims.ImsTestBase;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;

@RunWith(AndroidJUnit4.class)
public class DeviceCapabilityListenerTest extends ImsTestBase {

    private static final long HANDLER_WAIT_TIMEOUT_MS = 2000L;
    private static final long HANDLER_SENT_DELAY_MS = 1000L;

    @Mock DeviceCapabilityInfo mDeviceCapability;
    @Mock PublishController.PublishControllerCallback mCallback;
    @Mock ImsMmTelManager mImsMmTelManager;
    @Mock ImsRcsManager mImsRcsManager;
    @Mock ProvisioningManager mProvisioningManager;
    @Mock DeviceCapabilityListener.ImsMmTelManagerFactory mImsMmTelMgrFactory;
    @Mock DeviceCapabilityListener.ImsRcsManagerFactory mImsRcsMgrFactory;
    @Mock DeviceCapabilityListener.ProvisioningManagerFactory mProvisioningMgrFactory;

    int mSubId = 1;

    @Before
    public void setUp() throws Exception {
        super.setUp();
        doReturn(mImsMmTelManager).when(mImsMmTelMgrFactory).getImsMmTelManager(anyInt());
        doReturn(mImsRcsManager).when(mImsRcsMgrFactory).getImsRcsManager(anyInt());
        doReturn(mProvisioningManager).when(mProvisioningMgrFactory).
                getProvisioningManager(anyInt());

        doReturn(true).when(mDeviceCapability).updateTtyPreferredMode(anyInt());
        doReturn(true).when(mDeviceCapability).updateAirplaneMode(anyBoolean());
        doReturn(true).when(mDeviceCapability).updateMobileData(anyBoolean());
        doReturn(true).when(mDeviceCapability).updateVtSetting(anyBoolean());
        doReturn(true).when(mDeviceCapability).updateVtSetting(anyBoolean());
        doReturn(true).when(mDeviceCapability).updateMmtelCapabilitiesChanged(any());
    }

    @After
    public void tearDown() throws Exception {
        super.tearDown();
    }

    @Test
    @SmallTest
    public void testTurnOnListener() throws Exception {
        DeviceCapabilityListener deviceCapListener = createDeviceCapabilityListener();

        deviceCapListener.initialize();

        verify(mContext).registerReceiver(any(), any());
        verify(mProvisioningManager).registerProvisioningChangedCallback(any(), any());
    }

    @Test
    @SmallTest
    public void testDestroy() throws Exception {
        DeviceCapabilityListener deviceCapListener = createDeviceCapabilityListener();
        deviceCapListener.initialize();

        // The listener is destroyed.
        deviceCapListener.onDestroy();

        verify(mContext).unregisterReceiver(any());
        verify(mProvisioningManager).unregisterProvisioningChangedCallback(any());
    }

    @Test
    @SmallTest
    public void testTtyPreferredModeChange() throws Exception {
        DeviceCapabilityListener deviceCapListener = createDeviceCapabilityListener();
        final BroadcastReceiver receiver = deviceCapListener.mReceiver;

        Intent intent = new Intent(TelecomManager.ACTION_TTY_PREFERRED_MODE_CHANGED);
        receiver.onReceive(mContext, intent);

        Handler handler = deviceCapListener.getHandler();
        waitForHandlerActionDelayed(handler, HANDLER_WAIT_TIMEOUT_MS, HANDLER_SENT_DELAY_MS);

        verify(mDeviceCapability).updateTtyPreferredMode(anyInt());
        verify(mCallback).requestPublishFromInternal(
                PublishController.PUBLISH_TRIGGER_TTY_PREFERRED_CHANGE);
    }

    @Test
    @SmallTest
    public void testAirplaneModeChange() throws Exception {
        DeviceCapabilityListener deviceCapListener = createDeviceCapabilityListener();
        final BroadcastReceiver receiver = deviceCapListener.mReceiver;

        Intent intent = new Intent(Intent.ACTION_AIRPLANE_MODE_CHANGED);
        receiver.onReceive(mContext, intent);

        Handler handler = deviceCapListener.getHandler();
        waitForHandlerActionDelayed(handler, HANDLER_WAIT_TIMEOUT_MS, HANDLER_SENT_DELAY_MS);

        verify(mDeviceCapability).updateAirplaneMode(anyBoolean());
        verify(mCallback).requestPublishFromInternal(
                PublishController.PUBLISH_TRIGGER_AIRPLANE_MODE_CHANGE);
    }

    @Test
    @SmallTest
    public void testMmtelRegistration() throws Exception {
        DeviceCapabilityListener deviceCapListener = createDeviceCapabilityListener();
        deviceCapListener.setImsCallbackRegistered(true);
        RegistrationCallback registrationCallback = deviceCapListener.mMmtelRegistrationCallback;

        registrationCallback.onRegistered(1);

        Handler handler = deviceCapListener.getHandler();
        waitForHandlerActionDelayed(handler, HANDLER_WAIT_TIMEOUT_MS, HANDLER_SENT_DELAY_MS);

        verify(mDeviceCapability).updateImsMmtelRegistered(1);
        verify(mCallback).requestPublishFromInternal(
                PublishController.PUBLISH_TRIGGER_MMTEL_REGISTERED);
    }

    @Test
    @SmallTest
    public void testMmTelUnregistration() throws Exception {
        DeviceCapabilityListener deviceCapListener = createDeviceCapabilityListener();
        deviceCapListener.setImsCallbackRegistered(true);
        RegistrationCallback registrationCallback = deviceCapListener.mMmtelRegistrationCallback;

        ImsReasonInfo info = new ImsReasonInfo(ImsReasonInfo.CODE_LOCAL_NOT_REGISTERED, -1, "");
        registrationCallback.onUnregistered(info);

        Handler handler = deviceCapListener.getHandler();
        waitForHandlerActionDelayed(handler, HANDLER_WAIT_TIMEOUT_MS, HANDLER_SENT_DELAY_MS);

        verify(mDeviceCapability).updateImsMmtelUnregistered();
        verify(mCallback).requestPublishFromInternal(
                PublishController.PUBLISH_TRIGGER_MMTEL_UNREGISTERED);
    }

    @Test
    @SmallTest
    public void testRcsRegistration() throws Exception {
        DeviceCapabilityListener deviceCapListener = createDeviceCapabilityListener();
        deviceCapListener.setImsCallbackRegistered(true);
        RegistrationCallback registrationCallback = deviceCapListener.mRcsRegistrationCallback;
        ImsRegistrationAttributes attr = new ImsRegistrationAttributes.Builder(
                ImsRegistrationImplBase.REGISTRATION_TECH_LTE).build();
        // Notify DeviceCapabilityListener that registered has caused a change and requires publish
        doReturn(true).when(mDeviceCapability).updateImsRcsRegistered(attr);

        registrationCallback.onRegistered(attr);
        Handler handler = deviceCapListener.getHandler();
        waitForHandlerActionDelayed(handler, HANDLER_WAIT_TIMEOUT_MS, HANDLER_SENT_DELAY_MS);

        verify(mDeviceCapability).updateImsRcsRegistered(attr);
        verify(mCallback).requestPublishFromInternal(
                PublishController.PUBLISH_TRIGGER_RCS_REGISTERED);
    }

    @Test
    @SmallTest
    public void testRcsUnregistration() throws Exception {
        DeviceCapabilityListener deviceCapListener = createDeviceCapabilityListener();
        deviceCapListener.setImsCallbackRegistered(true);
        RegistrationCallback registrationCallback = deviceCapListener.mRcsRegistrationCallback;
        // Notify DeviceCapabilityListener that unregistered has caused a change and requires
        // publish.
        doReturn(true).when(mDeviceCapability).updateImsRcsUnregistered();

        ImsReasonInfo info = new ImsReasonInfo(ImsReasonInfo.CODE_LOCAL_NOT_REGISTERED, -1, "");
        registrationCallback.onUnregistered(info);

        Handler handler = deviceCapListener.getHandler();
        waitForHandlerActionDelayed(handler, HANDLER_WAIT_TIMEOUT_MS, HANDLER_SENT_DELAY_MS);

        verify(mDeviceCapability).updateImsRcsUnregistered();
        verify(mCallback).requestPublishFromInternal(
                PublishController.PUBLISH_TRIGGER_RCS_UNREGISTERED);
    }

    @Test
    @SmallTest
    public void testMmtelCapabilityChange() throws Exception {
        DeviceCapabilityListener deviceCapListener = createDeviceCapabilityListener();
        ImsMmTelManager.CapabilityCallback callback = deviceCapListener.mMmtelCapabilityCallback;

        MmTelFeature.MmTelCapabilities capabilities = new MmTelFeature.MmTelCapabilities();
        callback.onCapabilitiesStatusChanged(capabilities);

        Handler handler = deviceCapListener.getHandler();
        waitForHandlerActionDelayed(handler, HANDLER_WAIT_TIMEOUT_MS, HANDLER_SENT_DELAY_MS);

        verify(mDeviceCapability).updateMmtelCapabilitiesChanged(capabilities);
        verify(mCallback).requestPublishFromInternal(
                PublishController.PUBLISH_TRIGGER_MMTEL_CAPABILITY_CHANGE);
    }

    private DeviceCapabilityListener createDeviceCapabilityListener() {
        DeviceCapabilityListener deviceCapListener = new DeviceCapabilityListener(mContext,
                mSubId, mDeviceCapability, mCallback);
        deviceCapListener.setImsMmTelManagerFactory(mImsMmTelMgrFactory);
        deviceCapListener.setImsRcsManagerFactory(mImsRcsMgrFactory);
        deviceCapListener.setProvisioningMgrFactory(mProvisioningMgrFactory);
        return deviceCapListener;
    }
}
