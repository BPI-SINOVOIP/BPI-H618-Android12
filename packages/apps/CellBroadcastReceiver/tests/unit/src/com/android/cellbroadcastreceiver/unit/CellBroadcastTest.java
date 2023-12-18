/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.cellbroadcastreceiver.unit;

import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.eq;

import android.content.Context;
import android.content.res.Resources;
import android.os.PersistableBundle;
import android.telephony.CarrierConfigManager;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.util.SparseArray;

import com.android.internal.telephony.ISub;

import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

public abstract class CellBroadcastTest {

    protected static String TAG;

    private SparseArray<PersistableBundle> mBundles = new SparseArray<>();

    MockedServiceManager mMockedServiceManager;

    @Mock
    Context mContext;
    @Mock
    CarrierConfigManager mCarrierConfigManager;
    @Mock
    TelephonyManager mTelephonyManager;
    @Mock
    Resources mResources;
    @Mock
    ISub.Stub mSubService;

    protected void setUp(String tag) throws Exception {
        TAG = tag;
        MockitoAnnotations.initMocks(this);
        // A hack to return mResources from static method
        // CellBroadcastSettings.getResources(context).
        doReturn(mSubService).when(mSubService).queryLocalInterface(anyString());
        doReturn(mSubService).when(mSubService).asBinder();
        doReturn(SubscriptionManager.INVALID_SUBSCRIPTION_ID).when(mSubService).getDefaultSubId();
        doReturn(SubscriptionManager.INVALID_SUBSCRIPTION_ID).when(mSubService).getDefaultSmsSubId();
        mMockedServiceManager = new MockedServiceManager();
        mMockedServiceManager.replaceService("isub", mSubService);
        TelephonyManager.disableServiceHandleCaching();
        SubscriptionManager.clearCaches();
        SubscriptionManager.disableCaching();

        initContext();
    }

    private void initContext() {
        doReturn(mCarrierConfigManager).when(mContext)
                .getSystemService(eq(Context.CARRIER_CONFIG_SERVICE));
        doReturn(Context.TELEPHONY_SERVICE).when(mContext).getSystemServiceName(
                TelephonyManager.class);
        doReturn(mTelephonyManager).when(mContext).getSystemService(Context.TELEPHONY_SERVICE);
        doReturn(mResources).when(mContext).getResources();
        doReturn(mContext).when(mContext).getApplicationContext();
        doReturn(new String[]{""}).when(mResources).getStringArray(anyInt());
        doReturn(TelephonyManager.SIM_STATE_LOADED).when(mTelephonyManager)
                .getSimApplicationState(anyInt());
    }

    void carrierConfigSetStringArray(int subId, String key, String[] values) {
        if (mBundles.get(subId) == null) {
            mBundles.put(subId, new PersistableBundle());
        }
        mBundles.get(subId).putStringArray(key, values);
        doReturn(mBundles.get(subId)).when(mCarrierConfigManager).getConfigForSubId(eq(subId));
    }

    void putResources(int id, String[] values) {
        doReturn(values).when(mResources).getStringArray(eq(id));
    }

    protected void tearDown() throws Exception {
        mMockedServiceManager.restoreAllServices();
    }

    protected static void logd(String s) {
        Log.d(TAG, s);
    }
}
