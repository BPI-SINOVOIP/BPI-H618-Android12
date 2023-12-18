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

package com.android.bluetooth.mapclient;

import static android.Manifest.permission.BLUETOOTH_CONNECT;
import static org.mockito.Mockito.*;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothMapClient;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.SdpMasRecord;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.provider.Telephony.Sms;
import android.telephony.SubscriptionManager;
import android.test.mock.MockContentProvider;
import android.test.mock.MockContentResolver;
import android.util.Log;

import androidx.test.InstrumentationRegistry;
import androidx.test.filters.MediumTest;
import androidx.test.rule.ServiceTestRule;
import androidx.test.runner.AndroidJUnit4;

import com.android.bluetooth.R;
import com.android.bluetooth.TestUtils;
import com.android.bluetooth.btservice.AdapterService;
import com.android.bluetooth.btservice.storage.DatabaseManager;
import com.android.vcard.VCardConstants;
import com.android.vcard.VCardEntry;
import com.android.vcard.VCardProperty;

import org.junit.After;
import org.junit.Assert;
import org.junit.Assume;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import java.util.HashMap;
import java.util.Map;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class MapClientStateMachineTest {

    private static final String TAG = "MapStateMachineTest";
    private static final String FOLDER_SENT = "sent";

    private static final int ASYNC_CALL_TIMEOUT_MILLIS = 100;
    private static final int DISCONNECT_TIMEOUT = 3000;
    @Rule
    public final ServiceTestRule mServiceRule = new ServiceTestRule();
    private BluetoothAdapter mAdapter;
    private MceStateMachine mMceStateMachine = null;
    private BluetoothDevice mTestDevice;
    private Context mTargetContext;
    private Handler mHandler;
    private ArgumentCaptor<Intent> mIntentArgument = ArgumentCaptor.forClass(Intent.class);
    @Mock
    private AdapterService mAdapterService;
    @Mock
    private DatabaseManager mDatabaseManager;
    @Mock
    private MapClientService mMockMapClientService;
    private MockContentResolver mMockContentResolver;
    private MockSmsContentProvider mMockContentProvider;
    @Mock
    private MasClient mMockMasClient;

    @Mock
    private RequestPushMessage mMockRequestPushMessage;

    @Mock
    private SubscriptionManager mMockSubscriptionManager;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mTargetContext = InstrumentationRegistry.getTargetContext();
        mMockContentProvider = new MockSmsContentProvider();
        mMockContentResolver = new MockContentResolver();

        Assume.assumeTrue("Ignore test when MapClientService is not enabled",
                mTargetContext.getResources().getBoolean(R.bool.profile_supported_mapmce));
        TestUtils.setAdapterService(mAdapterService);
        when(mAdapterService.getDatabase()).thenReturn(mDatabaseManager);
        doReturn(true, false).when(mAdapterService).isStartedProfile(anyString());
        TestUtils.startService(mServiceRule, MapClientService.class);
        mMockContentResolver.addProvider("sms", mMockContentProvider);
        mMockContentResolver.addProvider("mms", mMockContentProvider);
        mMockContentResolver.addProvider("mms-sms", mMockContentProvider);

        when(mMockMapClientService.getContentResolver()).thenReturn(mMockContentResolver);
        when(mMockMapClientService.getSystemService(Context.TELEPHONY_SUBSCRIPTION_SERVICE))
                .thenReturn(mMockSubscriptionManager);

        doReturn(mTargetContext.getResources()).when(mMockMapClientService).getResources();

        // This line must be called to make sure relevant objects are initialized properly
        mAdapter = BluetoothAdapter.getDefaultAdapter();
        // Get a device for testing
        mTestDevice = mAdapter.getRemoteDevice("00:01:02:03:04:05");

        when(mMockMasClient.makeRequest(any(Request.class))).thenReturn(true);
        mMceStateMachine = new MceStateMachine(mMockMapClientService, mTestDevice, mMockMasClient);
        Assert.assertNotNull(mMceStateMachine);
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
        mHandler = new Handler();
    }

    @After
    public void tearDown() throws Exception {
        if (!mTargetContext.getResources().getBoolean(R.bool.profile_supported_mapmce)) {
            return;
        }

        if (mMceStateMachine != null) {
            mMceStateMachine.doQuit();
        }
        TestUtils.stopService(mServiceRule, MapClientService.class);
        TestUtils.clearAdapterService(mAdapterService);
    }

    /**
     * Test that default state is STATE_CONNECTING
     */
    @Test
    public void testDefaultConnectingState() {
        Log.i(TAG, "in testDefaultConnectingState");
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTING, mMceStateMachine.getState());
    }

    /**
     * Test transition from STATE_CONNECTING --> (receive MSG_MAS_DISCONNECTED) -->
     * STATE_DISCONNECTED
     */
    @Test
    public void testStateTransitionFromConnectingToDisconnected() {
        Log.i(TAG, "in testStateTransitionFromConnectingToDisconnected");
        setupSdpRecordReceipt();
        Message msg = Message.obtain(mHandler, MceStateMachine.MSG_MAS_DISCONNECTED);
        mMceStateMachine.sendMessage(msg);

        // Wait until the message is processed and a broadcast request is sent to
        // to MapClientService to change
        // state from STATE_CONNECTING to STATE_DISCONNECTED
        verify(mMockMapClientService,
                timeout(ASYNC_CALL_TIMEOUT_MILLIS).times(2)).sendBroadcast(
                mIntentArgument.capture(), eq(BLUETOOTH_CONNECT),
                any(Bundle.class));
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED, mMceStateMachine.getState());
    }

    /**
     * Test transition from STATE_CONNECTING --> (receive MSG_MAS_CONNECTED) --> STATE_CONNECTED
     */
    @Test
    public void testStateTransitionFromConnectingToConnected() {
        Log.i(TAG, "in testStateTransitionFromConnectingToConnected");

        setupSdpRecordReceipt();
        Message msg = Message.obtain(mHandler, MceStateMachine.MSG_MAS_CONNECTED);
        mMceStateMachine.sendMessage(msg);

        // Wait until the message is processed and a broadcast request is sent to
        // to MapClientService to change
        // state from STATE_CONNECTING to STATE_CONNECTED
        verify(mMockMapClientService,
                timeout(ASYNC_CALL_TIMEOUT_MILLIS).times(2)).sendBroadcast(
                mIntentArgument.capture(), eq(BLUETOOTH_CONNECT),
                any(Bundle.class));
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED, mMceStateMachine.getState());
    }

    /**
     * Test transition from STATE_CONNECTING --> (receive MSG_MAS_CONNECTED) --> STATE_CONNECTED -->
     * (receive MSG_MAS_DISCONNECTED) --> STATE_DISCONNECTED
     */
    @Test
    public void testStateTransitionFromConnectedWithMasDisconnected() {
        Log.i(TAG, "in testStateTransitionFromConnectedWithMasDisconnected");

        setupSdpRecordReceipt();
        Message msg = Message.obtain(mHandler, MceStateMachine.MSG_MAS_CONNECTED);
        mMceStateMachine.sendMessage(msg);

        // Wait until the message is processed and a broadcast request is sent to
        // to MapClientService to change
        // state from STATE_CONNECTING to STATE_CONNECTED
        verify(mMockMapClientService,
                timeout(ASYNC_CALL_TIMEOUT_MILLIS).times(2)).sendBroadcast(
                mIntentArgument.capture(), eq(BLUETOOTH_CONNECT),
                any(Bundle.class));
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED, mMceStateMachine.getState());

        msg = Message.obtain(mHandler, MceStateMachine.MSG_MAS_DISCONNECTED);
        mMceStateMachine.sendMessage(msg);
        verify(mMockMapClientService,
                timeout(ASYNC_CALL_TIMEOUT_MILLIS).times(4)).sendBroadcast(
                mIntentArgument.capture(), eq(BLUETOOTH_CONNECT),
                any(Bundle.class));

        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED, mMceStateMachine.getState());
    }


    /**
     * Test receiving an empty event report
     */
    @Test
    public void testReceiveEmptyEvent() {
        setupSdpRecordReceipt();
        Message msg = Message.obtain(mHandler, MceStateMachine.MSG_MAS_CONNECTED);
        mMceStateMachine.sendMessage(msg);

        // Wait until the message is processed and a broadcast request is sent to
        // to MapClientService to change
        // state from STATE_CONNECTING to STATE_CONNECTED
        verify(mMockMapClientService,
                timeout(ASYNC_CALL_TIMEOUT_MILLIS).times(2)).sendBroadcast(
                mIntentArgument.capture(), eq(BLUETOOTH_CONNECT),
                any(Bundle.class));
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED, mMceStateMachine.getState());

        // Send an empty notification event, verify the mMceStateMachine is still connected
        Message notification = Message.obtain(mHandler, MceStateMachine.MSG_NOTIFICATION);
        mMceStateMachine.getCurrentState().processMessage(msg);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED, mMceStateMachine.getState());
    }

    /**
     * Test set message status
     */
    @Test
    public void testSetMessageStatus() {
        setupSdpRecordReceipt();
        Message msg = Message.obtain(mHandler, MceStateMachine.MSG_MAS_CONNECTED);
        mMceStateMachine.sendMessage(msg);

        // Wait until the message is processed and a broadcast request is sent to
        // to MapClientService to change
        // state from STATE_CONNECTING to STATE_CONNECTED
        verify(mMockMapClientService,
                timeout(ASYNC_CALL_TIMEOUT_MILLIS).times(2)).sendBroadcast(
                mIntentArgument.capture(), eq(BLUETOOTH_CONNECT),
                any(Bundle.class));
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED, mMceStateMachine.getState());
        Assert.assertTrue(
                mMceStateMachine.setMessageStatus("123456789AB", BluetoothMapClient.READ));
    }


    /**
     * Test disconnect
     */
    @Test
    public void testDisconnect() {
        setupSdpRecordReceipt();
        doAnswer(invocation -> {
            mMceStateMachine.sendMessage(MceStateMachine.MSG_MAS_DISCONNECTED);
            return null;
        }).when(mMockMasClient).shutdown();
        Message msg = Message.obtain(mHandler, MceStateMachine.MSG_MAS_CONNECTED);
        mMceStateMachine.sendMessage(msg);

        // Wait until the message is processed and a broadcast request is sent to
        // to MapClientService to change
        // state from STATE_CONNECTING to STATE_CONNECTED
        verify(mMockMapClientService,
                timeout(ASYNC_CALL_TIMEOUT_MILLIS).times(2)).sendBroadcast(
                mIntentArgument.capture(), eq(BLUETOOTH_CONNECT),
                any(Bundle.class));
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED, mMceStateMachine.getState());

        mMceStateMachine.disconnect();
        verify(mMockMapClientService,
                timeout(ASYNC_CALL_TIMEOUT_MILLIS).times(4)).sendBroadcast(
                mIntentArgument.capture(), eq(BLUETOOTH_CONNECT),
                any(Bundle.class));
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED, mMceStateMachine.getState());
    }

    /**
     * Test disconnect timeout
     */
    @Test
    public void testDisconnectTimeout() {
        setupSdpRecordReceipt();
        Message msg = Message.obtain(mHandler, MceStateMachine.MSG_MAS_CONNECTED);
        mMceStateMachine.sendMessage(msg);

        // Wait until the message is processed and a broadcast request is sent to
        // to MapClientService to change
        // state from STATE_CONNECTING to STATE_CONNECTED
        verify(mMockMapClientService,
                timeout(ASYNC_CALL_TIMEOUT_MILLIS).times(2)).sendBroadcast(
                mIntentArgument.capture(), eq(BLUETOOTH_CONNECT),
                any(Bundle.class));
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED, mMceStateMachine.getState());

        mMceStateMachine.disconnect();
        verify(mMockMapClientService,
                after(DISCONNECT_TIMEOUT / 2).times(3)).sendBroadcast(
                mIntentArgument.capture(), eq(BLUETOOTH_CONNECT),
                any(Bundle.class));
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTING, mMceStateMachine.getState());

        verify(mMockMapClientService,
                timeout(DISCONNECT_TIMEOUT).times(4)).sendBroadcast(
                mIntentArgument.capture(), eq(BLUETOOTH_CONNECT),
                any(Bundle.class));
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED, mMceStateMachine.getState());
    }

    /**
     * Test sending a message
     */
    @Test
    public void testSendSMSMessage() {
        setupSdpRecordReceipt();
        Message msg = Message.obtain(mHandler, MceStateMachine.MSG_MAS_CONNECTED);
        mMceStateMachine.sendMessage(msg);
        TestUtils.waitForLooperToFinishScheduledTask(mMceStateMachine.getHandler().getLooper());
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED, mMceStateMachine.getState());

        String testMessage = "Hello World!";
        Uri[] contacts = new Uri[] {Uri.parse("tel://5551212")};

        verify(mMockMasClient, times(0)).makeRequest(any(RequestPushMessage.class));
        mMceStateMachine.sendMapMessage(contacts, testMessage, null, null);
        verify(mMockMasClient, timeout(ASYNC_CALL_TIMEOUT_MILLIS).times(1))
                .makeRequest(any(RequestPushMessage.class));
    }

    /**
     * Test message sent successfully
     */
    @Test
    public void testSMSMessageSent() {
        setupSdpRecordReceipt();
        Message msg = Message.obtain(mHandler, MceStateMachine.MSG_MAS_CONNECTED);
        mMceStateMachine.sendMessage(msg);
        TestUtils.waitForLooperToFinishScheduledTask(mMceStateMachine.getHandler().getLooper());
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED, mMceStateMachine.getState());

        String testMessage = "Hello World!";
        VCardEntry recipient = new VCardEntry();
        VCardProperty property = new VCardProperty();
        property.setName(VCardConstants.PROPERTY_TEL);
        property.addValues("555-1212");
        recipient.addProperty(property);
        Bmessage testBmessage = new Bmessage();
        testBmessage.setType(Bmessage.Type.SMS_GSM);
        testBmessage.setBodyContent(testMessage);
        testBmessage.addRecipient(recipient);
        RequestPushMessage testRequest =
                new RequestPushMessage(FOLDER_SENT, testBmessage, null, false, false);
        when(mMockRequestPushMessage.getMsgHandle()).thenReturn("12345");
        when(mMockRequestPushMessage.getBMsg()).thenReturn(testBmessage);
        Message msgSent = Message.obtain(mHandler, MceStateMachine.MSG_MAS_REQUEST_COMPLETED,
                mMockRequestPushMessage);

        mMceStateMachine.sendMessage(msgSent);
        TestUtils.waitForLooperToFinishScheduledTask(mMceStateMachine.getHandler().getLooper());
        Assert.assertEquals(1, mMockContentProvider.mInsertOperationCount);
    }

    private void setupSdpRecordReceipt() {
        // Perform first part of MAP connection logic.
        verify(mMockMapClientService,
                timeout(ASYNC_CALL_TIMEOUT_MILLIS).times(1)).sendBroadcast(
                mIntentArgument.capture(), eq(BLUETOOTH_CONNECT),
                any(Bundle.class));
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTING, mMceStateMachine.getState());

        // Setup receipt of SDP record
        SdpMasRecord record = new SdpMasRecord(1, 1, 1, 1, 1, 1, "MasRecord");
        Message msg = Message.obtain(mHandler, MceStateMachine.MSG_MAS_SDP_DONE, record);
        mMceStateMachine.sendMessage(msg);
    }

    private class MockSmsContentProvider extends MockContentProvider {
        Map<Uri, ContentValues> mContentValues = new HashMap<>();
        int mInsertOperationCount = 0;

        @Override
        public int delete(Uri uri, String selection, String[] selectionArgs) {
            return 0;
        }

        @Override
        public Uri insert(Uri uri, ContentValues values) {
            mInsertOperationCount++;
            return Uri.withAppendedPath(Sms.CONTENT_URI, String.valueOf(mInsertOperationCount));
        }

        @Override
        public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
                String sortOrder) {
            Cursor cursor = Mockito.mock(Cursor.class);

            when(cursor.moveToFirst()).thenReturn(true);
            when(cursor.moveToNext()).thenReturn(true).thenReturn(false);

            when(cursor.getLong(anyInt())).thenReturn((long) mContentValues.size());
            when(cursor.getString(anyInt())).thenReturn(String.valueOf(mContentValues.size()));
            return cursor;
        }
    }

}
