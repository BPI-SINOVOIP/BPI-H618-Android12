/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.net;

import android.content.Context;
import android.os.RemoteException;
import android.annotation.SdkConstant;
import android.annotation.SdkConstant.SdkConstantType;
import android.net.DhcpInfo;
import android.net.LinkProperties;
import android.os.Binder;
import android.os.IBinder;
import android.os.Handler;
import android.os.Message;
import android.os.Looper;
import android.os.HandlerThread;
import android.os.RemoteException;
import android.util.Log;
import android.net.IpConfiguration;
import java.util.List;
/**
 * A class to control the pppoe network.
 *
 * @hide
 */
public class PppoeManager {
    private static final String TAG = "PppoeManager";

    private final IPppoeManager mService;
    private final Context mContext;
    public static boolean DEBUG = true;

    private static void LOG(String msg) {
        if (DEBUG) {
            Log.d(TAG, msg);
        }
    }
    public PppoeManager(Context context, IPppoeManager service) {
        mContext = context;
        mService = service;
    }
    /**
     * @hide
     */
    public static final String EXTRA_PPPOE_ERRMSG = "pppoe_errmsg";

    /**
     * @hide
     */

    public static final String PPPOE_STATE_CHANGED_ACTION  = "android.net.pppoe.PPPOE_STATE_CHANGED";

    /**
     * @hide
     */
    public static final String EXTRA_PPPOE_STATE = "pppoe_state";

    /**
     * @hide
     */
    public static final String EXTRA_PREVIOUS_PPPOE_STATE = "previous_pppoe_state";
    /**
     * @hide
     */
    public static final int PPPOE_STATE_DISCONNECTED = 0;
    /**
     * @hide
     */
    public static final int PPPOE_STATE_CONNECTING = 1;
    /**
     * @hide
     */
    public static final int PPPOE_STATE_CONNECTED = 2;

    /**
     * @hide
     */
    public static final int PPPOE_STATE_DISCONNECTING = 3;

    /**
     * @hide
     */
    public static final int PPPOE_EVENT_CONNECTING = 0;
    public static final int PPPOE_EVENT_CONNECT_SUCCESSED = 1;
    public static final int PPPOE_EVENT_CONNECT_FAILED = 2;
    public static final int PPPOE_EVENT_DISCONNECTING = 3;
    public static final int PPPOE_EVENT_DISCONNECT_SUCCESSED = 4;

    public boolean setupPppoe(String iface, String user, String password) {
        try {
            return mService.setupPppoe(iface,user, password);
        } catch (RemoteException e) {
            return false;
        }
    }

    public boolean connectPppoe(String iface) {
        try {
            mService.startPppoe(iface);
        } catch (RemoteException e) {
            Log.e(TAG, "startPppoe failed"+e);
            return false;
        }
        return true;
    }

    public boolean disconnectPppoe(String iface) {
        try {
            mService.stopPppoe(iface);
        } catch (RemoteException e) {
            Log.e(TAG, "startPppoe failed"+e);
            return false;
        }
        return true;

    }

    public int getPppoeState(String Iface) {
        try {
            return mService.getPppoeState(Iface);
        } catch (RemoteException e) {
            Log.e(TAG, "stopPppoe failed");
            return -1;
        }
    }

    public boolean isPppoeEnabled() {
        try {
            return mService.isPppoeEnabled();
        } catch (RemoteException e) {
            Log.w(TAG,"isAvailable excute failed");
            return false;
        }
    }

    public List<String> getPppoeUserInfo(String iface) {
        try {
            return mService.getPppoeUserInfo(iface);
        } catch (RemoteException e) {
            Log.w(TAG,"can't get Pppoe User and Password!");
            return null;
        }
    }

    public String getPppoeInterfaceName() {
        try {
            return mService.getPppoeInterfaceName();
        } catch (RemoteException e) {
            Log.e(TAG, "can't get PPPoE interface name");
            return null;
        }
    }

    public LinkProperties getPppoelinkProperties() {
        try {
            return mService.getPppoelinkProperties();
        } catch (RemoteException e) {
            Log.w(TAG,"can't get Pppoe linkProperties");
            return null;
        }

    }

}
