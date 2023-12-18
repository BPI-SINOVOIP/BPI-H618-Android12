/*
 * Copyright 2016 The Android Open Source Project
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

package com.android.bluetooth.map;

import static org.mockito.Mockito.*;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteException;
import android.net.Uri;
import android.os.Looper;
import android.os.RemoteException;
import android.os.UserManager;
import android.provider.Telephony.Mms;
import android.provider.Telephony.Sms;
import android.telephony.TelephonyManager;
import android.test.mock.MockContentProvider;
import android.test.mock.MockContentResolver;

import androidx.test.InstrumentationRegistry;
import androidx.test.filters.MediumTest;
import androidx.test.runner.AndroidJUnit4;

import com.android.bluetooth.R;

import org.junit.Assert;
import org.junit.Assume;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;

import java.io.IOException;
import java.util.HashSet;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class BluetoothMapContentObserverTest {
    static final String TEST_NUMBER_ONE = "5551212";
    static final String TEST_NUMBER_TWO = "5551234";
    private Context mTargetContext;

    static class ExceptionTestProvider extends MockContentProvider {
        HashSet<String> mContents = new HashSet<String>();
        public ExceptionTestProvider(Context context) {
            super(context);
        }

        @Override
        public Cursor query(Uri uri, String[] b, String s, String[] c, String d) {
            // Throw exception for SMS queries for easy initialization
            if (Sms.CONTENT_URI.equals(uri)) throw new SQLiteException();

            // Return a cursor otherwise for Thread IDs
            Cursor cursor = Mockito.mock(Cursor.class);
            when(cursor.moveToFirst()).thenReturn(true);
            when(cursor.getLong(anyInt())).thenReturn(0L);
            return cursor;
        }

        @Override
        public Uri insert(Uri uri, ContentValues values) {
            // Store addresses for later verification
            Object address = values.get(Mms.Addr.ADDRESS);
            if (address != null) mContents.add((String) address);
            return Uri.withAppendedPath(Mms.Outbox.CONTENT_URI, "0");
        }
    }

    @Before
    public void setUp() {
        mTargetContext = InstrumentationRegistry.getTargetContext();
        Assume.assumeTrue("Ignore test when BluetoothMapService is not enabled",
                mTargetContext.getResources().getBoolean(R.bool.profile_supported_map));
    }

    @Test
    public void testInitMsgList() {
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
        Context mockContext = mock(Context.class);
        MockContentResolver mockResolver = new MockContentResolver();
        ExceptionTestProvider mockProvider = new ExceptionTestProvider(mockContext);
        mockResolver.addProvider("sms", mockProvider);

        TelephonyManager mockTelephony = mock(TelephonyManager.class);
        UserManager mockUserService = mock(UserManager.class);
        BluetoothMapMasInstance mockMas = mock(BluetoothMapMasInstance.class);

        // Functions that get called when BluetoothMapContentObserver is created
        when(mockUserService.isUserUnlocked()).thenReturn(true);
        when(mockContext.getContentResolver()).thenReturn(mockResolver);
        when(mockContext.getSystemService(Context.TELEPHONY_SERVICE)).thenReturn(mockTelephony);
        when(mockContext.getSystemService(Context.USER_SERVICE)).thenReturn(mockUserService);

        BluetoothMapContentObserver observer;
        try {
            // The constructor of BluetoothMapContentObserver calls initMsgList
            observer = new BluetoothMapContentObserver(mockContext, null, mockMas, null, true);
        } catch (RemoteException e) {
            Assert.fail("Failed to created BluetoothMapContentObserver object");
        } catch (SQLiteException e) {
            Assert.fail("Threw SQLiteException instead of Assert.failing cleanly");
        }
    }

    @Test
    public void testPushGroupMMS() {
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
        Context mockContext = mock(Context.class);
        MockContentResolver mockResolver = new MockContentResolver();
        ExceptionTestProvider mockProvider = new ExceptionTestProvider(mockContext);

        mockResolver.addProvider("sms", mockProvider);
        mockResolver.addProvider("mms", mockProvider);
        mockResolver.addProvider("mms-sms", mockProvider);
        TelephonyManager mockTelephony = mock(TelephonyManager.class);
        UserManager mockUserService = mock(UserManager.class);
        BluetoothMapMasInstance mockMas = mock(BluetoothMapMasInstance.class);

        // Functions that get called when BluetoothMapContentObserver is created
        when(mockUserService.isUserUnlocked()).thenReturn(true);
        when(mockContext.getContentResolver()).thenReturn(mockResolver);
        when(mockContext.getSystemService(Context.TELEPHONY_SERVICE)).thenReturn(mockTelephony);
        when(mockContext.getSystemService(Context.USER_SERVICE)).thenReturn(mockUserService);

        BluetoothMapbMessageMime message = new BluetoothMapbMessageMime();
        message.setType(BluetoothMapUtils.TYPE.MMS);
        message.setFolder("telecom/msg/outbox");
        message.addSender("Zero", "0");
        message.addRecipient("One", new String[] {TEST_NUMBER_ONE}, null);
        message.addRecipient("Two", new String[] {TEST_NUMBER_TWO}, null);
        BluetoothMapbMessageMime.MimePart body =  message.addMimePart();
        try {
            body.mContentType = "text/plain";
            body.mData = "HelloWorld".getBytes("utf-8");
        } catch (Exception e) {
            Assert.fail("Failed to setup test message");
        }

        BluetoothMapAppParams appParams = new BluetoothMapAppParams();
        BluetoothMapFolderElement folderElement = new BluetoothMapFolderElement("outbox", null);

        try {
            // The constructor of BluetoothMapContentObserver calls initMsgList
            BluetoothMapContentObserver observer =
                    new BluetoothMapContentObserver(mockContext, null, mockMas, null, true);
            observer.pushMessage(message, folderElement, appParams, null);
        } catch (RemoteException e) {
            Assert.fail("Failed to created BluetoothMapContentObserver object");
        } catch (SQLiteException e) {
            Assert.fail("Threw SQLiteException instead of Assert.failing cleanly");
        } catch (IOException e) {
            Assert.fail("Threw IOException");
        } catch (NullPointerException e) {
            //expected that the test case will end in a NPE as part of the sendMultimediaMessage
            //pendingSendIntent
        }

        // Validate that 3 addresses were inserted into the database with 2 being the recipients
        Assert.assertEquals(3, mockProvider.mContents.size());
        Assert.assertTrue(mockProvider.mContents.contains(TEST_NUMBER_ONE));
        Assert.assertTrue(mockProvider.mContents.contains(TEST_NUMBER_TWO));
    }

}
