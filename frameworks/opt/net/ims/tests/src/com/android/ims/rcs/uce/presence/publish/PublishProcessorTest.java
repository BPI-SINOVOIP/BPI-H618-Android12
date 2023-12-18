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

import static android.telephony.ims.RcsContactPresenceTuple.TUPLE_BASIC_STATUS_OPEN;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.net.Uri;
import android.telephony.ims.RcsContactPresenceTuple;
import android.telephony.ims.RcsContactUceCapability;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SmallTest;

import com.android.ims.ImsTestBase;
import com.android.ims.RcsFeatureManager;
import com.android.ims.rcs.uce.presence.publish.PublishController.PublishControllerCallback;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;

@RunWith(AndroidJUnit4.class)
public class PublishProcessorTest extends ImsTestBase {

    @Mock RcsFeatureManager mRcsFeatureManager;
    @Mock DeviceCapabilityInfo mDeviceCapabilities;
    @Mock PublishControllerCallback mPublishCtrlCallback;
    @Mock PublishProcessorState mProcessorState;
    @Mock PublishRequestResponse mResponseCallback;

    private int mSub = 1;
    private long mTaskId = 1L;

    @Before
    public void setUp() throws Exception {
        super.setUp();
        doReturn(true).when(mProcessorState).isPublishAllowedAtThisTime();
        doReturn(mTaskId).when(mProcessorState).getCurrentTaskId();

        doReturn(true).when(mDeviceCapabilities).isImsRegistered();
        RcsContactUceCapability capability = getRcsContactUceCapability();
        doReturn(capability).when(mDeviceCapabilities).getDeviceCapabilities(anyInt(), any());

        doReturn(mTaskId).when(mResponseCallback).getTaskId();
    }

    @After
    public void tearDown() throws Exception {
        super.tearDown();
    }

    @Test
    @SmallTest
    public void testDoPublish() throws Exception {
        PublishProcessor publishProcessor = getPublishProcessor();

        publishProcessor.doPublish(PublishController.PUBLISH_TRIGGER_SERVICE);

        verify(mDeviceCapabilities).getDeviceCapabilities(anyInt(), any());
        verify(mProcessorState).setPublishingFlag(true);
        verify(mRcsFeatureManager).requestPublication(any(), any());
        verify(mPublishCtrlCallback).setupRequestCanceledTimer(anyLong(), anyLong());
    }

    @Test
    @SmallTest
    public void testPublishWithoutResetRetryCount() throws Exception {
        PublishProcessor publishProcessor = getPublishProcessor();

        publishProcessor.doPublish(PublishController.PUBLISH_TRIGGER_RETRY);

        verify(mProcessorState, never()).resetRetryCount();
    }

    @Test
    @SmallTest
    public void testNotPublishWhenImsNotRegistered() throws Exception {
        doReturn(false).when(mDeviceCapabilities).isImsRegistered();
        PublishProcessor publishProcessor = getPublishProcessor();

        publishProcessor.doPublish(PublishController.PUBLISH_TRIGGER_RETRY);

        verify(mRcsFeatureManager, never()).requestPublication(any(), any());
    }

    @Test
    @SmallTest
    public void testNotPublishWhenReachMaximumRetries() throws Exception {
        doReturn(true).when(mProcessorState).isPublishingNow();
        doReturn(mTaskId).when(mProcessorState).getCurrentTaskId();
        doReturn(mTaskId).when(mResponseCallback).getTaskId();
        doReturn(true).when(mResponseCallback).needRetry();
        doReturn(true).when(mProcessorState).isReachMaximumRetries();
        PublishProcessor publishProcessor = getPublishProcessor();

        publishProcessor.onNetworkResponse(mResponseCallback);

        verify(mPublishCtrlCallback).updatePublishRequestResult(anyInt(), any(), any());
        verify(mResponseCallback).onDestroy();
        verify(mProcessorState).setPublishingFlag(false);
        verify(mPublishCtrlCallback).clearRequestCanceledTimer();
    }

    @Test
    @SmallTest
    public void testNotPublishWhenCurrentTimeNotAllowed() throws Exception {
        doReturn(false).when(mProcessorState).isPublishAllowedAtThisTime();
        PublishProcessor publishProcessor = getPublishProcessor();

        publishProcessor.doPublish(PublishController.PUBLISH_TRIGGER_RETRY);

        verify(mPublishCtrlCallback).requestPublishFromInternal(
                eq(PublishController.PUBLISH_TRIGGER_RETRY));
        verify(mRcsFeatureManager, never()).requestPublication(any(), any());
    }

    @Test
    @SmallTest
    public void testCommandErrorWithRetry() throws Exception {
        doReturn(true).when(mProcessorState).isPublishingNow();
        doReturn(mTaskId).when(mProcessorState).getCurrentTaskId();
        doReturn(mTaskId).when(mResponseCallback).getTaskId();
        doReturn(true).when(mResponseCallback).needRetry();
        PublishProcessor publishProcessor = getPublishProcessor();

        publishProcessor.onCommandError(mResponseCallback);

        verify(mProcessorState).increaseRetryCount();
        verify(mPublishCtrlCallback).requestPublishFromInternal(
                eq(PublishController.PUBLISH_TRIGGER_RETRY));
        verify(mResponseCallback).onDestroy();
        verify(mProcessorState).setPublishingFlag(false);
        verify(mPublishCtrlCallback).clearRequestCanceledTimer();
    }

    @Test
    @SmallTest
    public void testCommandErrorWithoutRetry() throws Exception {
        doReturn(true).when(mProcessorState).isPublishingNow();
        doReturn(mTaskId).when(mProcessorState).getCurrentTaskId();
        doReturn(mTaskId).when(mResponseCallback).getTaskId();
        doReturn(false).when(mResponseCallback).needRetry();
        PublishProcessor publishProcessor = getPublishProcessor();

        publishProcessor.onCommandError(mResponseCallback);

        verify(mPublishCtrlCallback).updatePublishRequestResult(anyInt(), any(), any());
        verify(mResponseCallback).onDestroy();
        verify(mProcessorState).setPublishingFlag(false);
        verify(mPublishCtrlCallback).clearRequestCanceledTimer();
    }

    @Test
    @SmallTest
    public void testNetworkResponseWithRetry() throws Exception {
        doReturn(true).when(mProcessorState).isPublishingNow();
        doReturn(mTaskId).when(mProcessorState).getCurrentTaskId();
        doReturn(mTaskId).when(mResponseCallback).getTaskId();
        doReturn(true).when(mResponseCallback).needRetry();
        PublishProcessor publishProcessor = getPublishProcessor();

        publishProcessor.onNetworkResponse(mResponseCallback);

        verify(mProcessorState).increaseRetryCount();
        verify(mPublishCtrlCallback).requestPublishFromInternal(
                eq(PublishController.PUBLISH_TRIGGER_RETRY));
        verify(mResponseCallback).onDestroy();
        verify(mProcessorState).setPublishingFlag(false);
        verify(mPublishCtrlCallback).clearRequestCanceledTimer();
    }

    @Test
    @SmallTest
    public void testNetworkResponseSuccessful() throws Exception {
        doReturn(true).when(mProcessorState).isPublishingNow();
        doReturn(mTaskId).when(mProcessorState).getCurrentTaskId();
        doReturn(mTaskId).when(mResponseCallback).getTaskId();
        doReturn(false).when(mResponseCallback).needRetry();
        doReturn(true).when(mResponseCallback).isRequestSuccess();
        PublishProcessor publishProcessor = getPublishProcessor();

        publishProcessor.onNetworkResponse(mResponseCallback);

        verify(mPublishCtrlCallback).updatePublishRequestResult(anyInt(), any(), any());
        verify(mResponseCallback).onDestroy();
        verify(mProcessorState).setPublishingFlag(false);
        verify(mPublishCtrlCallback).clearRequestCanceledTimer();
    }

    @Test
    @SmallTest
    public void testCancelPublishRequest() throws Exception {
        PublishProcessor publishProcessor = getPublishProcessor();

        publishProcessor.cancelPublishRequest(mTaskId);

        verify(mProcessorState).setPublishingFlag(false);
        verify(mPublishCtrlCallback).clearRequestCanceledTimer();
    }

    private PublishProcessor getPublishProcessor() {
        PublishProcessor publishProcessor = new PublishProcessor(mContext, mSub,
                mDeviceCapabilities, mPublishCtrlCallback);
        publishProcessor.setProcessorState(mProcessorState);
        publishProcessor.onRcsConnected(mRcsFeatureManager);
        return publishProcessor;
    }

    private RcsContactUceCapability getRcsContactUceCapability() {
        Uri contact = Uri.fromParts("sip", "test", null);

        RcsContactUceCapability.PresenceBuilder builder =
                new RcsContactUceCapability.PresenceBuilder(contact,
                        RcsContactUceCapability.SOURCE_TYPE_CACHED,
                        RcsContactUceCapability.REQUEST_RESULT_FOUND);

        RcsContactPresenceTuple.Builder tupleBuilder = new RcsContactPresenceTuple.Builder(
                TUPLE_BASIC_STATUS_OPEN, RcsContactPresenceTuple.SERVICE_ID_MMTEL, "1.0");

        builder.addCapabilityTuple(tupleBuilder.build());
        return builder.build();
    }
}
